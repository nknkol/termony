include ../utils/Makefrag

ifndef OHOS_ARCH
$(error OHOS_ARCH is not set. Please export OHOS_ARCH (e.g. aarch64 or x86_64) before running make.)
endif

TARGET_NAME = binarysigntool
TARGET_BIN  = $(TARGET_NAME)
TARGET      = build/$(TARGET_BIN)
SHIM_CONFIG_PATH = $(shell pwd)/shim/config.h

SECUREC_DIR   = $(CURDIR)/temp/securec
SECUREC_ZIP   = download/securec.zip
SECUREC_STAMP = $(SECUREC_DIR)/include/securec.h

JSON_DIR   = $(CURDIR)/temp/nlohmann_json
JSON_ZIP   = download/nlohmann_json.zip
JSON_STAMP = $(JSON_DIR)/include/nlohmann/json.hpp

ZLIB_SRC_DIR = $(abspath $(CURDIR)/../zlib/temp/zlib-1.3.1)
ZLIB_STAMP   = $(ZLIB_SRC_DIR)/contrib/minizip/unzip.h

HAPSIGN_REPO_ZIP     = download/hapsigner_repo.zip
BINSIGN_CPP_SRC_DIR  = $(CURDIR)/temp/binary_sign_tool
BINSIGN_CPP_STAMP    = $(BINSIGN_CPP_SRC_DIR)/main.cpp
HAPSIGN_CPP_SRC_DIR  = $(CURDIR)/temp/hapsigntool_cpp
HAPSIGN_CPP_STAMP    = $(HAPSIGN_CPP_SRC_DIR)/main.cpp
ELFIO_VERSION        = Release_3.12
ELFIO_DIR            = $(CURDIR)/temp/elfio
ELFIO_ZIP            = download/elfio-$(ELFIO_VERSION).zip
ELFIO_STAMP          = $(ELFIO_DIR)/elfio/elfio.hpp

INTERNAL_INCLUDES = \
  -I$(BINSIGN_CPP_SRC_DIR)/api/include \
  -I$(BINSIGN_CPP_SRC_DIR)/signer/include \
  -I$(BINSIGN_CPP_SRC_DIR)/cmd/include \
  -I$(BINSIGN_CPP_SRC_DIR)/codesigning/datastructure/include \
  -I$(BINSIGN_CPP_SRC_DIR)/codesigning/fsverity/include \
  -I$(BINSIGN_CPP_SRC_DIR)/codesigning/utils/include \
  -I$(BINSIGN_CPP_SRC_DIR)/codesigning/sign/include \
  -I$(BINSIGN_CPP_SRC_DIR)/common/include \
  -I$(BINSIGN_CPP_SRC_DIR)/hap/verify/include \
  -I$(BINSIGN_CPP_SRC_DIR)/hap/config/include \
  -I$(BINSIGN_CPP_SRC_DIR)/hap/entity/include \
  -I$(BINSIGN_CPP_SRC_DIR)/hap/provider/include \
  -I$(BINSIGN_CPP_SRC_DIR)/hap/sign/include \
  -I$(BINSIGN_CPP_SRC_DIR)/hap/utils/include \
  -I$(BINSIGN_CPP_SRC_DIR)/profile/include \
  -I$(BINSIGN_CPP_SRC_DIR)/utils/include \
  -I$(CURDIR)/../sysroot/include

INTERNAL_INCLUDES += \
  -I$(HAPSIGN_CPP_SRC_DIR)/api/include \
  -I$(HAPSIGN_CPP_SRC_DIR)/signer/include \
  -I$(HAPSIGN_CPP_SRC_DIR)/cmd/include \
  -I$(HAPSIGN_CPP_SRC_DIR)/codesigning/datastructure/include \
  -I$(HAPSIGN_CPP_SRC_DIR)/codesigning/fsverity/include \
  -I$(HAPSIGN_CPP_SRC_DIR)/codesigning/utils/include \
  -I$(HAPSIGN_CPP_SRC_DIR)/codesigning/sign/include \
  -I$(HAPSIGN_CPP_SRC_DIR)/common/include \
  -I$(HAPSIGN_CPP_SRC_DIR)/hap/verify/include \
  -I$(HAPSIGN_CPP_SRC_DIR)/hap/config/include \
  -I$(HAPSIGN_CPP_SRC_DIR)/hap/entity/include \
  -I$(HAPSIGN_CPP_SRC_DIR)/hap/provider/include \
  -I$(HAPSIGN_CPP_SRC_DIR)/hap/sign/include \
  -I$(HAPSIGN_CPP_SRC_DIR)/hap/utils/include \
  -I$(HAPSIGN_CPP_SRC_DIR)/profile/include \
  -I$(HAPSIGN_CPP_SRC_DIR)/utils/include

