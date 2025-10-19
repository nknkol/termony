ifeq ($(OHOS_ARCH),aarch64)
PKGS=yyjson \
	horpkg \
	busybox \
	bash \
	ncurses \
	readline \
	zlib \
	lz4 \
	openssl \
	c-ares \
	curl \
	tar \
	xz \
	libusb \
	hdc
else ifeq ($(OHOS_ARCH),x86_64)
PKGS=yyjson \
	horpkg \
	busybox \
	bash \
	ncurses \
	readline \
	zlib \
	lz4 \
	openssl \
	c-ares \
	curl \
	tar \
	xz \
	libusb \
	hdc
else
	echo "Unsupported OHOS_ARCH=$(OHOS_ARCH)"
	exit 1
endif

STAMP=$(patsubst %,%/.stamp,$(PKGS))

all: copy

copy: horpkg-base.hnp
	rm -f ../entry/hnp/$(OHOS_ABI)/*.hnp
	cp $^ ../entry/hnp/$(OHOS_ABI)
	cp $^ ../entry/hnp/$(OHOS_ABI)/horpkg-public.hnp

horpkg-base.hnp: $(STAMP) utils/pbcopy utils/pbpaste utils/empty.a Makefile hnp.json
	# reduce size
	rm -rfv sysroot/share/man
	rm -rfv sysroot/share/doc
	rm -rfv sysroot/share/info
	# add pasteboard utilities
	cp utils/pbcopy sysroot/bin
	cp utils/pbpaste sysroot/bin
	# let gcc work out of box
	mkdir -p sysroot/$(OHOS_ARCH)-unknown-linux-musl/lib
	cp $(OHOS_SDK_HOME)/native/sysroot/usr/lib/$(OHOS_ARCH)-linux-ohos/crt1.o sysroot/$(OHOS_ARCH)-unknown-linux-musl/lib/crt1.o
	cp $(OHOS_SDK_HOME)/native/sysroot/usr/lib/$(OHOS_ARCH)-linux-ohos/crti.o sysroot/$(OHOS_ARCH)-unknown-linux-musl/lib/crti.o
	cp $(OHOS_SDK_HOME)/native/sysroot/usr/lib/$(OHOS_ARCH)-linux-ohos/crtn.o sysroot/$(OHOS_ARCH)-unknown-linux-musl/lib/crtn.o
	cp $(OHOS_SDK_HOME)/native/llvm/lib/clang/15.0.4/lib/$(OHOS_ARCH)-linux-ohos/clang_rt.crtbegin.o sysroot/$(OHOS_ARCH)-unknown-linux-musl/lib/crtbegin.o
	cp $(OHOS_SDK_HOME)/native/llvm/lib/clang/15.0.4/lib/$(OHOS_ARCH)-linux-ohos/clang_rt.crtend.o sysroot/$(OHOS_ARCH)-unknown-linux-musl/lib/crtend.o
	cp $(OHOS_SDK_HOME)/native/llvm/lib/clang/15.0.4/lib/$(OHOS_ARCH)-linux-ohos/libclang_rt.builtins.a sysroot/$(OHOS_ARCH)-unknown-linux-musl/lib/libgcc.a
	cp $(OHOS_SDK_HOME)/native/llvm/lib/clang/15.0.4/lib/$(OHOS_ARCH)-linux-ohos/libclang_rt.builtins.a sysroot/$(OHOS_ARCH)-unknown-linux-musl/lib/libgcc_s.a
	cp utils/empty.a sysroot/$(OHOS_ARCH)-unknown-linux-musl/lib/libgcc_eh.a
	# create hnp manually
	# $(OHOS_SDK_HOME)/toolchains/hnpcli pack -i sysroot -n base -v 1.0
	cp hnp.json sysroot
	rm -f horpkg-base.hnp
	zip -r horpkg-base.hnp sysroot

%/.stamp: %/Makefile
	make -C $(patsubst %/.stamp,%,$@)
	touch $@

rebuild-%:
	make -C $(patsubst rebuild-%,%,$@)
	touch $(patsubst rebuild-%,%/.stamp,$@)

clean:
	rm -f $(STAMP)
	rm -rf sysroot
