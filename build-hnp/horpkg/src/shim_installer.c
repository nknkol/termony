#define _POSIX_C_SOURCE 200809L

#include "shim_installer.h"

#include "config.h"
#include "hdc.h"
#include "hap_parser.h"
#include "install.h"
#include "logger.h"
#include "utils.h"

#include <yyjson.h>
#include <zip.h>

#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <spawn.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

extern char **environ;

#define HORPKG_SHIM_MANIFEST_PATH      "manifest.json"
#define HORPKG_RUNTIME_BUNDLE          "org.horpkg.runtime"
#define HORPKG_RUNTIME_HNP_NAME        "horpkgruntime.hnp"
#define HORPKG_MAX_SHIMS_PER_MANIFEST  32
#define HORPKG_MAX_ENV_VARS            16
#define HORPKG_MAX_DEPENDENCIES        16
#define HORPKG_SHIM_MANIFEST_NOT_FOUND 1

typedef struct {
    char name[64];
    char value[256];
} shim_env_var_t;

typedef struct {
    char name[64];
    char target_path[PATH_MAX];
    char install_path[PATH_MAX];
    char lib_env[64];
    char lib_path[PATH_MAX];
    shim_env_var_t envs[HORPKG_MAX_ENV_VARS];
    size_t env_count;
    char dependencies[HORPKG_MAX_DEPENDENCIES][128];
    size_t dep_count;
} shim_entry_t;

typedef struct {
    char package[128];
    char version[64];
    shim_entry_t entries[HORPKG_MAX_SHIMS_PER_MANIFEST];
    size_t entry_count;
} shim_manifest_t;

typedef struct {
    char runtime_dir[PATH_MAX];
    char tmp_dir[PATH_MAX];
    char build_dir[PATH_MAX];
    char hap_path[PATH_MAX];
    char hnp_path[PATH_MAX];
    char state_path[PATH_MAX];
    char signed_hap_path[PATH_MAX];
} shim_runtime_paths_t;

typedef struct {
    char clang_path[PATH_MAX];
    char target[64];
    char sysroot[PATH_MAX];
    bool use_zig_driver;
} shim_toolchain_t;

typedef struct {
    char build_dir[PATH_MAX];
    char output_path[PATH_MAX];
} shim_build_artifact_t;

static const char *k_shim_template_source =
"#define _POSIX_C_SOURCE 200809L\n"
"#include <stdio.h>\n"
"#include <stdlib.h>\n"
"#include <unistd.h>\n"
"#include <string.h>\n"
"#include <errno.h>\n"
"#include <stddef.h>\n"
"#include \"config.h\"\n"
"#define X(name, value) { name, value },\n"
"static const struct {\n"
"    const char *name;\n"
"    const char *value;\n"
"} CUSTOM_ENV_VARS[] = {\n"
"#ifdef SHIM_ENV_VARS\n"
"    SHIM_ENV_VARS\n"
"#endif\n"
"    { NULL, NULL }\n"
"};\n"
"#undef X\n"
"static void set_env_var(const char *name, const char *value) {\n"
"    if (setenv(name, value, 1) != 0) {\n"
"        fprintf(stderr, \"shim: Failed to set environment variable: %s\\n\", name);\n"
"        perror(\"shim: setenv error\");\n"
"    }\n"
"}\n"
"static void setup_library_path(void) {\n"
"    const char *custom_path = CUSTOM_LIB_PATH;\n"
"    const char *env_var_name = LIB_PATH_ENV_VAR;\n"
"    if (!custom_path || custom_path[0] == '\\0' || !env_var_name || env_var_name[0] == '\\0') {\n"
"        return;\n"
"    }\n"
"    const char *old_path = getenv(env_var_name);\n"
"    char *merged = NULL;\n"
"    if (old_path && old_path[0] != '\\0') {\n"
"        size_t len = strlen(custom_path) + strlen(old_path) + 2;\n"
"        merged = (char *)malloc(len);\n"
"        if (!merged) {\n"
"            perror(\"shim: malloc failed\");\n"
"            exit(126);\n"
"        }\n"
"        snprintf(merged, len, \"%s:%s\", custom_path, old_path);\n"
"    }\n"
"    set_env_var(env_var_name, merged ? merged : custom_path);\n"
"    free(merged);\n"
"}\n"
"int main(int argc, char *argv[]) {\n"
"    (void)argc;\n"
"    setup_library_path();\n"
"    for (int i = 0; CUSTOM_ENV_VARS[i].name != NULL; i++) {\n"
"        set_env_var(CUSTOM_ENV_VARS[i].name, CUSTOM_ENV_VARS[i].value);\n"
"    }\n"
"    execv(TARGET_COMMAND_PATH, argv);\n"
"    fprintf(stderr, \"shim: FATAL: Failed to execute target command: %s\\n\", TARGET_COMMAND_PATH);\n"
"    perror(\"shim: execv error\");\n"
"    return (errno == ENOENT) ? 127 : 126;\n"
"}\n";

static int ensure_dir_recursive(const char *path);
static int ensure_parent_dir(const char *path);
static int copy_file(const char *src, const char *dst);
static bool file_exists(const char *path);
static bool command_is_available(const char *cmd);
static void detect_toolchain(shim_toolchain_t *tc);
static int runtime_paths_init(shim_runtime_paths_t *paths);
static int ensure_runtime_hap(const shim_runtime_paths_t *paths);
static int runtime_hnp_entry_name(const shim_runtime_paths_t *paths, char *entry, size_t entry_size);
static int ensure_runtime_hnp(const shim_runtime_paths_t *paths);
static int read_zip_entry(const char *zip_path, const char *entry_name, char **buffer_out, size_t *size_out);
static bool manifest_contains_command(const shim_manifest_t *manifest, const char *command);
static int update_shim_state(const shim_manifest_t *manifest, const shim_runtime_paths_t *paths);
static int load_manifest_from_hnp(const char *hnp_path, shim_manifest_t *manifest, const char *fallback_pkg);
static int write_config_header(const shim_entry_t *entry, const char *config_path);
static int write_template_source(const char *template_path);
static int compile_shim_binary(const shim_entry_t *entry,
                               const shim_runtime_paths_t *paths,
                               const shim_toolchain_t *tc,
                               shim_build_artifact_t *artifact);
