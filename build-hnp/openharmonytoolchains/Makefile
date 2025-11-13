SHELL := /bin/bash

REPO_ROOT := $(abspath $(CURDIR)/../../..)
WORKSPACE_ABS ?= /workspace
REPO_WORKSPACE := $(REPO_ROOT)/workspace

OHOS_SDK_HOME ?= $(WORKSPACE_ABS)/command-line-tools/sdk/default/openharmony
OHOS_NATIVE_TOOLS_JAR_DIR ?=

include ../utils/Makefrag

BUILD_DIR := $(CURDIR)/build
LIB_STAGE_DIR := $(BUILD_DIR)/lib
RUNTIME_DIR := $(BUILD_DIR)/runtime
SHIM_CONFIG_DIR := $(CURDIR)/shim-config
DOWNLOAD_DIR := $(CURDIR)/download
JAR_CACHE_DIR := $(DOWNLOAD_DIR)/jars
TEMP_DIR := $(CURDIR)/temp
TARGET_JDK_DIR := $(TEMP_DIR)/target-openjdk

ifndef OHOS_ARCH
$(error OHOS_ARCH is not set. Please export OHOS_ARCH (e.g. aarch64) before building.)
endif

OPENJDK_HOST_HOME ?= $(WORKSPACE_ABS)/openjdk-host
JLINK_BIN := $(OPENJDK_HOST_HOME)/bin/jlink
TARGET_JDK_ARCHIVE ?= OpenJDK25U-jdk_aarch64_alpine-linux_hotspot_25.0.1_8.tar.gz
TARGET_JDK_URL ?= https://github.com/adoptium/temurin25-binaries/releases/download/jdk-25.0.1+8/$(TARGET_JDK_ARCHIVE)
TARGET_JDK_TARBALL := $(DOWNLOAD_DIR)/$(TARGET_JDK_ARCHIVE)
TARGET_JDK_SENTINEL := $(TARGET_JDK_DIR)/.installed
TARGET_JMOD_DIR := $(TARGET_JDK_DIR)/jmods
TARGET_JDK_PRUNE_DIRS ?= demo man legal include
TARGET_JDK_PRUNE_FILES ?= lib/src.zip

ifeq ($(wildcard $(JLINK_BIN)),)
$(error OpenJDK host jlink not found at $(JLINK_BIN). Run support/setup-graalvm-jdk.sh)
endif

TOOLS := app_check_tool app_packing_tool app_unpacking_tool hap-sign-tool
JAR_FILENAMES := $(addsuffix .jar,$(TOOLS))

JLINK_MODULES ?= java.base,java.logging,java.xml,java.naming,java.security.jgss,java.security.sasl,java.management,java.desktop,java.sql,java.scripting,jdk.unsupported
JAR_BASE_URL ?= https://github.com/openharmony-tools/openharmony-jars/releases/download/latest

INSTALL_JDK_DIR := ../sysroot/opt/jdk

SYSROOT_DIR := $(abspath $(CURDIR)/../sysroot)
MUSL_SYSROOT_DIR := $(SYSROOT_DIR)/$(OHOS_ARCH)-unknown-linux-musl
SDK_SYSROOT_DIR := $(OHOS_SDK_HOME)/native/sysroot
ifeq ($(wildcard $(MUSL_SYSROOT_DIR)),)
ACTIVE_SYSROOT := $(SYSROOT_DIR)
else
ACTIVE_SYSROOT := $(MUSL_SYSROOT_DIR)
endif

define existing_paths
$(strip $(foreach dir,$(1),$(if $(wildcard $(dir)),$(dir),)))
endef

SYSROOT_INCLUDE_DIRS := $(call existing_paths,$(SYSROOT_DIR)/include $(ACTIVE_SYSROOT)/include $(SDK_SYSROOT_DIR)/usr/include $(SDK_SYSROOT_DIR)/usr/include/$(OHOS_ARCH)-linux-ohos)
SYSROOT_LIBRARY_DIRS := $(call existing_paths,$(SYSROOT_DIR)/lib $(SYSROOT_DIR)/libexec $(ACTIVE_SYSROOT)/lib $(SDK_SYSROOT_DIR)/usr/lib $(SDK_SYSROOT_DIR)/usr/lib/$(OHOS_ARCH)-linux-ohos)

