# 引入通用的构建配置和宏
include ../utils/Makefrag

# libzip 源代码下载信息 (更新为 1.11.4)
SOURCE_URL = https://libzip.org/download/libzip-1.11.4.tar.gz
SOURCE_FILE = libzip-1.11.4.tar.gz
SOURCE_DIR = libzip-1.11.4

# CMake 配置参数
# - CMAKE_INSTALL_PREFIX: 安装路径，使用 Makefrag 中的 $(PREFIX)
# - CMAKE_SYSTEM_NAME/PROCESSOR: 交叉编译目标系统和架构
# - BUILD_SHARED_LIBS/BUILD_STATIC_LIBS: 构建动态库，禁用静态库
# - CMAKE_BUILD_TYPE: 构建类型
# - ENABLE_*: 配置 libzip 的依赖项，这里启用 OpenSSL (假设已编译)
# - BUILD_TOOLS: 通常不需要在目标 sysroot 中安装命令行工具
# - ZLIB_INCLUDE_DIR/ZLIB_LIBRARY: 显式指定 Zlib 路径
CMAKE_ARGS = -DCMAKE_INSTALL_PREFIX=$(PREFIX) \
             -DCMAKE_SYSTEM_NAME=Linux \
             -DCMAKE_SYSTEM_PROCESSOR=$(OHOS_ARCH) \
             -DBUILD_SHARED_LIBS=ON \
             -DBUILD_STATIC_LIBS=OFF \
             -DCMAKE_BUILD_TYPE=RelWithDebInfo \
             -DENABLE_GNUTLS=OFF \
             -DENABLE_MBEDTLS=OFF \
             -DENABLE_OPENSSL=ON \
             -DBUILD_TOOLS=OFF \
             -DZLIB_INCLUDE_DIR=$(shell pwd)/../sysroot/include \
             -DZLIB_LIBRARY=$(shell pwd)/../sysroot/lib/libz.so

# 使用 Makefrag 中定义的 CMake 包构建宏
$(eval $(call define_cmake_package))