static void cleanup_artifact(const shim_build_artifact_t *artifact);
static int add_file_to_zip(zip_t *archive, const char *entry_name, const char *file_path);
static int add_shims_to_hnp(const shim_manifest_t *manifest,
                            const shim_runtime_paths_t *paths,
                            const shim_build_artifact_t *artifacts,
                            size_t artifact_count);
static int replace_hnp_in_hap(const shim_runtime_paths_t *paths);
static int install_runtime_bundle(const shim_runtime_paths_t *paths);

int shim_install_from_hnp(const char *package_name, const char *hnp_path) {
    if (!hnp_path || !hnp_path[0]) {
        log_warn("Shim installer skipped: invalid HNP path.");
        return 0;
    }

    log_info("Loading shim manifest from %s ...", hnp_path);
    shim_manifest_t manifest = {0};
    int manifest_status = load_manifest_from_hnp(hnp_path, &manifest, package_name);
    if (manifest_status == HORPKG_SHIM_MANIFEST_NOT_FOUND) {
        log_debug("No shim manifest detected in %s; skipping shim installation.", hnp_path);
        return 0;
    } else if (manifest_status != 0) {
        log_error("Failed to parse shim manifest from %s.", hnp_path);
        return -1;
    }

    if (manifest.entry_count == 0) {
        log_info("Shim manifest in %s is empty. Nothing to install.", hnp_path);
        return 0;
    }

    shim_runtime_paths_t paths = {0};
    log_debug("Initializing runtime workspace at ~/.horpkg/runtime ...");
    if (runtime_paths_init(&paths) != 0) {
        log_error("Failed to initialize runtime workspace for shim installation.");
        return -1;
    }

    log_debug("Ensuring local copy of org.horpkg.runtime.hap ...");
    if (ensure_runtime_hap(&paths) != 0) {
        log_error("Unable to prepare working copy of %s.", HORPKG_RUNTIME_BUNDLE);
        return -1;
    }

    log_debug("Extracting %s from runtime HAP ...", HORPKG_RUNTIME_HNP_NAME);
    if (ensure_runtime_hnp(&paths) != 0) {
        log_error("Unable to extract %s from runtime HAP.", HORPKG_RUNTIME_HNP_NAME);
        return -1;
    }

    shim_toolchain_t toolchain = {0};
    log_info("Detecting Zig toolchain ...");
    detect_toolchain(&toolchain);
    log_debug("Toolchain result: compiler=%s, use_zig_driver=%d, target=%s, sysroot=%s",
              toolchain.clang_path,
              (int)toolchain.use_zig_driver,
              toolchain.target[0] ? toolchain.target : "(default)",
              toolchain.sysroot[0] ? toolchain.sysroot : "(default)");

    shim_build_artifact_t artifacts[HORPKG_MAX_SHIMS_PER_MANIFEST] = {0};
    size_t artifact_count = 0;

    for (size_t i = 0; i < manifest.entry_count; i++) {
        shim_build_artifact_t artifact = {0};
        log_info("Building shim %zu/%zu (%s)...", i + 1, manifest.entry_count, manifest.entries[i].name);
        if (compile_shim_binary(&manifest.entries[i], &paths, &toolchain, &artifact) != 0) {
            for (size_t j = 0; j < artifact_count; j++) {
                cleanup_artifact(&artifacts[j]);
            }
            return -1;
        }
        artifacts[artifact_count++] = artifact;
    }

    log_info("Injecting %zu shim artifact(s) into runtime hnp ...", artifact_count);
    int result = add_shims_to_hnp(&manifest, &paths, artifacts, artifact_count);
    for (size_t i = 0; i < artifact_count; i++) {
        cleanup_artifact(&artifacts[i]);
    }

    if (result != 0) {
        return -1;
    }

    log_info("Updating runtime HAP with patched hnp ...");
    if (replace_hnp_in_hap(&paths) != 0) {
        log_error("Failed to update %s within runtime HAP.", HORPKG_RUNTIME_HNP_NAME);
        return -1;
    }

    log_info("Signing & installing updated runtime bundle ...");
    if (install_runtime_bundle(&paths) != 0) {
        log_error("Failed to install patched runtime bundle with new shims.");
        return -1;
    }

    log_debug("Persisting shim metadata ...");
    if (update_shim_state(&manifest, &paths) != 0) {
        log_warn("Runtime shim installation succeeded but failed to persist shim metadata.");
    }

    print_success_fmt("Installed %zu shim(s) for package %s.", manifest.entry_count, manifest.package);
    return 0;
}

// ----------------------- Helper Implementations -----------------------

static bool file_exists(const char *path) {
    return path && access(path, F_OK) == 0;
}

static int ensure_dir_recursive(const char *path) {
    if (!path || !path[0]) {
        return -1;
    }

    char buffer[PATH_MAX];
    if (snprintf(buffer, sizeof(buffer), "%s", path) < 0) {
        return -1;
    }

    size_t len = strlen(buffer);
    if (len == 0) {
        return -1;
    }

    for (size_t i = 1; i < len; i++) {
        if (buffer[i] == '/') {
            buffer[i] = '\0';
            if (buffer[0] != '\0' && mkdir(buffer, 0755) != 0 && errno != EEXIST) {
                return -1;
            }
            buffer[i] = '/';
        }
    }

    if (mkdir(buffer, 0755) != 0 && errno != EEXIST) {
        return -1;
    }
    return 0;
}

