#ifndef HORPKG_ACL_WHITELIST_H
#define HORPKG_ACL_WHITELIST_H

typedef enum {
    PERM_TYPE_OPEN,            // 开放权限 (无需处理)
    PERM_TYPE_ACL_AUTO,        // 受限权限 (自动签名支持，需添加到 Profile)
    PERM_TYPE_RESTRICTED_MANUAL // 受限权限 (需手动申请，报错)
} perm_check_result_t;

/**
 * @brief Check permission type.
 * 
 * @param permission The permission name to check.
 * @return The permission type classification.
 */
perm_check_result_t check_permission_type(const char *permission);

#endif // HORPKG_ACL_WHITELIST_H