include ../utils/Makefrag

# ===== Versions & download locations =====
RESTOOL_REF ?= master
RESTOOL_URL := https://gitee.com/openharmony/developtools_global_resource_tool/repository/archive/$(RESTOOL_REF).zip
RESTOOL_ARCHIVE := download/developtools_global_resource_tool-$(RESTOOL_REF).zip

JSONCPP_REF ?= master
JSONCPP_URL := https://gitee.com/openharmony/third_party_jsoncpp/repository/archive/$(JSONCPP_REF).zip
JSONCPP_ARCHIVE := download/third_party_jsoncpp-$(JSONCPP_REF).zip

LIBPNG_REF ?= master
LIBPNG_URL := https://gitee.com/openharmony/third_party_libpng/repository/archive/$(LIBPNG_REF).zip
LIBPNG_ARCHIVE := download/third_party_libpng-$(LIBPNG_REF).zip

ZLIB_REF ?= master
ZLIB_URL := https://gitee.com/openharmony/third_party_zlib/repository/archive/$(ZLIB_REF).zip
ZLIB_ARCHIVE := download/third_party_zlib-$(ZLIB_REF).zip

SECUREC_REF ?= OpenHarmony-5.0.1-Release
SECUREC_URL := https://gitee.com/openharmony/third_party_bounds_checking_function/repository/archive/$(SECUREC_REF).zip
SECUREC_ARCHIVE := download/third_party_bounds_checking_function-$(SECUREC_REF).zip

CJSON_REF ?= master
CJSON_URL := https://gitee.com/openharmony/third_party_cJSON/repository/archive/$(CJSON_REF).zip
CJSON_ARCHIVE := download/third_party_cJSON-$(CJSON_REF).zip

# ===== Layout =====
TEMP_DIR := $(CURDIR)/temp
THIRD_PARTY_DIR := $(TEMP_DIR)/third_party
THIRD_PARTY_LINK := $(CURDIR)/third_party

RESTOOL_SRC_DIR := $(TEMP_DIR)/developtools_global_resource_tool
RESTOOL_BUILD_DIR := $(RESTOOL_SRC_DIR)/build/linux
RESTOOL_READY := $(RESTOOL_SRC_DIR)/.ready

JSONCPP_DIR := $(THIRD_PARTY_DIR)/jsoncpp
LIBPNG_DIR := $(THIRD_PARTY_DIR)/libpng
ZLIB_DIR := $(THIRD_PARTY_DIR)/zlib
SECUREC_DIR := $(THIRD_PARTY_DIR)/bounds_checking_function
CJSON_DIR := $(THIRD_PARTY_DIR)/cjson

