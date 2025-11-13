#define _POSIX_C_SOURCE 200809L

#include "core_hnp_installer.h"

#include "config.h"
#include "hap_parser.h"
#include "hdc.h"
#include "install.h"
#include "logger.h"
#include "utils.h"
#include "download.h"

#include <yyjson.h>
#include <zip.h>

#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <spawn.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

extern char **environ;

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

typedef struct {
    char core_dir[PATH_MAX];
    char tmp_dir[PATH_MAX];
    char hap_path[PATH_MAX];
    char state_path[PATH_MAX];
    char signed_hap_path[PATH_MAX];
    char hnp_root[PATH_MAX];
    char abi[32];
} core_paths_t;

typedef struct {
    char package[128];
    char version[64];
    char file_name[PATH_MAX];
    char entry_path[PATH_MAX];
    char source_url[1024];
    char source_sha256[129];
    int64_t installed_at;
} core_state_entry_t;

static int core_paths_init(core_paths_t *paths);
static int ensure_core_hap_base(const core_paths_t *paths);
static int reset_core_hap(const core_paths_t *paths);
static int copy_file(const char *src, const char *dst);
static const char *current_abi(void);
static int append_hnp_to_hap(const core_paths_t *paths,
                             const char *hnp_path,
                             const char *entry_path);
static int rewrite_module_json(zip_t *hap, const core_state_entry_t *entries, size_t count);
static int install_core_bundle(const core_paths_t *paths, const char *bundle_name);
static int core_state_load(const core_paths_t *paths, core_state_entry_t **entries_out, size_t *count_out);
static int core_state_save(const core_paths_t *paths, const core_state_entry_t *entries, size_t count);
static int ensure_local_entry_file(const core_paths_t *paths, core_state_entry_t *entry);
static int rebuild_core_hap(const core_paths_t *paths, core_state_entry_t *entries, size_t count);
static int ensure_dir_recursive(const char *path);
static int ensure_parent_dir(const char *path);
static int remove_dir_recursive(const char *path);
static int recreate_directory(const char *path);
static int extract_hap_contents(const char *hap_path, const char *dest_dir);
static int run_command(char *const argv[]);
static int find_in_path(const char *name, char *out_path, size_t out_size);
static int locate_restool(char *out_path, size_t out_size);
static int zip_add_or_replace(zip_t *hap, const char *entry_name, const char *file_path);
static int replace_hap_entry(const char *hap_path, const char *entry_name, const char *file_path);
static int regenerate_resources_index(const core_paths_t *paths, const char *bundle_name);

static void safe_copy_string(char *dst, size_t dst_size, const char *src) {
    if (!dst || dst_size == 0) return;
    if (src) {
        strncpy(dst, src, dst_size - 1);
        dst[dst_size - 1] = '\0';
    } else {
        dst[0] = '\0';
    }
}

static bool packages_array_contains(yyjson_mut_val *packages, const char *file_name) {
    if (!packages || !file_name || !file_name[0]) {
        return false;
    }
    size_t idx, max;
    yyjson_mut_val *item;
    yyjson_mut_arr_foreach(packages, idx, max, item) {
        const char *pkg = yyjson_mut_get_str(yyjson_mut_obj_get(item, "package"));
        if (pkg && strcmp(pkg, file_name) == 0) {
            return true;
        }
    }
    return false;
}

