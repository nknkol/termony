# build-hnp 编译依赖迁移方案

## 文档信息
- 版本: 1.0
- 创建日期: 2026-02-03
- 目标: 最大程度降低依赖编译需求

---

## 一、方案概述

### 1.1 目标
- 减少需要编译的包数量
- 最大化使用预编译二进制
- 缩短构建时间
- 降低构建复杂性

### 1.2 策略分类
| 策略 | 说明 | 优先级 |
|------|------|--------|
| 使用系统自带库 | 利用目标系统已有的库 | P0 |
| 使用预编译二进制 | 使用官方或社区提供的预编译包 | P0 |
| 功能裁剪 | 移除不必要的依赖 | P1 |
| 静态链接替代动态 | 减少运行时依赖 | P2 |
| 替代方案 | 使用更轻量的替代库 | P2 |

---

## 二、现状分析

### 2.1 当前需要编译的包统计

| 包名 | 是否必需 | 编译时间估算 | 是否可替代 |
|------|---------|-------------|-----------|
| zlib | 必需 | 低 | 是 (系统库) |
| openssl | 必需 | 高 | 是 (系统库) |
| libusb | 可选 | 低 | 是 (功能裁剪) |
| lz4 | 可选 | 低 | 是 (功能裁剪) |
| xz | 可选 | 低 | 是 (系统库) |
| c-ares | 可选 | 低 | 是 (功能裁剪) |
| yyjson | 必需 | 低 | 否 (项目特有) |
| curl | 必需 | 中 | 是 (系统库) |
| tar | 可选 | 中 | 是 (系统库) |
| hdc-lite | 必需 | 高 | 否 (OpenHarmony 专用) |
| libzip | 可选 | 中 | 是 (功能裁剪) |
| horpkg | 必需 | 低 | 否 (项目核心) |
| bzip2 | 可选 | 低 | 是 (系统库) |
| elf-loader | 必需 | 中 | 否 (项目核心) |
| userlandexec | 必需 | 高 | 否 (项目核心) |
| restool | 可选 | 高 | 是 (功能裁剪) |
| openharmonytoolchains | 必需 | 很高 | 是 (使用系统 JDK) |
| shim | 必需 | 低 | 否 (必需) |

### 2.2 编译依赖链

```
必需核心链:
zlib → openssl → curl → horpkg
       ↘ libzip ↗

OpenHarmony 专用链:
hdc-lite, elf-loader, userlandexec, horpkg, shim

可选依赖链:
lz4, xz, c-ares, yyjson, libusb, tar, bzip2, restool, openharmonytoolchains
```

---

## 三、迁移方案详细设计

### 3.1 高优先级迁移 (P0)

#### 3.1.1 使用 OpenHarmony 系统库

**目标包**: zlib, openssl, curl, xz, bzip2, tar

**方案**:
1. **zlib** - 使用系统自带
   - 位置: `/usr/lib/libz.so` 或 OHOS SDK 内置
   - 操作: 移除 zlib 包编译，直接链接系统库
   - 风险: 版本不兼容
   - 验证: 检查 OHOS SDK 是否包含 zlib

2. **openssl** - 使用系统自带
   - 位置: `/usr/lib/libssl.so`, `/usr/lib/libcrypto.so`
   - 操作: 移除 openssl 包编译，直接链接系统库
   - 风险: 版本不兼容，API 差异
   - 验证: 检查 OHOS SDK 是否包含 openssl

3. **curl** - 使用系统自带
   - 位置: `/usr/lib/libcurl.so`
   - 操作: 移除 curl 包编译，直接链接系统库
   - 风险: 可能缺少某些功能
   - 验证: 检查 OHOS SDK 是否包含 curl

4. **xz** - 使用系统自带
   - 位置: `/usr/lib/liblzma.so`
   - 操作: 移除 xz 包编译，直接链接系统库
   - 风险: 版本不兼容
   - 验证: 检查 OHOS SDK 是否包含 xz

5. **bzip2** - 使用系统自带
   - 位置: `/usr/lib/libbz2.so`
   - 操作: 移除 bzip2 包编译，直接链接系统库
   - 风险: 静态库改动态库
   - 验证: 检查 OHOS SDK 是否包含 bzip2

6. **tar** - 使用系统自带
   - 位置: `/bin/tar`
   - 操作: 移除 tar 包编译，使用系统命令
   - 风险: 功能差异
   - 验证: 检查 OHOS 系统是否包含 tar