PATCH_DIR := $(CURDIR)/patches
RESTOOL_PATCHES := $(wildcard $(PATCH_DIR)/*.patch)

THIRD_PARTY_STAMPS := \
	$(JSONCPP_DIR)/.stamp \
	$(LIBPNG_DIR)/.stamp \
	$(ZLIB_DIR)/.stamp \
	$(SECUREC_DIR)/.stamp \
	$(CJSON_DIR)/.stamp


RESTOOL_OUTPUT := build$(PREFIX)/horpkgbin/restool
SHIM_NAME := restool
SHIM_CONFIG_PATH := $(shell pwd)/shim/config.h
FINAL_STAMP := build/.stamp

.PHONY: all clean distclean

all: $(FINAL_STAMP)
	@echo "==> [restool] build completed"

$(FINAL_STAMP): $(RESTOOL_READY) $(THIRD_PARTY_STAMPS)
	rm -rf build file.lst
	mkdir -p $(dir $(RESTOOL_OUTPUT))

	rm -rf $(THIRD_PARTY_LINK)
	mkdir -p $(THIRD_PARTY_DIR)
	ln -sfn $(THIRD_PARTY_DIR) $(THIRD_PARTY_LINK)

	rm -rf $(RESTOOL_BUILD_DIR)
	cmake -S $(RESTOOL_SRC_DIR) -B $(RESTOOL_BUILD_DIR) \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_INSTALL_PREFIX=$(CURDIR)/build$(PREFIX) \
		-DCMAKE_C_COMPILER="$(CC)" \
		-DCMAKE_CXX_COMPILER="$(CXX)" \
		-DCMAKE_AR="$(AR)" \
		-DCMAKE_RANLIB="$(RANLIB)" \
		-DCMAKE_LINKER="$(LD)"
	cmake --build $(RESTOOL_BUILD_DIR) -j $(shell nproc)

	$(STRIP) $(RESTOOL_BUILD_DIR)/restool
	cp $(RESTOOL_BUILD_DIR)/restool $(RESTOOL_OUTPUT)

	mkdir -p ../sysroot
	cp -rfv build$(PREFIX)/. ../sysroot | tee file.lst
	@if [ ! -f "$(SHIM_CONFIG_PATH)" ]; then \
		echo "ERROR: Shim config file not found at $(SHIM_CONFIG_PATH)" >&2; \
		echo "Please create it and ensure TARGET_COMMAND_PATH matches $(PREFIX)/horpkgbin/$(SHIM_NAME)." >&2; \
		exit 1; \
	fi
	$(MAKE) -C ../shim install SHIM_NAME=$(SHIM_NAME) CONFIG_PATH=$(SHIM_CONFIG_PATH) 2>&1 | tee -a file.lst
	rm -rf $(THIRD_PARTY_LINK)
	touch $@

# ===== restool sources =====
$(RESTOOL_READY): $(RESTOOL_SRC_DIR)/.unpacked
	@if [ -n "$(RESTOOL_PATCHES)" ]; then \
		cd $(RESTOOL_SRC_DIR) && \
		for patch_file in $(RESTOOL_PATCHES); do \
			echo "==> [restool] Applying $$patch_file"; \
			patch -p1 -N < $$patch_file || { echo "Failed to apply $$patch_file"; exit 1; }; \
		done; \
	else \
		echo "==> [restool] No patches to apply"; \
	fi
	touch $@

$(RESTOOL_SRC_DIR)/.unpacked: $(RESTOOL_ARCHIVE) $(RESTOOL_PATCHES)
	rm -rf $(RESTOOL_SRC_DIR)
	mkdir -p $(TEMP_DIR)/restool_zip
	unzip -q $(RESTOOL_ARCHIVE) -d $(TEMP_DIR)/restool_zip
	RESTOOL_EXTRACT=$$(find $(TEMP_DIR)/restool_zip -maxdepth 1 -type d -name "developtools_global_resource_tool*" | head -n 1); \
	if [ -z "$$RESTOOL_EXTRACT" ]; then \
		echo "无法在压缩包中找到 developtools_global_resource_tool 源码" >&2; \
		exit 1; \
	fi; \
	mkdir -p $(TEMP_DIR); \
	mv "$$RESTOOL_EXTRACT" $(RESTOOL_SRC_DIR)
	rm -rf $(TEMP_DIR)/restool_zip
	touch $@

# ===== third_party/jsoncpp =====
$(JSONCPP_DIR)/.stamp: $(JSONCPP_ARCHIVE)
	rm -rf $(JSONCPP_DIR)
	mkdir -p $(THIRD_PARTY_DIR)
	mkdir -p $(TEMP_DIR)/jsoncpp_zip
	unzip -q $(JSONCPP_ARCHIVE) -d $(TEMP_DIR)/jsoncpp_zip
	SRC=$$(find $(TEMP_DIR)/jsoncpp_zip -maxdepth 1 -type d -name "third_party_jsoncpp*" | head -n 1); \
	if [ -z "$$SRC" ]; then \
		echo "JSONCPP 源码未找到" >&2; \
		exit 1; \
	fi; \
	mkdir -p $(JSONCPP_DIR); \
	python3 "$$SRC/install.py" --gen-dir $(JSONCPP_DIR) --source-file "$$SRC"; \
	SUB=$$(find $(JSONCPP_DIR) -maxdepth 1 -type d -name "jsoncpp-*"); \
	if [ -n "$$SUB" ]; then \
		cp -Rf "$$SUB/." $(JSONCPP_DIR)/; \
		rm -rf "$$SUB"; \
	fi
	rm -rf $(TEMP_DIR)/jsoncpp_zip
	touch $@

# ===== third_party/libpng =====
$(LIBPNG_DIR)/.stamp: $(LIBPNG_ARCHIVE)
	rm -rf $(LIBPNG_DIR)
	mkdir -p $(LIBPNG_DIR)
	mkdir -p $(TEMP_DIR)/libpng_zip
	unzip -q $(LIBPNG_ARCHIVE) -d $(TEMP_DIR)/libpng_zip
	SRC=$$(find $(TEMP_DIR)/libpng_zip -maxdepth 1 -type d -name "third_party_libpng*" | head -n 1); \
	if [ -z "$$SRC" ]; then \
		echo "libpng 源码未找到" >&2; \
		exit 1; \
	fi; \
	python3 "$$SRC/install.py" --gen-dir $(LIBPNG_DIR) --source-dir "$$SRC"
	SUB=$$(find $(LIBPNG_DIR) -maxdepth 1 -type d -name "libpng-*"); \
	if [ -n "$$SUB" ]; then \
		cp -Rf "$$SUB/." $(LIBPNG_DIR)/; \
		rm -rf "$$SUB"; \
	fi
	rm -rf $(TEMP_DIR)/libpng_zip
	touch $@

# ===== third_party/zlib =====
$(ZLIB_DIR)/.stamp: $(ZLIB_ARCHIVE)
	rm -rf $(ZLIB_DIR)
	mkdir -p $(THIRD_PARTY_DIR)
	mkdir -p $(TEMP_DIR)/zlib_zip
	unzip -q $(ZLIB_ARCHIVE) -d $(TEMP_DIR)/zlib_zip
	SRC=$$(find $(TEMP_DIR)/zlib_zip -maxdepth 1 -type d -name "third_party_zlib*" | head -n 1); \
	if [ -z "$$SRC" ]; then \
		echo "zlib 源码未找到" >&2; \
		exit 1; \
	fi; \
	mkdir -p $(ZLIB_DIR); \
	cp -Rf "$$SRC/." $(ZLIB_DIR)/
	rm -rf $(TEMP_DIR)/zlib_zip
	touch $@

# ===== third_party/securec =====
$(SECUREC_DIR)/.stamp: $(SECUREC_ARCHIVE)
	rm -rf $(SECUREC_DIR)
	mkdir -p $(THIRD_PARTY_DIR)
	mkdir -p $(TEMP_DIR)/securec_zip
	unzip -q $(SECUREC_ARCHIVE) -d $(TEMP_DIR)/securec_zip
	SRC=$$(find $(TEMP_DIR)/securec_zip -maxdepth 1 -type d -name "third_party_bounds_checking_function*" | head -n 1); \
	if [ -z "$$SRC" ]; then \
		echo "securec 源码未找到" >&2; \
		exit 1; \
	fi; \
	mkdir -p $(SECUREC_DIR); \
	cp -Rf "$$SRC/." $(SECUREC_DIR)/
	rm -rf $(TEMP_DIR)/securec_zip
	touch $@

# ===== third_party/cJSON =====
$(CJSON_DIR)/.stamp: $(CJSON_ARCHIVE)
	rm -rf $(CJSON_DIR)
	mkdir -p $(THIRD_PARTY_DIR)
	mkdir -p $(TEMP_DIR)/cjson_zip
	unzip -q $(CJSON_ARCHIVE) -d $(TEMP_DIR)/cjson_zip
	SRC=$$(find $(TEMP_DIR)/cjson_zip -maxdepth 1 -type d -name "third_party_cJSON*" | head -n 1); \
	if [ -z "$$SRC" ]; then \
		echo "cJSON 源码未找到" >&2; \
		exit 1; \
	fi; \
	mkdir -p $(CJSON_DIR); \
	cp -Rf "$$SRC/." $(CJSON_DIR)/; \
	SUB=$$(find $(CJSON_DIR) -maxdepth 1 -type d -name "cJSON-*"); \
	if [ -n "$$SUB" ]; then \
		cp -Rf "$$SUB/." $(CJSON_DIR)/; \
		rm -rf "$$SUB"; \
	fi
	rm -rf $(TEMP_DIR)/cjson_zip
	touch $@

# ===== downloads =====
$(RESTOOL_ARCHIVE):
	mkdir -p download
	$(WGET) -O $@ $(RESTOOL_URL)

$(JSONCPP_ARCHIVE):
	mkdir -p download
	$(WGET) -O $@ $(JSONCPP_URL)

$(LIBPNG_ARCHIVE):
	mkdir -p download
	$(WGET) -O $@ $(LIBPNG_URL)

$(ZLIB_ARCHIVE):
	mkdir -p download
	$(WGET) -O $@ $(ZLIB_URL)

$(SECUREC_ARCHIVE):
	mkdir -p download
	$(WGET) -O $@ $(SECUREC_URL)

$(CJSON_ARCHIVE):
	mkdir -p download
	$(WGET) -O $@ $(CJSON_URL)

clean:
	rm -rf build file.lst $(TEMP_DIR) $(RESTOOL_BUILD_DIR) $(THIRD_PARTY_LINK)

distclean: clean
	rm -rf download $(THIRD_PARTY_DIR)