static int ensure_dir_recursive(const char *path) {
    if (!path || !path[0]) {
        return -1;
    }
    char buffer[PATH_MAX];
    if (snprintf(buffer, sizeof(buffer), "%s", path) < 0) {
        return -1;
    }
    for (char *p = buffer + 1; *p; ++p) {
        if (*p == '/') {
            *p = '\0';
            if (mkdir(buffer, 0755) != 0 && errno != EEXIST) {
                return -1;
            }
            *p = '/';
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

static int remove_dir_recursive(const char *path) {
    if (!path || !path[0]) {
        return -1;
    }
    DIR *dir = opendir(path);
    if (!dir) {
        if (errno == ENOENT) {
            return 0;
        }
        return -1;
    }
    struct dirent *entry;
    int status = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        char child[PATH_MAX];
        if (snprintf(child, sizeof(child), "%s/%s", path, entry->d_name) < 0) {
            status = -1;
            break;
        }
        struct stat st;
        if (lstat(child, &st) != 0) {
            status = -1;
            break;
        }
        if (S_ISDIR(st.st_mode)) {
            if (remove_dir_recursive(child) != 0) {
                status = -1;
                break;
            }
        } else {
            if (unlink(child) != 0) {
                status = -1;
                break;
            }
        }
    }
    closedir(dir);
    if (status == 0 && rmdir(path) != 0) {
        status = -1;
    }
    return status;
}

static int recreate_directory(const char *path) {
    if (remove_dir_recursive(path) != 0) {
        return -1;
    }
    return create_dir_if_not_exists(path);
}

static int extract_hap_contents(const char *hap_path, const char *dest_dir) {
    if (recreate_directory(dest_dir) != 0) {
        return -1;
    }
    int err = 0;
    zip_t *archive = zip_open(hap_path, ZIP_RDONLY, &err);
    if (!archive) {
        log_error("Failed to open %s for extraction (zip err=%d).", hap_path, err);
        return -1;
    }

    zip_int64_t count = zip_get_num_entries(archive, 0);
    for (zip_int64_t i = 0; i < count; i++) {
        const char *name = zip_get_name(archive, i, 0);
        if (!name) {
            continue;
        }
        char out_path[PATH_MAX];
        int written = snprintf(out_path, sizeof(out_path), "%s/%s", dest_dir, name);
        if (written < 0 || (size_t)written >= sizeof(out_path)) {
            zip_close(archive);
            return -1;
        }
        size_t name_len = strlen(name);
        if (name_len > 0 && name[name_len - 1] == '/') {
            if (ensure_dir_recursive(out_path) != 0) {
                zip_close(archive);
                return -1;
            }
            continue;
        }

        zip_file_t *zf = zip_fopen_index(archive, i, 0);
        if (!zf) {
            zip_close(archive);
            return -1;
        }
        if (ensure_parent_dir(out_path) != 0) {
            zip_fclose(zf);
            zip_close(archive);
            return -1;
        }
        FILE *out = fopen(out_path, "wb");
        if (!out) {
            zip_fclose(zf);
            zip_close(archive);
            return -1;
        }
        char buffer[16384];
        zip_int64_t read_bytes;
        while ((read_bytes = zip_fread(zf, buffer, sizeof(buffer))) > 0) {
            if (fwrite(buffer, 1, (size_t)read_bytes, out) != (size_t)read_bytes) {
                fclose(out);
                zip_fclose(zf);
                zip_close(archive);
                return -1;
            }
        }
        fclose(out);
        zip_fclose(zf);
        if (read_bytes < 0) {
            zip_close(archive);
            return -1;
        }
    }
    zip_close(archive);
    return 0;
}

static int run_command(char *const argv[]) {
    if (!argv || !argv[0]) {
        return -1;
    }

    int pipefd[2];
    if (pipe(pipefd) != 0) {
        log_error("Failed to create pipe for %s: %s", argv[0], strerror(errno));
        return -1;
    }

    pid_t pid = fork();
    if (pid == -1) {
        log_error("Failed to fork for %s: %s", argv[0], strerror(errno));
        close(pipefd[0]);
        close(pipefd[1]);
        return -1;
    }

    if (pid == 0) {
        // child
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[0]);
        close(pipefd[1]);
        execvp(argv[0], argv);
        _exit(127);
    }

    close(pipefd[1]);
    FILE *stream = fdopen(pipefd[0], "r");
    if (!stream) {
        log_error("Failed to read output from %s", argv[0]);
        close(pipefd[0]);
        waitpid(pid, NULL, 0);
        return -1;
    }

    char line[512];
    while (fgets(line, sizeof(line), stream)) {
        size_t len = strlen(line);
        if (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
            line[len - 1] = '\0';
        }
        log_info("[restool] %s", line);
    }
    fclose(stream);

    int status = 0;
    if (waitpid(pid, &status, 0) == -1) {
        log_error("Failed to wait for %s: %s", argv[0], strerror(errno));
        return -1;
    }
    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        log_error("%s exited with status %d", argv[0], status);
        return -1;
    }
    return 0;
}