BINSIGN_INCLUDES = -I$(SECUREC_DIR)/include -I$(JSON_DIR)/include -I$(ZLIB_SRC_DIR) -I$(ELFIO_DIR) -I$(ELFIO_DIR)/elfio $(INTERNAL_INCLUDES)

BINSIGN_CPP_SRCS = \
  $(BINSIGN_CPP_SRC_DIR)/main.cpp \
  $(BINSIGN_CPP_SRC_DIR)/api/src/sign_tool_service_impl.cpp \
  $(BINSIGN_CPP_SRC_DIR)/utils/src/file_utils.cpp \
  $(BINSIGN_CPP_SRC_DIR)/codesigning/fsverity/src/fs_verity_descriptor.cpp \
  $(BINSIGN_CPP_SRC_DIR)/codesigning/fsverity/src/fs_verity_generator.cpp \
  $(BINSIGN_CPP_SRC_DIR)/codesigning/fsverity/src/merkle_tree_builder.cpp \
  $(BINSIGN_CPP_SRC_DIR)/codesigning/sign/src/code_signing.cpp \
  $(BINSIGN_CPP_SRC_DIR)/common/src/options.cpp \
  $(BINSIGN_CPP_SRC_DIR)/hap/entity/src/param_constants.cpp \
  $(BINSIGN_CPP_SRC_DIR)/hap/provider/src/self_sign_sign_provider.cpp \
  $(BINSIGN_CPP_SRC_DIR)/hap/provider/src/local_sign_provider.cpp \
  $(BINSIGN_CPP_SRC_DIR)/hap/provider/src/sign_provider.cpp \
  $(BINSIGN_CPP_SRC_DIR)/hap/sign/src/sign_elf.cpp \
  $(BINSIGN_CPP_SRC_DIR)/hap/sign/src/bc_pkcs7_generator.cpp \
  $(BINSIGN_CPP_SRC_DIR)/hap/verify/src/verify_elf.cpp \
  $(BINSIGN_CPP_SRC_DIR)/profile/src/profile_info.cpp \
  $(BINSIGN_CPP_SRC_DIR)/profile/src/profile_sign_tool.cpp \
  $(BINSIGN_CPP_SRC_DIR)/cmd/src/cmd_util.cpp \
  $(BINSIGN_CPP_SRC_DIR)/cmd/src/params_run_tool.cpp \
  $(BINSIGN_CPP_SRC_DIR)/cmd/src/params_trust_list.cpp

HAPSIGN_CPP_SRCS = \
  $(HAPSIGN_CPP_SRC_DIR)/utils/src/verify_cert_openssl_utils.cpp \
  $(HAPSIGN_CPP_SRC_DIR)/utils/src/key_store_helper.cpp \
  $(HAPSIGN_CPP_SRC_DIR)/utils/src/string_utils.cpp \
  $(HAPSIGN_CPP_SRC_DIR)/utils/src/verify_hap_openssl_utils.cpp \
  $(HAPSIGN_CPP_SRC_DIR)/codesigning/fsverity/src/fs_verity_digest.cpp \
  $(HAPSIGN_CPP_SRC_DIR)/codesigning/fsverity/src/fs_verity_hash_algorithm.cpp \
  $(HAPSIGN_CPP_SRC_DIR)/codesigning/fsverity/src/fs_verity_descriptor_with_sign.cpp \
  $(HAPSIGN_CPP_SRC_DIR)/codesigning/utils/src/fs_digest_utils.cpp \
  $(HAPSIGN_CPP_SRC_DIR)/codesigning/utils/src/cms_utils.cpp \
  $(HAPSIGN_CPP_SRC_DIR)/codesigning/sign/src/bc_signeddata_generator.cpp \
  $(HAPSIGN_CPP_SRC_DIR)/common/src/byte_buffer.cpp \
  $(HAPSIGN_CPP_SRC_DIR)/common/src/digest_parameter.cpp \
  $(HAPSIGN_CPP_SRC_DIR)/common/src/digest_common.cpp \
  $(HAPSIGN_CPP_SRC_DIR)/common/src/localization_adapter.cpp \
  $(HAPSIGN_CPP_SRC_DIR)/hap/config/src/signer_config.cpp \
  $(HAPSIGN_CPP_SRC_DIR)/hap/entity/src/content_digest_algorithm.cpp \
  $(HAPSIGN_CPP_SRC_DIR)/hap/entity/src/signature_algorithm_helper.cpp \
  $(HAPSIGN_CPP_SRC_DIR)/hap/provider/src/remote_sign_provider.cpp \
  $(HAPSIGN_CPP_SRC_DIR)/hap/utils/src/dynamic_lib_handle.cpp \
  $(HAPSIGN_CPP_SRC_DIR)/profile/src/profile_verify.cpp \
  $(HAPSIGN_CPP_SRC_DIR)/profile/src/pkcs7_data.cpp \
  $(HAPSIGN_CPP_SRC_DIR)/signer/src/local_signer.cpp \
  $(HAPSIGN_CPP_SRC_DIR)/signer/src/signer_factory.cpp \
  $(HAPSIGN_CPP_SRC_DIR)/cmd/src/params.cpp

