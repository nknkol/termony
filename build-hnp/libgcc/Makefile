include ../utils/Makefrag
  
LIBGCC_VERSION := 13.2.1_git20231014-r0
LIBGCC_URL := https://dl-cdn.alpinelinux.org/alpine/v3.19/main/aarch64/libgcc-$(LIBGCC_VERSION).apk
LIBSTDC_URL := https://dl-cdn.alpinelinux.org/alpine/v3.19/main/aarch64/libstdc++-$(LIBGCC_VERSION).apk

LIBGCC_APK := download/libgcc-$(LIBGCC_VERSION).apk
LIBSTDC_APK := download/libstdc++-$(LIBGCC_VERSION).apk

.PHONY: all clean

all: $(LIBGCC_APK) $(LIBSTDC_APK)
	rm -rf build
	mkdir -p build

	tar -xf $(LIBGCC_APK) -C build
	tar -xf $(LIBSTDC_APK) -C build

	mkdir -p ../sysroot/lib
	cp -rfv build/usr/lib/. ../sysroot/lib/ | tee file.lst

$(LIBGCC_APK):
	mkdir -p download
	$(WGET) -O $@ $(LIBGCC_URL)

$(LIBSTDC_APK):
	mkdir -p download
	$(WGET) -O $@ $(LIBSTDC_URL)

clean:
	rm -rf build download file.lst