static int find_in_path(const char *name, char *out_path, size_t out_size) {
    if (!name || !out_path || out_size == 0) {
        return -1;
    }
    const char *path_env = getenv("PATH");
    if (!path_env) {
        return -1;
    }
    char *paths = strdup(path_env);
    if (!paths) {
        return -1;
    }
    int result = -1;
    char *saveptr = NULL;
    for (char *token = strtok_r(paths, ":", &saveptr); token; token = strtok_r(NULL, ":", &saveptr)) {
        char candidate[PATH_MAX];
        int written = snprintf(candidate, sizeof(candidate), "%s/%s", token, name);
        if (written < 0 || (size_t)written >= sizeof(candidate)) {
            continue;
        }
        if (access(candidate, X_OK) == 0) {
            if ((size_t)written < out_size) {
                snprintf(out_path, out_size, "%s", candidate);
                result = 0;
            }
            break;
        }
    }
    free(paths);
    return result;
}

static int locate_restool(char *out_path, size_t out_size) {
    if (!out_path || out_size == 0) {
        return -1;
    }
    const char *override = getenv("HORPKG_RESTOOL");
    if (override && override[0]) {
        if (access(override, X_OK) == 0) {
            snprintf(out_path, out_size, "%s", override);
            return 0;
        }
        log_error("HORPKG_RESTOOL is set but not executable: %s", override);
        return -1;
    }

    char exec_dir[PATH_MAX];
    if (horpkg_self_dir(exec_dir, sizeof(exec_dir)) == 0) {
        char candidate[PATH_MAX];
        int written = snprintf(candidate, sizeof(candidate), "%s/restool", exec_dir);
        if (written > 0 && (size_t)written < sizeof(candidate) && access(candidate, X_OK) == 0) {
            snprintf(out_path, out_size, "%s", candidate);
            return 0;
        }
    }

    if (find_in_path("restool", out_path, out_size) == 0) {
        return 0;
    }

    log_error("restool executable not found. Ensure it is in PATH or set HORPKG_RESTOOL.");
    return -1;
}

static int replace_hap_entry(const char *hap_path, const char *entry_name, const char *file_path) {
    int err = 0;
    zip_t *hap = zip_open(hap_path, ZIP_CHECKCONS, &err);
    if (!hap) {
        log_error("Failed to open %s for updating %s (zip err=%d).", hap_path, entry_name, err);
        return -1;
    }
    int rc = zip_add_or_replace(hap, entry_name, file_path);
    if (zip_close(hap) != 0) {
        return -1;
    }
    return rc;
}

static int regenerate_resources_index(const core_paths_t *paths, const char *bundle_name) {
    if (!paths || !bundle_name || !bundle_name[0]) {
        return -1;
    }
    char restool_path[PATH_MAX];
    if (locate_restool(restool_path, sizeof(restool_path)) != 0) {
        return -1;
    }

    char extract_dir[PATH_MAX];
    char restool_out[PATH_MAX];
    snprintf(extract_dir, sizeof(extract_dir), "%s/restool_extract", paths->tmp_dir);
    snprintf(restool_out, sizeof(restool_out), "%s/restool_out", paths->tmp_dir);
    log_info("restool input dir: %s", extract_dir);
    log_info("restool output dir: %s", restool_out);

    if (extract_hap_contents(paths->hap_path, extract_dir) != 0) {
        log_error("Failed to extract %s for resources.index regeneration.", paths->hap_path);
        return -1;
    }
    if (recreate_directory(restool_out) != 0) {
        return -1;
    }

    char module_json_path[PATH_MAX];
    snprintf(module_json_path, sizeof(module_json_path), "%s/module.json", extract_dir);
    if (access(module_json_path, R_OK) != 0) {
        log_error("module.json missing at %s", module_json_path);
        return -1;
    }
    char header_path[PATH_MAX];
    snprintf(header_path, sizeof(header_path), "%s/ResourceTable.txt", restool_out);

    log_info("Regenerating resources.index using restool.");
    char *argv[] = {
        restool_path,
        "-i", extract_dir,
        "-j", module_json_path,
        "-p", (char *)bundle_name,
        "-o", restool_out,
        "-r", header_path,
        "-f",
        NULL
    };
    if (run_command(argv) != 0) {
        log_error("restool failed while rebuilding resources.index.");
        return -1;
    }

    char new_index_path[PATH_MAX];
    snprintf(new_index_path, sizeof(new_index_path), "%s/resources.index", restool_out);
    if (access(new_index_path, R_OK) != 0) {
        log_error("restool did not produce %s", new_index_path);
        return -1;
    }
    log_info("restool produced resources.index at %s", new_index_path);

    if (replace_hap_entry(paths->hap_path, "resources.index", new_index_path) != 0) {
        return -1;
    }

    return 0;
}

