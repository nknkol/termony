include ../utils/Makefrag

ifndef OHOS_ARCH
$(error OHOS_ARCH is not set. Please export OHOS_ARCH (e.g. aarch64 or x86_64) before running make.)
endif

TARGET_NAME       = hnpcli
TARGET_BIN        = $(TARGET_NAME)
TARGET            = build/$(TARGET_BIN)
SHIM_CONFIG_PATH  = $(CURDIR)/shim/config.h

HNPCLI_COMMIT   = 1127ab803eb25231625d3bfe4e63d4713949572c
HNPCLI_ZIP      = download/startup_appspawn-$(HNPCLI_COMMIT).zip
HNPCLI_SRC_DIR  = $(CURDIR)/temp/startup_appspawn
HNPCLI_STAMP    = $(HNPCLI_SRC_DIR)/service/hnp/hnpcli_main.c

SECUREC_DIR     = $(CURDIR)/temp/securec
SECUREC_ZIP     = download/securec.zip
SECUREC_STAMP   = $(SECUREC_DIR)/include/securec.h

CJSON_DIR       = $(CURDIR)/temp/cjson
CJSON_ZIP       = download/cjson.zip
CJSON_STAMP     = $(CJSON_DIR)/cJSON.h

ZLIB_SRC_DIR    = $(abspath $(CURDIR)/../zlib/temp/zlib-1.3.1)
ZLIB_STAMP      = $(ZLIB_SRC_DIR)/contrib/minizip/unzip.h

HNPCLI_INCLUDES = \
	-I$(HNPCLI_SRC_DIR)/service/hnp/base \
	-I$(HNPCLI_SRC_DIR)/service/hnp/pack/include \
	-I$(SECUREC_DIR)/include \
	-I$(CJSON_DIR) \
	-I$(ZLIB_SRC_DIR) \
	-I$(ZLIB_SRC_DIR)/contrib/minizip

HNPCLI_SRCS = \
	$(HNPCLI_SRC_DIR)/service/hnp/base/hnp_file.c \
	$(HNPCLI_SRC_DIR)/service/hnp/base/hnp_json.c \
	$(HNPCLI_SRC_DIR)/service/hnp/base/hnp_log.c \
	$(HNPCLI_SRC_DIR)/service/hnp/base/hnp_zip.c \
	$(HNPCLI_SRC_DIR)/service/hnp/hnpcli_main.c \
	$(HNPCLI_SRC_DIR)/service/hnp/pack/src/hnp_pack.c \
	$(CJSON_DIR)/cJSON.c \
	$(ZLIB_SRC_DIR)/contrib/minizip/ioapi.c \
	$(ZLIB_SRC_DIR)/contrib/minizip/unzip.c \
	$(ZLIB_SRC_DIR)/contrib/minizip/zip.c

HNPCLI_CFLAGS  = $(CFLAGS) $(HNPCLI_INCLUDES) -DHNP_CLI
HNPCLI_LDFLAGS = $(LDFLAGS)
HNPCLI_LIBS    = -lz