static int ensure_parent_dir(const char *path) {
    if (!path) {
        return -1;
    }
    char buffer[PATH_MAX];
    if (snprintf(buffer, sizeof(buffer), "%s", path) < 0) {
        return -1;
    }
    char *slash = strrchr(buffer, '/');
    if (!slash) {
        return 0;
    }
    *slash = '\0';
    if (buffer[0] == '\0') {
        return 0;
    }
    return ensure_dir_recursive(buffer);
}

static int copy_file(const char *src, const char *dst) {
    if (ensure_parent_dir(dst) != 0) {
        return -1;
    }

    FILE *in = fopen(src, "rb");
    if (!in) {
        return -1;
    }
    FILE *out = fopen(dst, "wb");
    if (!out) {
        fclose(in);
        return -1;
    }

    char buffer[16384];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), in)) > 0) {
        if (fwrite(buffer, 1, bytes, out) != bytes) {
            fclose(in);
            fclose(out);
            return -1;
        }
    }

    fclose(in);
    fclose(out);
    return 0;
}

static bool command_is_available(const char *cmd) {
    if (!cmd || !cmd[0]) {
        return false;
    }
    log_debug("command_is_available: checking %s", cmd);
    if (strchr(cmd, '/')) {
        int ok = (access(cmd, X_OK) == 0);
        log_debug("command_is_available: %s absolute %s", cmd, ok ? "OK" : "NOT FOUND");
        return ok;
    }
    const char *path_env = getenv("PATH");
    if (!path_env) {
        log_warn("command_is_available: PATH is NULL when checking %s", cmd);
        return false;
    }
    char *paths = strdup(path_env);
    if (!paths) {
        log_error("command_is_available: strdup failed for PATH while checking %s", cmd);
        return false;
    }
    bool found = false;
    char *saveptr = NULL;
    for (char *token = strtok_r(paths, ":", &saveptr); token; token = strtok_r(NULL, ":", &saveptr)) {
        char candidate[PATH_MAX];
        int written = snprintf(candidate, sizeof(candidate), "%s/%s", token, cmd);
        if (written > 0 && (size_t)written < sizeof(candidate) && access(candidate, X_OK) == 0) {
            found = true;
            break;
        }
    }
    free(paths);
    log_debug("command_is_available: %s %s", cmd, found ? "found" : "missing");
    return found;
}

static void detect_toolchain(shim_toolchain_t *tc) {
    if (!tc) return;
    memset(tc, 0, sizeof(*tc));

    const char *override = getenv("HORPKG_SHIM_CLANG");
    log_debug("detect_toolchain: HORPKG_SHIM_CLANG=%s", override ? override : "(null)");
    tc->use_zig_driver = true;

    if (override && override[0]) {
        snprintf(tc->clang_path, sizeof(tc->clang_path), "%s", override);
        if (!strstr(override, "zig")) {
            tc->use_zig_driver = false;
        }
        log_info("Using custom compiler command: %s", tc->clang_path);
    } else {
        snprintf(tc->clang_path, sizeof(tc->clang_path), "%s", "zig");
        log_info("Default compiler command: zig");
    }

    if (!command_is_available(tc->clang_path)) {
        log_error("Unable to locate compiler command '%s'. Ensure zig shim is registered in PATH or set HORPKG_SHIM_CLANG.", tc->clang_path);
    } else {
        log_debug("Compiler command '%s' validated.", tc->clang_path);
    }

    const char *target_override = getenv("HORPKG_SHIM_TARGET");
    if (target_override && target_override[0]) {
        snprintf(tc->target, sizeof(tc->target), "%s", target_override);
        log_debug("Using custom target: %s", tc->target);
    }

    const char *sysroot_override = getenv("HORPKG_SHIM_SYSROOT");
    if (sysroot_override && sysroot_override[0]) {
        snprintf(tc->sysroot, sizeof(tc->sysroot), "%s", sysroot_override);
        log_debug("Using custom sysroot: %s", tc->sysroot);
    }
}

static int runtime_paths_init(shim_runtime_paths_t *paths) {
    if (!paths) return -1;
    memset(paths, 0, sizeof(*paths));

    char *runtime_dir = get_config_path("runtime");
    if (!runtime_dir) {
        return -1;
    }
    if (create_dir_if_not_exists(runtime_dir) != 0) {
        free(runtime_dir);
        return -1;
    }
    snprintf(paths->runtime_dir, sizeof(paths->runtime_dir), "%s", runtime_dir);
    free(runtime_dir);

    snprintf(paths->tmp_dir, sizeof(paths->tmp_dir), "%s/tmp", paths->runtime_dir);
    snprintf(paths->build_dir, sizeof(paths->build_dir), "%s/build", paths->runtime_dir);
    create_dir_if_not_exists(paths->tmp_dir);
    create_dir_if_not_exists(paths->build_dir);

    snprintf(paths->hap_path, sizeof(paths->hap_path), "%s/%s.hap", paths->runtime_dir, HORPKG_RUNTIME_BUNDLE);
    snprintf(paths->hnp_path, sizeof(paths->hnp_path), "%s/%s", paths->runtime_dir, HORPKG_RUNTIME_HNP_NAME);
    snprintf(paths->state_path, sizeof(paths->state_path), "%s/state.json", paths->runtime_dir);
    snprintf(paths->signed_hap_path, sizeof(paths->signed_hap_path), "%s/%s.signed.hap", paths->runtime_dir, HORPKG_RUNTIME_BUNDLE);
    return 0;
}

static int ensure_runtime_hap(const shim_runtime_paths_t *paths) {
    if (file_exists(paths->hap_path)) {
        return 0;
    }
    char resource_path[PATH_MAX];
    if (horpkg_find_resource("org.horpkg.runtime.hap", resource_path, sizeof(resource_path)) != 0) {
        log_error("Bundled org.horpkg.runtime.hap not found.");
        return -1;
    }
    log_info("Seeding runtime HAP from resources: %s", resource_path);
    return copy_file(resource_path, paths->hap_path);
}