**实施步骤**:
```bash
# 1. 检查系统库
find $(OHOS_SDK_HOME) -name "libz.so*" -o -name "libssl.so*" -o -name "libcurl.so*" -o -name "liblzma.so*" -o -name "libbz2.so*"

# 2. 如果存在，修改 Makefile
# 在 utils/Makefrag 中添加条件判断
ifdef USE_SYSTEM_LIBS
  LIBZ_PATH = $(shell find $(OHOS_SDK_HOME) -name "libz.so" | head -1)
  CFLAGS += -I$(dir $(LIBZ_PATH))../include
  LDFLAGS += -L$(dir $(LIBZ_PATH)) -lz
endif

# 3. 在主 Makefile 中移除相应包
PKGS = yyjson \
       libzip \
       horpkg \
       elf-loader \
       userlandexec \
       hdc-lite \
       restool \
       shim
```

**预期收益**: 减少编译时间约 60%

---

#### 3.1.2 移除非必需的 restool 包

**目标包**: restool

**方案**:
1. 分析 restool 的实际用途
2. 如果非核心功能，直接移除
3. 如果可选，提供环境变量控制

**实施步骤**:
```makefile
# 在主 Makefile 中添加条件
ifndef BUILD_RESTOOL
  PKGS := $(filter-out restool,$(PKGS))
endif
```

**预期收益**: 减少编译时间约 15%

---

#### 3.1.3 使用系统 JDK 替代 openharmonytoolchains

**目标包**: openharmonytoolchains

**方案**:
1. 检查 OpenHarmony 系统是否内置 JDK
2. 如果存在，直接使用系统 JDK
3. 仅部署 JAR 文件，不编译 shim

**实施步骤**:
```bash
# 1. 检查系统 JDK
find $(OHOS_SDK_HOME) -name "java" -type f

# 2. 如果存在，修改 Makefile
ifdef USE_SYSTEM_JDK
  # 跳过 openharmonytoolchains 编译
  PKGS := $(filter-out openharmonytoolchains,$(PKGS))
endif

# 3. 仅复制 JAR 文件
install-system-jdk:
  mkdir -p ../sysroot/opt/jdk/lib
  cp $(OHOS_SDK_HOME)/toolchains/lib/*.jar ../sysroot/opt/jdk/lib/
```

**预期收益**: 减少编译时间约 10%

---

### 3.2 中优先级迁移 (P1)

#### 3.2.1 功能裁剪 - 移除可选压缩库

**目标包**: lz4, xz, c-ares, libusb

**方案**:
1. **lz4** - 如果 horpkg 不使用 lz4 压缩，移除
2. **xz** - 已考虑使用系统库
3. **c-ares** - 如果 curl 可以使用系统 DNS，移除
4. **libusb** - 如果 hdc-lite 可以工作在非 USB 模式，移除

**实施步骤**:
```makefile
# 在主 Makefile 中添加条件
ifndef ENABLE_COMPRESSION
  PKGS := $(filter-out lz4 xz,$(PKGS))
endif

ifndef ENABLE_USB
  PKGS := $(filter-out libusb,$(PKGS))
endif

ifndef ENABLE_CARES
  PKGS := $(filter-out c-ares,$(PKGS))
endif
```

**horpkg 修改**:
```c
// 移除对 lz4 的依赖
#ifdef HAVE_LZ4
//    lz4 相关代码
#endif
```

**预期收益**: 减少编译时间约 10%

---

#### 3.2.2 简化 libzip 依赖

**目标包**: libzip

**方案**:
1. 如果 horpkg 仅需要基础的 zip 操作，实现简单版本
2. 或者使用系统的 libzip（如果存在）

**实施步骤**:
```makefile
# 检查系统 libzip
ifdef USE_SYSTEM_LIBZIP
  PKGS := $(filter-out libzip,$(PKGS))
endif
```

**预期收益**: 减少编译时间约 5%

---

### 3.3 低优先级迁移 (P2)

#### 3.2.1 静态链接替代动态链接

**目标**: 减少运行时依赖

**方案**:
1. 将某些动态库改为静态链接
2. 缺点: 增加二进制大小

**实施步骤**:
```makefile
# 修改 LDFLAGS
LDFLAGS += -static-libgcc -static-libstdc++

# 或在编译时指定
./configure --enable-static --disable-shared
```

---

#### 3.2.2 使用替代库

**目标包**: c-ares → 使用系统 resolver
          yyjson → 使用 cJSON 或系统 JSON 库

**方案**:
1. 如果 yyjson 功能简单，替换为更轻量的方案
2. 使用系统 DNS 解析替代 c-ares

**实施步骤**:
```makefile
# 修改 curl 配置
CONFIG_ARGS += --disable-threaded-resolver --disable-ares
```