all: $(SECUREC_STAMP) $(CJSON_STAMP) $(ZLIB_STAMP) $(HNPCLI_STAMP)
	@rm -rf build
	@mkdir -p build/obj

	@SRCS="$(HNPCLI_SRCS)"; \
	for src in $$SRCS; do \
		if [ ! -f "$$src" ]; then echo "Missing source $$src"; exit 1; fi; \
		case "$$src" in \
			$(CURDIR)/*) rel=$${src#$(CURDIR)/}; obj="build/obj/$${rel%.c}.o";; \
			*) sanitized=$$(echo $$src | tr '/:.' '___'); obj="build/obj/external_$$sanitized.o";; \
		esac; \
		mkdir -p "$$(dirname $$obj)"; \
		echo "Compiling $$src..."; \
		$(CC) $(HNPCLI_CFLAGS) -c $$src -o $$obj || { echo "编译失败: $$src"; exit 1; }; \
	done

	@SECUREC_SRCS=$$(find $(SECUREC_DIR)/src -name "*.c"); \
	for src in $$SECUREC_SRCS; do \
		case "$$src" in \
			$(CURDIR)/*) rel=$${src#$(CURDIR)/}; obj="build/obj/$${rel%.c}.o";; \
			*) sanitized=$$(echo $$src | tr '/:.' '___'); obj="build/obj/external_$$sanitized.o";; \
		esac; \
		mkdir -p "$$(dirname $$obj)"; \
		echo "Compiling $$src..."; \
		$(CC) $(HNPCLI_CFLAGS) -c $$src -o $$obj || { echo "编译失败: $$src"; exit 1; }; \
	done

	@OBJS=$$(find build/obj -name "*.o"); \
	$(CC) $(HNPCLI_LDFLAGS) $$OBJS -o $(TARGET) $(HNPCLI_LIBS)

	@mkdir -p ../sysroot/horpkgbin
	@cp -fv $(TARGET) ../sysroot/horpkgbin/ | tee file.lst

	@if [ ! -f "$(SHIM_CONFIG_PATH)" ]; then \
		echo "ERROR: Shim config file not found at $(SHIM_CONFIG_PATH)" >&2; \
		echo "Please create 'shim/config.h' inside '$(shell pwd)'." >&2; \
		exit 1; \
	fi
	@echo "==> [hnpcli] Building and installing shim wrapper -> ../sysroot/bin/$(TARGET_NAME)"
	$(MAKE) -C ../shim install SHIM_NAME=$(TARGET_NAME) CONFIG_PATH=$(SHIM_CONFIG_PATH)

$(HNPCLI_STAMP): $(HNPCLI_ZIP)
	@rm -rf $(HNPCLI_SRC_DIR)
	@mkdir -p $(CURDIR)/temp
	@unzip -q $< -d $(CURDIR)/temp
	@SRC_DIR=$$(find $(CURDIR)/temp -maxdepth 1 -type d -name "startup_appspawn*" | head -n 1); \
	if [ -z "$$SRC_DIR" ]; then \
		echo "错误: 无法在 $(CURDIR)/temp 的解压档案中找到 startup_appspawn 源代码目录。"; \
		exit 1; \
	fi; \
	mv "$$SRC_DIR" $(HNPCLI_SRC_DIR)
	@touch $@

$(SECUREC_STAMP): $(SECUREC_ZIP)
	@rm -rf $(SECUREC_DIR)
	@mkdir -p $(CURDIR)/temp
	@if ! unzip -q $< -d $(CURDIR)/temp; then \
		echo "==> securec archive corrupted, re-downloading..."; \
		rm -f $<; \
		$(MAKE) $(SECUREC_ZIP); \
		unzip -q $< -d $(CURDIR)/temp; \
	fi
	@SRC=$$(find $(CURDIR)/temp -maxdepth 1 -type d -name "third_party_bounds_checking_function*" | head -n 1); \
	if [ -z "$$SRC" ]; then \
		echo "错误: 未找到 securec 源目录。"; \
		exit 1; \
	fi; \
	mv "$$SRC" $(SECUREC_DIR)
	@touch $@

$(CJSON_STAMP): $(CJSON_ZIP)
	@rm -rf $(CJSON_DIR)
	@mkdir -p $(CURDIR)/temp
	@if ! unzip -q $< -d $(CURDIR)/temp; then \
		echo "==> cJSON archive corrupted, re-downloading..."; \
		rm -f $<; \
		$(MAKE) $(CJSON_ZIP); \
		unzip -q $< -d $(CURDIR)/temp; \
	fi
	@SRC=$$(find $(CURDIR)/temp -maxdepth 1 -type d -name "third_party_cJSON*" | head -n 1); \
	if [ -z "$$SRC" ]; then \
		echo "错误: 未找到 cJSON 源目录。"; \
		exit 1; \
	fi; \
	mkdir -p $(CJSON_DIR); \
	cp -Rf "$$SRC/." $(CJSON_DIR)/; \
	SUB=$$(find $(CJSON_DIR) -maxdepth 1 -type d -name "cJSON-*"); \
	if [ -n "$$SUB" ]; then \
		cp -Rf "$$SUB/." $(CJSON_DIR)/; \
		rm -rf "$$SUB"; \
	fi
	@touch $@

$(ZLIB_STAMP):
	@echo "Error: zlib sources not found. Please run 'make -C ../zlib' first."
	@exit 1

$(HNPCLI_ZIP):
	@mkdir -p download
	@$(WGET) -O $@ https://gitee.com/openharmony/startup_appspawn/repository/archive/$(HNPCLI_COMMIT).zip

$(SECUREC_ZIP):
	@mkdir -p download
	@$(WGET) -O $@ https://gitee.com/openharmony/third_party_bounds_checking_function/repository/archive/OpenHarmony-5.0.1-Release.zip

$(CJSON_ZIP):
	@mkdir -p download
	@$(WGET) -O $@ https://gitee.com/openharmony/third_party_cJSON/repository/archive/OpenHarmony-5.0.1-Release.zip

.PHONY: all

.SECONDARY: $(SECUREC_ZIP) $(CJSON_ZIP) $(HNPCLI_ZIP)
.SECONDARY: $(SECUREC_STAMP) $(CJSON_STAMP) $(HNPCLI_STAMP)