static int runtime_hnp_entry_name(const shim_runtime_paths_t *paths, char *entry, size_t entry_size) {
    if (!entry || entry_size == 0) return -1;
    int err = 0;
    zip_t *hap = zip_open(paths->hap_path, ZIP_RDONLY, &err);
    if (!hap) {
        log_error("Failed to open runtime HAP (zip err=%d).", err);
        return -1;
    }

    zip_int64_t count = zip_get_num_entries(hap, 0);
    bool found = false;
    for (zip_int64_t i = 0; i < count; i++) {
        const char *name = zip_get_name(hap, i, 0);
        if (!name) continue;
        const char *needle = HORPKG_RUNTIME_HNP_NAME;
        size_t name_len = strlen(name);
        size_t needle_len = strlen(needle);
        if (name_len >= needle_len && strcmp(name + name_len - needle_len, needle) == 0) {
            snprintf(entry, entry_size, "%s", name);
            found = true;
            break;
        }
    }

    zip_close(hap);
    return found ? 0 : -1;
}

static int ensure_runtime_hnp(const shim_runtime_paths_t *paths) {
    if (file_exists(paths->hnp_path)) {
        return 0;
    }

    char entry_path[PATH_MAX];
    if (runtime_hnp_entry_name(paths, entry_path, sizeof(entry_path)) != 0) {
        log_error("Runtime HAP does not contain %s.", HORPKG_RUNTIME_HNP_NAME);
        return -1;
    }

    int err = 0;
    zip_t *hap = zip_open(paths->hap_path, ZIP_RDONLY, &err);
    if (!hap) {
        log_error("Failed to open runtime HAP for extraction (zip err=%d).", err);
        return -1;
    }

    zip_file_t *zf = zip_fopen(hap, entry_path, 0);
    if (!zf) {
        zip_close(hap);
        log_error("Unable to read %s from runtime HAP.", entry_path);
        return -1;
    }

    if (ensure_parent_dir(paths->hnp_path) != 0) {
        zip_fclose(zf);
        zip_close(hap);
        return -1;
    }

    FILE *out = fopen(paths->hnp_path, "wb");
    if (!out) {
        zip_fclose(zf);
        zip_close(hap);
        return -1;
    }

    char buffer[16384];
    zip_int64_t read_bytes;
    while ((read_bytes = zip_fread(zf, buffer, sizeof(buffer))) > 0) {
        if (fwrite(buffer, 1, (size_t)read_bytes, out) != (size_t)read_bytes) {
            fclose(out);
            zip_fclose(zf);
            zip_close(hap);
            return -1;
        }
    }

    fclose(out);
    zip_fclose(zf);
    if (zip_close(hap) != 0) {
        return -1;
    }
    return 0;
}

static int read_zip_entry(const char *zip_path, const char *entry_name, char **buffer_out, size_t *size_out) {
    if (!buffer_out || !size_out) return -1;
    *buffer_out = NULL;
    *size_out = 0;

    int err = 0;
    zip_t *archive = zip_open(zip_path, ZIP_RDONLY, &err);
    if (!archive) {
        return -1;
    }

    zip_int64_t index = zip_name_locate(archive, entry_name, ZIP_FL_ENC_UTF_8);
    if (index < 0) {
        zip_close(archive);
        return HORPKG_SHIM_MANIFEST_NOT_FOUND;
    }

    struct zip_stat st;
    zip_stat_init(&st);
    if (zip_stat_index(archive, index, 0, &st) != 0) {
        zip_close(archive);
        return -1;
    }

    zip_file_t *file = zip_fopen_index(archive, index, 0);
    if (!file) {
        zip_close(archive);
        return -1;
    }

    char *buffer = malloc((size_t)st.size + 1);
    if (!buffer) {
        zip_fclose(file);
        zip_close(archive);
        return -1;
    }

    zip_int64_t total = zip_fread(file, buffer, st.size);
    zip_fclose(file);
    zip_close(archive);

    if (total != (zip_int64_t)st.size) {
        free(buffer);
        return -1;
    }

    buffer[st.size] = '\0';
    *buffer_out = buffer;
    *size_out = (size_t)st.size;
    return 0;
}

static bool manifest_contains_command(const shim_manifest_t *manifest, const char *command) {
    if (!manifest || !command) return false;
    for (size_t i = 0; i < manifest->entry_count; i++) {
        if (strcmp(manifest->entries[i].name, command) == 0) {
            return true;
        }
    }
    return false;
}

