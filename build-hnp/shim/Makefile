include ../utils/Makefrag

SRC = shim.c
BUILD_DIR = build
INSTALL_DIR = ../sysroot/bin
SHIM_BINARY = $(BUILD_DIR)/$(SHIM_NAME)

ifneq ($(CONFIG_PATH),)
	CONFIG_INCLUDE_DIR = $(dir $(CONFIG_PATH))
	SHIM_INCLUDES = -I$(CONFIG_INCLUDE_DIR)
endif

.PHONY: all clean install
all: $(SHIM_BINARY)

$(SHIM_BINARY): $(SRC)
	@if [ -z "$(SHIM_NAME)" ]; then echo "ERROR: SHIM_NAME is not set." >&2; exit 1; fi
	@if [ -z "$(CONFIG_PATH)" ]; then echo "ERROR: CONFIG_PATH is not set." >&2; exit 1; fi
	@if [ ! -f "$(CONFIG_PATH)" ]; then echo "ERROR: Config file not found: $(CONFIG_PATH)" >&2; exit 1; fi
	@if [ -z "$(CONFIG_INCLUDE_DIR)" ]; then echo "ERROR: CONFIG_INCLUDE_DIR is empty." >&2; exit 1; fi

	mkdir -p $(BUILD_DIR)

	@echo "==> [SHIM] Using config: $(CONFIG_PATH)"
	@echo "==> [SHIM] Compiling $(SRC) -> $(SHIM_BINARY)"
	$(CC) $(CFLAGS) $(SHIM_INCLUDES) -o "$(SHIM_BINARY)" $(SRC) $(LDFLAGS)

	@echo "==> [SHIM] Stripping $(SHIM_BINARY)"
	$(STRIP) "$(SHIM_BINARY)"
	@echo "==> [SHIM] Build complete: $(SHIM_BINARY)"

install: all
	@echo "==> [SHIM] Installing $(SHIM_BINARY) to $(INSTALL_DIR)"
	mkdir -p $(INSTALL_DIR)
	cp -f "$(SHIM_BINARY)" "$(INSTALL_DIR)/"
	@echo "==> [SHIM] Install complete."


# 清理
clean:
	@echo "==> [SHIM] Cleaning local build directory..."
	rm -rf $(BUILD_DIR)