int core_install_hnp(const char *package_name, const char *package_version, const char *hnp_path, const char *source_url, const char *source_sha256) {
    if (!hnp_path || !hnp_path[0]) {
        log_error("Invalid HNP path for core installation.");
        return -1;
    }
    if (!package_name || !package_name[0]) {
        log_error("Package name is required for core installation.");
        return -1;
    }

    const char *pkg_name = package_name ? package_name : "unknown";
    const char *pkg_version = (package_version && package_version[0]) ? package_version : "0.0.0";
    log_info("core_install_hnp: package=%s version=%s hnp=%s", pkg_name, pkg_version, hnp_path);

    core_paths_t paths = {0};
    if (core_paths_init(&paths) != 0) {
        log_error("Failed to prepare local workspace for org.horpkg.core.");
        return -1;
    }
    log_debug("core_install_hnp: workspace initialized at %s", paths.core_dir);

    if (ensure_core_hap_base(&paths) != 0) {
        log_error("Unable to seed org.horpkg.core.hap.");
        return -1;
    }
    log_debug("core_install_hnp: ensured base HAP at %s", paths.hap_path);

    core_state_entry_t *entries = NULL;
    size_t entry_count = 0;
    if (core_state_load(&paths, &entries, &entry_count) != 0) {
        log_error("Failed to load core state.");
        return -1;
    }
    log_debug("core_install_hnp: loaded %zu core state entries from %s", entry_count, paths.state_path);

    const char *basename = strrchr(hnp_path, '/');
    basename = basename ? basename + 1 : hnp_path;

    char local_hnp_path[PATH_MAX];
    snprintf(local_hnp_path, sizeof(local_hnp_path), "%s/%s", paths.hnp_root, basename);
    if (copy_file(hnp_path, local_hnp_path) != 0) {
        log_error("Failed to copy HNP into core workspace.");
        free(entries);
        return -1;
    }
    log_info("core_install_hnp: copied %s -> %s", hnp_path, local_hnp_path);

    log_debug("core_install_hnp: scanning %zu stored entries for %s", entry_count, pkg_name);
    size_t idx = SIZE_MAX;
    log_info("core_install_hnp: log_debug1");
    for (size_t i = 0; i < entry_count; i++) {
        log_info("core_install_hnp: log_debug2");
        if (strcmp(entries[i].package, package_name) == 0) {
            log_info("core_install_hnp: log_debug3");
            idx = i;
            break;
        }
        log_info("core_install_hnp: log_debug4");
    }
    log_info("core_install_hnp: log_debug5");
    if (idx == SIZE_MAX) {
        log_info("core_install_hnp: log_debug6");
        size_t new_count = entry_count + 1;
        core_state_entry_t *resized = realloc(entries, new_count * sizeof(core_state_entry_t));
        if (!resized) {
            log_info("core_install_hnp: log_debug7");
            free(entries);
            log_error("Failed to expand core state entries.");
            return -1;
        }
        entries = resized;
        idx = entry_count;
        entry_count = new_count;
        log_info("core_install_hnp: log_debug8");
    }
    log_debug("core_install_hnp: writing package entry at index %zu (entries=%zu)", idx, entry_count);

    core_state_entry_t *entry = &entries[idx];
    log_debug("core_install_hnp: entry pointer %p", (void*)entry);
    safe_copy_string(entry->package, sizeof(entry->package), package_name);
    safe_copy_string(entry->version, sizeof(entry->version), (package_version && package_version[0]) ? package_version : "0.0.0");
    safe_copy_string(entry->file_name, sizeof(entry->file_name), basename);
    snprintf(entry->entry_path, sizeof(entry->entry_path), "hnp/%s/%s", paths.abi, basename);
    safe_copy_string(entry->source_url, sizeof(entry->source_url), source_url);
    safe_copy_string(entry->source_sha256, sizeof(entry->source_sha256), source_sha256);
    entry->installed_at = (int64_t)time(NULL);

    log_debug("core_install_hnp: calling core_state_save (entries=%zu)", entry_count);
    if (core_state_save(&paths, entries, entry_count) != 0) {
        free(entries);
        log_error("Failed to persist core state.");
        return -1;
    }
    log_debug("core_install_hnp: core state persisted with %zu entr%s", entry_count, entry_count == 1 ? "y" : "ies");

    int rebuild_result = rebuild_core_hap(&paths, entries, entry_count);
    free(entries);
    if (rebuild_result == 0) {
        print_success_fmt("Installed %s into org.horpkg.core.", package_name);
        log_info("core_install_hnp: rebuild succeeded for %s (entries=%zu)", package_name, entry_count);
    }
    return rebuild_result;
}

