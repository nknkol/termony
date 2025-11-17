include ../utils/Makefrag

ifeq ($(strip $(OHOS_ARCH)),)
$(error OHOS_ARCH is not set. Please source your env (e.g. support/setup-env.sh) or export OHOS_ARCH=aarch64|x86_64)
endif

SOURCE_URL = https://github.com/NixOS/patchelf/releases/download/0.18.0/patchelf-0.18.0.tar.bz2
SOURCE_FILE = patchelf-0.18.0.tar.bz2
SOURCE_DIR = patchelf-0.18.0
HOST_TRIPLE ?= $(OHOS_ARCH)-unknown-linux-musl
CONFIG_ARGS = --prefix=$(PREFIX) --host=$(HOST_TRIPLE)

$(eval $(call define_autotools_package))

# Override download rule to auto-redownload when cache is corrupted.
download/$(SOURCE_FILE):
	mkdir -p download
	@if [ -f download/$(SOURCE_FILE) ]; then \
		if bzip2 -t download/$(SOURCE_FILE) >/dev/null 2>&1; then \
			echo "Using cached $(SOURCE_FILE)"; \
		else \
			echo "Corrupted cache detected, re-downloading $(SOURCE_FILE)"; \
			rm -f download/$(SOURCE_FILE); \
		fi; \
	fi
	@if [ ! -f download/$(SOURCE_FILE) ]; then \
		cd download && $(WGET) -O $(SOURCE_FILE) $(SOURCE_URL); \
	fi
