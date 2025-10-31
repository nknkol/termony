# HarmonyOS 应用签名实际 API 调用流程

## 目录
- [1. 流程概述](#1-流程概述)
- [2. 登录认证流程](#2-登录认证流程)
- [3. 完整签名配置流程（DevEco）](#3-完整签名配置流程deveco)
- [4. 快速签名流程（小白助手）](#4-快速签名流程小白助手)
- [5. 流程对比分析](#5-流程对比分析)
- [6. 实现代码示例](#6-实现代码示例)

---

## 1. 流程概述

HarmonyOS应用签名主要包含三个阶段：

```
┌─────────────────┐      ┌──────────────────┐      ┌─────────────────┐
│  阶段1：认证     │ ───> │  阶段2：签名配置  │ ───> │  阶段3：应用签名 │
│  Login & Auth   │      │  Sign Configure  │      │  App Signing    │
└─────────────────┘      └──────────────────┘      └─────────────────┘
  - IDE授权申请              - 证书管理                - 使用证书和配置
  - 获取Token               - 设备管理                - 签名HAP包
  - 验证用户信息            - 生成Provision           - 安装到设备
```

---

## 2. 登录认证流程

### 2.1 流程图

```
用户打开IDE
    │
    ├─> 1. IDE发起授权申请
    │   GET /console/DevEcoIDE/apply
    │   （浏览器打开，用户登录）
    │
    ├─> 2. 获取用户信息
    │   GET /devspaceapi/v1/userinfo
    │
    ├─> 3. 查询协议内容
    │   GET /authrouter/agreement/v1/queryAgreementContent
    │
    ├─> 4. 检查协议签署状态（2次）
    │   GET /authrouter/agreement/v1/queryAgreementRecord?agrType=10349
    │   GET /authrouter/agreement/v1/queryAgreementRecord?agrType=681
    │
    └─> 5. 生成临时Token
        GET /authrouter/auth/api/temptoken
        
认证完成 ✓
```

### 2.2 详细调用序列

| 步骤 | API | 方法 | 作用 | 关键参数/返回 |
|------|-----|------|------|--------------|
| 1 | `/console/DevEcoIDE/apply` | GET | IDE授权申请 | port=3904, appid=1007, code=授权码 |
| 2 | `/devspaceapi/v1/userinfo` | GET | 获取用户信息 | 返回userId, nickname, csrfToken |
| 3 | `/authrouter/agreement/v1/queryAgreementContent` | GET | 查询协议内容 | language=zh-CN |
| 4 | `/authrouter/agreement/v1/queryAgreementRecord` | GET | 检查协议签署（开发者协议） | agrType=10349 |
| 5 | `/authrouter/agreement/v1/queryAgreementRecord` | GET | 检查协议签署（隐私协议） | agrType=681 |
| 6 | `/authrouter/auth/api/temptoken` | GET | 生成临时Token | 返回tempToken用于后续认证 |

### 2.3 关键数据流

```bash
# 1. IDE授权申请（浏览器完成）
GET https://cn.devecostudio.huawei.com/console/DevEcoIDE/apply?port=3904&appid=1007&code=20698961dd4f420c8b44f49010c6f0cc

# 2. 获取用户信息
GET https://cn.devecostudio.huawei.com/devspaceapi/v1/userinfo?_=1761829347134
Response: {
  "body": {
    "userId": "220086000132459978",
    "nickname": "飞舟仁",
    "csrfToken": "DCFE7529C47A942820E1A69ABC389CC4560075BD683ED4EFCFBEDE55E7E99ADF",
    "isRealName": true
  }
}

# 3. 生成临时Token
GET https://cn.devecostudio.huawei.com/authrouter/auth/api/temptoken
Response: {
  "tempToken": "55e644424387e06431ef695ca28aec27..."
}
```

---

## 3. 完整签名配置流程（DevEco）

这是**DevEco Studio自动签名配置**的完整流程，包含证书的创建和管理。

### 3.1 流程图

```
编译应用
    │
    ├─> 第一阶段：认证检查
    │   ├─> 1-3. 尝试获取团队列表（401失败×3）
    │   ├─> 4-6. 检查JWT Token（成功×3）
    │   └─> 7-9. 重新获取团队列表（成功×3）
    │
    ├─> 第二阶段：证书管理
    │   ├─> 10. 获取现有证书列表
    │   ├─> 11. 删除旧证书（可选）
    │   ├─> 12. 添加新证书（提交CSR）
    │   ├─> 13. 再次获取证书列表（确认）
    │   └─> 14. 重新申请证书下载URL
    │
    ├─> 第三阶段：设备和配置
    │   ├─> 15-16. 获取设备列表（2次）
    │   ├─> 17. 创建Provision配置文件
    │   └─> 18. 重新申请Provision下载URL
    │
    └─> 签名完成 ✓
```

### 3.2 详细调用序列

#### 阶段1：认证检查（请求1-9）

```
目的：确保当前会话有效，能够访问云服务API

1-3.  GET  /api/ups/user-permission-service/v1/user-team-list  [401×3]
      ↓ Token可能过期，需要刷新
      
4-6.  GET  /authrouter/auth/api/jwToken/check  [200×3]
      ↓ 验证JWT Token，获取新的accessToken
      
7-9.  GET  /api/ups/user-permission-service/v1/user-team-list  [200×3]
      ✓ 使用新Token成功获取团队列表
```

**关键数据**:
```json
// JWT Token检查响应
{
  "status": true,
  "userInfo": {
    "userId": "220086000132459978",
    "accessToken": "DgEAANzA9X16n0gLi2gZeJXuVJ6O7JRm...",
    "realName": true
  }
}
```

#### 阶段2：证书管理（请求10-14）

```
目的：创建或更新调试证书

10. POST   /api/cps/harmony-cert-manage/v1/cert/list
    ↓ 获取现有证书列表，检查是否需要更新
    
11. DELETE /api/cps/harmony-cert-manage/v1/cert/delete
    ↓ 删除旧的或过期的证书
    
12. POST   /api/cps/harmony-cert-manage/v1/cert/add
    ↓ 提交CSR，申请新的调试证书
    
13. POST   /api/cps/harmony-cert-manage/v1/cert/list
    ↓ 确认新证书已创建
    
14. POST   /api/amis/app-manage/v1/objects/url/reapply
    ✓ 获取证书文件下载URL
```

**关键数据**:
```bash
# 12. 添加证书请求
POST /api/cps/harmony-cert-manage/v1/cert/add
Content-Type: application/x-www-form-urlencoded

certType=1&
csr=-----BEGIN+NEW+CERTIFICATE+REQUEST-----...-----END+NEW+CERTIFICATE+REQUEST-----&
certName=auto_debug_220086000132459978.cer

# 响应
{
  "ret": {"code": 0, "msg": "OK"},
  "harmonyCert": {
    "id": "1808235417024089280",
    "certName": "auto_debug_220086000132459978.cer",
    "certObjectId": "CN/2025103013/1761830459368-6b777819-797b-4407-8751-f2e9446ce5a8.cer",
    "expireTime": 1777382459000,
    "publicKeySha256": "CC:3C:E5:84:40:FF:1F:F6:B6:CB:56:FE:02:85:40:24:..."
  }
}

# 14. 获取下载URL
POST /api/amis/app-manage/v1/objects/url/reapply
{
  "objectIds": ["CN/2025103013/1761830459368-6b777819-797b-4407-8751-f2e9446ce5a8.cer"]
}

# 响应
{
  "objects": [{
    "objectId": "CN/2025103013/...",
    "url": "https://nsp-appgallery-agcfs-drcn.obs.cn-north-2.myhuaweicloud.cn/...",
    "validTime": 300
  }]
}
```

#### 阶段3：设备和配置（请求15-18）

```
目的：创建Provision配置文件，绑定证书和设备

15-16. GET  /api/cps/device-manage/v1/device/list  [200×2]
       ↓ 获取所有已注册的调试设备
       
17.    POST /api/cps/provision-manage/v1/ide/test/provision/add
       ↓ 创建Provision配置文件
       
18.    POST /api/amis/app-manage/v1/objects/url/reapply
       ✓ 获取Provision文件下载URL
```

**关键数据**:
```bash
# 15. 获取设备列表
GET /api/cps/device-manage/v1/device/list?start=1&pageSize=100&encodeFlag=0

# 响应
{
  "ret": {"code": 0},
  "totalCount": 2,
  "list": [
    {
      "id": "1795250237237907968",
      "deviceName": "xiaobai-device-D7C074A10F",
      "udid": "D7C074A10F19DE713784339184E77ADD5178AAE6C56ACB663D5A52A117CED653",
      "deviceType": 4
    }
  ]
}

# 17. 创建Provision
POST /api/cps/provision-manage/v1/ide/test/provision/add
{
  "provisionName": "xiaobai-debug_com_baitude_myapplication",
  "aclPermissionList": [],
  "deviceList": ["1795250237237907968", "1644441656348398016"],
  "certList": ["1808235417024089280"],
  "packageName": "com.baitude.myapplication"
}

# 响应
{
  "ret": {"code": 0},
  "provisionFileUrl": "https://nsp-appgallery-agcfs-drcn.obs.cn-north-2.myhuaweicloud.cn/.../xiaobai-debug_com_baitude_myapplication.p7b?..."
}
```

### 3.3 时序图

```
IDE客户端              认证服务              云服务API             对象存储
    │                    │                    │                    │
    ├─[1-3]─获取团队列表──────────────────────>│ (401)
    │                    │                    │
    ├─[4-6]─检查Token────>│                    │
    │<──────返回新Token───┤                    │
    │                    │                    │
    ├─[7-9]─获取团队列表──────────────────────>│ (200)
    │                    │                    │
    ├─[10]──获取证书列表─────────────────────>│
    │<──────返回证书列表───────────────────────┤
    │                    │                    │
    ├─[11]──删除旧证书──────────────────────>│
    │                    │                    │
    ├─[12]──添加新证书(CSR)─────────────────>│
    │<──────返回证书ID────────────────────────┤
    │                    │                    │
    ├─[13]──确认证书列表────────────────────>│
    │                    │                    │
    ├─[14]──申请证书URL─────────────────────>│
    │                    │                    │──下载URL──>│
    │<──────返回下载URL────────────────────────┤            │
    │                    │                    │            │
    ├─────下载证书文件─────────────────────────────────────>│
    │<────证书.cer文件─────────────────────────────────────┤
    │                    │                    │            │
    ├─[15-16]─获取设备列表──────────────────>│            │
    │                    │                    │            │
    ├─[17]──创建Provision────────────────────>│            │
    │<──────Provision URL─────────────────────┤            │
    │                    │                    │            │
    ├─[18]──申请Provision URL────────────────>│            │
    │                    │                    │──下载URL──>│
    │<──────返回下载URL────────────────────────┤            │
    │                    │                    │            │
    ├─────下载Provision──────────────────────────────────>│
    │<────.p7b文件────────────────────────────────────────┤
    │                    │                    │            │
```

---

## 4. 快速签名流程（小白助手）

这是**小白助手**的简化流程，适用于**已有证书**的情况。

### 4.1 流程图

```
应用安装
    │
    ├─> 1. 创建Provision配置文件
    │   （证书已存在，直接使用）
    │
    ├─> 2-3. 获取设备列表（2次）
    │
    ├─> 4. 验证临时Token
    │
    ├─> 5. 检查JWT Token
    │
    ├─> 6. 获取用户团队列表
    │
    └─> 7. 再次获取设备列表
    
签名完成 ✓
```

### 4.2 详细调用序列

| 步骤 | API | 方法 | 作用 | 说明 |
|------|-----|------|------|------|
| 1 | `/provision-manage/v1/ide/test/provision/add` | POST | 创建Provision | 直接使用已有证书ID |
| 2-3 | `/device-manage/v1/device/list` | GET | 获取设备列表 | 获取2次确保数据最新 |
| 4 | `/authrouter/auth/api/temptoken/check` | GET | 验证临时Token | 确认Token有效性 |
| 5 | `/authrouter/auth/api/jwToken/check` | GET | 检查JWT Token | 获取用户信息 |
| 6 | `/user-permission-service/v1/user-team-list` | GET | 获取团队列表 | 可选的权限检查 |
| 7 | `/device-manage/v1/device/list` | GET | 再次获取设备 | 最终确认设备列表 |

### 4.3 关键数据

```bash
# 1. 直接创建Provision（证书已存在）
POST /api/cps/provision-manage/v1/ide/test/provision/add
{
  "provisionName": "xiaobai-debug_com_baitude_myapplication",
  "aclPermissionList": [],
  "deviceList": ["1795250237237907968", "1644441656348398016"],
  "certList": ["1795252540640612864"],  # 使用已有证书ID
  "packageName": "com.baitude.myapplication"
}

# 响应
{
  "ret": {"code": 0},
  "provisionFileUrl": "https://.../xiaobai-debug_com_baitude_myapplication.p7b?..."
}

# 4. 验证临时Token
GET /authrouter/auth/api/temptoken/check?site=CN&tempToken=55e644424387e06431...&appid=1007&version=0.0.0

# 响应
{
  "status": true,
  "message": "success"
}
```

### 4.4 时序图

```
小白助手客户端        认证服务              云服务API             对象存储
    │                    │                    │                    │
    ├─[1]──创建Provision────────────────────>│
    │                    │                    │──生成.p7b──>│
    │<──────Provision URL─────────────────────┤            │
    │                    │                    │            │
    ├─[2-3]─获取设备列表────────────────────>│            │
    │                    │                    │            │
    ├─[4]──验证TempToken─>│                    │            │
    │<──────验证通过───────┤                    │            │
    │                    │                    │            │
    ├─[5]──检查JWT Token─>│                    │            │
    │<──────用户信息───────┤                    │            │
    │                    │                    │            │
    ├─[6]──获取团队列表──────────────────────>│            │
    │                    │                    │            │
    ├─[7]──获取设备列表──────────────────────>│            │
    │                    │                    │            │
    ├─────下载Provision──────────────────────────────────>│
    │<────.p7b文件────────────────────────────────────────┤
    │                    │                    │            │
```

---

## 5. 流程对比分析

### 5.1 两种流程对比

| 对比项 | DevEco完整流程 | 小白助手快速流程 |
|--------|---------------|-----------------|
| **API调用次数** | 18次 | 7次 |
| **是否需要证书管理** | ✓ 需要（创建/更新证书） | ✗ 不需要（使用已有证书） |
| **认证检查** | 完整的重试机制（3次失败+3次成功） | 简单验证（1次） |
| **设备列表获取** | 2次 | 3次 |
| **适用场景** | 首次配置、证书过期、自动化构建 | 快速签名、已配置环境 |
| **复杂度** | 高 | 低 |

### 5.2 核心差异点

#### DevEco流程特点：
1. **自动处理Token失效**：401错误后自动刷新Token
2. **完整证书生命周期**：查询→删除→创建→下载
3. **多次确认机制**：每个关键步骤都有确认步骤
4. **适合持续集成**：可以完全自动化

#### 小白助手流程特点：
1. **前置条件要求**：证书必须已存在
2. **快速执行**：跳过证书管理，直接创建配置
3. **轻量级验证**：只做必要的Token验证
4. **适合开发调试**：开发者已配置好环境后快速使用

### 5.3 选择建议

```
┌─────────────────────────────────────────┐
│           是否首次使用？                 │
└─────────────┬───────────────────────────┘
              │
       ┌──────┴──────┐
       │             │
      是             否
       │             │
       ↓             ↓
  DevEco流程    是否需要更新证书？
                     │
              ┌──────┴──────┐
              │             │
             是             否
              │             │
              ↓             ↓
         DevEco流程    小白助手流程
```

---

## 6. 实现代码示例

### 6.1 完整签名配置实现（Python）

```python
import requests
import time
from typing import Dict, List, Optional

class HarmonyOSSignManager:
    """HarmonyOS签名管理器"""
    
    def __init__(self, cookie: str):
        self.base_url = "https://connect-api.cloud.huawei.com"
        self.auth_url = "https://cn.devecostudio.huawei.com"
        self.headers = {
            "Cookie": cookie,
            "Accept": "application/json",
            "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64)"
        }
    
    def check_and_refresh_token(self) -> bool:
        """检查并刷新JWT Token"""
        print("[1] 检查JWT Token...")
        
        # 尝试获取团队列表
        response = requests.get(
            f"{self.base_url}/api/ups/user-permission-service/v1/user-team-list",
            headers=self.headers
        )
        
        if response.status_code == 401:
            print("   Token已过期，刷新中...")
            # 检查JWT Token以刷新
            for i in range(3):
                refresh_response = requests.get(
                    f"{self.auth_url}/authrouter/auth/api/jwToken/check",
                    headers=self.headers
                )
                if refresh_response.status_code == 200:
                    data = refresh_response.json()
                    if data.get("status"):
                        print(f"   Token刷新成功 (尝试 {i+1}/3)")
                        # 更新accessToken到headers
                        new_token = data.get("userInfo", {}).get("accessToken")
                        if new_token:
                            self.headers["Authorization"] = f"Bearer {new_token}"
                        return True
            print("   Token刷新失败")
            return False
        
        print("   Token有效 ✓")
        return True
    
    def get_cert_list(self) -> List[Dict]:
        """获取证书列表"""
        print("[2] 获取证书列表...")
        response = requests.post(
            f"{self.base_url}/api/cps/harmony-cert-manage/v1/cert/list",
            headers={**self.headers, "Content-Type": "application/json"},
            json={}
        )
        
        if response.status_code == 200:
            data = response.json()
            cert_list = data.get("certList", [])
            print(f"   找到 {len(cert_list)} 个证书")
            return cert_list
        
        print("   获取失败 ✗")
        return []
    
    def delete_cert(self, cert_id: str) -> bool:
        """删除证书"""
        print(f"[3] 删除证书 {cert_id}...")
        response = requests.delete(
            f"{self.base_url}/api/cps/harmony-cert-manage/v1/cert/delete",
            headers={**self.headers, "Content-Type": "application/json"},
            json={"id": cert_id}
        )
        
        if response.status_code == 200:
            print("   删除成功 ✓")
            return True
        
        print("   删除失败 ✗")
        return False
    
    def add_cert(self, csr: str, cert_name: str) -> Optional[Dict]:
        """添加证书"""
        print(f"[4] 添加新证书 {cert_name}...")
        
        import urllib.parse
        data = {
            "certType": "1",
            "csr": csr,
            "certName": cert_name
        }
        
        response = requests.post(
            f"{self.base_url}/api/cps/harmony-cert-manage/v1/cert/add",
            headers={**self.headers, "Content-Type": "application/x-www-form-urlencoded"},
            data=urllib.parse.urlencode(data)
        )
        
        if response.status_code == 200:
            result = response.json()
            if result.get("ret", {}).get("code") == 0:
                cert = result.get("harmonyCert")
                print(f"   证书创建成功 ✓")
                print(f"   证书ID: {cert.get('id')}")
                print(f"   过期时间: {time.strftime('%Y-%m-%d', time.localtime(cert.get('expireTime')/1000))}")
                return cert
        
        print("   证书创建失败 ✗")
        return None
    
    def download_cert(self, cert_object_id: str) -> Optional[str]:
        """获取证书下载URL"""
        print(f"[5] 获取证书下载URL...")
        
        response = requests.post(
            f"{self.base_url}/api/amis/app-manage/v1/objects/url/reapply",
            headers={**self.headers, "Content-Type": "application/json"},
            json={"objectIds": [cert_object_id]}
        )
        
        if response.status_code == 200:
            result = response.json()
            objects = result.get("objects", [])
            if objects:
                url = objects[0].get("url")
                valid_time = objects[0].get("validTime")
                print(f"   URL获取成功 ✓ (有效期: {valid_time}秒)")
                return url
        
        print("   URL获取失败 ✗")
        return None
    
    def get_device_list(self) -> List[Dict]:
        """获取设备列表"""
        print("[6] 获取设备列表...")
        
        response = requests.get(
            f"{self.base_url}/api/cps/device-manage/v1/device/list",
            headers=self.headers,
            params={"start": 1, "pageSize": 100, "encodeFlag": 0}
        )
        
        if response.status_code == 200:
            data = response.json()
            device_list = data.get("list", [])
            print(f"   找到 {len(device_list)} 个设备")
            for device in device_list:
                print(f"   - {device.get('deviceName')} (UDID: {device.get('udid')[:16]}...)")
            return device_list
        
        print("   获取失败 ✗")
        return []
    
    def create_provision(self, provision_name: str, package_name: str, 
                        cert_ids: List[str], device_ids: List[str]) -> Optional[str]:
        """创建Provision配置文件"""
        print(f"[7] 创建Provision配置...")
        
        payload = {
            "provisionName": provision_name,
            "aclPermissionList": [],
            "deviceList": device_ids,
            "certList": cert_ids,
            "packageName": package_name
        }
        
        response = requests.post(
            f"{self.base_url}/api/cps/provision-manage/v1/ide/test/provision/add",
            headers={**self.headers, "Content-Type": "application/json"},
            json=payload
        )
        
        if response.status_code == 200:
            result = response.json()
            if result.get("ret", {}).get("code") == 0:
                url = result.get("provisionFileUrl")
                print(f"   Provision创建成功 ✓")
                print(f"   文件名: {provision_name}.p7b")
                return url
        
        print("   Provision创建失败 ✗")
        return None
    
    def full_sign_workflow(self, package_name: str, csr: str) -> bool:
        """完整签名工作流程"""
        print("\n" + "="*60)
        print("开始完整签名配置流程")
        print("="*60 + "\n")
        
        # 1. 检查并刷新Token
        if not self.check_and_refresh_token():
            return False
        
        # 2. 获取现有证书
        cert_list = self.get_cert_list()
        
        # 3. 删除旧证书（可选）
        for cert in cert_list:
            if "auto_debug" in cert.get("certName", ""):
                self.delete_cert(cert.get("id"))
        
        # 4. 添加新证书
        cert_name = f"auto_debug_{int(time.time())}.cer"
        new_cert = self.add_cert(csr, cert_name)
        if not new_cert:
            return False
        
        # 5. 下载证书
        cert_url = self.download_cert(new_cert.get("certObjectId"))
        if cert_url:
            # 这里可以下载证书文件
            print(f"   证书下载链接: {cert_url[:80]}...")
        
        # 6. 获取设备列表
        devices = self.get_device_list()
        if not devices:
            print("   错误：没有找到已注册的设备")
            return False
        
        # 7. 创建Provision
        provision_name = f"{package_name.replace('.', '_')}_provision"
        device_ids = [d.get("id") for d in devices]
        cert_ids = [new_cert.get("id")]
        
        provision_url = self.create_provision(
            provision_name, package_name, cert_ids, device_ids
        )
        
        if provision_url:
            print(f"   Provision下载链接: {provision_url[:80]}...")
            print("\n" + "="*60)
            print("签名配置完成 ✓")
            print("="*60)
            return True
        
        return False


# 使用示例
if __name__ == "__main__":
    # 从浏览器Cookie中获取认证信息
    cookie = "hwid_account=YOUR_COOKIE_HERE"
    
    # 创建签名管理器
    manager = HarmonyOSSignManager(cookie)
    
    # 读取CSR文件
    with open("request.csr", "r") as f:
        csr = f.read()
    
    # 执行完整签名流程
    success = manager.full_sign_workflow(
        package_name="com.example.myapp",
        csr=csr
    )
    
    if success:
        print("\n✓ 签名配置成功！现在可以编译和签名应用了。")
    else:
        print("\n✗ 签名配置失败，请检查错误信息。")
```

### 6.2 快速签名流程实现

```python
class QuickSignManager:
    """快速签名管理器（适用于已有证书的情况）"""
    
    def __init__(self, cookie: str):
        self.base_url = "https://connect-api.cloud.huawei.com"
        self.auth_url = "https://cn.devecostudio.huawei.com"
        self.headers = {
            "Cookie": cookie,
            "Accept": "application/json"
        }
    
    def quick_sign_workflow(self, package_name: str, cert_id: str) -> bool:
        """快速签名工作流程（使用已有证书）"""
        print("\n" + "="*60)
        print("开始快速签名流程")
        print("="*60 + "\n")
        
        # 1. 获取设备列表
        print("[1] 获取设备列表...")
        response = requests.get(
            f"{self.base_url}/api/cps/device-manage/v1/device/list",
            headers=self.headers,
            params={"start": 1, "pageSize": 100, "encodeFlag": 0}
        )
        
        if response.status_code != 200:
            print("   获取设备列表失败 ✗")
            return False
        
        devices = response.json().get("list", [])
        device_ids = [d.get("id") for d in devices]
        print(f"   找到 {len(devices)} 个设备 ✓")
        
        # 2. 创建Provision
        print(f"[2] 创建Provision配置...")
        provision_name = f"{package_name.replace('.', '_')}_quick"
        
        response = requests.post(
            f"{self.base_url}/api/cps/provision-manage/v1/ide/test/provision/add",
            headers={**self.headers, "Content-Type": "application/json"},
            json={
                "provisionName": provision_name,
                "aclPermissionList": [],
                "deviceList": device_ids,
                "certList": [cert_id],
                "packageName": package_name
            }
        )
        
        if response.status_code == 200:
            result = response.json()
            if result.get("ret", {}).get("code") == 0:
                provision_url = result.get("provisionFileUrl")
                print(f"   Provision创建成功 ✓")
                print(f"   下载链接: {provision_url[:80]}...")
                print("\n" + "="*60)
                print("快速签名完成 ✓")
                print("="*60)
                return True
        
        print("   Provision创建失败 ✗")
        return False


# 使用示例
if __name__ == "__main__":
    cookie = "hwid_account=YOUR_COOKIE_HERE"
    manager = QuickSignManager(cookie)
    
    # 使用已有的证书ID
    success = manager.quick_sign_workflow(
        package_name="com.example.myapp",
        cert_id="1795252540640612864"  # 从证书列表获取
    )
```

### 6.3 Shell脚本实现

```bash
#!/bin/bash
# HarmonyOS自动签名脚本

COOKIE="hwid_account=YOUR_COOKIE_HERE"
BASE_URL="https://connect-api.cloud.huawei.com"
AUTH_URL="https://cn.devecostudio.huawei.com"
PACKAGE_NAME="com.example.myapp"

echo "========================================"
echo "HarmonyOS 自动签名配置"
echo "========================================"

# 1. 检查JWT Token
echo "[1] 检查JWT Token..."
jwt_response=$(curl -s -X GET "$AUTH_URL/authrouter/auth/api/jwToken/check" \
  -H "Cookie: $COOKIE" \
  -H "Accept: application/json")

if [[ $jwt_response == *'"status":true'* ]]; then
    echo "   Token有效 ✓"
else
    echo "   Token无效 ✗"
    exit 1
fi

# 2. 获取证书列表
echo "[2] 获取证书列表..."
cert_list=$(curl -s -X POST "$BASE_URL/api/cps/harmony-cert-manage/v1/cert/list" \
  -H "Cookie: $COOKIE" \
  -H "Content-Type: application/json" \
  -d '{}')

cert_id=$(echo $cert_list | jq -r '.certList[0].id')
echo "   使用证书ID: $cert_id ✓"

# 3. 获取设备列表
echo "[3] 获取设备列表..."
device_list=$(curl -s -X GET "$BASE_URL/api/cps/device-manage/v1/device/list?start=1&pageSize=100&encodeFlag=0" \
  -H "Cookie: $COOKIE")

device_ids=$(echo $device_list | jq -r '.list[].id' | jq -R -s -c 'split("\n")[:-1]')
echo "   找到设备: $device_ids ✓"

# 4. 创建Provision
echo "[4] 创建Provision配置..."
provision_name="${PACKAGE_NAME//./_}_auto"

provision_response=$(curl -s -X POST "$BASE_URL/api/cps/provision-manage/v1/ide/test/provision/add" \
  -H "Cookie: $COOKIE" \
  -H "Content-Type: application/json" \
  -d "{
    \"provisionName\": \"$provision_name\",
    \"aclPermissionList\": [],
    \"deviceList\": $device_ids,
    \"certList\": [\"$cert_id\"],
    \"packageName\": \"$PACKAGE_NAME\"
  }")

provision_url=$(echo $provision_response | jq -r '.provisionFileUrl')

if [[ $provision_url != "null" ]]; then
    echo "   Provision创建成功 ✓"
    echo "   下载链接: ${provision_url:0:80}..."
    
    # 5. 下载Provision文件
    echo "[5] 下载Provision文件..."
    curl -s -o "${provision_name}.p7b" "$provision_url"
    echo "   文件已保存: ${provision_name}.p7b ✓"
    
    echo "========================================"
    echo "签名配置完成 ✓"
    echo "========================================"
else
    echo "   Provision创建失败 ✗"
    exit 1
fi
```

---

## 7. 常见问题

### Q1: 为什么DevEco流程要调用3次相同的API？
**A**: 这是一种**重试机制**。例如：
- 前3次团队列表请求失败（401）→ 触发Token刷新
- 刷新Token后再调用3次→ 确保稳定性
- 多次调用可以应对网络波动

### Q2: Provision配置文件的下载URL为什么只有5分钟有效期？
**A**: 安全考虑。临时URL防止配置文件被未授权访问。需要在有效期内及时下载。

### Q3: 可以跳过证书管理直接使用吗？
**A**: 可以，这就是**小白助手流程**。前提是：
- 证书已经创建
- 证书未过期
- 知道证书ID

### Q4: 如何获取设备的UDID？
**A**: 设备连接电脑后，通过DevEco Studio或hdc命令获取：
```bash
hdc shell bm get --udid
```

### Q5: 一个Provision可以支持多少个设备？
**A**: 根据华为开发者账号类型不同：
- 个人账号：通常最多100个设备
- 企业账号：可能有更高限制

---

## 8. 总结

### 核心要点

1. **认证是基础**：所有API调用都依赖有效的Cookie和Token
2. **证书是核心**：调试证书是签名的必需品
3. **Provision是桥梁**：连接证书、设备和应用
4. **URL时效性**：下载链接仅5分钟有效，需及时处理

### 流程选择

- **首次配置** → DevEco完整流程
- **日常开发** → 小白助手快速流程  
- **自动化CI/CD** → DevEco完整流程（可脚本化）
- **多应用管理** → 混合使用两种流程

---

**文档版本**: v1.0  
**更新日期**: 2025-10-30  
**基于**: 真实抓包数据分析