// --------------------- Helpers ---------------------

static int core_paths_init(core_paths_t *paths) {
    if (!paths) return -1;
    memset(paths, 0, sizeof(*paths));

    char *core_dir = get_config_path("core");
    if (!core_dir) {
        return -1;
    }
    if (create_dir_if_not_exists(core_dir) != 0) {
        free(core_dir);
        return -1;
    }
    snprintf(paths->core_dir, sizeof(paths->core_dir), "%s", core_dir);
    free(core_dir);

    snprintf(paths->tmp_dir, sizeof(paths->tmp_dir), "%s/tmp", paths->core_dir);
    create_dir_if_not_exists(paths->tmp_dir);

    snprintf(paths->hap_path, sizeof(paths->hap_path), "%s/org.horpkg.core.hap", paths->core_dir);
    snprintf(paths->state_path, sizeof(paths->state_path), "%s/core_state.json", paths->core_dir);
    snprintf(paths->signed_hap_path, sizeof(paths->signed_hap_path), "%s/org.horpkg.core.signed.hap", paths->core_dir);

    snprintf(paths->abi, sizeof(paths->abi), "%s", current_abi());

    char hnp_base[PATH_MAX];
    snprintf(hnp_base, sizeof(hnp_base), "%s/hnp", paths->core_dir);
    create_dir_if_not_exists(hnp_base);

    snprintf(paths->hnp_root, sizeof(paths->hnp_root), "%s/%s", hnp_base, paths->abi);
    create_dir_if_not_exists(paths->hnp_root);
    return 0;
}

static int ensure_core_hap_base(const core_paths_t *paths) {
    if (access(paths->hap_path, F_OK) == 0) {
        return 0;
    }
    char resource_path[PATH_MAX];
    if (horpkg_find_resource("org.horpkg.core.hap", resource_path, sizeof(resource_path)) != 0) {
        log_error("Bundled org.horpkg.core.hap not found inside resources.");
        return -1;
    }
    log_info("Seeding org.horpkg.core.hap from %s", resource_path);
    int rc = copy_file(resource_path, paths->hap_path);
    if (rc == 0) {
        log_debug("ensure_core_hap_base: seeded runtime HAP at %s", paths->hap_path);
    }
    return rc;
}

static int reset_core_hap(const core_paths_t *paths) {
    char resource_path[PATH_MAX];
    if (horpkg_find_resource("org.horpkg.core.hap", resource_path, sizeof(resource_path)) != 0) {
        log_error("Bundled org.horpkg.core.hap not found when resetting.");
        return -1;
    }
    int rc = copy_file(resource_path, paths->hap_path);
    if (rc == 0) {
        log_debug("reset_core_hap: restored base HAP from %s", resource_path);
    }
    return rc;
}

static int copy_file(const char *src, const char *dst) {
    FILE *in = fopen(src, "rb");
    if (!in) {
        log_error("Failed to open %s for reading.", src);
        return -1;
    }
    FILE *out = fopen(dst, "wb");
    if (!out) {
        fclose(in);
        log_error("Failed to open %s for writing.", dst);
        return -1;
    }

    char buffer[16384];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), in)) > 0) {
        if (fwrite(buffer, 1, bytes, out) != bytes) {
            fclose(in);
            fclose(out);
            log_error("I/O error while copying %s -> %s", src, dst);
            return -1;
        }
    }

    fclose(in);
    fclose(out);
    return 0;
}

static const char *current_abi(void) {
    const char *abi = getenv("OHOS_ABI");
    if (abi && abi[0]) {
        return abi;
    }
    return "arm64-v8a";
}

static int zip_add_or_replace(zip_t *hap, const char *entry_name, const char *file_path) {
    zip_source_t *src = zip_source_file(hap, file_path, 0, 0);
    if (!src) {
        log_error("Failed to create zip source for %s -> %s", file_path, entry_name);
        return -1;
    }
    zip_int64_t idx = zip_name_locate(hap, entry_name, ZIP_FL_ENC_UTF_8);
    int rc;
    if (idx >= 0) {
        rc = zip_file_replace(hap, idx, src, ZIP_FL_ENC_UTF_8);
    } else {
        rc = (zip_file_add(hap, entry_name, src, ZIP_FL_ENC_UTF_8) < 0) ? -1 : 0;
    }
    if (rc != 0) {
        log_error("Failed to add %s into core HAP: %s", entry_name, zip_strerror(hap));
        zip_source_free(src);
        return -1;
    }
    return 0;
}