static int update_shim_state(const shim_manifest_t *manifest, const shim_runtime_paths_t *paths) {
    if (!manifest || !paths) return -1;

    yyjson_doc *old_doc = NULL;
    if (file_exists(paths->state_path)) {
        old_doc = yyjson_read_file(paths->state_path,
                                   YYJSON_READ_ALLOW_COMMENTS | YYJSON_READ_ALLOW_TRAILING_COMMAS,
                                   NULL, NULL);
    }

    yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val *root = yyjson_mut_obj(doc);
    yyjson_mut_doc_set_root(doc, root);
    yyjson_mut_obj_add_int(doc, root, "version", 1);
    yyjson_mut_val *shims_arr = yyjson_mut_arr(doc);
    yyjson_mut_obj_add_val(doc, root, "shims", shims_arr);

    if (old_doc) {
        yyjson_val *old_root = yyjson_doc_get_root(old_doc);
        yyjson_val *old_shims = yyjson_obj_get(old_root, "shims");
        size_t idx, max;
        yyjson_val *item;
        yyjson_arr_foreach(old_shims, idx, max, item) {
            const char *cmd = yyjson_get_str(yyjson_obj_get(item, "command"));
            if (manifest_contains_command(manifest, cmd)) {
                continue;
            }
            const char *pkg = yyjson_get_str(yyjson_obj_get(item, "package"));
            const char *ver = yyjson_get_str(yyjson_obj_get(item, "version"));
            const char *target = yyjson_get_str(yyjson_obj_get(item, "target"));
            const char *install_path = yyjson_get_str(yyjson_obj_get(item, "installPath"));
            const char *lib_env = yyjson_get_str(yyjson_obj_get(item, "libEnv"));
            const char *lib_path = yyjson_get_str(yyjson_obj_get(item, "libPath"));
            int64_t installed_at = yyjson_get_int(yyjson_obj_get(item, "installedAt"));

            yyjson_mut_val *obj = yyjson_mut_obj(doc);
            yyjson_mut_obj_add_str(doc, obj, "package", pkg ? pkg : "unknown");
            yyjson_mut_obj_add_str(doc, obj, "version", ver ? ver : "0.0.0");
            yyjson_mut_obj_add_str(doc, obj, "command", cmd ? cmd : "unknown");
            yyjson_mut_obj_add_str(doc, obj, "target", target ? target : "");
            yyjson_mut_obj_add_str(doc, obj, "installPath", install_path ? install_path : "");
            yyjson_mut_obj_add_str(doc, obj, "libEnv", lib_env ? lib_env : "");
            yyjson_mut_obj_add_str(doc, obj, "libPath", lib_path ? lib_path : "");
            yyjson_mut_obj_add_int(doc, obj, "installedAt", installed_at);

            yyjson_mut_val *env_arr = yyjson_mut_arr(doc);
            yyjson_val *old_env = yyjson_obj_get(item, "env");
            size_t eidx, emax;
            yyjson_val *env_item;
            yyjson_arr_foreach(old_env, eidx, emax, env_item) {
                const char *name = yyjson_get_str(yyjson_obj_get(env_item, "name"));
                const char *value = yyjson_get_str(yyjson_obj_get(env_item, "value"));
                yyjson_mut_val *env_obj = yyjson_mut_obj(doc);
                yyjson_mut_obj_add_str(doc, env_obj, "name", name ? name : "");
                yyjson_mut_obj_add_str(doc, env_obj, "value", value ? value : "");
                yyjson_mut_arr_append(env_arr, env_obj);
            }
            yyjson_mut_obj_add_val(doc, obj, "env", env_arr);

            yyjson_mut_val *dep_arr = yyjson_mut_arr(doc);
            yyjson_val *old_deps = yyjson_obj_get(item, "dependencies");
            size_t didx, dmax;
            yyjson_val *dep_item;
            yyjson_arr_foreach(old_deps, didx, dmax, dep_item) {
                const char *dep = yyjson_get_str(dep_item);
                yyjson_mut_arr_append(dep_arr, yyjson_mut_str(doc, dep ? dep : ""));
            }
            yyjson_mut_obj_add_val(doc, obj, "dependencies", dep_arr);

            yyjson_mut_arr_append(shims_arr, obj);
        }
        yyjson_doc_free(old_doc);
    }

    time_t now = time(NULL);

    for (size_t i = 0; i < manifest->entry_count; i++) {
        const shim_entry_t *entry = &manifest->entries[i];
        yyjson_mut_val *obj = yyjson_mut_obj(doc);
        yyjson_mut_obj_add_str(doc, obj, "package", manifest->package);
        yyjson_mut_obj_add_str(doc, obj, "version", manifest->version);
        yyjson_mut_obj_add_str(doc, obj, "command", entry->name);
        yyjson_mut_obj_add_str(doc, obj, "target", entry->target_path);
        yyjson_mut_obj_add_str(doc, obj, "installPath", entry->install_path);
        yyjson_mut_obj_add_str(doc, obj, "libEnv", entry->lib_env);
        yyjson_mut_obj_add_str(doc, obj, "libPath", entry->lib_path);
        yyjson_mut_obj_add_int(doc, obj, "installedAt", (int64_t)now);

        yyjson_mut_val *env_arr = yyjson_mut_arr(doc);
        for (size_t e = 0; e < entry->env_count; e++) {
            yyjson_mut_val *env_obj = yyjson_mut_obj(doc);
            yyjson_mut_obj_add_str(doc, env_obj, "name", entry->envs[e].name);
            yyjson_mut_obj_add_str(doc, env_obj, "value", entry->envs[e].value);
            yyjson_mut_arr_append(env_arr, env_obj);
        }
        yyjson_mut_obj_add_val(doc, obj, "env", env_arr);

        yyjson_mut_val *dep_arr = yyjson_mut_arr(doc);
        for (size_t d = 0; d < entry->dep_count; d++) {
            yyjson_mut_arr_append(dep_arr, yyjson_mut_str(doc, entry->dependencies[d]));
        }
        yyjson_mut_obj_add_val(doc, obj, "dependencies", dep_arr);

        yyjson_mut_arr_append(shims_arr, obj);
    }

    if (yyjson_mut_write_file(paths->state_path, doc,
                              YYJSON_WRITE_PRETTY | YYJSON_WRITE_ESCAPE_SLASHES,
                              NULL, NULL) == false) {
        yyjson_mut_doc_free(doc);
        log_warn("Failed to persist shim metadata at %s.", paths->state_path);
        return -1;
    }

    yyjson_mut_doc_free(doc);
    return 0;
}