CPP_SRCS = $(BINSIGN_CPP_SRCS) $(HAPSIGN_CPP_SRCS)

BINSIGN_CFLAGS   = $(CFLAGS)   $(BINSIGN_INCLUDES)
BINSIGN_CXXFLAGS = $(CXXFLAGS) $(BINSIGN_INCLUDES) -std=c++17 -fno-rtti -Wno-c++20-extensions
BINSIGN_LDFLAGS  = $(LDFLAGS)
BINSIGN_LIBS     = -lssl -lcrypto -lbz2 -lz

all: $(SECUREC_STAMP) $(JSON_STAMP) $(ZLIB_STAMP) $(ELFIO_STAMP) $(BINSIGN_CPP_STAMP) $(HAPSIGN_CPP_STAMP)
	@rm -rf build
	@mkdir -p build/obj

	@CPP_SRCS="$(CPP_SRCS)"; \
	for src in $$CPP_SRCS; do \
		if [ ! -f "$$src" ]; then echo "Missing source $$src"; exit 1; fi; \
		case "$$src" in \
			$(CURDIR)/*) rel=$${src#$(CURDIR)/}; obj="build/obj/$${rel%.cpp}.o";; \
			*) base=$$(basename "$$src" .cpp); obj="build/obj/external_$$base.o";; \
		 esac; \
		mkdir -p "$$(dirname $$obj)"; \
		echo "Compiling $$src..."; \
		$(CXX) $(BINSIGN_CXXFLAGS) -c $$src -o $$obj || { echo "编译失败: $$src"; exit 1; }; \
	done

	@C_SRCS=$$(find $(SECUREC_DIR)/src -name "*.c"); \
	C_SRCS="$$C_SRCS $(ZLIB_SRC_DIR)/contrib/minizip/ioapi.c $(ZLIB_SRC_DIR)/contrib/minizip/unzip.c"; \
	for src in $$C_SRCS; do \
		case "$$src" in \
			$(CURDIR)/*) rel=$${src#$(CURDIR)/}; obj="build/obj/$${rel%.c}.o";; \
			*) sanitized=$$(echo $$src | tr '/:.' '___'); obj="build/obj/external_$$sanitized.o";; \
		 esac; \
		mkdir -p "$$(dirname $$obj)"; \
		echo "Compiling $$src..."; \
		$(CC) $(BINSIGN_CFLAGS) -c $$src -o $$obj || { echo "编译失败: $$src"; exit 1; }; \
	done

	@OBJS=$$(find build/obj -name "*.o"); \
	$(CXX) $(BINSIGN_LDFLAGS) $$OBJS -o $(TARGET) $(BINSIGN_LIBS)

	@mkdir -p ../sysroot/horpkgbin
	@cp -fv $(TARGET) ../sysroot/horpkgbin/ | tee file.lst

	@if [ ! -f "$(SHIM_CONFIG_PATH)" ]; then \
		echo "ERROR: Shim config file not found at $(SHIM_CONFIG_PATH)" >&2; \
		echo "Please create 'shim/config.h' inside '$(shell pwd)'." >&2; \
		exit 1; \
	fi
	@echo "==> [binarysigntool] Building and installing shim wrapper -> ../sysroot/bin/$(TARGET_NAME)"
	$(MAKE) -C ../shim install SHIM_NAME=$(TARGET_NAME) CONFIG_PATH=$(SHIM_CONFIG_PATH)

$(SECUREC_STAMP): $(SECUREC_ZIP)
	@rm -rf $(SECUREC_DIR)
	@mkdir -p $(CURDIR)/temp
	@unzip -q $< -d $(CURDIR)/temp
	@mv $(CURDIR)/temp/third_party_bounds_checking_function-OpenHarmony-5.0.1-Release $(SECUREC_DIR)
	@touch $@

$(JSON_STAMP): $(JSON_ZIP)
	@rm -rf $(JSON_DIR)
	@mkdir -p $(CURDIR)/temp
	@unzip -q $< -d $(CURDIR)/temp
	@mv $(CURDIR)/temp/json-3.12.0 $(JSON_DIR)
	@touch $@

$(ELFIO_STAMP): $(ELFIO_ZIP)
	@rm -rf $(ELFIO_DIR) $(CURDIR)/temp/ELFIO-$(ELFIO_VERSION)
	@mkdir -p $(CURDIR)/temp
	@if ! unzip -q "$<" -d $(CURDIR)/temp; then \
		echo "==> ELFIO archive corrupted, re-downloading..."; \
		rm -f "$<"; \
		$(MAKE) $(ELFIO_ZIP); \
		unzip -q "$<" -d $(CURDIR)/temp; \
	fi
	@mv $(CURDIR)/temp/ELFIO-$(ELFIO_VERSION) $(ELFIO_DIR)
	@touch $@

$(BINSIGN_CPP_STAMP) $(HAPSIGN_CPP_STAMP): $(HAPSIGN_REPO_ZIP)
	@rm -rf $(BINSIGN_CPP_SRC_DIR) $(HAPSIGN_CPP_SRC_DIR)
	@mkdir -p $(CURDIR)/temp
	@unzip -q $< -d $(CURDIR)/temp
	@TEMP_REPO_DIR=$$(find $(CURDIR)/temp -maxdepth 1 -type d -name "developtools_hapsigner*"); \
	if [ -n "$$TEMP_REPO_DIR" ] && [ -d "$$TEMP_REPO_DIR/binary_sign_tool" ] && [ -d "$$TEMP_REPO_DIR/hapsigntool_cpp" ]; then \
		echo "在 $$TEMP_REPO_DIR 找到仓库, 正在同步 'binary_sign_tool' 和 'hapsigntool_cpp'"; \
		mv "$$TEMP_REPO_DIR/binary_sign_tool" $(BINSIGN_CPP_SRC_DIR); \
		mv "$$TEMP_REPO_DIR/hapsigntool_cpp" $(HAPSIGN_CPP_SRC_DIR); \
		rm -rf "$$TEMP_REPO_DIR"; \
	else \
		echo "错误: 无法在 $(CURDIR)/temp 的解压档案中找到需要的源代码目录。"; \
		exit 1; \
	fi
	@touch $(BINSIGN_CPP_STAMP) $(HAPSIGN_CPP_STAMP)

$(SECUREC_ZIP):
	@mkdir -p download
	@$(WGET) -O $@ https://gitee.com/openharmony/third_party_bounds_checking_function/repository/archive/OpenHarmony-5.0.1-Release.zip

$(JSON_ZIP):
	@mkdir -p download
	@$(WGET) -O $@ https://github.com/nlohmann/json/archive/refs/tags/v3.12.0.zip

$(HAPSIGN_REPO_ZIP):
	@mkdir -p download
	@$(WGET) -O $@ https://gitee.com/openharmony/developtools_hapsigner/repository/archive/master.zip

$(ELFIO_ZIP):
	@mkdir -p download
	@$(WGET) -O $@ https://github.com/serge1/ELFIO/archive/refs/tags/$(ELFIO_VERSION).zip

.PHONY: all

.SECONDARY: $(SECUREC_ZIP) $(JSON_ZIP) $(HAPSIGN_REPO_ZIP) $(ELFIO_ZIP)
.SECONDARY: $(SECUREC_STAMP) $(JSON_STAMP) $(ELFIO_STAMP) $(BINSIGN_CPP_STAMP) $(HAPSIGN_CPP_STAMP)
