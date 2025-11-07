include ../utils/Makefrag

TARGET_NAME = hapsigntool
TARGET_BIN  = $(TARGET_NAME)
TARGET      = build/$(TARGET_BIN)
SHIM_CONFIG_PATH = $(shell pwd)/shim/config.h

SECUREC_DIR = $(CURDIR)/temp/securec
SECUREC_ZIP = download/securec.zip
SECUREC_STAMP = $(SECUREC_DIR)/include/securec.h

JSON_DIR = $(CURDIR)/temp/nlohmann_json
JSON_ZIP = download/nlohmann_json.zip
JSON_STAMP = $(JSON_DIR)/include/nlohmann/json.hpp

ZLIB_SRC_DIR = $(abspath $(CURDIR)/../zlib/temp/zlib-1.3.1)
ZLIB_STAMP = $(ZLIB_SRC_DIR)/contrib/minizip/unzip.h

HAPSIGNTOOL_REPO_ZIP = download/hapsigner_repo.zip
HAPSIGNTOOL_CPP_SRC_DIR = $(CURDIR)/temp/hapsigntool_cpp
HAPSIGNTOOL_CPP_STAMP = $(HAPSIGNTOOL_CPP_SRC_DIR)/main.cpp

INTERNAL_INCLUDES = \
    -I$(HAPSIGNTOOL_CPP_SRC_DIR)/api/include \
    -I$(HAPSIGNTOOL_CPP_SRC_DIR)/signer/include \
    -I$(HAPSIGNTOOL_CPP_SRC_DIR)/cmd/include \
    -I$(HAPSIGNTOOL_CPP_SRC_DIR)/codesigning/datastructure/include \
    -I$(HAPSIGNTOOL_CPP_SRC_DIR)/codesigning/fsverity/include \
    -I$(HAPSIGNTOOL_CPP_SRC_DIR)/codesigning/utils/include \
    -I$(HAPSIGNTOOL_CPP_SRC_DIR)/codesigning/sign/include \
    -I$(HAPSIGNTOOL_CPP_SRC_DIR)/common/include \
    -I$(HAPSIGNTOOL_CPP_SRC_DIR)/hap/verify/include \
    -I$(HAPSIGNTOOL_CPP_SRC_DIR)/hap/config/include \
    -I$(HAPSIGNTOOL_CPP_SRC_DIR)/hap/entity/include \
    -I$(HAPSIGNTOOL_CPP_SRC_DIR)/hap/provider/include \
    -I$(HAPSIGNTOOL_CPP_SRC_DIR)/hap/sign/include \
    -I$(HAPSIGNTOOL_CPP_SRC_DIR)/hap/utils/include \
    -I$(HAPSIGNTOOL_CPP_SRC_DIR)/profile/include \
    -I$(HAPSIGNTOOL_CPP_SRC_DIR)/utils/include \
    -I$(HAPSIGNTOOL_CPP_SRC_DIR)/zip/include \
    -I$(CURDIR)/../sysroot/include

HAPSIGNTOOL_INCLUDES = \
  -I$(SECUREC_DIR)/include \
  -I$(JSON_DIR)/include \
  -I$(ZLIB_SRC_DIR) \
  $(INTERNAL_INCLUDES)

HAPSIGNTOOL_CFLAGS = $(CFLAGS) $(HAPSIGNTOOL_INCLUDES)
HAPSIGNTOOL_CXXFLAGS = $(CXXFLAGS) $(HAPSIGNTOOL_INCLUDES) -std=c++17

HAPSIGNTOOL_LDFLAGS = $(LDFLAGS)
HAPSIGNTOOL_LIBS    = -lssl -lcrypto -lbz2 -lz