static int load_manifest_from_hnp(const char *hnp_path, shim_manifest_t *manifest, const char *fallback_pkg) {
    if (!manifest) return -1;
    memset(manifest, 0, sizeof(*manifest));

    char *buffer = NULL;
    size_t size = 0;
    int status = read_zip_entry(hnp_path, HORPKG_SHIM_MANIFEST_PATH, &buffer, &size);
    if (status != 0) {
        return status;
    }

    yyjson_doc *doc = yyjson_read(buffer, size, YYJSON_READ_ALLOW_COMMENTS | YYJSON_READ_ALLOW_TRAILING_COMMAS);
    free(buffer);
    if (!doc) {
        return -1;
    }

    yyjson_val *root = yyjson_doc_get_root(doc);
    const char *pkg = yyjson_get_str(yyjson_obj_get(root, "package"));
    const char *ver = yyjson_get_str(yyjson_obj_get(root, "version"));

    if (!pkg || !pkg[0]) {
        pkg = (fallback_pkg && fallback_pkg[0]) ? fallback_pkg : "unknown";
    }
    if (!ver) {
        ver = "0.0.0";
    }

    snprintf(manifest->package, sizeof(manifest->package), "%s", pkg);
    snprintf(manifest->version, sizeof(manifest->version), "%s", ver);

    yyjson_val *shims = yyjson_obj_get(root, "shims");
    if (!shims || !yyjson_is_arr(shims)) {
        yyjson_doc_free(doc);
        return -1;
    }

    size_t idx, max;
    yyjson_val *shim_val;
    yyjson_arr_foreach(shims, idx, max, shim_val) {
        if (manifest->entry_count >= HORPKG_MAX_SHIMS_PER_MANIFEST) {
            log_warn("Shim manifest from %s exceeds supported limit (%d).",
                     manifest->package, HORPKG_MAX_SHIMS_PER_MANIFEST);
            break;
        }

        const char *name = yyjson_get_str(yyjson_obj_get(shim_val, "name"));
        const char *target = yyjson_get_str(yyjson_obj_get(shim_val, "target"));
        if (!name || !target || !name[0] || !target[0]) {
            log_warn("Encountered incomplete shim definition; skipping.");
            continue;
        }
        if (strchr(name, '/')) {
            log_warn("Shim name '%s' contains '/'; skipping for safety.", name);
            continue;
        }

        shim_entry_t *entry = &manifest->entries[manifest->entry_count];
        memset(entry, 0, sizeof(*entry));

        snprintf(entry->name, sizeof(entry->name), "%s", name);
        snprintf(entry->target_path, sizeof(entry->target_path), "%s", target);

        const char *install_path = yyjson_get_str(yyjson_obj_get(shim_val, "installPath"));
        if (!install_path) {
            install_path = yyjson_get_str(yyjson_obj_get(shim_val, "path"));
        }
        if (!install_path || !install_path[0]) {
            snprintf(entry->install_path, sizeof(entry->install_path), "bin/%s", entry->name);
        } else {
            while (*install_path == '/') install_path++;
            snprintf(entry->install_path, sizeof(entry->install_path), "%s", install_path);
        }

        const char *lib_env = yyjson_get_str(yyjson_obj_get(shim_val, "libEnv"));
        const char *lib_path = yyjson_get_str(yyjson_obj_get(shim_val, "libPath"));
        if (!lib_env) lib_env = "";
        if (!lib_path) lib_path = "";
        snprintf(entry->lib_env, sizeof(entry->lib_env), "%s", lib_env);
        snprintf(entry->lib_path, sizeof(entry->lib_path), "%s", lib_path);

        yyjson_val *vars = yyjson_obj_get(shim_val, "variables");
        if (!vars) vars = yyjson_obj_get(shim_val, "env");
        size_t vidx, vmax;
        yyjson_val *k, *v;
        if (vars && yyjson_is_obj(vars)) {
            yyjson_obj_foreach(vars, vidx, vmax, k, v) {
                if (entry->env_count >= HORPKG_MAX_ENV_VARS) {
                    log_warn("Shim %s env vars exceed limit (%d).", entry->name, HORPKG_MAX_ENV_VARS);
                    break;
                }
                const char *key = yyjson_get_str(k);
                const char *val = yyjson_get_str(v);
                if (!key || !key[0] || !val) continue;
                snprintf(entry->envs[entry->env_count].name,
                         sizeof(entry->envs[entry->env_count].name), "%s", key);
                snprintf(entry->envs[entry->env_count].value,
                         sizeof(entry->envs[entry->env_count].value), "%s", val);
                entry->env_count++;
            }
        }

        yyjson_val *deps = yyjson_obj_get(shim_val, "dependencies");
        if (deps && yyjson_is_arr(deps)) {
            size_t didx, dmax;
            yyjson_val *dep;
            yyjson_arr_foreach(deps, didx, dmax, dep) {
                if (entry->dep_count >= HORPKG_MAX_DEPENDENCIES) {
                    log_warn("Shim %s dependency list truncated to %d.", entry->name, HORPKG_MAX_DEPENDENCIES);
                    break;
                }
                const char *dep_str = yyjson_get_str(dep);
                if (!dep_str) continue;
                snprintf(entry->dependencies[entry->dep_count],
                         sizeof(entry->dependencies[entry->dep_count]), "%s", dep_str);
                entry->dep_count++;
            }
        }

        manifest->entry_count++;
    }

    yyjson_doc_free(doc);
    return 0;
}

static void escape_c_string(FILE *fp, const char *value) {
    if (!value) return;
    for (const char *p = value; *p; ++p) {
        switch (*p) {
            case '\\': fputs("\\\\", fp); break;
            case '\"': fputs("\\\"", fp); break;
            case '\n': fputs("\\n", fp); break;
            case '\r': fputs("\\r", fp); break;
            case '\t': fputs("\\t", fp); break;
            default: fputc(*p, fp); break;
        }
    }
}