---

## 四、分阶段实施计划

### Phase 1: 验证系统库可用性 (1-2 天)

**目标**: 确定哪些库可以直接使用系统版本

**任务**:
1. [ ] 检查 OHOS SDK 中的系统库
2. [ ] 验证系统库版本兼容性
3. [ ] 测试链接系统库的可行性
4. [ ] 生成系统库可用性报告

**输出**:
- `system-libs-availability.md` - 系统库可用性报告

---

### Phase 2: 实施高优先级迁移 (3-5 天)

**目标**: 实现 P0 级别的迁移

**任务**:
1. [ ] 修改 Makefile 支持使用系统库
2. [ ] 移除不必要的包 (restool)
3. [ ] 实现使用系统 JDK 的方案
4. [ ] 更新文档
5. [ ] 测试验证

**输出**:
- 修改后的 Makefile
- 迁移测试报告

---

### Phase 3: 实施中优先级迁移 (2-3 天)

**目标**: 实现 P1 级别的迁移

**任务**:
1. [ ] 添加功能裁剪选项
2. [ ] 修改 horpkg 移除可选依赖
3. [ ] 简化 libzip 依赖
4. [ ] 测试验证

**输出**:
- 功能裁剪配置选项
- 测试报告

---

### Phase 4: 优化和文档 (1-2 天)

**目标**: 优化方案，完善文档

**任务**:
1. [ ] 性能对比测试
2. [ ] 编写迁移指南
3. [ ] 更新 README
4. [ ] 生成最终报告

**输出**:
- `MIGRATION_GUIDE.md` - 迁移指南
- `PERFORMANCE_REPORT.md` - 性能报告

---

## 五、配置选项设计

### 5.1 环境变量

| 变量 | 默认值 | 说明 |
|------|--------|------|
| `USE_SYSTEM_LIBS` | `false` | 使用系统库 |
| `BUILD_RESTOOL` | `false` | 构建 restool |
| `ENABLE_COMPRESSION` | `true` | 启用压缩库 |
| `ENABLE_USB` | `true` | 启用 USB 支持 |
| `ENABLE_CARES` | `true` | 启用 c-ares |
| `USE_SYSTEM_JDK` | `false` | 使用系统 JDK |
| `USE_SYSTEM_LIBZIP` | `false` | 使用系统 libzip |

### 5.2 Makefile 配置示例

```makefile
# 主 Makefile 开头添加

# 系统库配置
USE_SYSTEM_LIBS ?= false
ifeq ($(USE_SYSTEM_LIBS),true)
  # 移除以下包的编译
  PKGS := $(filter-out zlib openssl curl xz bzip2,$(PKGS))

  # 添加系统库路径
  SYSROOT_LIBS := $(shell find $(OHOS_SDK_HOME) -type d -name lib)
  CFLAGS += $(foreach dir,$(SYSROOT_LIBS),-I$(dir)/../include)
  LDFLAGS += $(foreach dir,$(SYSROOT_LIBS),-L$(dir))
endif

# 功能裁剪配置
ENABLE_COMPRESSION ?= true
ifeq ($(ENABLE_COMPRESSION),false)
  PKGS := $(filter-out lz4 xz,$(PKGS))
endif

ENABLE_USB ?= true
ifeq ($(ENABLE_USB),false)
  PKGS := $(filter-out libusb,$(PKGS))
endif

ENABLE_CARES ?= true
ifeq ($(ENABLE_CARES),false)
  PKGS := $(filter-out c-ares,$(PKGS))
endif

# 可选工具配置
BUILD_RESTOOL ?= false
ifeq ($(BUILD_RESTOOL),false)
  PKGS := $(filter-out restool,$(PKGS))
endif

# JDK 配置
USE_SYSTEM_JDK ?= false
ifeq ($(USE_SYSTEM_JDK),true)
  PKGS := $(filter-out openharmonytoolchains,$(PKGS))
endif

# libzip 配置
USE_SYSTEM_LIBZIP ?= false
ifeq ($(USE_SYSTEM_LIBZIP),true)
  PKGS := $(filter-out libzip,$(PKGS))
endif
```

---

## 六、预期效果

### 6.1 编译时间对比

| 场景 | 原始编译时间 | 优化后编译时间 | 减少 |
|------|------------|--------------|------|
| 完整编译 | 100% | 100% | 0% |
| Phase 2 (P0) | 100% | 25% | 75% |
| Phase 3 (P1) | 100% | 15% | 85% |
| 最小化编译 | 100% | 10% | 90% |

### 6.2 包数量对比

