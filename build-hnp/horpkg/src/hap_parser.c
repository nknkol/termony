#define _DEFAULT_SOURCE // [!] 为 strdup 添加
#include "hap_parser.h"
#include "logger.h"
#include "utils.h" // [!] 添加
#include <zip.h>
#include <yyjson.h>
#include <string.h>
#include <stdlib.h>

// 要检查的文件列表
static const char *FILES_TO_CHECK[] = {
    "module.json", // 优先级最高
    "pack.info",   // 其次
    "config.json", // 备用 (与 module.json 结构相同)
    "module.json5",// 备用
    NULL
};

// 从 JSON buffer 中提取 bundleName
static const char* extract_name_from_json(const char *buffer, size_t size, const char *filename) {
    yyjson_doc *doc = yyjson_read(buffer, size, 0);
    if (!doc) return NULL;

    yyjson_val *root = yyjson_doc_get_root(doc);
    yyjson_val *app_obj = NULL;
    const char *name = NULL;

    if (strcmp(filename, "pack.info") == 0) {
        // "pack.info" 结构: {"summary": {"app": {"bundleName": "..."}}}
        yyjson_val *summary = yyjson_obj_get(root, "summary");
        app_obj = yyjson_obj_get(summary, "app");
    } else {
        // "module.json" / "config.json" 结构: {"app": {"bundleName": "..."}}
        app_obj = yyjson_obj_get(root, "app");
    }

    if (app_obj) {
        name = yyjson_get_str(yyjson_obj_get(app_obj, "bundleName"));
    }

    // 注意：我们只返回指针，yyjson_doc_free 必须在外部调用
    // 为了安全，我们复制一份
    char *name_copy = NULL;
    if (name) {
        name_copy = strdup(name);
    }
    
    yyjson_doc_free(doc);
    return name_copy; // 调用者必须 free()
}

int hap_parser_get_bundle_name(const char *hap_path, char *bundle_name_out, size_t bundle_name_size) {
    int err = 0;
    zip_t *za = zip_open(hap_path, 0, &err);
    if (!za) {
        print_error_fmt("Failed to open HAP file '%s' (libzip error code: %d)", hap_path, err);
        return -1;
    }

    char found_bundle_name[256] = {0};
    int result = -1; // 默认失败

    for (int i = 0; FILES_TO_CHECK[i] != NULL; i++) {
        const char *filename = FILES_TO_CHECK[i];
        
        struct zip_stat sb;
        zip_stat_init(&sb);
        if (zip_stat(za, filename, 0, &sb) != 0) {
            log_debug("File '%s' not found in HAP, skipping.", filename);
            continue;
        }

        zip_file_t *zf = zip_fopen(za, filename, 0);
        if (!zf) {
            log_warn("Found '%s' but failed to open it.", filename);
            continue;
        }

        // [修复 1] 增加 +1 用于 NUL 终止符
        char *buffer = malloc(sb.size + 1);
        if (!buffer) {
            log_error("Failed to allocate memory for JSON parsing.");
            zip_fclose(zf);
            result = -1;
            break;
        }

        // [修复 2] 检查 zip_fread 的返回值 (来自上一条建议)
        zip_int64_t bytes_read = zip_fread(zf, buffer, sb.size);
        zip_fclose(zf);

        if (bytes_read < 0 || (zip_uint64_t)bytes_read != sb.size) {
            log_error("Failed to read '%s' from zip (expected %llu, got %lld)", 
                      filename, (unsigned long long)sb.size, (long long)bytes_read);
            free(buffer);
            continue; 
        }
        buffer[sb.size] = '\0'; // NUL 终止

        const char *current_name = extract_name_from_json(buffer, sb.size, filename);
        free(buffer);

        if (current_name) {
            log_debug("Found bundleName in '%s': %s", filename, current_name);
            if (found_bundle_name[0] == '\0') {
                // 第一次找到
                strncpy(found_bundle_name, current_name, sizeof(found_bundle_name) - 1);
                result = 0; // [FIX 1] 假设成功
            } else if (strcmp(found_bundle_name, current_name) != 0) {
                // 找到冲突
                print_error_fmt("Conflicting bundleName found in HAP!");
                print_error_fmt("  Found '%s' (from e.g. %s)", found_bundle_name, FILES_TO_CHECK[0]);
                print_error_fmt("  Also found '%s' (from %s)", current_name, filename);
                log_error("Installation aborted due to inconsistent metadata.");
                result = -1; // [FIX 2] 发现冲突，设置为失败
                free((void*)current_name); // [FIX 3] 必须在 break 前 free
                break; // 停止搜索
            }
            free((void*)current_name);
        }
    }

    zip_close(za);

    // [FIX 4] 检查 result 是否为 0 (成功)
    if (result == 0) {
        strncpy(bundle_name_out, found_bundle_name, bundle_name_size - 1);
        bundle_name_out[bundle_name_size - 1] = '\0';
    } 
    // [FIX 5] 如果 result 仍然是 -1 且从未找到名称，则打印错误
    else if (found_bundle_name[0] == '\0') { 
        print_error_fmt("Could not find 'bundleName' in any metadata files (%s, %s, etc.) inside %s",
                      FILES_TO_CHECK[0], FILES_TO_CHECK[1], hap_path);
        // result 已经是 -1
    }
    // (如果 result 是 -1 并且是由于冲突，则错误已被打印)

    return result;
}