WRAPPER_INCLUDE_FLAGS := $(foreach dir,$(SYSROOT_INCLUDE_DIRS),-I"$(dir)")
WRAPPER_LIBRARY_FLAGS := $(foreach dir,$(SYSROOT_LIBRARY_DIRS),-L"$(dir)") -Wl,-rpath-link,"$(SYSROOT_DIR)/lib" -Wl,-rpath-link,"$(ACTIVE_SYSROOT)/lib" -Wl,-rpath-link,"$(SDK_SYSROOT_DIR)/usr/lib/$(OHOS_ARCH)-linux-ohos"

RAW_JAR_DIRS := \
  $(strip $(OHOS_NATIVE_TOOLS_JAR_DIR)) \
  $(JAR_CACHE_DIR) \
  $(OHOS_SDK_HOME)/toolchains/lib \
  $(WORKSPACE_ABS)/command-line-tools/sdk/default/openharmony/toolchains/lib \
  $(REPO_WORKSPACE)/command-line-tools/sdk/default/openharmony/toolchains/lib

JAR_DIRS := $(strip $(foreach dir,$(RAW_JAR_DIRS),$(if $(wildcard $(dir)),$(dir),)))

ifeq ($(strip $(JAR_DIRS)),)
$(warning No HarmonyOS jar directories found. Run support/setup-graalvm-jdk.sh after installing the SDK.)
endif

define locate_jar
$(strip $(firstword $(foreach dir,$(JAR_DIRS),$(if $(wildcard $(dir)/$(1)),$(dir)/$(1),))))
endef

STAGED_JARS := $(addprefix $(LIB_STAGE_DIR)/,$(JAR_FILENAMES))
SHIM_CONFIGS := $(addprefix $(SHIM_CONFIG_DIR)/,$(addsuffix /config.h,$(TOOLS)))
RUNTIME_SENTINEL := $(RUNTIME_DIR)/bin/java

TMUX_TMPDIR ?= /data/storage/el2/base/cache
JAVA_TMPDIR ?= /data/storage/el2/base/cache
SYSROOT_ENV_PATH := $(PREFIX)/

.PHONY: all clean print-jars install

all: install

install: $(TARGET_JDK_SENTINEL) $(RUNTIME_SENTINEL) $(STAGED_JARS) $(SHIM_CONFIGS)
	@echo "==> Installing JDK into $(INSTALL_JDK_DIR)..."
	rm -rf $(INSTALL_JDK_DIR)
	mkdir -p $(INSTALL_JDK_DIR)
	cp -a $(RUNTIME_DIR)/. $(INSTALL_JDK_DIR)/

	@echo "==> Copying HarmonyOS jars into JDK lib..."
	for jar in $(STAGED_JARS); do \
		cp -f "$$jar" "$(INSTALL_JDK_DIR)/lib/"; \
	done

	@echo "==> Building shim binaries..."
	for tool in $(TOOLS); do \
		CONFIG_FILE="$(abspath $(SHIM_CONFIG_DIR)/$$tool/config.h)"; \
		if [ "$$tool" = "hap-sign-tool" ]; then \
			SHIM_NAME="hapsigntool"; \
		else \
			SHIM_NAME="$$tool"; \
		fi; \
		$(MAKE) -C ../shim install SHIM_NAME="$$SHIM_NAME" CONFIG_PATH="$$CONFIG_FILE"; \
	done

print-jars:
	@for jar in $(JAR_FILENAMES); do \
		path="$(call locate_jar,$${jar})"; \
		if [ -n "$${path}" ]; then \
			printf "%-24s -> %s\n" "$${jar}" "$${path}"; \
		else \
			printf "%-24s -> <missing>\n" "$${jar}"; \
		fi; \
	done

$(LIB_STAGE_DIR):
	@mkdir -p $@

$(RUNTIME_DIR):
	@mkdir -p $@

$(LIB_STAGE_DIR)/%.jar: | $(LIB_STAGE_DIR)
	@jar_path="$(call locate_jar,$*.jar)"; \
	if [ -z "$${jar_path}" ]; then \
		echo "==> Jar $*.jar missing, downloading..."; \
		$(MAKE) download-jar NAME=$* || { echo "❌ Failed to download $*.jar"; exit 1; }; \
		jar_path="$(call locate_jar,$*.jar)"; \
	fi; \
	echo "==> Staging $${jar_path}"; \
	cp -f "$${jar_path}" "$@"

$(TARGET_JDK_TARBALL):
	@mkdir -p $(DOWNLOAD_DIR)
	@echo "==> Downloading target OpenJDK archive $(TARGET_JDK_ARCHIVE)"
	@$(WGET) -O $@ $(TARGET_JDK_URL)