| 场景 | 编译包数 |
|------|---------|
| 完整编译 | 18 |
| Phase 2 (P0) | 8 |
| Phase 3 (P1) | 5 |
| 最小化编译 | 3 (horpkg, elf-loader, userlandexec) |

### 6.3 二进制大小对比

| 场景 | 估计大小变化 |
|------|------------|
| 使用系统动态库 | 减少 40-50% |
| 静态链接 | 增加 20-30% |
| 移除可选功能 | 减少 10-15% |

---

## 七、风险评估

### 7.1 高风险项

| 风险 | 影响 | 概率 | 缓解措施 |
|------|------|------|---------|
| 系统库版本不兼容 | 高 | 中 | 版本检查、API 兼容测试 |
| 移除必需功能 | 高 | 低 | 功能分析、灰度发布 |
| API 变更导致编译失败 | 中 | 低 | 接口抽象、适配层 |

### 7.2 中风险项

| 风险 | 影响 | 概率 | 缓解措施 |
|------|------|------|---------|
| 性能下降 | 中 | 低 | 性能测试、优化 |
| 运行时依赖缺失 | 中 | 中 | 依赖检查、部署验证 |

### 7.3 低风险项

| 风险 | 影响 | 概率 | 缓解措施 |
|------|------|------|---------|
| 二进制大小增加 | 低 | 低 | 静态分析、优化 |
| 构建脚本复杂度增加 | 低 | 低 | 文档、测试 |

---

## 八、回退方案

### 8.1 快速回退

如果迁移出现问题，可以快速回退到原始配置:

```bash
# 恢复原始配置
export USE_SYSTEM_LIBS=false
export BUILD_RESTOOL=false
export ENABLE_COMPRESSION=true
export ENABLE_USB=true
export ENABLE_CARES=true
export USE_SYSTEM_JDK=false
export USE_SYSTEM_LIBZIP=false

# 清理并重新编译
make clean
make all
```

### 8.2 版本管理

使用 Git 分支管理迁移:

```bash
# 创建迁移分支
git checkout -b migration/phase1

# 完成迁移后，可以创建标签
git tag migration/phase1-completed

# 如果需要回退
git checkout main
```

---

## 九、测试计划

### 9.1 单元测试

- [ ] 各包的功能测试
- [ ] 库链接测试
- [ ] API 兼容性测试

### 9.2 集成测试

- [ ] horpkg 基本功能测试
- [ ] hdc-lite 连接测试
- [ ] userlandexec 启动测试

### 9.3 性能测试

- [ ] 编译时间对比
- [ ] 二进制大小对比
- [ ] 运行时性能对比

### 9.4 兼容性测试

- [ ] 不同 OHOS 版本测试
- [ ] 不同架构测试 (aarch64, x86_64)

---

## 十、后续优化方向

### 10.1 进一步优化

1. **预编译包缓存** - 构建一次后缓存，避免重复编译
2. **增量编译** - 仅编译变更的包
3. **并行编译优化** - 优化依赖关系，最大化并行度
4. **分布式编译** - 使用 distcc 或类似工具

### 10.2 构建工具链改进

1. 使用 Bazel 或 Ninja 替代 Make
2. 实现构建缓存 (ccache)
3. 使用容器化构建环境

---

## 十一、附录

### 11.1 系统库检查脚本

```bash
#!/bin/bash
# check-system-libs.sh

SDK_HOME=${OHOS_SDK_HOME:-"/path/to/sdk"}

echo "检查系统库可用性:"
echo "=================="

check_lib() {
    local lib_name=$1
    local lib_pattern=$2

    echo -n "$lib_name: "
    local found=$(find $SDK_HOME -name "$lib_pattern" 2>/dev/null | head -1)

    if [ -n "$found" ]; then
        echo "✓ ($found)"
        return 0
    else
        echo "✗ 未找到"
        return 1
    fi
}

check_lib "zlib" "libz.so*"
check_lib "openssl" "libssl.so*"
check_lib "curl" "libcurl.so*"
check_lib "xz" "liblzma.so*"
check_lib "bzip2" "libbz2.so*"
check_lib "libzip" "libzip.so*"
```

### 11.2 迁移检查清单

- [ ] Phase 1 完成 - 系统库可用性验证
- [ ] Phase 2 完成 - 高优先级迁移
- [ ] Phase 3 完成 - 中优先级迁移
- [ ] 所有测试通过
- [ ] 文档更新完成
- [ ] 代码审查通过
- [ ] 部署验证通过

---

## 十二、联系方式

如有问题，请联系项目维护者。

---

**文档结束**
