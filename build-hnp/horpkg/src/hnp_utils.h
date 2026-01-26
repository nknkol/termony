#ifndef HORPKG_HNP_UTILS_H
#define HORPKG_HNP_UTILS_H

#include <stddef.h>

/**
 * @brief Checks if 'hnpcli' and 'restool' are available in PATH or via environment variables.
 * @return 0 if available, -1 otherwise.
 */
int hnp_utils_check_tools(void);

/**
 * @brief Repackages and normalizes HNP files inside a HAP package.
 * 
 * This function:
 * 1. Extracts the HAP.
 * 2. Scans for HNP packages defined in module.json.
 * 3. Repacks each HNP using 'hnpcli'.
 * 4. Regenerates resources.index using 'restool'.
 * 5. Rebuilds the HAP file.
 * 
 * @param input_hap_path Path to the original HAP file.
 * @param output_hap_path Path where the new, normalized HAP will be saved.
 * @return 0 on success, -1 on failure.
 */
int hnp_repack_hap(const char *input_hap_path, const char *output_hap_path);

#endif // HORPKG_HNP_UTILS_H