static int write_config_header(const shim_entry_t *entry, const char *config_path) {
    if (ensure_parent_dir(config_path) != 0) {
        return -1;
    }
    FILE *fp = fopen(config_path, "w");
    if (!fp) {
        return -1;
    }

    fprintf(fp, "#ifndef SHIM_CONFIG_H\n#define SHIM_CONFIG_H\n\n");
    fprintf(fp, "#define TARGET_COMMAND_PATH \"");
    escape_c_string(fp, entry->target_path);
    fprintf(fp, "\"\n");

    fprintf(fp, "#define LIB_PATH_ENV_VAR \"");
    escape_c_string(fp, entry->lib_env);
    fprintf(fp, "\"\n");

    fprintf(fp, "#define CUSTOM_LIB_PATH \"");
    escape_c_string(fp, entry->lib_path);
    fprintf(fp, "\"\n\n");

    fprintf(fp, "#define SHIM_ENV_VARS \\\n");
    if (entry->env_count == 0) {
        fprintf(fp, "    /* no custom env vars */\n");
    } else {
        for (size_t i = 0; i < entry->env_count; i++) {
            fprintf(fp, "    X(\"");
            escape_c_string(fp, entry->envs[i].name);
            fprintf(fp, "\", \"");
            escape_c_string(fp, entry->envs[i].value);
            fprintf(fp, "\") \\\n");
        }
        fprintf(fp, "    /* END_SHIM_ENV_VARS */\n");
    }

    fprintf(fp, "\n#endif\n");
    fclose(fp);
    return 0;
}

static int write_template_source(const char *template_path) {
    if (ensure_parent_dir(template_path) != 0) {
        return -1;
    }
    FILE *fp = fopen(template_path, "w");
    if (!fp) {
        return -1;
    }
    if (fputs(k_shim_template_source, fp) == EOF) {
        fclose(fp);
        return -1;
    }
    fclose(fp);
    return 0;
}

static int create_unique_build_dir(const shim_runtime_paths_t *paths, const char *shim_name, char *dir_out, size_t len) {
    long timestamp = (long)time(NULL);
    for (int attempt = 0; attempt < 5; attempt++) {
        if (snprintf(dir_out, len, "%s/%s_%ld_%d", paths->build_dir, shim_name, timestamp, attempt) >= (int)len) {
            continue;
        }
        if (ensure_dir_recursive(dir_out) == 0) {
            return 0;
        }
    }
    return -1;
}

static int compile_shim_binary(const shim_entry_t *entry,
                               const shim_runtime_paths_t *paths,
                               const shim_toolchain_t *tc,
                               shim_build_artifact_t *artifact) {
    if (!artifact) return -1;
    memset(artifact, 0, sizeof(*artifact));

    if (create_unique_build_dir(paths, entry->name, artifact->build_dir, sizeof(artifact->build_dir)) != 0) {
        log_error("Failed to prepare build directory for shim %s.", entry->name);
        return -1;
    }

    char config_path[PATH_MAX];
    if (snprintf(config_path, sizeof(config_path), "%s/config.h", artifact->build_dir) < 0) {
        return -1;
    }
    if (write_config_header(entry, config_path) != 0) {
        log_error("Failed to write config for shim %s.", entry->name);
        return -1;
    }

    char template_path[PATH_MAX];
    if (snprintf(template_path, sizeof(template_path), "%s/shim_template.c", artifact->build_dir) < 0) {
        return -1;
    }
    if (write_template_source(template_path) != 0) {
        log_error("Failed to write template source for shim %s.", entry->name);
        return -1;
    }

    if (snprintf(artifact->output_path, sizeof(artifact->output_path), "%s/%s.shim", artifact->build_dir, entry->name) < 0) {
        return -1;
    }

    char build_include[PATH_MAX];
    snprintf(build_include, sizeof(build_include), "%s", artifact->build_dir);

    const char *clang_path = tc->clang_path[0] ? tc->clang_path : "clang";
    const bool has_target = tc->target[0] != '\0';
    const bool has_sysroot = tc->sysroot[0] != '\0';

    const char *argv[24];
    size_t argc = 0;
    argv[argc++] = clang_path;
    if (tc->use_zig_driver) {
        argv[argc++] = "cc";
    }
    argv[argc++] = "-std=c11";
    argv[argc++] = "-O2";
    argv[argc++] = "-Wall";
    argv[argc++] = "-Wextra";
    argv[argc++] = "-I";
    argv[argc++] = build_include;
    if (has_target) {
        argv[argc++] = "-target";
        argv[argc++] = tc->target;
    }
    if (has_sysroot) {
        argv[argc++] = "--sysroot";
        argv[argc++] = tc->sysroot;
    }
    argv[argc++] = "-o";
    argv[argc++] = artifact->output_path;
    argv[argc++] = template_path;
    argv[argc] = NULL;

    log_info("Compiling shim '%s' using %s.", entry->name, clang_path);

    pid_t pid;
    int spawn_status;
    if (strchr(clang_path, '/')) {
        spawn_status = posix_spawn(&pid, clang_path, NULL, NULL, (char * const *)argv, environ);
    } else {
        spawn_status = posix_spawnp(&pid, clang_path, NULL, NULL, (char * const *)argv, environ);
    }
    if (spawn_status != 0) {
        log_error("Failed to launch compiler %s for shim %s (errno=%d).", clang_path, entry->name, spawn_status);
        return -1;
    }

    int status = 0;
    if (waitpid(pid, &status, 0) == -1) {
        log_error("Failed to wait for compiler process (shim %s).", entry->name);
        return -1;
    }
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        log_error("Compiler exited with code %d for shim %s.", WEXITSTATUS(status), entry->name);
        return -1;
    }

    return 0;
}

static void remove_dir_recursive(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) {
        unlink(path);
        return;
    }

    struct dirent *entry;
    char buffer[PATH_MAX];

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        if (snprintf(buffer, sizeof(buffer), "%s/%s", path, entry->d_name) < 0) {
            continue;
        }
        bool is_dir = false;
#ifdef DT_DIR
        if (entry->d_type == DT_DIR) {
            is_dir = true;
        } else
#endif
#ifdef DT_UNKNOWN
        if (entry->d_type == DT_UNKNOWN) {
            struct stat st = {0};
            if (stat(buffer, &st) == 0 && S_ISDIR(st.st_mode)) {
                is_dir = true;
            }
        } else
