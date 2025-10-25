# 包含父级的 Makefrag 来获取 WGET 等变量
include ../utils/Makefrag

# 目标程序
TARGET_NAME = hapsigntool
TARGET      = build/$(TARGET_NAME)

# 编译器
CC      := $(CC)
CXX     := $(CXX)
LD      := $(CXX)
AR      := $(AR)
STRIP   := $(STRIP)

# 目录
OBJ_DIR   = build/obj
DEP_DIR   = build/dep

# --- 依赖定义 ---
# 1. 定义 securec 依赖
SECUREC_DIR = $(CURDIR)/temp/securec
SECUREC_ZIP = download/securec.zip
SECUREC_STAMP = $(SECUREC_DIR)/include/securec.h

# 2. 定义 nlohmann_json 依赖
JSON_DIR = $(CURDIR)/temp/nlohmann_json
JSON_ZIP = download/nlohmann_json.zip
JSON_STAMP = $(JSON_DIR)/include/nlohmann/json.hpp

# 3. 定义 zlib/minizip 依赖
ZLIB_SRC_DIR = $(abspath $(CURDIR)/../zlib/temp/zlib-1.3.1)
ZLIB_STAMP = $(ZLIB_SRC_DIR)/contrib/minizip/unzip.h

# 4. (新) 定义 hapsigntool_cpp 源码依赖
HAPSIGNTOOL_REPO_ZIP = download/hapsigner_repo.zip
HAPSIGNTOOL_CPP_SRC_DIR = $(CURDIR)/temp/hapsigntool_cpp
HAPSIGNTOOL_CPP_STAMP = $(HAPSIGNTOOL_CPP_SRC_DIR)/main.cpp # 使用 main.cpp 作为依赖标记

# 内部头文件 (已修改，指向 HAPSIGNTOOL_CPP_SRC_DIR)
INTERNAL_INCLUDES = \
    -I$(HAPSIGNTOOL_CPP_SRC_DIR)/api/include \
  -I$(HAPSIGNTOOL_CPP_SRC_DIR)/signer/include \
    -I$(HAPSIGNTOOL_CPP_SRC_DIR)/cmd/include \
    -I$(HAPSIGNTOOL_CPP_SRC_DIR)/codesigning/datastructure/include \
    -I$(HAPSIGNTOOL_CPP_SRC_DIR)/codesigning/fsverity/include \
    -I$(HAPSIGNTOOL_CPP_SRC_DIR)/codesigning/utils/include \
    -I$(HAPSIGNTOOL_CPP_SRC_DIR)/codesigning/sign/include \
    -I$(HAPSIGNTOOL_CPP_SRC_DIR)/common/include \
    -I$(HAPSIGNTOOL_CPP_SRC_DIR)/hap/verify/include \
    -I$(HAPSIGNTOOL_CPP_SRC_DIR)/hap/config/include \
    -I$(HAPSIGNTOOL_CPP_SRC_DIR)/hap/entity/include \
    -I$(HAPSIGNTOOL_CPP_SRC_DIR)/hap/provider/include \
    -I$(HAPSIGNTOOL_CPP_SRC_DIR)/hap/sign/include \
    -I$(HAPSIGNTOOL_CPP_SRC_DIR)/hap/utils/include \
    -I$(HAPSIGNTOOL_CPP_SRC_DIR)/profile/include \
    -I$(HAPSIGNTOOL_CPP_SRC_DIR)/utils/include \
    -I$(HAPSIGNTOOL_CPP_SRC_DIR)/zip/include \
    -I$(CURDIR)/../sysroot/include

# 外部头文件
EXTERNAL_INCLUDES =

# --- 编译标志 (已修改) ---
# 4. 定义通用的 Includes 和 Flags (用于 C 和 C++)
HAPSIGNTOOL_INCLUDES = -I$(SECUREC_DIR)/include \
-I$(JSON_DIR)/include -I$(ZLIB_SRC_DIR) $(INTERNAL_INCLUDES) $(EXTERNAL_INCLUDES)

# 5. C 编译标志
#    使用 Makefrag 中的 $(CFLAGS) + $(CPPFLAGS) + 我们的 Includes
HAPSIGNTOOL_CFLAGS = $(CFLAGS) $(CPPFLAGS) $(HAPSIGNTOOL_INCLUDES)

# 6. C++ 编译标志
#    使用 Makefrag 中的 $(CXXFLAGS) + $(CPPFLAGS) + 我们的 Includes + C++17
#    (已修正： -std:c++17 改为 -std=c++17)
HAPSIGNTOOL_CXXFLAGS = $(CXXFLAGS) $(CPPFLAGS) $(HAPSIGNTOOL_INCLUDES) -std=c++17