static int append_hnp_to_hap(const core_paths_t *paths,
                             const char *hnp_path,
                             const char *entry_path) {
    int err = 0;
    zip_t *hap = zip_open(paths->hap_path, ZIP_CHECKCONS, &err);
    if (!hap) {
        log_error("Failed to open %s (zip err=%d).", paths->hap_path, err);
        return -1;
    }

    if (zip_add_or_replace(hap, entry_path, hnp_path) != 0) {
        zip_close(hap);
        return -1;
    }

    if (zip_close(hap) != 0) {
        log_error("Failed to finalize patched org.horpkg.core.hap.");
        return -1;
    }
    return 0;
}

static int rewrite_module_json(zip_t *hap, const core_state_entry_t *entries, size_t count) {
    zip_int64_t idx = zip_name_locate(hap, "module.json", ZIP_FL_ENC_UTF_8);
    if (idx < 0) {
        log_error("module.json not found inside org.horpkg.core.hap.");
        return -1;
    }

    struct zip_stat st;
    zip_stat_init(&st);
    if (zip_stat_index(hap, idx, 0, &st) != 0) {
        log_error("Failed to stat module.json inside core hap.");
        return -1;
    }

    zip_file_t *zf = zip_fopen_index(hap, idx, 0);
    if (!zf) {
        log_error("Failed to open module.json for reading.");
        return -1;
    }

    char *buffer = malloc(st.size + 1);
    if (!buffer) {
        zip_fclose(zf);
        return -1;
    }

    zip_int64_t read_bytes = zip_fread(zf, buffer, st.size);
    zip_fclose(zf);
    if (read_bytes != (zip_int64_t)st.size) {
        free(buffer);
        log_error("Failed to read module.json completely.");
        return -1;
    }
    buffer[st.size] = '\0';

    yyjson_read_flag read_flag = YYJSON_READ_ALLOW_COMMENTS | YYJSON_READ_ALLOW_TRAILING_COMMAS;
    yyjson_doc *doc = yyjson_read(buffer, st.size, read_flag);
    free(buffer);
    if (!doc) {
        log_error("Failed to parse module.json.");
        return -1;
    }

    yyjson_mut_doc *mut_doc = yyjson_doc_mut_copy(doc, NULL);
    yyjson_doc_free(doc);
    if (!mut_doc) {
        log_error("Failed to create mutable JSON document for module.json.");
        return -1;
    }

    yyjson_mut_val *root = yyjson_mut_doc_get_root(mut_doc);
    if (!root) {
        yyjson_mut_doc_free(mut_doc);
        return -1;
    }

    yyjson_mut_val *module = yyjson_mut_obj_get(root, "module");
    if (!module || !yyjson_mut_is_obj(module)) {
        module = yyjson_mut_obj(mut_doc);
        yyjson_mut_obj_add_val(mut_doc, root, "module", module);
    }

    yyjson_mut_val *packages = yyjson_mut_arr(mut_doc);

    for (size_t i = 0; i < count; i++) {
        if (!entries[i].file_name[0]) {
            continue;
        }
        if (packages_array_contains(packages, entries[i].file_name)) {
            continue;
        }
        yyjson_mut_val *new_pkg = yyjson_mut_obj(mut_doc);
        yyjson_mut_obj_add_str(mut_doc, new_pkg, "package", entries[i].file_name);
        yyjson_mut_obj_add_str(mut_doc, new_pkg, "type", "public");
        yyjson_mut_arr_append(packages, new_pkg);
    }

    yyjson_mut_obj_remove_keyn(module, "hnpPackages", strlen("hnpPackages"));
    yyjson_mut_obj_add_val(mut_doc, module, "hnpPackages", packages);

    size_t out_len = 0;
    yyjson_write_flag write_flag = YYJSON_WRITE_PRETTY | YYJSON_WRITE_ESCAPE_SLASHES;
    char *out = yyjson_mut_write(mut_doc, write_flag, &out_len);
    yyjson_mut_doc_free(mut_doc);
    if (!out) {
        log_error("Failed to serialize updated module.json.");
        return -1;
    }

    zip_source_t *src = zip_source_buffer(hap, out, out_len, 1);
    if (!src) {
        free(out);
        log_error("Failed to create zip source for module.json.");
        return -1;
    }

    if (zip_file_replace(hap, idx, src, ZIP_FL_ENC_UTF_8) != 0) {
        log_error("Failed to replace module.json inside core hap.");
        zip_source_free(src);
        return -1;
    }

    return 0;
}

