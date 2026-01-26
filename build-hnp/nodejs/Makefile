include ../utils/Makefrag

NODE_VERSION := 20.11.0
NODE_ARCHIVE := node-v$(NODE_VERSION)-linux-arm64.tar.xz
NODE_URL := https://nodejs.org/dist/v$(NODE_VERSION)/$(NODE_ARCHIVE)
NODE_DIR_NAME := node-v$(NODE_VERSION)-linux-arm64
NODE_DOWNLOAD := download/$(NODE_ARCHIVE)

# Installation Paths (On Device)
INSTALL_PREFIX := $(PREFIX)/opt/nodejs
REAL_NODE_BIN := $(INSTALL_PREFIX)/bin/node
REAL_NPM_JS := $(INSTALL_PREFIX)/lib/node_modules/npm/bin/npm-cli.js
REAL_NPX_JS := $(INSTALL_PREFIX)/lib/node_modules/npm/bin/npx-cli.js

# Writable User Data Paths (For NPM/NPX config)
CACHE_DIR := /data/storage/el2/base/cache
NPM_PREFIX := $(CACHE_DIR)/node_global
NPM_CACHE := $(CACHE_DIR)/node_cache
NPM_USERCONFIG := $(CACHE_DIR)/npmrc

# PATH Injection: Prepend global bin and internal node bin
# Utilizes shim.c's library path logic (repurposed for PATH)
NODE_PATH_PREPEND := $(NPM_PREFIX)/bin:$(INSTALL_PREFIX)/bin

.PHONY: all clean

all: $(NODE_DOWNLOAD)
	rm -rf build shim_config
	mkdir -p build shim_config

	# 1. Extract and Install Node.js
	tar -xf $(NODE_DOWNLOAD) -C build
	# Create destination structure matching $(PREFIX)
	mkdir -p build$(PREFIX)/opt
	# Move extracted folder to build/$(PREFIX)/opt/nodejs
	mv build/$(NODE_DIR_NAME) build$(INSTALL_PREFIX)
	
	# Copy to sysroot (staging area for packing)
	mkdir -p ../sysroot
	cp -rfv build/$(PREFIX)/. ../sysroot > file.lst

	# 2. Generate Shims

	# --- Shim 1: node ---
	# Simple passthrough to the real binary
	mkdir -p shim_config/node
	echo '#ifndef CONFIG_H' > shim_config/node/config.h
	echo '#define CONFIG_H' >> shim_config/node/config.h
	echo '#define TARGET_COMMAND_PATH "$(REAL_NODE_BIN)"' >> shim_config/node/config.h
	echo '#endif' >> shim_config/node/config.h
	
	$(MAKE) -C ../shim install SHIM_NAME=node CONFIG_PATH=$(shell pwd)/shim_config/node/config.h

	# --- Shim 2: npm ---
	# Injects environment variables and arguments to run npm-cli.js
	mkdir -p shim_config/npm
	echo '#ifndef CONFIG_H' > shim_config/npm/config.h
	echo '#define CONFIG_H' >> shim_config/npm/config.h
	echo '#define TARGET_COMMAND_PATH "$(REAL_NODE_BIN)"' >> shim_config/npm/config.h
	# Pass npm-cli.js as the first argument
	echo '#define SHIM_EXTRA_ARGS X("$(REAL_NPM_JS)")' >> shim_config/npm/config.h
	# Set required environment variables
	echo '#define SHIM_ENV_VARS \' >> shim_config/npm/config.h
	echo '    X("NPM_CONFIG_PREFIX", "$(NPM_PREFIX)") \' >> shim_config/npm/config.h
	echo '    X("NPM_CONFIG_CACHE", "$(NPM_CACHE)") \' >> shim_config/npm/config.h
	echo '    X("NPM_CONFIG_USERCONFIG", "$(NPM_USERCONFIG)")' >> shim_config/npm/config.h
	# Hack: Use LIB_PATH logic to prepend to PATH
	echo '#define LIB_PATH_ENV_VAR "PATH"' >> shim_config/npm/config.h
	echo '#define CUSTOM_LIB_PATH "$(NODE_PATH_PREPEND)"' >> shim_config/npm/config.h
	echo '#endif' >> shim_config/npm/config.h

	$(MAKE) -C ../shim install SHIM_NAME=npm CONFIG_PATH=$(shell pwd)/shim_config/npm/config.h

	# --- Shim 3: npx ---
	# Similar to npm but points to npx-cli.js
	mkdir -p shim_config/npx
	echo '#ifndef CONFIG_H' > shim_config/npx/config.h
	echo '#define CONFIG_H' >> shim_config/npx/config.h
	echo '#define TARGET_COMMAND_PATH "$(REAL_NODE_BIN)"' >> shim_config/npx/config.h
	echo '#define SHIM_EXTRA_ARGS X("$(REAL_NPX_JS)")' >> shim_config/npx/config.h
	echo '#define SHIM_ENV_VARS \' >> shim_config/npx/config.h
	echo '    X("NPM_CONFIG_PREFIX", "$(NPM_PREFIX)") \' >> shim_config/npx/config.h
	echo '    X("NPM_CONFIG_CACHE", "$(NPM_CACHE)") \' >> shim_config/npx/config.h
	echo '    X("NPM_CONFIG_USERCONFIG", "$(NPM_USERCONFIG)")' >> shim_config/npx/config.h
	echo '#define LIB_PATH_ENV_VAR "PATH"' >> shim_config/npx/config.h
	echo '#define CUSTOM_LIB_PATH "$(NODE_PATH_PREPEND)"' >> shim_config/npx/config.h
	echo '#endif' >> shim_config/npx/config.h

	$(MAKE) -C ../shim install SHIM_NAME=npx CONFIG_PATH=$(shell pwd)/shim_config/npx/config.h

$(NODE_DOWNLOAD):
	mkdir -p download
	$(WGET) -O $@ $(NODE_URL)

clean:
	rm -rf build temp download shim_config file.lst
