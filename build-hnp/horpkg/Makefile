include ../utils/Makefrag

all:
	rm -rf temp build
	mkdir -p build$(PREFIX)/bin
	# 编译 horpkg
	$(CC) $(CFLAGS) -Wno-unused-parameter -std=c11 \
		-o build$(PREFIX)/bin/horpkg \
		src/*.c \
		-I../sysroot/include \
		-L../sysroot/lib -lcurl -lyyjson
	# 复制到 sysroot
	mkdir -p ../sysroot
	cp -rfv build$(PREFIX)/. ../sysroot | tee file.lst #