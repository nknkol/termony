include ../utils/Makefrag

export uname := Linux

SOURCE_URL = https://gitee.com/openharmony/third_party_bzip2/repository/archive/OpenHarmony-5.0.1-Release.zip
SOURCE_FILE = bzip2-OpenHarmony-5.0.1-Release.zip
SOURCE_UNZIP_DIR = third_party_bzip2-OpenHarmony-5.0.1-Release
BZIP2_SRC_DIR = $(CURDIR)/temp/$(SOURCE_UNZIP_DIR)
SETUP_STAMP = $(BZIP2_SRC_DIR)/.setup_done
VPATH = $(BZIP2_SRC_DIR)
C_SRCS_NAMES = \
    blocksort.c \
    bzlib.c \
    compress.c \
    crctable.c \
    decompress.c \
    huffman.c \
    randtable.c
SRC_FILES = $(addprefix $(BZIP2_SRC_DIR)/, $(C_SRCS_NAMES))
OBJS = $(addprefix build/obj/, $(C_SRCS_NAMES:.c=.o))
TARGET_LIB = build/libbz2.a
all: $(TARGET_LIB) install

$(SETUP_STAMP): download/$(SOURCE_FILE)
	@echo "Cleaning up bzip2 directories..."
	rm -rf temp build
	mkdir -p temp
	@echo "Unpacking bzip2 source archive and applying patches..."
	cd temp && unzip -q ../download/$(SOURCE_FILE)
	# 运行 install.sh, 它会处理内部的 tar.gz 和 patch
	sh $(BZIP2_SRC_DIR)/install.sh $(BZIP2_SRC_DIR)
	@touch $@

build/obj:
	mkdir -p $@
	
$(BZIP2_SRC_DIR)/%.c: $(SETUP_STAMP)
	@true

build/obj/%.o: $(BZIP2_SRC_DIR)/%.c | build/obj
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) $(CPPFLAGS) -I$(BZIP2_SRC_DIR) -c $< -o $@

$(TARGET_LIB): $(OBJS)
	@echo "Creating static library $(TARGET_LIB)..."
	$(AR) rcs $@ $(OBJS)

install: $(TARGET_LIB)
	@echo "Installing bzip2 to sysroot..."
	mkdir -p ../sysroot/lib
	mkdir -p ../sysroot/include
	cp -fv $(TARGET_LIB) ../sysroot/lib/ | tee file.lst
	cp -fv $(BZIP2_SRC_DIR)/bzlib.h ../sysroot/include/ | tee -a file.lst # 使用 -a 追加到文件
	@echo "Installation complete."

download/$(SOURCE_FILE):
	mkdir -p download
	cd download && $(WGET) -O $(SOURCE_FILE) $(SOURCE_URL)

.PHONY: all install

.SECONDARY: $(OBJS) $(SRC_FILES)
