include ../utils/Makefrag

GCOMPAT_VERSION := 1.1.0-r4
GCOMPAT_APK := download/gcompat-$(GCOMPAT_VERSION).apk
GCOMPAT_URL := https://dl-cdn.alpinelinux.org/alpine/edge/main/aarch64/gcompat-$(GCOMPAT_VERSION).apk

LIBUCONTEXT_VERSION := 1.5-r0
LIBUCONTEXT_APK := download/libucontext-$(LIBUCONTEXT_VERSION).apk
LIBUCONTEXT_URL := https://dl-cdn.alpinelinux.org/alpine/edge/main/aarch64/libucontext-$(LIBUCONTEXT_VERSION).apk

MUSL_OBSTACK_VERSION := 1.2.3-r2
MUSL_OBSTACK_APK := download/musl-obstack-$(MUSL_OBSTACK_VERSION).apk
MUSL_OBSTACK_URL := https://dl-cdn.alpinelinux.org/alpine/edge/main/aarch64/musl-obstack-$(MUSL_OBSTACK_VERSION).apk

.PHONY: all clean

all: $(GCOMPAT_APK) $(LIBUCONTEXT_APK) $(MUSL_OBSTACK_APK)
	rm -rf build shim_config
	mkdir -p build

	mkdir -p build/temp
	tar -xf $(GCOMPAT_APK) -C build/temp
	tar -xf $(LIBUCONTEXT_APK) -C build/temp
	tar -xf $(MUSL_OBSTACK_APK) -C build/temp

	mkdir -p ../sysroot/lib
	# Copy libraries from /lib (gcompat) and /usr/lib (likely others) to sysroot
	if [ -d build/temp/lib ]; then cp -rfv build/temp/lib/. ../sysroot/lib/; fi
	if [ -d build/temp/usr/lib ]; then cp -rfv build/temp/usr/lib/. ../sysroot/lib/; fi

$(GCOMPAT_APK):
	mkdir -p download
	$(WGET) -O $@ $(GCOMPAT_URL)

$(LIBUCONTEXT_APK):
	mkdir -p download
	$(WGET) -O $@ $(LIBUCONTEXT_URL)

$(MUSL_OBSTACK_APK):
	mkdir -p download
	$(WGET) -O $@ $(MUSL_OBSTACK_URL)

clean:
	rm -rf build temp download file.lst