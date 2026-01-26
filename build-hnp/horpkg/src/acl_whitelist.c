#include "acl_whitelist.h"
#include <string.h>

// 1. 自动签名支持的 ACL 权限 (需添加到 Profile)
static const char *ACL_AUTO_LIST[] = {
    // 6.0.2 Beta1
    "ohos.permission.SUBSCRIBE_NOTIFICATION",
    "ohos.permission.ACCESS_USER_FULL_DISK",
    "ohos.permission.CUSTOM_SCREEN_RECORDING",
    "ohos.permission.GET_IP_MAC_INFO",
    // 6.0.1 Release
    "ohos.permission.SET_SYSTEMSHARE_APPLAUNCHTRUSTLIST",
    "ohos.permission.HOOK_KEY_EVENT",
    "ohos.permission.WEB_NATIVE_MESSAGING",
    // 6.0.0 Beta3
    "ohos.permission.CUSTOMIZE_SAVE_BUTTON",
    "ohos.permission.GET_ABILITY_INFO",
    "ohos.permission.LINKTURBO",
    "ohos.permission.GET_WIFI_LOCAL_MAC",
    "ohos.permission.GET_ETHERNET_LOCAL_MAC",
    "ohos.permission.USE_FLOAT_BALL",
    "ohos.permission.READ_LOCAL_DEVICE_NAME",
    "ohos.permission.ACCESS_NET_TRACE_INFO",
    "ohos.permission.KEEP_BACKGROUND_RUNNING_SYSTEM",
    "ohos.permission.atomicService.MANAGE_STORAGE",
    "ohos.permission.MANAGE_SCREEN_TIME_GUARD",
    // 5.1.0 Release
    "ohos.permission.ACCESS_DDK_USB_SERIAL",
    "ohos.permission.ACCESS_DDK_SCSI_PERIPHERAL",
    "ohos.permission.USE_FRAUD_APP_PICKER",
    // 5.0.5 Release
    "ohos.permission.kernel.DISABLE_GOTPLT_RO_PROTECTION",
    "ohos.permission.MANAGE_APN_SETTING",
    // 5.0.3 Release
    "ohos.permission.READ_WRITE_USB_DEV",
    "ohos.permission.USE_FRAUD_CALL_LOG_PICKER",
    "ohos.permission.USE_FRAUD_MESSAGES_PICKER",
    "ohos.permission.ACCESS_DISK_PHY_INFO",
    "ohos.permission.SET_PAC_URL",
    "ohos.permission.PERSONAL_MANAGE_RESTRICTIONS",
    "ohos.permission.START_PROVISIONING_MESSAGE",
    "ohos.permission.PRELOAD_FILE",
    "ohos.permission.kernel.ALLOW_WRITABLE_CODE_MEMORY",
    "ohos.permission.kernel.DISABLE_CODE_MEMORY_PROTECTION",
    "ohos.permission.kernel.ALLOW_EXECUTABLE_FORT_MEMORY",
    "ohos.permission.GET_WIFI_PEERS_MAC",
    "ohos.permission.READ_WRITE_DESKTOP_DIRECTORY",
    "ohos.permission.MANAGE_PASTEBOARD_APP_SHARE_OPTION",
    "ohos.permission.MANAGE_UDMF_APP_SHARE_OPTION",
    "ohos.permission.READ_WRITE_USER_FILE",
    // 5.0.0 Release
    "ohos.permission.READ_CONTACTS",
    "ohos.permission.WRITE_CONTACTS",
    "ohos.permission.READ_AUDIO",
    "ohos.permission.WRITE_AUDIO",
    "ohos.permission.READ_IMAGEVIDEO",
    "ohos.permission.READ_PASTEBOARD",
    "ohos.permission.WRITE_IMAGEVIDEO",
    "ohos.permission.ACCESS_DDK_USB",
    "ohos.permission.ACCESS_DDK_HID",
    "ohos.permission.SYSTEM_FLOAT_WINDOW",
    "ohos.permission.FILE_ACCESS_PERSIST",
    "ohos.permission.INPUT_MONITORING",
    "ohos.permission.INTERCEPT_INPUT_EVENT",
    "ohos.permission.SHORT_TERM_WRITE_IMAGEVIDEO",
    NULL
};