#endif
        {
            struct stat st = {0};
            if (stat(buffer, &st) == 0 && S_ISDIR(st.st_mode)) {
                is_dir = true;
            }
        }

        if (is_dir) {
            remove_dir_recursive(buffer);
        } else {
            unlink(buffer);
        }
    }

    closedir(dir);
    rmdir(path);
}

static void cleanup_artifact(const shim_build_artifact_t *artifact) {
    if (!artifact) return;
    remove_dir_recursive(artifact->build_dir);
}

static int add_file_to_zip(zip_t *archive, const char *entry_name, const char *file_path) {
    zip_source_t *src = zip_source_file(archive, file_path, 0, 0);
    if (!src) {
        log_error("Failed to add %s to archive: %s", entry_name, zip_strerror(archive));
        return -1;
    }

    zip_int64_t index = zip_name_locate(archive, entry_name, ZIP_FL_ENC_UTF_8);
    if (index >= 0) {
        if (zip_file_replace(archive, index, src, ZIP_FL_ENC_UTF_8) != 0) {
            log_error("Failed to replace %s in archive: %s", entry_name, zip_strerror(archive));
            zip_source_free(src);
            return -1;
        }
    } else {
        if (zip_file_add(archive, entry_name, src, ZIP_FL_ENC_UTF_8) < 0) {
            log_error("Failed to add %s to archive: %s", entry_name, zip_strerror(archive));
            zip_source_free(src);
            return -1;
        }
    }
    return 0;
}

static int add_shims_to_hnp(const shim_manifest_t *manifest,
                            const shim_runtime_paths_t *paths,
                            const shim_build_artifact_t *artifacts,
                            size_t artifact_count) {
    if (artifact_count == 0) {
        return 0;
    }

    int err = 0;
    zip_t *archive = zip_open(paths->hnp_path, ZIP_CHECKCONS, &err);
    if (!archive) {
        log_error("Failed to open %s for update (zip err=%d).", paths->hnp_path, err);
        return -1;
    }

    for (size_t i = 0; i < artifact_count; i++) {
        const shim_entry_t *entry = &manifest->entries[i];
        char relative_path[PATH_MAX];
        const char *install_rel = entry->install_path[0] ? entry->install_path : "bin";

        if (strncmp(install_rel, "sysroot/", 8) == 0) {
            snprintf(relative_path, sizeof(relative_path), "%s", install_rel + 8);
        } else {
            snprintf(relative_path, sizeof(relative_path), "%s", install_rel);
        }

        while (relative_path[0] == '/') {
            memmove(relative_path, relative_path + 1, strlen(relative_path));
        }

        char archive_entry[PATH_MAX];
        if (snprintf(archive_entry, sizeof(archive_entry), "sysroot/%s", relative_path) < 0) {
            zip_close(archive);
            return -1;
        }

        log_info("Embedding shim '%s' at %s inside %s.", entry->name, archive_entry, HORPKG_RUNTIME_HNP_NAME);
        if (add_file_to_zip(archive, archive_entry, artifacts[i].output_path) != 0) {
            zip_close(archive);
            return -1;
        }
    }

    if (zip_close(archive) != 0) {
        log_error("Failed to finalize %s with new shim payloads.", HORPKG_RUNTIME_HNP_NAME);
        return -1;
    }

    return 0;
}

static int replace_hnp_in_hap(const shim_runtime_paths_t *paths) {
    char entry_path[PATH_MAX];
    if (runtime_hnp_entry_name(paths, entry_path, sizeof(entry_path)) != 0) {
        log_error("Unable to locate %s entry inside runtime HAP.", HORPKG_RUNTIME_HNP_NAME);
        return -1;
    }

    int err = 0;
    zip_t *hap = zip_open(paths->hap_path, ZIP_CHECKCONS, &err);
    if (!hap) {
        log_error("Failed to open runtime HAP for modification (zip err=%d).", err);
        return -1;
    }

    zip_source_t *src = zip_source_file(hap, paths->hnp_path, 0, 0);
    if (!src) {
        log_error("Failed to create zip source for %s.", HORPKG_RUNTIME_HNP_NAME);
        zip_close(hap);
        return -1;
    }

    zip_int64_t index = zip_name_locate(hap, entry_path, ZIP_FL_ENC_UTF_8);
    if (index >= 0) {
        if (zip_file_replace(hap, index, src, ZIP_FL_ENC_UTF_8) != 0) {
            log_error("Failed to replace %s inside runtime HAP.", entry_path);
            zip_source_free(src);
            zip_close(hap);
            return -1;
        }
    } else {
        if (zip_file_add(hap, entry_path, src, ZIP_FL_ENC_UTF_8) < 0) {
            log_error("Failed to append %s into runtime HAP.", entry_path);
            zip_source_free(src);
            zip_close(hap);
            return -1;
        }
    }

    if (zip_close(hap) != 0) {
        log_error("Failed to finalize runtime HAP after shim injection.");
        return -1;
    }
    return 0;
}

static int install_runtime_bundle(const shim_runtime_paths_t *paths) {
    char bundle_name[256];
    if (hap_parser_get_bundle_name(paths->hap_path, bundle_name, sizeof(bundle_name)) != 0) {
        log_error("Failed to parse bundleName from patched runtime HAP.");
        return -1;
    }

    if (sign_hap(paths->hap_path, bundle_name, paths->signed_hap_path) != 0) {
        log_error("Failed to sign patched runtime HAP for %s.", bundle_name);
        return -1;
    }

    if (hdc_install_hap(paths->signed_hap_path, bundle_name, NULL) != 0) {
        unlink(paths->signed_hap_path);
        log_error("Device installation failed for %s.", bundle_name);
        return -1;
    }

    unlink(paths->signed_hap_path);
    return 0;
}