$(TARGET_JDK_SENTINEL): $(TARGET_JDK_TARBALL)
	@echo "==> Extracting target OpenJDK"
	@rm -rf $(TARGET_JDK_DIR)
	@mkdir -p $(dir $(TARGET_JDK_DIR))
	@TMP_DIR=$$(mktemp -d); \
	  TOP_DIR=$$(tar -tf $< | sed -n '1p' | cut -d/ -f1); \
	  tar -xzf $< -C $$TMP_DIR; \
	  mv $$TMP_DIR/$$TOP_DIR $(TARGET_JDK_DIR); \
	  rm -rf $$TMP_DIR
	@for dir in $(TARGET_JDK_PRUNE_DIRS); do \
		if [ -e "$(TARGET_JDK_DIR)/$$dir" ]; then \
			echo "==> Pruning $${dir} from target JDK"; \
			rm -rf "$(TARGET_JDK_DIR)/$$dir"; \
		fi; \
	done
	@for file in $(TARGET_JDK_PRUNE_FILES); do \
		if [ -e "$(TARGET_JDK_DIR)/$$file" ]; then \
			echo "==> Removing $${file} from target JDK"; \
			rm -f "$(TARGET_JDK_DIR)/$$file"; \
		fi; \
	done
	@touch $@

$(RUNTIME_SENTINEL): $(TARGET_JDK_SENTINEL) | $(RUNTIME_DIR)
	@echo "==> Running jlink (modules: $(JLINK_MODULES))"
	rm -rf $(RUNTIME_DIR)
	$(JLINK_BIN) \
		--module-path "$(TARGET_JMOD_DIR)" \
		--add-modules "$(JLINK_MODULES)" \
		--strip-debug \
		--no-man-pages \
		--no-header-files \
		--compress=2 \
		--output "$(RUNTIME_DIR)"
	@echo "==> Replacing launcher with target musl binary"
	cp -f "$(TARGET_JDK_DIR)/bin/java" "$(RUNTIME_DIR)/bin/java"
	@echo "==> Overlaying target musl native libraries"
	@if ls "$(TARGET_JDK_DIR)/lib"/lib*.so >/dev/null 2>&1; then \
		cp -f "$(TARGET_JDK_DIR)"/lib/lib*.so "$(RUNTIME_DIR)/lib/"; \
	fi
	@rm -rf "$(RUNTIME_DIR)/lib/server"
	cp -a "$(TARGET_JDK_DIR)/lib/server" "$(RUNTIME_DIR)/lib/"

$(SHIM_CONFIG_DIR)/%/config.h:
	@mkdir -p $(dir $@)
	@{ \
	  printf '/* Auto-generated shim config for %s */\n' "$*"; \
	  printf '#ifndef SHIM_CONFIG_H\n#define SHIM_CONFIG_H\n\n'; \
	  printf '#define TARGET_COMMAND_PATH "%s"\n\n' "$(PREFIX)/opt/jdk/bin/java"; \
	  printf '#define LIB_PATH_ENV_VAR "LD_LIBRARY_PATH"\n'; \
	  printf '#define CUSTOM_LIB_PATH "%s"\n\n' "$(PREFIX)/lib"; \
	  printf '#define SHIM_EXTRA_ARGS \\\n'; \
	  printf '    X("-jar") \\\n'; \
	  printf '    X("%s")\n\n' "$(PREFIX)/opt/jdk/lib/$*.jar"; \
	  printf '#define SHIM_ENV_VARS \\\n'; \
	  printf '    X("TMUX_TMPDIR", "%s") \\\n' "$(TMUX_TMPDIR)"; \
	  printf '    X("TMPDIR", "%s") \\\n' "$(JAVA_TMPDIR)"; \
	  printf '    X("TMP", "%s") \\\n' "$(JAVA_TMPDIR)"; \
	  printf '    X("TEMP", "%s") \\\n' "$(JAVA_TMPDIR)"; \
	  printf '    X("JAVA_TOOL_OPTIONS", "-Djava.io.tmpdir=%s") \\\n' "$(JAVA_TMPDIR)"; \
	  printf '    X("SYSROOT", "%s")\n' "$(SYSROOT_ENV_PATH)"; \
	  printf '\n#endif /* SHIM_CONFIG_H */\n'; \
	} > $@

download-jar:
	@mkdir -p $(JAR_CACHE_DIR)
	@if [ -z "$(NAME)" ]; then echo "NAME not set"; exit 1; fi
	@echo "==> Downloading jar $(NAME).jar"
	@$(WGET) -O $(JAR_CACHE_DIR)/$(NAME).jar $(JAR_BASE_URL)/$(NAME).jar

clean:
	rm -rf $(BUILD_DIR)