# 链接标志和库
HAPSIGNTOOL_LDFLAGS = $(LDFLAGS)
HAPSIGNTOOL_LIBS    = -lssl -lcrypto -lbz2 -lz
# --- 标志定义结束 ---


# --- 源文件 ---
# C++ 源文件 (已修改，指向 HAPSIGNTOOL_CPP_SRC_DIR)
CPP_SRCS = $(wildcard $(HAPSIGNTOOL_CPP_SRC_DIR)/api/src/*.cpp) \
           $(wildcard $(HAPSIGNTOOL_CPP_SRC_DIR)/signer/src/*.cpp) \
           $(wildcard $(HAPSIGNTOOL_CPP_SRC_DIR)/cmd/src/*.cpp) \
    $(wildcard $(HAPSIGNTOOL_CPP_SRC_DIR)/codesigning/datastructure/src/*.cpp) \
           $(wildcard $(HAPSIGNTOOL_CPP_SRC_DIR)/codesigning/fsverity/src/*.cpp) \
           $(wildcard $(HAPSIGNTOOL_CPP_SRC_DIR)/codesigning/utils/src/*.cpp) \
           $(wildcard $(HAPSIGNTOOL_CPP_SRC_DIR)/codesigning/sign/src/*.cpp) \
           $(wildcard $(HAPSIGNTOOL_CPP_SRC_DIR)/common/src/*.cpp) \
           $(wildcard $(HAPSIGNTOOL_CPP_SRC_DIR)/hap/verify/src/*.cpp) \
           $(wildcard $(HAPSIGNTOOL_CPP_SRC_DIR)/hap/config/src/*.cpp) \
           $(wildcard $(HAPSIGNTOOL_CPP_SRC_DIR)/hap/entity/src/*.cpp) \
        $(wildcard $(HAPSIGNTOOL_CPP_SRC_DIR)/hap/provider/src/*.cpp) \
           $(wildcard $(HAPSIGNTOOL_CPP_SRC_DIR)/hap/sign/src/*.cpp) \
           $(wildcard $(HAPSIGNTOOL_CPP_SRC_DIR)/hap/utils/src/*.cpp) \
           $(wildcard $(HAPSIGNTOOL_CPP_SRC_DIR)/profile/src/*.cpp) \
           $(wildcard $(HAPSIGNTOOL_CPP_SRC_DIR)/utils/src/*.cpp) \
           $(wildcard $(HAPSIGNTOOL_CPP_SRC_DIR)/zip/src/*.cpp) \
           $(HAPSIGNTOOL_CPP_SRC_DIR)/main.cpp

# C 源文件 (securec + minizip) (保持不变)
C_SRCS = $(wildcard $(SECUREC_DIR)/src/*.c)
C_SRCS += $(ZLIB_SRC_DIR)/contrib/minizip/ioapi.c
C_SRCS += $(ZLIB_SRC_DIR)/contrib/minizip/unzip.c

# 合并所有源文件
SRCS \
= $(CPP_SRCS) $(C_SRCS)
# --- 源文件结束 ---

# 默认目标
all: install

# 目标文件和依赖文件 (保持不变)
OBJS = $(addprefix $(OBJ_DIR)/, $(addsuffix .o, $(basename $(SRCS))))
DEPS = $(addprefix $(DEP_DIR)/, $(addsuffix .d, $(basename $(SRCS))))

-include $(DEPS)

# --- 依赖规则 ---
# 规则：如何解压 securec.zip
$(SECUREC_STAMP): $(SECUREC_ZIP)
	@echo "Unpacking securec for hapsigntool..."
	@mkdir -p $(CURDIR)/temp
	@unzip -q $< -d $(CURDIR)/temp
	@mv $(CURDIR)/temp/third_party_bounds_checking_function-OpenHarmony-5.0.1-Release $(SECUREC_DIR)
	@touch $@

# 规则：如何下载 securec.zip
$(SECUREC_ZIP):
	@echo "Downloading securec source archive..."
	@mkdir -p $(dir $@)
	@$(WGET) -O $@ https://gitee.com/openharmony/third_party_bounds_checking_function/repository/archive/OpenHarmony-5.0.1-Release.zip

# 规则：如何解压 nlohmann_json.zip
$(JSON_STAMP): $(JSON_ZIP)
	@echo "Unpacking nlohmann_json for hapsigntool..."
	@mkdir -p $(CURDIR)/temp
	@unzip -q $< -d $(CURDIR)/temp
	@mv $(CURDIR)/temp/json-3.12.0 $(JSON_DIR)
	@touch $@

# 规则：如何下载 nlohmann_json.zip
$(JSON_ZIP):
	@echo "Downloading nlohmann_json source archive..."
	@mkdir -p $(dir $@)
	@$(WGET) -O $@ https://github.com/nlohmann/json/archive/refs/tags/v3.12.0.zip
# --- 

# --- (新) hapsigntool_cpp 源码规则 ---
# 规则：如何解压和复制 hapsigntool_cpp
$(HAPSIGNTOOL_CPP_STAMP): $(HAPSIGNTOOL_REPO_ZIP)
	@echo "Unpacking hapsigner repo and copying hapsigntool_cpp..."
	@mkdir -p $(CURDIR)/temp
	@unzip -q $< -d $(CURDIR)/temp
	@# 查找解压后的仓库目录 (通常类似 'developtools_hapsigner-master')
	@TEMP_REPO_DIR=$$(find $(CURDIR)/temp -maxdepth 1 -type d -name "developtools_hapsigner*") && \
	if [ -n "$$TEMP_REPO_DIR" ] && [ -d "$$TEMP_REPO_DIR/hapsigntool_cpp" ]; then \
		echo "Found repo at $$TEMP_REPO_DIR, moving 'hapsigntool_cpp' to $(HAPSIGNTOOL_CPP_SRC_DIR)"; \
		mv "$$TEMP_REPO_DIR/hapsigntool_cpp" $(HAPSIGNTOOL_CPP_SRC_DIR); \
		rm -rf "$$TEMP_REPO_DIR"; \
	else \
		echo "Error: Could not find 'hapsigntool_cpp' directory in unzipped archive at $(CURDIR)/temp."; \
		exit 1; \
	fi
	@touch $@

# 规则：如何下载 hapsigntool_cpp 仓库 (假设 master 分支)
$(HAPSIGNTOOL_REPO_ZIP):
	@echo "Downloading hapsigner repo source archive (master branch)..."
	@mkdir -p $(dir $@)
	@$(WGET) -O $@ https://gitee.com/openharmony/developtools_hapsigner/repository/archive/master.zip
# --- 

# --- 编译规则 (已修改，添加 HAPSIGNTOOL_CPP_STAMP 依赖) ---
# 编译 C++
$(OBJ_DIR)/%.o: %.cpp $(SECUREC_STAMP) $(JSON_STAMP) $(ZLIB_STAMP) $(HAPSIGNTOOL_CPP_STAMP)
	@echo "Compiling $<..."
	@mkdir -p $(dir $@)
	@mkdir \
-p $(dir $(DEP_DIR)/$(<:.cpp=.d))
	$(CXX) $(HAPSIGNTOOL_CXXFLAGS) -c $< -o $@
	@$(CXX) -MM $(HAPSIGNTOOL_CXXFLAGS) $< -MT $@ > $(DEP_DIR)/$(<:.cpp=.d)

# 编译 C
# (也添加 HAPSIGNTOOL_CPP_STAMP 依赖，因为 HAPSIGNTOOL_CFLAGS 间接包含了 hapsigntool_cpp 的头文件路径)
$(OBJ_DIR)/%.o: %.c $(SECUREC_STAMP) $(JSON_STAMP) $(ZLIB_STAMP) $(HAPSIGNTOOL_CPP_STAMP)
	@echo "Compiling $<..."
	@mkdir -p $(dir $@)
	@mkdir -p $(dir $(DEP_DIR)/$(<:.c=.d))
	$(CC) $(HAPSIGNTOOL_CFLAGS) -c $< -o $@
	@$(CC) -MM $(HAPSIGNTOOL_CFLAGS) $< -MT $@ > $(DEP_DIR)/$(<:.c=.d)
# --- 

# 链接
$(TARGET): $(OBJS)
	@echo "Linking $(TARGET_NAME)..."
	@mkdir -p $(dir $@)
	$(LD) $(HAPSIGNTOOL_LDFLAGS) $^ -o $@ $(HAPSIGNTOOL_LIBS)

# 清理
clean:
	@echo "Cleaning up..."
	rm -rf build
	rm -rf temp
	rm -rf download

# --- 添加 install 规则 ---
install: $(TARGET)
	@echo "Installing $(TARGET_NAME) to sysroot..."
	mkdir -p ../sysroot/bin
	cp -fv $(TARGET) ../sysroot/bin/ | \
tee file.lst
	@echo "Installation complete."

# 将 install 添加到 .PHONY 规则中
.PHONY: all install clean