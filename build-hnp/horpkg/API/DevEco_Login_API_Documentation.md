# HarmonyOS DevEco 登录与证书管理 API 文档

## 目录
- [1. 概述](#1-概述)
- [2. 认证相关 API](#2-认证相关-api)
- [3. 证书管理 API](#3-证书管理-api)
- [4. 设备管理 API](#4-设备管理-api)
- [5. Provision 配置文件管理 API](#5-provision-配置文件管理-api)
- [6. 文件下载 API](#6-文件下载-api)
- [7. 辅助工具函数](#7-辅助工具函数)
- [8. 完整工作流程示例](#8-完整工作流程示例)

---

## 1. 概述

### 1.1 基础信息
- **认证服务域名**: `https://cn.devecostudio.huawei.com`
- **云服务 API 域名**: `https://connect-api.cloud.huawei.com`
- **客户端标识**: Dart/3.7 (dart:io)

### 1.2 认证请求头规范
所有经过认证的 API 请求需要包含以下请求头：

```
user-agent: Dart/3.7 (dart:io)
uid: {用户ID}
oauth2token: {访问令牌}
teamid: {团队ID，通常与用户ID相同}
accept-encoding: gzip
content-type: {根据API而定}
Host: connect-api.cloud.huawei.com
```

### 1.3 认证流程
1. 用户通过浏览器完成登录，获取 tempToken
2. 使用 tempToken 换取 JWT Token
3. 使用 JWT Token 获取 AccessToken 和用户信息
4. 使用 AccessToken 访问其他受保护的 API

---

## 2. 认证相关 API

### 2.1 使用 tempToken 换取 JWT

**接口名称**: 使用临时Token换取JWT

**请求URL**: `GET https://cn.devecostudio.huawei.com/authrouter/auth/api/temptoken/check`

**请求方法**: GET

**请求参数**:
| 参数名 | 类型 | 必填 | 说明 |
|--------|------|------|------|
| site | string | 是 | 站点标识，固定值"CN" |
| tempToken | string | 是 | 从本地回调服务器获取的临时Token |
| appid | string | 是 | 应用ID，固定值"1007" |
| version | string | 是 | 版本号，固定值"0.0.0" |

**请求头**:
```
user-agent: Dart/3.7 (dart:io)
host: cn.devecostudio.huawei.com
content-type: application/json; charset=utf-8
```

**请求示例**:
```
GET https://cn.devecostudio.huawei.com/authrouter/auth/api/temptoken/check?site=CN&tempToken=YOUR_TEMP_TOKEN&appid=1007&version=0.0.0
```

**响应数据**: 直接返回 JWT Token 字符串（纯文本）

**作用**: 将浏览器登录获得的临时Token转换为JWT Token，用于后续认证

**代码实现**:
```python
def api_1_exchange_token_for_jwt(self, temp_token):
    url = f"{self.base_url_auth}/authrouter/auth/api/temptoken/check"
    params = {
        "site": "CN",
        "tempToken": temp_token,
        "appid": "1007",
        "version": "0.0.0"
    }
    headers = {
        "user-agent": "Dart/3.7 (dart:io)",
        "host": "cn.devecostudio.huawei.com",
        "content-type": "application/json; charset=utf-8"
    }
    response = self.session.get(url, params=params, headers=headers)
    if response.status_code == 200:
        return response.text  # JWT Token
    return None
```

---

### 2.2 使用 JWT 获取 AccessToken

**接口名称**: 使用JWT换取AccessToken

**请求URL**: `GET https://cn.devecostudio.huawei.com/authrouter/auth/api/jwToken/check`

**请求方法**: GET

**请求参数**: 无（通过请求头传递）

**请求头**:
```
user-agent: Dart/3.7 (dart:io)
host: cn.devecostudio.huawei.com
content-type: application/json; charset=utf-8
jwttoken: {JWT Token}
refresh: false
```

**响应数据结构**:
```json
{
  "status": true,
  "userInfo": {
    "name": "用户名称",
    "userId": "220086000132459978",
    "nickName": "用户昵称",
    "nationalCode": "CN",
    "accessToken": "DgEAANzA9X16n0gLi2gZeJXuVJ6O...",
    "headPicUrl": null,
    "realName": true
  }
}
```

**响应字段说明**:
- `status`: 认证状态，true表示成功
- `userInfo`: 用户信息对象
  - `name`: 用户显示名称
  - `userId`: 用户唯一ID
  - `nickName`: 用户昵称
  - `nationalCode`: 国家代码
  - `accessToken`: 访问令牌（用于后续API调用）
  - `headPicUrl`: 头像URL
  - `realName`: 是否实名认证

**作用**: 使用JWT Token获取用户的AccessToken和基本信息，AccessToken用于调用所有后续API

**代码实现**:
```python
def api_2_get_access_token(self, jwt_token):
    url = f"{self.base_url_auth}/authrouter/auth/api/jwToken/check"
    headers = {
        "user-agent": "Dart/3.7 (dart:io)",
        "host": "cn.devecostudio.huawei.com",
        "content-type": "application/json; charset=utf-8",
        "jwttoken": jwt_token,
        "refresh": "false"
    }
    response = self.session.get(url, headers=headers)
    if response.status_code == 200:
        data = response.json()
        if data.get("status") == True and "userInfo" in data:
            self.user_info = data["userInfo"]
            self.access_token = self.user_info.get("accessToken")
            self.user_id = self.user_info.get("userId")
            self.team_id = self.user_id
            return self.user_info
    return None
```

---

## 3. 证书管理 API

### 3.1 获取证书列表

**接口名称**: 获取证书列表

**请求URL**: `POST https://connect-api.cloud.huawei.com/api/cps/harmony-cert-manage/v1/cert/list`

**请求方法**: POST

**请求参数**: 无（请求体为空）

**请求头**:
```
user-agent: Dart/3.7 (dart:io)
uid: {用户ID}
oauth2token: {访问令牌}
teamid: {团队ID}
accept-encoding: gzip
content-type: application/x-www-form-urlencoded; charset=UTF-8
Host: connect-api.cloud.huawei.com
```

**请求体**: 空（`data=None`）

**响应数据结构**:
```json
{
  "ret": {
    "code": 0,
    "msg": "OK"
  },
  "certList": [
    {
      "id": "1795252540640612864",
      "createTime": 1760282779932,
      "certObjectId": "CN/2025101215/1760282779900-9c3a8aaf-fc46-4fa7-aafd-79cd0f90b428.cer",
      "certName": "horpkg",
      "certType": 1,
      "p12FingerPrintSha256": "",
      "expireTime": 1775834779000,
      "publicKeySha256": "08:B8:43:A1:49:DF:F9:2A:03:12:8F:95:58:DF:63:35:A5:75:35:63:5C:1D:EE:41:CB:05:A8:71:8B:55:85:4C",
      "sha256": "c2e2a428c55fb7fe9c127519e6156ab970b7a1efa854a6bae8732c5939f4c282",
      "status": 1
    }
  ]
}
```

**响应字段说明**:
- `ret`: 返回状态对象
  - `code`: 状态码，0表示成功
  - `msg`: 状态消息
- `certList`: 证书列表数组
  - `id`: 证书唯一ID
  - `createTime`: 创建时间戳（毫秒）
  - `certObjectId`: 证书对象存储ID（用于下载）
  - `certName`: 证书名称
  - `certType`: 证书类型（1=调试证书）
  - `expireTime`: 过期时间戳（毫秒）
  - `publicKeySha256`: 公钥SHA256指纹
  - `sha256`: 证书SHA256哈希值
  - `status`: 证书状态（1=有效）

**作用**: 获取当前用户所有的HarmonyOS签名证书列表

**代码实现**:
```python
def api_3_get_cert_list(self):
    url = f"{self.base_url_api}/api/cps/harmony-cert-manage/v1/cert/list"
    headers = self._get_auth_headers(
        content_type="application/x-www-form-urlencoded; charset=UTF-8"
    )
    response = self.session.post(url, headers=headers, data=None)
    if response.status_code == 200:
        data = response.json()
        if str(data.get("ret", {}).get("code")) == "0":
            return data
    return None
```

---

### 3.2 删除证书

**接口名称**: 删除证书

**请求URL**: `DELETE https://connect-api.cloud.huawei.com/api/cps/harmony-cert-manage/v1/cert/delete`

**请求方法**: DELETE

**请求参数**: 通过JSON请求体传递

**请求头**:
```
user-agent: Dart/3.7 (dart:io)
uid: {用户ID}
oauth2token: {访问令牌}
teamid: {团队ID}
accept-encoding: gzip
content-type: application/json
Host: connect-api.cloud.huawei.com
```

**请求体**:
```json
{
  "certIds": ["1795252540640612864"]
}
```

**请求体参数说明**:
- `certIds`: 要删除的证书ID数组

**响应数据结构**:
```json
{
  "ret": {
    "code": 0,
    "msg": "OK"
  }
}
```

**响应字段说明**:
- `ret`: 返回状态对象
  - `code`: 状态码，0表示成功
  - `msg`: 状态消息

**作用**: 删除指定ID的证书，通常用于删除过期证书或重新生成证书

**代码实现**:
```python
def api_4_delete_cert(self, cert_id: str):
    url = f"{self.base_url_api}/api/cps/harmony-cert-manage/v1/cert/delete"
    headers = self._get_auth_headers(content_type="application/json")
    data = {"certIds": [cert_id]}
    response = self.session.delete(url, headers=headers, json=data)
    if response.status_code == 200:
        data = response.json()
        if str(data.get("ret", {}).get("code")) == "0":
            return True
    return False
```

---

### 3.3 添加证书

**接口名称**: 添加证书

**请求URL**: `POST https://connect-api.cloud.huawei.com/api/cps/harmony-cert-manage/v1/cert/add`

**请求方法**: POST

**请求参数**: 通过Form-Urlencoded格式传递

**请求头**:
```
user-agent: Dart/3.7 (dart:io)
uid: {用户ID}
oauth2token: {访问令牌}
teamid: {团队ID}
accept-encoding: gzip
content-type: application/x-www-form-urlencoded; charset=UTF-8
Host: connect-api.cloud.huawei.com
```

**请求体参数**:
| 参数名 | 类型 | 必填 | 说明 |
|--------|------|------|------|
| certName | string | 是 | 证书名称 |
| csr | string | 是 | 证书签名请求（CSR）内容，PEM格式 |
| certType | int | 是 | 证书类型（1=调试证书） |

**请求体示例**:
```
certName=horpkg&csr=-----BEGIN CERTIFICATE REQUEST-----
MIIBXjCCAQQCAQAwgZoxCzAJBgNVBAYTAkNOMRAwDgYDVQQIEwdCZWlqaW5nMRAw
...
-----END CERTIFICATE REQUEST-----&certType=1
```

**响应数据结构**:
```json
{
  "ret": {
    "code": 0,
    "msg": "OK"
  },
  "harmonyCert": {
    "id": "1795252540640612864",
    "createTime": 1760282779932,
    "certObjectId": "CN/2025101215/1760282779900-9c3a8aaf-fc46-4fa7-aafd-79cd0f90b428.cer",
    "certName": "horpkg",
    "certType": 1,
    "expireTime": 1775834779000,
    "publicKeySha256": "08:B8:43:A1:49:DF:F9:2A:03:12:8F:95:58:DF:63:35:A5:75:35:63:5C:1D:EE:41:CB:05:A8:71:8B:55:85:4C",
    "sha256": "c2e2a428c55fb7fe9c127519e6156ab970b7a1efa854a6bae8732c5939f4c282",
    "status": 1
  }
}
```

**响应字段说明**:
- `ret`: 返回状态对象
- `harmonyCert`: 新创建的证书对象（字段含义同获取证书列表）

**作用**: 使用CSR创建新的签名证书，服务器会返回签名后的证书

**代码实现**:
```python
def api_5_add_cert(self, cert_name: str, csr_pem_string: str, cert_type: int):
    url = f"{self.base_url_api}/api/cps/harmony-cert-manage/v1/cert/add"
    headers = self._get_auth_headers(
        content_type="application/x-www-form-urlencoded; charset=UTF-8"
    )
    data = {
        "certName": cert_name,
        "csr": csr_pem_string,
        "certType": cert_type
    }
    response = self.session.post(url, headers=headers, data=data)
    if response.status_code == 200:
        data = response.json()
        if str(data.get("ret", {}).get("code")) == "0":
            return data.get("harmonyCert")
    return None
```

---

## 4. 设备管理 API

### 4.1 获取设备列表

**接口名称**: 获取设备列表

**请求URL**: `GET https://connect-api.cloud.huawei.com/api/cps/device-manage/v1/device/list`

**请求方法**: GET

**请求参数**:
| 参数名 | 类型 | 必填 | 说明 |
|--------|------|------|------|
| start | int | 是 | 起始位置，从1开始 |
| pageSize | int | 是 | 每页数量，建议100 |
| encodeFlag | int | 是 | 编码标志，固定值0 |

**请求头**:
```
user-agent: Dart/3.7 (dart:io)
uid: {用户ID}
oauth2token: {访问令牌}
teamid: {团队ID}
accept-encoding: gzip
Host: connect-api.cloud.huawei.com
```
**注意**: 不需要 content-type 请求头

**请求示例**:
```
GET https://connect-api.cloud.huawei.com/api/cps/device-manage/v1/device/list?start=1&pageSize=100&encodeFlag=0
```

**响应数据结构**:
```json
{
  "ret": {
    "code": 0,
    "msg": "OK"
  },
  "list": [
    {
      "id": "1234567890",
      "deviceName": "horpkg-device",
      "udid": "D7C074A10F19DE713784339184E77ADD5178AAE6C56ACB663D5A52A117CED653",
      "deviceType": 4,
      "createTime": 1760282779932,
      "status": 1
    }
  ],
  "total": 1
}
```

**响应字段说明**:
- `ret`: 返回状态对象
- `list`: 设备列表数组
  - `id`: 设备唯一ID
  - `deviceName`: 设备名称
  - `udid`: 设备唯一标识符（UDID）
  - `deviceType`: 设备类型（4=手机）
  - `createTime`: 创建时间戳（毫秒）
  - `status`: 设备状态（1=有效）
- `total`: 设备总数

**作用**: 获取当前用户已注册的所有调试设备列表

**代码实现**:
```python
def api_6_get_device_list(self):
    url = f"{self.base_url_api}/api/cps/device-manage/v1/device/list"
    headers = self._get_auth_headers()
    del headers['content-type']  # 删除content-type
    params = {"start": 1, "pageSize": 100, "encodeFlag": 0}
    response = self.session.get(url, headers=headers, params=params)
    if response.status_code == 200:
        data = response.json()
        if str(data.get("ret", {}).get("code")) == "0":
            return data.get('list', [])
    return None
```

---

### 4.2 添加设备

**接口名称**: 添加设备

**请求URL**: `POST https://connect-api.cloud.huawei.com/api/cps/device-manage/v1/device/add`

**请求方法**: POST

**请求参数**: 通过JSON请求体传递

**请求头**:
```
user-agent: Dart/3.7 (dart:io)
uid: {用户ID}
oauth2token: {访问令牌}
teamid: {团队ID}
accept-encoding: gzip
content-type: application/json; charset=utf-8
Host: connect-api.cloud.huawei.com
```

**请求体**:
```json
{
  "deviceName": "horpkg-device",
  "udid": "D7C074A10F19DE713784339184E77ADD5178AAE6C56ACB663D5A52A117CED653",
  "deviceType": 4
}
```

**请求体参数说明**:
- `deviceName`: 设备名称（自定义）
- `udid`: 设备唯一标识符（UDID），从设备获取
- `deviceType`: 设备类型（4=手机）

**响应数据结构**:
```json
{
  "ret": {
    "code": 0,
    "msg": "OK"
  }
}
```

**响应字段说明**:
- `ret`: 返回状态对象
  - `code`: 状态码，0表示成功
  - `msg`: 状态消息

**作用**: 注册新的调试设备，注册后可用于创建Provision配置文件

**代码实现**:
```python
def api_8_add_device(self, udid: str, device_name: str, device_type: int = 4):
    url = f"{self.base_url_api}/api/cps/device-manage/v1/device/add"
    headers = self._get_auth_headers(
        content_type="application/json; charset=utf-8"
    )
    data = {
        "deviceName": device_name,
        "udid": udid,
        "deviceType": device_type
    }
    response = self.session.post(url, headers=headers, json=data)
    if response.status_code == 200:
        data = response.json()
        if str(data.get("ret", {}).get("code")) == "0":
            return True
    return False
```

---

## 5. Provision 配置文件管理 API

### 5.1 创建 Provision Profile

**接口名称**: 创建Provision Profile

**请求URL**: `POST https://connect-api.cloud.huawei.com/api/cps/provision-manage/v1/ide/test/provision/add`

**请求方法**: POST

**请求参数**: 通过JSON请求体传递

**请求头**:
```
user-agent: Dart/3.7 (dart:io)
uid: {用户ID}
oauth2token: {访问令牌}
teamid: {团队ID}
accept-encoding: gzip
content-type: application/json
Host: connect-api.cloud.huawei.com
```

**请求体**:
```json
{
  "provisionName": "horpkg-profile",
  "aclPermissionList": [],
  "deviceList": ["1234567890"],
  "certList": ["1795252540640612864"],
  "packageName": "com.example.horpkgapp"
}
```

**请求体参数说明**:
- `provisionName`: Provision配置文件名称
- `aclPermissionList`: 权限列表（空数组表示无特殊权限）
- `deviceList`: 设备ID列表（从API 6获取）
- `certList`: 证书ID列表（从API 3获取）
- `packageName`: 应用包名

**响应数据结构**:
```json
{
  "ret": {
    "code": 0,
    "msg": "OK"
  },
  "provisionFileUrl": "CN/2025101215/provision-horpkg-1760282779900.p7b"
}
```

**响应字段说明**:
- `ret`: 返回状态对象
- `provisionFileUrl`: Provision文件的对象存储路径（用于后续下载）

**作用**: 创建Provision配置文件（.p7b），用于应用签名和设备授权

**代码实现**:
```python
def api_7_add_provision(self, profile_name: str, cert_id: str, 
                       device_ids: list, package_name: str):
    url = f"{self.base_url_api}/api/cps/provision-manage/v1/ide/test/provision/add"
    headers = self._get_auth_headers(content_type="application/json")
    data = {
        "provisionName": profile_name,
        "aclPermissionList": [],
        "deviceList": device_ids,
        "certList": [cert_id],
        "packageName": package_name
    }
    response = self.session.post(url, headers=headers, json=data)
    if response.status_code == 200:
        data = response.json()
        if str(data.get("ret", {}).get("code")) == "0":
            return data.get("provisionFileUrl")
    return None
```

---

## 6. 文件下载 API

### 6.1 获取文件下载URL

**接口名称**: 获取临时下载URL

**请求URL**: `POST https://connect-api.cloud.huawei.com/api/amis/app-manage/v1/objects/url/reapply`

**请求方法**: POST

**请求参数**: 通过Form-Urlencoded格式传递

**请求头**:
```
user-agent: Dart/3.7 (dart:io)
uid: {用户ID}
oauth2token: {访问令牌}
teamid: {团队ID}
accept-encoding: gzip
content-type: application/x-www-form-urlencoded; charset=UTF-8
Host: connect-api.cloud.huawei.com
```

**请求体参数**:
| 参数名 | 类型 | 必填 | 说明 |
|--------|------|------|------|
| sourceUrls | string | 是 | 源对象路径（从API返回的certObjectId或provisionFileUrl） |

**请求体示例**:
```
sourceUrls=CN/2025101215/1760282779900-9c3a8aaf-fc46-4fa7-aafd-79cd0f90b428.cer
```

**响应数据结构**:
```json
{
  "ret": {
    "code": 0,
    "msg": "OK"
  },
  "urlsInfo": [
    {
      "newUrl": "https://nsp-appgallery-agcfs-drcn.obs.cn-north-2.myhuaweicloud.cn/CN/2025101215/1760282779900-9c3a8aaf-fc46-4fa7-aafd-79cd0f90b428.cer?X-Amz-Algorithm=AWS4-HMAC-SHA256&...",
      "validTime": 300
    }
  ]
}
```

**响应字段说明**:
- `ret`: 返回状态对象
- `urlsInfo`: URL信息列表
  - `newUrl`: 临时下载URL（华为云OBS地址）
  - `validTime`: URL有效时间（秒），通常为300秒（5分钟）

**作用**: 为证书文件或Provision文件生成临时下载URL

**代码实现**:
```python
def api_9_get_download_url(self, source_url: str):
    url = f"{self.base_url_api}/api/amis/app-manage/v1/objects/url/reapply"
    headers = self._get_auth_headers(
        content_type="application/x-www-form-urlencoded; charset=UTF-8"
    )
    data = {"sourceUrls": source_url}
    response = self.session.post(url, headers=headers, data=data)
    if response.status_code == 200:
        data = response.json()
        if (str(data.get("ret", {}).get("code")) == "0" 
            and "urlsInfo" in data and data["urlsInfo"]):
            return data["urlsInfo"][0].get("newUrl")
    return None
```

---

### 6.2 下载文件

**接口名称**: 下载文件

**请求URL**: 从API 9返回的临时URL

**请求方法**: GET

**请求参数**: 无（URL已包含所有参数）

**请求头**: 无特殊要求

**响应数据**: 二进制文件内容（.cer或.p7b文件）

**作用**: 从华为云OBS下载证书或Provision文件

**代码实现**:
```python
def api_10_download_file(self, download_url: str, save_path: str):
    response = self.session.get(download_url, stream=True)
    if response.status_code == 200:
        with open(save_path, "wb") as f:
            for chunk in response.iter_content(chunk_size=8192):
                f.write(chunk)
        return True
    return False
```

---

## 7. 辅助工具函数

### 7.1 生成密钥库和CSR

**函数名称**: generate_files_and_get_csr

**功能说明**: 使用 keytool 工具生成 PKCS12 密钥库（.p12）和证书签名请求（.csr）

**依赖工具**: Java Development Kit (JDK) 中的 keytool 命令

**函数参数**:
| 参数名 | 类型 | 必填 | 说明 |
|--------|------|------|------|
| common_name | string | 是 | 证书通用名称（CN） |
| p12_filename | string | 是 | 输出的P12文件名 |
| csr_filename | string | 是 | 输出的CSR文件名 |
| p12_password | string | 是 | P12文件密码 |

**执行步骤**:
1. 使用 keytool 生成椭圆曲线（EC）密钥对和自签名证书
2. 生成证书签名请求（CSR）
3. 读取并返回CSR内容

**代码实现**:
```python
def generate_files_and_get_csr(common_name: str, p12_filename: str, 
                               csr_filename: str, p12_password: str):
    # 步骤1: 生成密钥对和P12文件
    dname = f"C=CN, ST=Beijing, L=Beijing, O=YourOrg, OU=Mobile, CN={common_name}"
    cmd_genkey = [
        "keytool", "-genkeypair",
        "-alias", CERT_ALIAS,
        "-keystore", p12_filename,
        "-storetype", "PKCS12",
        "-keyalg", "EC",
        "-keysize", "256",
        "-sigalg", "SHA256withECDSA",
        "-dname", dname,
        "-validity", "3650",
        "-storepass", p12_password,
        "-keypass", p12_password
    ]
    subprocess.run(cmd_genkey, capture_output=True, text=True, check=True)
    
    # 步骤2: 生成CSR
    cmd_certreq = [
        "keytool", "-certreq",
        "-alias", CERT_ALIAS,
        "-keystore", p12_filename,
        "-storetype", "PKCS12",
        "-file", csr_filename,
        "-sigalg", "SHA256withECDSA",
        "-storepass", p12_password
    ]
    subprocess.run(cmd_certreq, capture_output=True, text=True, check=True)
    
    # 步骤3: 读取CSR内容
    with open(csr_filename, "r") as f:
        return f.read()
```

---

### 7.2 导入证书到密钥库

**函数名称**: keytool_import_cert

**功能说明**: 将从服务器下载的证书（.cer）导入到本地P12密钥库

**依赖工具**: Java Development Kit (JDK) 中的 keytool 命令

**函数参数**:
| 参数名 | 类型 | 必填 | 说明 |
|--------|------|------|------|
| p12_filename | string | 是 | P12文件名 |
| cer_filename | string | 是 | 证书文件名 |
| p12_password | string | 是 | P12文件密码 |

**执行命令**:
```bash
keytool -importcert \
  -alias horpkg \
  -keystore horpkg.p12 \
  -storetype PKCS12 \
  -file horpkg.cer \
  -trustcacerts \
  -storepass 123456 \
  -noprompt
```

**代码实现**:
```python
def keytool_import_cert(p12_filename: str, cer_filename: str, p12_password: str):
    cmd_import = [
        "keytool", "-importcert",
        "-alias", CERT_ALIAS,
        "-keystore", p12_filename,
        "-storetype", "PKCS12",
        "-file", cer_filename,
        "-trustcacerts",
        "-storepass", p12_password,
        "-noprompt"
    ]
    proc = subprocess.run(cmd_import, capture_output=True, text=True)
    return proc.returncode == 0
```

---

### 7.3 验证密钥库

**函数名称**: verify_keystore

**功能说明**: 验证P12密钥库的内容和证书链

**依赖工具**: Java Development Kit (JDK) 中的 keytool 命令

**函数参数**:
| 参数名 | 类型 | 必填 | 说明 |
|--------|------|------|------|
| p12_filename | string | 是 | P12文件名 |
| p12_password | string | 是 | P12文件密码 |

**执行命令**:
```bash
keytool -list -v \
  -keystore horpkg.p12 \
  -storetype PKCS12 \
  -alias horpkg \
  -storepass 123456
```

**代码实现**:
```python
def verify_keystore(p12_filename: str, p12_password: str):
    cmd = [
        "keytool", "-list", "-v",
        "-keystore", p12_filename,
        "-storetype", "PKCS12",
        "-alias", CERT_ALIAS,
        "-storepass", p12_password
    ]
    proc = subprocess.run(cmd, capture_output=True, text=True)
    print(proc.stdout)
```

---

## 8. 完整工作流程示例

### 8.1 认证流程

```python
# 步骤1: 启动本地回调服务器并打开浏览器登录
server = LocalCallbackServer(port=3569)
temp_token = server.start_and_wait_for_token()

# 步骤2: 使用tempToken换取JWT
client = HuaweiApiClient()
jwt = client.api_1_exchange_token_for_jwt(temp_token)

# 步骤3: 使用JWT获取AccessToken
if jwt:
    client.api_2_get_access_token(jwt)
    # 现在client.access_token可用于后续API调用
```

---

### 8.2 证书管理工作流

```python
# 步骤1: 获取现有证书列表
cert_list_data = client.api_3_get_cert_list()
certs = cert_list_data.get("certList", [])

# 步骤2: 检查是否存在有效证书
valid_cert = None
for cert in certs:
    if cert.get("certName") == "horpkg" and cert.get("certType") == 1:
        if cert.get("expireTime") > time.time() * 1000:  # 未过期
            valid_cert = cert
            break

# 步骤3: 如果没有有效证书，创建新证书
if not valid_cert:
    # 生成P12和CSR
    csr_pem = generate_files_and_get_csr(
        common_name="horpkg",
        p12_filename="horpkg.p12",
        csr_filename="horpkg.csr",
        p12_password="123456"
    )
    
    # 上传CSR创建证书
    new_cert = client.api_5_add_cert(
        cert_name="horpkg",
        csr_pem_string=csr_pem,
        cert_type=1
    )
    valid_cert = new_cert
```

---

### 8.3 设备管理工作流

```python
# 步骤1: 获取设备列表
device_list = client.api_6_get_device_list()

# 步骤2: 检查目标设备是否已注册
target_udid = "D7C074A10F19DE713784339184E77ADD5178AAE6C56ACB663D5A52A117CED653"
device_id = None

for device in device_list:
    if device.get("udid") == target_udid:
        device_id = device.get("id")
        break

# 步骤3: 如果设备未注册，添加设备
if not device_id:
    success = client.api_8_add_device(
        udid=target_udid,
        device_name="horpkg-device",
        device_type=4
    )
    
    if success:
        # 重新获取设备列表以获取新设备ID
        device_list = client.api_6_get_device_list()
        for device in device_list:
            if device.get("udid") == target_udid:
                device_id = device.get("id")
                break
```

---

### 8.4 Provision 配置文件工作流

```python
# 步骤1: 创建Provision Profile
provision_url = client.api_7_add_provision(
    profile_name="horpkg-profile",
    cert_id=valid_cert.get("id"),
    device_ids=[device_id],
    package_name="com.example.horpkgapp"
)

# 步骤2: 获取Provision文件下载URL
p7b_download_url = client.api_9_get_download_url(
    source_url=provision_url
)

# 步骤3: 下载Provision文件
client.api_10_download_file(
    download_url=p7b_download_url,
    save_path="horpkg.p7b"
)
```

---

### 8.5 证书文件下载和P12打包工作流

```python
# 步骤1: 下载证书文件(.cer)
cert_object_id = valid_cert.get("certObjectId")
cer_download_url = client.api_9_get_download_url(
    source_url=cert_object_id
)
client.api_10_download_file(
    download_url=cer_download_url,
    save_path="horpkg.cer"
)

# 步骤2: 将证书导入P12密钥库
keytool_import_cert(
    p12_filename="horpkg.p12",
    cer_filename="horpkg.cer",
    p12_password="123456"
)

# 步骤3: 验证密钥库
verify_keystore(
    p12_filename="horpkg.p12",
    p12_password="123456"
)
```

---

### 8.6 完整端到端工作流

```python
"""
完整的自动化工作流：
1. 用户认证
2. 证书管理（检查/创建）
3. 设备管理（检查/注册）
4. 创建Provision配置
5. 下载所有必要文件
6. 打包P12密钥库
"""

# 配置参数
LOCAL_SERVER_PORT = 3569
TARGET_DEVICE_UUID = "D7C074A10F19DE713784339184E77ADD..."
TARGET_DEVICE_NAME = "horpkg-device"
PACKAGE_NAME = "com.example.horpkgapp"
CERT_NAME = "horpkg"
CERT_ALIAS = "horpkg"
CERT_TYPE = 1
PROFILE_NAME = "horpkg-profile"
P12_PASSWORD = "123456"

# 执行流程
if __name__ == "__main__":
    # 1. 认证流程
    server = LocalCallbackServer(port=LOCAL_SERVER_PORT)
    temp_token = server.start_and_wait_for_token()
    
    client = HuaweiApiClient()
    jwt = client.api_1_exchange_token_for_jwt(temp_token)
    if jwt:
        client.api_2_get_access_token(jwt)
    
    # 2. 证书管理工作流
    if client.access_token:
        valid_cert = run_certificate_workflow(
            client=client,
            cert_name=CERT_NAME,
            cert_type=CERT_TYPE,
            p12_filename="horpkg.p12",
            csr_filename="horpkg.csr",
            p12_password=P12_PASSWORD
        )
        
        # 3. 下载和打包工作流
        if valid_cert:
            run_download_and_package_workflow(
                client=client,
                cert_object=valid_cert,
                target_udid=TARGET_DEVICE_UUID,
                target_device_name=TARGET_DEVICE_NAME,
                profile_name=PROFILE_NAME,
                profile_type=CERT_TYPE,
                package_name=PACKAGE_NAME,
                cer_filename="horpkg.cer",
                profile_filename="horpkg.p7b",
                p12_filename="horpkg.p12",
                p12_password=P12_PASSWORD
            )
    
    print("\n[+] 最终输出文件:")
    print("    1. horpkg.csr  - 证书签名请求")
    print("    2. horpkg.cer  - 公钥证书")
    print("    3. horpkg.p7b  - Provision配置文件")
    print("    4. horpkg.p12  - PKCS12密钥库（包含私钥和证书链）")
```

---

## 9. 错误码说明

### 9.1 通用HTTP状态码

| 状态码 | 说明 | 解决方案 |
|--------|------|---------|
| 200 | 成功 | - |
| 401 | 未授权 | 检查AccessToken是否有效，重新认证 |
| 403 | 禁止访问 | 检查用户权限或实名认证状态 |
| 404 | 资源不存在 | 检查请求的资源ID是否正确 |
| 500 | 服务器内部错误 | 稍后重试或联系技术支持 |

### 9.2 业务错误码

业务错误码在响应的`ret.code`字段中返回：

| 错误码 | 说明 | 解决方案 |
|--------|------|---------|
| 0 | 成功 | - |
| 非0 | 业务失败 | 查看`ret.msg`字段获取具体错误信息 |

---

## 10. 注意事项

### 10.1 安全注意事项

1. **AccessToken保护**: AccessToken非常重要，需要妥善保管，不要泄露
2. **P12密码**: P12文件密码需要记住，用于后续的应用签名
3. **私钥保管**: keytool生成的P12文件包含私钥，需要安全保存
4. **临时URL有效期**: 下载URL有效期为300秒（5分钟），需要及时下载

### 10.2 使用限制

1. **证书有效期**: 调试证书有效期约为6个月到1年
2. **设备数量**: 调试设备列表有数量限制
3. **包名限制**: 一个Provision配置对应一个应用包名

### 10.3 最佳实践

1. **证书检查**: 定期检查证书是否过期
2. **P12备份**: 备份P12文件和密码
3. **设备管理**: 及时清理不用的测试设备
4. **错误处理**: 实现完善的错误重试机制
5. **日志记录**: 记录关键操作的日志便于排查问题

### 10.4 依赖工具

1. **Python**: 需要 Python 3.6+
2. **库依赖**: 
   - requests
   - cryptography
3. **Java JDK**: 需要安装 JDK（提供 keytool 工具）
4. **网络**: 需要能够访问华为云服务

---

## 11. 技术支持

如有问题，请访问：
- 开发者中心: https://developer.huawei.com
- 技术文档: https://developer.harmonyos.com
- 开发者论坛: https://developer.huawei.com/consumer/cn/forum

---

**文档版本**: v1.0  
**更新日期**: 2025-01-15  
**适用范围**: HarmonyOS DevEco Studio 自动化登录与证书管理  
**脚本版本**: V19+