// 2. 开放权限 (无需处理，直接放行)
// 合并了 User Grant 和 System Grant
static const char *OPEN_PERMS_LIST[] = {
    // User Grant
    "ohos.permission.ACCESS_BLUETOOTH",
    "ohos.permission.MEDIA_LOCATION",
    "ohos.permission.APP_TRACKING_CONSENT",
    "ohos.permission.ACTIVITY_MOTION",
    "ohos.permission.CAMERA",
    "ohos.permission.DISTRIBUTED_DATASYNC",
    "ohos.permission.LOCATION_IN_BACKGROUND",
    "ohos.permission.LOCATION",
    "ohos.permission.APPROXIMATELY_LOCATION",
    "ohos.permission.MICROPHONE",
    "ohos.permission.READ_CALENDAR",
    "ohos.permission.WRITE_CALENDAR",
    "ohos.permission.READ_HEALTH_DATA",
    "ohos.permission.ACCESS_NEARLINK",
    "ohos.permission.READ_WRITE_DOWNLOAD_DIRECTORY",
    "ohos.permission.READ_WRITE_DOCUMENTS_DIRECTORY",
    "ohos.permission.CUSTOM_SCREEN_CAPTURE",
    "ohos.permission.READ_MEDIA",
    "ohos.permission.WRITE_MEDIA",
    
    // System Grant
    "ohos.permission.USE_BLUETOOTH",
    "ohos.permission.GET_BUNDLE_INFO",
    "ohos.permission.PREPARE_APP_TERMINATE",
    "ohos.permission.PRINT",
    "ohos.permission.DISCOVER_BLUETOOTH",
    "ohos.permission.ACCELEROMETER",
    "ohos.permission.ACCESS_BIOMETRIC",
    "ohos.permission.ACCESS_NOTIFICATION_POLICY",
    "ohos.permission.GET_NETWORK_INFO",
    "ohos.permission.SET_NETWORK_INFO",
    "ohos.permission.GET_WIFI_INFO",
    "ohos.permission.GYROSCOPE",
    "ohos.permission.INTERNET",
    "ohos.permission.KEEP_BACKGROUND_RUNNING",
    "ohos.permission.NFC_CARD_EMULATION",
    "ohos.permission.NFC_TAG",
    "ohos.permission.PRIVACY_WINDOW",
    "ohos.permission.PUBLISH_AGENT_REMINDER",
    "ohos.permission.SET_WIFI_INFO",
    "ohos.permission.VIBRATE",
    "ohos.permission.CLEAN_BACKGROUND_PROCESSES",
    "ohos.permission.COMMONEVENT_STICKY",
    "ohos.permission.MODIFY_AUDIO_SETTINGS",
    "ohos.permission.RUNNING_LOCK",
    "ohos.permission.SET_WALLPAPER",
    "ohos.permission.ACCESS_CERT_MANAGER",
    "ohos.permission.hsdr.HSDR_ACCESS",
    "ohos.permission.RUN_DYN_CODE",
    "ohos.permission.READ_CLOUD_SYNC_CONFIG",
    "ohos.permission.STORE_PERSISTENT_DATA",
    "ohos.permission.ACCESS_EXTENSIONAL_DEVICE_DRIVER",
    "ohos.permission.READ_ACCOUNT_LOGIN_STATE",
    "ohos.permission.ACCESS_SERVICE_NAVIGATION_INFO",
    "ohos.permission.PROTECT_SCREEN_LOCK_DATA",
    "ohos.permission.ACCESS_CAR_DISTRIBUTED_ENGINE",
    "ohos.permission.WINDOW_TOPMOST",
    "ohos.permission.MANAGE_INPUT_INFRARED_EMITTER",
    "ohos.permission.INPUT_KEYBOARD_CONTROLLER",
    "ohos.permission.SET_ABILITY_INSTANCE_INFO",
    "ohos.permission.NDK_START_SELF_UI_ABILITY",
    "ohos.permission.GET_FILE_ICON",
    "ohos.permission.DETECT_GESTURE",
    "ohos.permission.kernel.NET_RAW",
    "ohos.permission.kernel.DEBUGGER",
    "ohos.permission.kernel.ALLOW_DEBUG",
    "ohos.permission.BACKGROUND_MANAGER_POWER_SAVE_MODE",
    "ohos.permission.SET_WINDOW_TRANSPARENT",
    "ohos.permission.START_WINDOW_BELOW_LOCK_SCREEN",
    "ohos.permission.kernel.IGNORE_LIBRARY_VALIDATION",
    "ohos.permission.TIMEOUT_SCREENOFF_DISABLE_LOCK",
    "ohos.permission.LOCK_WINDOW_CURSOR",
    NULL
};

perm_check_result_t check_permission_type(const char *permission) {
    if (!permission) return PERM_TYPE_OPEN;

    // 1. 检查是否为开放权限
    for (int i = 0; OPEN_PERMS_LIST[i] != NULL; i++) {
        if (strcmp(OPEN_PERMS_LIST[i], permission) == 0) {
            return PERM_TYPE_OPEN;
        }
    }

    // 2. 检查是否为自动签名支持的受限权限
    for (int i = 0; ACL_AUTO_LIST[i] != NULL; i++) {
        if (strcmp(ACL_AUTO_LIST[i], permission) == 0) {
            return PERM_TYPE_ACL_AUTO;
        }
    }

    // 3. 默认为需要手动申请的受限权限
    return PERM_TYPE_RESTRICTED_MANUAL;
}