static int core_state_load(const core_paths_t *paths, core_state_entry_t **entries_out, size_t *count_out) {
    if (!entries_out || !count_out) return -1;
    *entries_out = NULL;
    *count_out = 0;

    log_debug("core_state_load: looking for %s", paths->state_path);

    if (access(paths->state_path, F_OK) != 0) {
        log_debug("core_state_load: state file missing, nothing to load.");
        return 0;
    }

    yyjson_read_flag flags = YYJSON_READ_ALLOW_COMMENTS | YYJSON_READ_ALLOW_TRAILING_COMMAS;
    yyjson_doc *doc = yyjson_read_file(paths->state_path, flags, NULL, NULL);
    if (!doc) {
        return -1;
    }

    yyjson_val *root = yyjson_doc_get_root(doc);
    yyjson_val *arr = yyjson_obj_get(root, "hnps");
    if (!arr || !yyjson_is_arr(arr)) {
        yyjson_doc_free(doc);
        return 0;
    }

    size_t expected = yyjson_arr_size(arr);
    if (expected == 0) {
        yyjson_doc_free(doc);
        return 0;
    }

    core_state_entry_t *entries = calloc(expected, sizeof(core_state_entry_t));
    if (!entries) {
        yyjson_doc_free(doc);
        return -1;
    }

    size_t idx = 0, max = 0;
    yyjson_val *item;
    yyjson_arr_foreach(arr, idx, max, item) {
        const char *pkg = yyjson_get_str(yyjson_obj_get(item, "package"));
        if (!pkg || !pkg[0]) {
            continue;
        }
        core_state_entry_t *entry = &entries[*count_out];
        safe_copy_string(entry->package, sizeof(entry->package), pkg);
        safe_copy_string(entry->version, sizeof(entry->version), yyjson_get_str(yyjson_obj_get(item, "version")));
        safe_copy_string(entry->file_name, sizeof(entry->file_name), yyjson_get_str(yyjson_obj_get(item, "fileName")));
        safe_copy_string(entry->entry_path, sizeof(entry->entry_path), yyjson_get_str(yyjson_obj_get(item, "entry")));
        safe_copy_string(entry->source_url, sizeof(entry->source_url), yyjson_get_str(yyjson_obj_get(item, "sourceUrl")));
        safe_copy_string(entry->source_sha256, sizeof(entry->source_sha256), yyjson_get_str(yyjson_obj_get(item, "sourceSha256")));
        entry->installed_at = yyjson_get_int(yyjson_obj_get(item, "installedAt"));
        (*count_out)++;
        if (*count_out == expected) break;
    }

    log_debug("core_state_load: parsed %zu/%zu expected entries", *count_out, expected);

    *entries_out = entries;
    yyjson_doc_free(doc);
    return 0;
}

static int core_state_save(const core_paths_t *paths, const core_state_entry_t *entries, size_t count) {
    log_debug("core_state_save: storing %zu entries to %s", count, paths->state_path);
    yyjson_mut_doc *doc = yyjson_mut_doc_new(NULL);
    yyjson_mut_val *root = yyjson_mut_obj(doc);
    yyjson_mut_doc_set_root(doc, root);

    yyjson_mut_obj_add_int(doc, root, "version", 1);
    yyjson_mut_val *arr = yyjson_mut_arr(doc);
    yyjson_mut_obj_add_val(doc, root, "hnps", arr);

    for (size_t i = 0; i < count; i++) {
        const core_state_entry_t *entry = &entries[i];
        if (!entry->package[0]) continue;
        yyjson_mut_val *obj = yyjson_mut_obj(doc);
        yyjson_mut_obj_add_str(doc, obj, "package", entry->package);
        yyjson_mut_obj_add_str(doc, obj, "version", entry->version);
        yyjson_mut_obj_add_str(doc, obj, "fileName", entry->file_name);
        yyjson_mut_obj_add_str(doc, obj, "entry", entry->entry_path);
        yyjson_mut_obj_add_str(doc, obj, "sourceUrl", entry->source_url);
        yyjson_mut_obj_add_str(doc, obj, "sourceSha256", entry->source_sha256);
        yyjson_mut_obj_add_int(doc, obj, "installedAt", entry->installed_at);
        yyjson_mut_arr_append(arr, obj);
    }

    if (yyjson_mut_write_file(paths->state_path, doc,
                              YYJSON_WRITE_PRETTY | YYJSON_WRITE_ESCAPE_SLASHES,
                              NULL, NULL) == false) {
        yyjson_mut_doc_free(doc);
        return -1;
    }

    log_debug("core_state_save: write completed");
    yyjson_mut_doc_free(doc);
    return 0;
}

