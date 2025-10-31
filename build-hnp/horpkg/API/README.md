# HarmonyOS 开发者 API 文档合集

本文档集基于真实抓包数据分析，包含华为HarmonyOS开发者平台的完整API使用说明和签名流程。

## 📚 文档列表

### 1. [HarmonyOS_API_Documentation.md](./HarmonyOS_API_Documentation.md)
**完整的API使用文档**

包含14个API接口的详细说明：
- ✅ 认证相关 API（3个）
- ✅ 用户相关 API（2个）
- ✅ 证书管理 API（3个）
- ✅ 设备管理 API（1个）
- ✅ Provision配置文件管理 API（1个）
- ✅ 应用管理 API（1个）
- ✅ 协议相关 API（2个）
- ✅ IDE授权 API（1个）

**每个API都包含**：
- 完整的请求URL和方法
- 详细的参数说明
- 真实的请求/响应示例
- 字段含义详解
- 实际使用的curl命令示例

### 2. [HarmonyOS_Signing_Flow.md](./HarmonyOS_Signing_Flow.md)
**实际签名流程文档**

深入分析两种签名流程：
- 🔄 **DevEco完整流程**（18个API调用）
  - 适用于首次配置、证书过期、自动化构建
  - 包含完整的证书生命周期管理
  - 自动处理Token失效和重试
  
- ⚡ **小白助手快速流程**（7个API调用）
  - 适用于已有证书的快速签名
  - 跳过证书管理，直接创建配置
  - 适合日常开发调试

**包含内容**：
- 详细的时序图
- 完整的Python实现代码
- Shell脚本示例
- 流程对比分析
- 常见问题解答

## 🎯 快速开始

### 场景1: 首次配置签名环境

1. 查看 `HarmonyOS_API_Documentation.md` 了解API详情
2. 按照 `HarmonyOS_Signing_Flow.md` 中的**DevEco完整流程**操作
3. 使用提供的Python代码或Shell脚本自动化配置

### 场景2: 快速签名已有应用

1. 确保已有调试证书
2. 按照 `HarmonyOS_Signing_Flow.md` 中的**小白助手快速流程**操作
3. 直接创建Provision配置文件

### 场景3: API集成开发

1. 参考 `HarmonyOS_API_Documentation.md` 中的API说明
2. 根据需求调用相应的API接口
3. 使用文档中的curl示例进行测试

## 📊 数据来源

所有API和流程均基于以下真实抓包文件分析：

| 文件 | 说明 | API数量 |
|------|------|---------|
| 登陆流程.har | 登录和认证流程 | 6个API |
| DevEco.har | 完整的编译签名流程 | 18个API调用 |
| 小白助手.har | 快速签名安装流程 | 7个API调用 |

## 🔑 核心流程对比

```
┌─────────────────────────────────────────────────────────────┐
│                   DevEco 完整流程 (18步)                     │
├─────────────────────────────────────────────────────────────┤
│ 1-9.  认证检查（Token刷新+重试机制）                         │
│ 10.   获取现有证书列表                                       │
│ 11.   删除旧证书                                            │
│ 12.   添加新证书（提交CSR）                                  │
│ 13.   确认证书创建                                          │
│ 14.   获取证书下载URL                                        │
│ 15-16. 获取设备列表（2次）                                   │
│ 17.   创建Provision配置                                      │
│ 18.   获取Provision下载URL                                   │
└─────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────┐
│                 小白助手 快速流程 (7步)                      │
├─────────────────────────────────────────────────────────────┤
│ 1.    直接创建Provision（使用已有证书）                     │
│ 2-3.  获取设备列表（2次）                                    │
│ 4.    验证临时Token                                         │
│ 5.    检查JWT Token                                         │
│ 6.    获取用户团队列表                                       │
│ 7.    再次获取设备列表                                       │
└─────────────────────────────────────────────────────────────┘
```

## 🛠️ 技术栈

- **抓包工具**: HAR格式（HTTP Archive）
- **API协议**: REST API + JSON
- **认证方式**: Cookie + JWT Token
- **编程语言**: Python、Shell
- **适用平台**: HarmonyOS DevEco Studio

## 📝 使用示例

### Python 完整流程
```python
from harmony_sign_manager import HarmonyOSSignManager

# 初始化管理器
manager = HarmonyOSSignManager(cookie="hwid_account=YOUR_COOKIE")

# 读取CSR
with open("request.csr", "r") as f:
    csr = f.read()

# 执行完整签名配置
manager.full_sign_workflow(
    package_name="com.example.myapp",
    csr=csr
)
```

### Shell 快速签名
```bash
# 配置环境变量
export COOKIE="hwid_account=YOUR_COOKIE"
export PACKAGE_NAME="com.example.myapp"
export CERT_ID="1795252540640612864"

# 执行快速签名脚本
./quick_sign.sh
```

## ⚠️ 注意事项

1. **Cookie安全**: Cookie包含认证信息，请妥善保管
2. **URL时效**: 下载链接仅5分钟有效，需及时下载
3. **证书有效期**: 调试证书通常6个月-1年有效期
4. **设备限制**: 个人账号最多100个设备
5. **实名认证**: 需要完成华为账号实名认证

## 🔍 API端点概览

### 认证服务域名
```
https://cn.devecostudio.huawei.com
```

### 云服务API域名
```
https://connect-api.cloud.huawei.com
```

### 核心API列表
- `/authrouter/auth/api/temptoken` - 获取临时Token
- `/authrouter/auth/api/jwToken/check` - 检查JWT Token
- `/api/cps/harmony-cert-manage/v1/cert/list` - 获取证书列表
- `/api/cps/harmony-cert-manage/v1/cert/add` - 添加证书
- `/api/cps/device-manage/v1/device/list` - 获取设备列表
- `/api/cps/provision-manage/v1/ide/test/provision/add` - 创建Provision

## 📖 相关资源

- **华为开发者联盟**: https://developer.huawei.com
- **HarmonyOS文档**: https://developer.harmonyos.com
- **DevEco Studio**: https://developer.harmonyos.com/cn/develop/deveco-studio

## 🤝 贡献

本文档基于实际抓包数据分析整理，如发现任何错误或需要补充，欢迎反馈。

## 📄 许可

本文档仅供学习和研究使用。请遵守华为开发者协议和相关法律法规。

---

**文档版本**: v1.0  
**最后更新**: 2025-10-30  
**分析来源**: 真实HAR抓包文件  
**覆盖API**: 14个接口，31次实际调用