all: $(SECUREC_STAMP) $(JSON_STAMP) $(ZLIB_STAMP) $(HAPSIGNTOOL_CPP_STAMP)
	@rm -rf build
	@mkdir -p build/obj

	@CPP_SRCS=$$(find $(HAPSIGNTOOL_CPP_SRC_DIR) -name "*.cpp") && \
	for src in $$CPP_SRCS; do \
		if [ ! -f "$$src" ]; then echo "Missing source $$src"; exit 1; fi; \
		case "$$src" in \
			$(CURDIR)/*) rel=$${src#$(CURDIR)/}; obj="build/obj/$${rel%.cpp}.o";; \
			*) base=$$(basename "$$src" .cpp); obj="build/obj/external_$$base.o";; \
		esac; \
		mkdir -p "$$(dirname $$obj)"; \
		echo "Compiling $$src..."; \
		$(CXX) $(HAPSIGNTOOL_CXXFLAGS) -c $$src -o $$obj || { echo "编译失败: $$src"; exit 1; }; \
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
		$(CC) $(HAPSIGNTOOL_CFLAGS) -c $$src -o $$obj || { echo "编译失败: $$src"; exit 1; }; \
	done

	@OBJS=$$(find build/obj -name "*.o"); \
	$(CXX) $(HAPSIGNTOOL_LDFLAGS) $$OBJS -o $(TARGET) $(HAPSIGNTOOL_LIBS)

	@mkdir -p ../sysroot/horpkgbin
	@cp -fv $(TARGET) ../sysroot/horpkgbin/ | tee file.lst

	@if [ ! -f "$(SHIM_CONFIG_PATH)" ]; then \
		echo "ERROR: Shim config file not found at $(SHIM_CONFIG_PATH)" >&2; \
		echo "Please create 'shim/config.h' inside the '$(shell pwd)' directory." >&2; \
		exit 1; \
	fi
	@echo "==> [hapsigntool] Building and installing shim wrapper -> ../sysroot/bin/$(TARGET_NAME)"
	$(MAKE) -C ../shim install SHIM_NAME=$(TARGET_NAME) CONFIG_PATH=$(SHIM_CONFIG_PATH)

$(SECUREC_STAMP): $(SECUREC_ZIP)
	@rm -rf $(SECUREC_DIR)
	@mkdir -p $(CURDIR)/temp
	@if ! unzip -q $< -d $(CURDIR)/temp; then \
		echo "==> securec archive corrupted, re-downloading..."; \
		rm -f $<; \
		$(MAKE) $(SECUREC_ZIP); \
		unzip -q $< -d $(CURDIR)/temp; \
	fi
	@mv $(CURDIR)/temp/third_party_bounds_checking_function-OpenHarmony-5.0.1-Release $(SECUREC_DIR)
	@touch $@

$(JSON_STAMP): $(JSON_ZIP)
	@rm -rf $(JSON_DIR)
	@mkdir -p $(CURDIR)/temp
	@if ! unzip -q $< -d $(CURDIR)/temp; then \
		echo "==> nlohmann_json archive corrupted, re-downloading..."; \
		rm -f $<; \
		$(MAKE) $(JSON_ZIP); \
		unzip -q $< -d $(CURDIR)/temp; \
	fi
	@mv $(CURDIR)/temp/json-3.12.0 $(JSON_DIR)
	@touch $@

$(HAPSIGNTOOL_CPP_STAMP): $(HAPSIGNTOOL_REPO_ZIP)
	@rm -rf $(HAPSIGNTOOL_CPP_SRC_DIR)
	@mkdir -p $(CURDIR)/temp
	@if ! unzip -q $< -d $(CURDIR)/temp; then \
		echo "==> hapsigner_repo archive corrupted, re-downloading..."; \
		rm -f $<; \
		$(MAKE) $(HAPSIGNTOOL_REPO_ZIP); \
		unzip -q $< -d $(CURDIR)/temp; \
	fi
	@TEMP_REPO_DIR=$$(find $(CURDIR)/temp -maxdepth 1 -type d -name "developtools_hapsigner*") && \
	if [ -n "$$TEMP_REPO_DIR" ] && [ -d "$$TEMP_REPO_DIR/hapsigntool_cpp" ]; then \
		echo "在 $$TEMP_REPO_DIR 找到仓库, 正在移动 'hapsigntool_cpp' 到 $(HAPSIGNTOOL_CPP_SRC_DIR)"; \
		mv "$$TEMP_REPO_DIR/hapsigntool_cpp" $(HAPSIGNTOOL_CPP_SRC_DIR); \
		rm -rf "$$TEMP_REPO_DIR"; \
	else \
		echo "错误: 无法在 $(CURDIR)/temp 的解压档案中找到 'hapsigntool_cpp' 目录。"; \
		exit 1; \
	fi
	@touch $@

$(SECUREC_ZIP):
	@mkdir -p download
	@$(WGET) -O $@ https://gitee.com/openharmony/third_party_bounds_checking_function/repository/archive/OpenHarmony-5.0.1-Release.zip

$(JSON_ZIP):
	@mkdir -p download
	@$(WGET) -O $@ https://github.com/nlohmann/json/archive/refs/tags/v3.12.0.zip

$(HAPSIGNTOOL_REPO_ZIP):
	@mkdir -p download
	@$(WGET) -O $@ https://gitee.com/openharmony/developtools_hapsigner/repository/archive/master.zip

.PHONY: all

.SECONDARY: $(SECUREC_ZIP) $(JSON_ZIP) $(HAPSIGNTOOL_REPO_ZIP)
.SECONDARY: $(SECUREC_STAMP) $(JSON_STAMP) $(HAPSIGNTOOL_CPP_STAMP)