static int ensure_local_entry_file(const core_paths_t *paths, core_state_entry_t *entry) {
    if (!entry || !entry->file_name[0]) {
        return -1;
    }
    char local_path[PATH_MAX];
    snprintf(local_path, sizeof(local_path), "%s/%s", paths->hnp_root, entry->file_name);
    if (access(local_path, R_OK) == 0) {
        return 0;
    }

    if (!entry->source_url[0]) {
        log_error("Missing local HNP %s and no source URL recorded.", entry->file_name);
        return -1;
    }

    log_warn("Local HNP %s missing. Re-downloading from %s ...", entry->file_name, entry->source_url);
    create_dir_if_not_exists(paths->hnp_root);
    if (download_file(entry->source_url, local_path) != 0) {
        log_error("Failed to re-download %s.", entry->file_name);
        return -1;
    }
    return 0;
}

static int rebuild_core_hap(const core_paths_t *paths, core_state_entry_t *entries, size_t count) {
    log_info("rebuild_core_hap: rebuilding core HAP with %zu entries", count);
    if (reset_core_hap(paths) != 0) {
        log_error("Failed to reset core hap from base.");
        return -1;
    }
    log_debug("rebuild_core_hap: base HAP reset at %s", paths->hap_path);

    int err = 0;
    zip_t *hap = zip_open(paths->hap_path, ZIP_CHECKCONS, &err);
    if (!hap) {
        log_error("Failed to open %s for module rewrite.", paths->hap_path);
        return -1;
    }
    if (rewrite_module_json(hap, entries, count) != 0) {
        zip_close(hap);
        return -1;
    }
    log_debug("rebuild_core_hap: module.json rewritten for %s", paths->hap_path);
    if (zip_close(hap) != 0) {
        log_error("Failed to update module.json for core hap.");
        return -1;
    }

    char bundle_name[256];
    if (hap_parser_get_bundle_name(paths->hap_path, bundle_name, sizeof(bundle_name)) != 0) {
        log_error("Failed to parse bundle name from org.horpkg.core.hap");
        return -1;
    }
    if (regenerate_resources_index(paths, bundle_name) != 0) {
        return -1;
    }

    for (size_t i = 0; i < count; i++) {
        if (ensure_local_entry_file(paths, &entries[i]) != 0) {
            return -1;
        }
        char local_path[PATH_MAX];
        snprintf(local_path, sizeof(local_path), "%s/%s", paths->hnp_root, entries[i].file_name);
        log_debug("rebuild_core_hap: appending %s -> %s", local_path, entries[i].entry_path);
        if (append_hnp_to_hap(paths, local_path, entries[i].entry_path) != 0) {
            return -1;
        }
    }

    if (install_core_bundle(paths, bundle_name) != 0) {
        return -1;
    }
    log_info("rebuild_core_hap: installed updated core bundle to device");

    print_success_fmt("Installed %zu HNP(s) into org.horpkg.core.", count);
    return 0;
}

static int install_core_bundle(const core_paths_t *paths, const char *bundle_name) {
    char parsed_bundle[256];
    const char *name = bundle_name;
    if (!name || !name[0]) {
        if (hap_parser_get_bundle_name(paths->hap_path, parsed_bundle, sizeof(parsed_bundle)) != 0) {
            log_error("Failed to parse bundle name from org.horpkg.core.hap");
            return -1;
        }
        name = parsed_bundle;
    }

    if (sign_hap(paths->hap_path, name, paths->signed_hap_path) != 0) {
        log_error("Failed to sign patched org.horpkg.core.hap");
        return -1;
    }

    if (hdc_install_hap(paths->signed_hap_path, name, NULL) != 0) {
        unlink(paths->signed_hap_path);
        log_error("Device installation failed for %s", name);
        return -1;
    }

    unlink(paths->signed_hap_path);
    return 0;
}
