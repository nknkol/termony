# 华为 HarmonyOS 开发者 API 使用文档

## 目录
- [1. 概述](#1-概述)
- [2. 认证相关 API](#2-认证相关-api)
- [3. 用户相关 API](#3-用户相关-api)
- [4. 证书管理 API](#4-证书管理-api)
- [5. 设备管理 API](#5-设备管理-api)
- [6. Provision 配置文件管理 API](#6-provision-配置文件管理-api)
- [7. 应用管理 API](#7-应用管理-api)
- [8. 协议相关 API](#8-协议相关-api)

---

## 1. 概述

### 1.1 基础信息
- **主域名**: 
  - 认证服务: `https://cn.devecostudio.huawei.com`
  - 云服务API: `https://connect-api.cloud.huawei.com`

### 1.2 通用请求头
所有API请求需要包含以下请求头：

```
Cookie: hwid_account=your_account_token
User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36
Accept: application/json, text/plain, */*
```

### 1.3 认证流程
1. 获取授权码（code）通过浏览器登录
2. 使用 code 换取 tempToken
3. 验证 tempToken 获取 JWT Token
4. 使用 JWT Token 访问其他 API

---

## 2. 认证相关 API

### 2.1 获取临时Token

**接口名称**: 获取临时Token

**请求URL**: `GET https://cn.devecostudio.huawei.com/authrouter/auth/api/temptoken`

**请求方法**: GET

**请求参数**: 无需参数（通过Cookie中的认证信息自动获取）

**请求头**:
```
Cookie: hwid_account=your_account_token
Accept: application/json
```

**响应数据结构**:
```json
{
  "tempToken": "55e644424387e06431ef695ca28aec2743a4cc9381c6a5cf3e1834d10c50fe4d09f72beb03c3c951bbffc17994394551285fee50bb8301b374ce105bbf793169c3b080f4d702f91df79c4a6f3b1ca79be2fc112966f51edb51e79aa997c21139762c0c28850291c0356afb5f233fe75f70449225563b18e264df93c65b2b0dff34ede94161849232913288ac61090a4dffa606b66c320bcceb8ecef9a93925366511b570815675f7a3056b113b0c05dc98ebc51a0a38bfc588fa828c621ef72be40a9365504814dac4b1c84099c427619f3562c2f5ece597638fa87da18479717fd9cf0bd78f733b61e0b962f101be08c82d5e003f5ff3c4c091553e68319b693ca00518cd77f1d3943761ef8e25cb8a6feab407f165fcd76370d804eed1d09f1db15e2a391de335435bf133e7a7eb0262d012be74d3ad5a09ff70216fb245065dc735789501b0e2a9d5a609c998614fdb160ed16cb64eb4bcebb4684c5004545c7d19f48c78845acd11eb7410fae4c4abfa3a3a7ddd724c1b665f7debe3968fd804fdd141c9f99ebb537ab294df94c854403cce24b58194a5a585497ae8b59a3294a39cb9d6726c2f61d6c730f3494356fa340f23b518546073ac85f257178065fecf455efc7bc034ac4aa8dbdc01a4bfcf1f86880af91f87c24d752bed8ddc0ee94ff8d0fa903104208384cff3d0cc85426857e9e20a22843a46ab58553ea5"
}
```

**响应字段说明**:
- `tempToken`: 临时认证令牌，用于后续的Token验证

**作用**: 生成一个临时的认证令牌，用于在IDE或其他客户端中进行身份验证

**使用示例**:
```bash
curl -X GET "https://cn.devecostudio.huawei.com/authrouter/auth/api/temptoken" \
  -H "Cookie: hwid_account=your_account_token" \
  -H "Accept: application/json"
```

---

### 2.2 验证临时Token

**接口名称**: 验证临时Token

**请求URL**: `GET https://cn.devecostudio.huawei.com/authrouter/auth/api/temptoken/check`

**请求方法**: GET

**请求参数**:
| 参数名 | 类型 | 必填 | 说明 |
|--------|------|------|------|
| site | string | 是 | 站点标识，固定值"CN" |
| tempToken | string | 是 | 临时Token，从上一步获取 |
| appid | string | 是 | 应用ID，如"1007" |
| version | string | 是 | 版本号，如"0.0.0" |

**请求示例**:
```
GET https://cn.devecostudio.huawei.com/authrouter/auth/api/temptoken/check?site=CN&tempToken=55e644...&appid=1007&version=0.0.0
```

**响应数据结构**:
```json
{
  "status": true,
  "message": "success"
}
```

**响应字段说明**:
- `status`: 验证状态，true表示验证成功
- `message`: 返回消息

**作用**: 验证临时Token的有效性，确保Token未过期且有效

**使用示例**:
```bash
curl -X GET "https://cn.devecostudio.huawei.com/authrouter/auth/api/temptoken/check?site=CN&tempToken=YOUR_TEMP_TOKEN&appid=1007&version=0.0.0" \
  -H "Accept: application/json"
```

---

### 2.3 检查JWT Token

**接口名称**: 检查JWT Token

**请求URL**: `GET https://cn.devecostudio.huawei.com/authrouter/auth/api/jwToken/check`

**请求方法**: GET

**请求参数**: 无（通过Cookie中的Token自动验证）

**请求头**:
```
Cookie: hwid_account=your_account_token
Accept: application/json
```

**响应数据结构**:
```json
{
  "status": true,
  "userInfo": {
    "name": "飞舟****",
    "userId": "220086000132459978",
    "nickName": "飞舟****",
    "nationalCode": "CN",
    "accessToken": "DgEAANzA9X16n0gLi2gZeJXuVJ6O7JRmyuKwS5We8TBhwAV6TK877iRf2uEKurV4f7HV5baqMhXo/tf/GQAAyd9VJpsTRayn9Lckt3ryLWU6DwIuuW+opU7cK5BhpQKwvchdZSg=",
    "headPicUrl": null,
    "realName": true
  }
}
```

**响应字段说明**:
- `status`: Token验证状态
- `userInfo`: 用户信息对象
  - `name`: 用户显示名称
  - `userId`: 用户唯一ID
  - `nickName`: 用户昵称
  - `nationalCode`: 国家代码
  - `accessToken`: 访问令牌
  - `headPicUrl`: 头像URL
  - `realName`: 是否实名认证

**作用**: 检查当前JWT Token的有效性并返回用户基本信息

**使用示例**:
```bash
curl -X GET "https://cn.devecostudio.huawei.com/authrouter/auth/api/jwToken/check" \
  -H "Cookie: hwid_account=your_account_token" \
  -H "Accept: application/json"
```

---

## 3. 用户相关 API

### 3.1 获取用户信息

**接口名称**: 获取用户信息

**请求URL**: `GET https://cn.devecostudio.huawei.com/devspaceapi/v1/userinfo`

**请求方法**: GET

**请求参数**:
| 参数名 | 类型 | 必填 | 说明 |
|--------|------|------|------|
| _ | string | 否 | 时间戳，用于防止缓存 |

**请求头**:
```
Cookie: hwid_account=your_account_token
Accept: application/json
```

**响应数据结构**:
```json
{
  "body": {
    "csrfToken": "DCFE7529C47A942820E1A69ABC389CC4560075BD683ED4EFCFBEDE55E7E99ADF",
    "isRealName": true,
    "nickname": "飞舟仁",
    "userId": "220086000132459978"
  },
  "code": 200,
  "success": true
}
```

**响应字段说明**:
- `body`: 响应主体
  - `csrfToken`: CSRF防护令牌
  - `isRealName`: 是否实名认证
  - `nickname`: 用户昵称
  - `userId`: 用户ID
- `code`: HTTP状态码
- `success`: 请求是否成功

**作用**: 获取当前登录用户的详细信息，包括CSRF Token用于后续操作

**使用示例**:
```bash
curl -X GET "https://cn.devecostudio.huawei.com/devspaceapi/v1/userinfo?_=1761829347134" \
  -H "Cookie: hwid_account=your_account_token" \
  -H "Accept: application/json"
```

---

### 3.2 获取用户团队列表

**接口名称**: 获取用户团队列表

**请求URL**: `GET https://connect-api.cloud.huawei.com/api/ups/user-permission-service/v1/user-team-list`

**请求方法**: GET

**请求参数**: 无

**请求头**:
```
Cookie: hwid_account=your_account_token
Accept: application/json
```

**响应数据结构**:
```json
{
  "ret": {
    "code": 0,
    "msg": "OK"
  },
  "teamList": []
}
```

**响应字段说明**:
- `ret`: 返回状态
  - `code`: 状态码，0表示成功
  - `msg`: 状态消息
- `teamList`: 团队列表数组

**作用**: 获取当前用户所属的团队列表，用于团队协作场景

**使用示例**:
```bash
curl -X GET "https://connect-api.cloud.huawei.com/api/ups/user-permission-service/v1/user-team-list" \
  -H "Cookie: hwid_account=your_account_token" \
  -H "Accept: application/json"
```

---

## 4. 证书管理 API

### 4.1 获取证书列表

**接口名称**: 获取证书列表

**请求URL**: `POST https://connect-api.cloud.huawei.com/api/cps/harmony-cert-manage/v1/cert/list`

**请求方法**: POST

**请求参数**: 请求体为空（通过认证信息获取当前用户的证书）

**请求头**:
```
Cookie: hwid_account=your_account_token
Content-Type: application/json
Accept: application/json
```

**请求体**: `null` 或 `{}`

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
      "certName": "xiaobai-debug",
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
  - `certObjectId`: 证书对象存储ID
  - `certName`: 证书名称
  - `certType`: 证书类型（1=调试证书）
  - `p12FingerPrintSha256`: P12证书指纹（调试证书为空）
  - `expireTime`: 过期时间戳（毫秒）
  - `publicKeySha256`: 公钥SHA256指纹
  - `sha256`: 证书SHA256哈希值
  - `status`: 证书状态（1=有效）

**作用**: 获取当前用户所有的HarmonyOS签名证书列表

**使用示例**:
```bash
curl -X POST "https://connect-api.cloud.huawei.com/api/cps/harmony-cert-manage/v1/cert/list" \
  -H "Cookie: hwid_account=your_account_token" \
  -H "Content-Type: application/json" \
  -H "Accept: application/json" \
  -d '{}'
```

---

### 4.2 添加证书

**接口名称**: 添加证书

**请求URL**: `POST https://connect-api.cloud.huawei.com/api/cps/harmony-cert-manage/v1/cert/add`

**请求方法**: POST

**请求参数**:
| 参数名 | 类型 | 必填 | 说明 |
|--------|------|------|------|
| certType | int | 是 | 证书类型，1=调试证书 |
| csr | string | 是 | 证书签名请求（CSR），需要URL编码 |
| certName | string | 是 | 证书名称 |

**请求头**:
```
Content-Type: application/x-www-form-urlencoded
Cookie: hwid_account=your_account_token
Accept: application/json
```

**请求体**:
```
certType=1&csr=-----BEGIN+NEW+CERTIFICATE+REQUEST-----%0AMIIBNTCB3AIBADBKMQkwBwYDVQQGEwAxCTAHBgNVBAgTADEJMAcGA1UEBxMAMQkw%0ABwYDVQQKEwAxCTAHBgNVBAsTADERMA8GA1UEAxMIRGVidWdLZXkwWTATBgcqhkjO%0APQIBBggqhkjOPQMBBwNCAATD3wednKaohxIU8JgMqaUBUY0J5vTbVQoJfldkH7ww%0AngXgAm%2Faz2pmbmPzyo9rFFCf1u0zGSuxOJyVXrPeJFPnoDAwLgYJKoZIhvcNAQkO%0AMSEwHzAdBgNVHQ4EFgQUKZD0ciJg%2FLfWU4D8yEp1Itt8oGAwCgYIKoZIzj0EAwMD%0ASAAwRQIhAJ8K1FCjw9y47pUgx5hWV4RufKhYPJr6ks81jCvUc62%2FAiACZ3ekNDRJ%0Auqc0stEaN4F3IUCtocxBc8P1AylLbktzuw%3D%3D%0A-----END+NEW+CERTIFICATE+REQUEST-----&certName=auto_debug_220086000132459978.cer
```

**响应数据结构**:
```json
{
  "ret": {
    "code": 0,
    "msg": "OK"
  },
  "harmonyCert": {
    "id": "1808235417024089280",
    "createTime": 1761830459404,
    "certObjectId": "CN/2025103013/1761830459368-6b777819-797b-4407-8751-f2e9446ce5a8.cer",
    "certName": "auto_debug_220086000132459978.cer",
    "certType": 1,
    "p12FingerPrintSha256": "",
    "expireTime": 1777382459000,
    "publicKeySha256": "CC:3C:E5:84:40:FF:1F:F6:B6:CB:56:FE:02:85:40:24:9F:4C:FF:DC:C1:66:F3:3D:97:B8:27:46:95:D0:4C:EE",
    "sha256": "bd30a2bda5ad2dbd49c75462debe229bd9f6f85ecf85f994e73f5ec64da5ad5d",
    "status": 1
  }
}
```

**响应字段说明**:
- `ret`: 返回状态对象
- `harmonyCert`: 新创建的证书对象，字段含义同证书列表

**作用**: 使用CSR（证书签名请求）创建一个新的HarmonyOS调试证书

**CSR生成说明**:
CSR需要使用OpenSSL或其他工具生成，包含公钥信息。生成后需要进行URL编码。

**使用示例**:
```bash
# 生成CSR（示例）
openssl req -new -key private.key -out request.csr

# 提交证书申请
curl -X POST "https://connect-api.cloud.huawei.com/api/cps/harmony-cert-manage/v1/cert/add" \
  -H "Cookie: hwid_account=your_account_token" \
  -H "Content-Type: application/x-www-form-urlencoded" \
  -H "Accept: application/json" \
  --data-urlencode "certType=1" \
  --data-urlencode "csr=$(cat request.csr)" \
  --data-urlencode "certName=my_debug_cert.cer"
```

---

### 4.3 删除证书

**接口名称**: 删除证书

**请求URL**: `DELETE https://connect-api.cloud.huawei.com/api/cps/harmony-cert-manage/v1/cert/delete`

**请求方法**: DELETE

**请求参数**: 通过请求体传递

**请求头**:
```
Cookie: hwid_account=your_account_token
Content-Type: application/json
Accept: application/json
```

**请求体**:
```json
{
  "id": "1808235417024089280"
}
```

**请求体参数说明**:
- `id`: 要删除的证书ID（从证书列表中获取）

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

**作用**: 删除指定的证书

**使用示例**:
```bash
curl -X DELETE "https://connect-api.cloud.huawei.com/api/cps/harmony-cert-manage/v1/cert/delete" \
  -H "Cookie: hwid_account=your_account_token" \
  -H "Content-Type: application/json" \
  -H "Accept: application/json" \
  -d '{"id": "1808235417024089280"}'
```

---

## 5. 设备管理 API

### 5.1 获取设备列表

**接口名称**: 获取设备列表

**请求URL**: `GET https://connect-api.cloud.huawei.com/api/cps/device-manage/v1/device/list`

**请求方法**: GET

**请求参数**:
| 参数名 | 类型 | 必填 | 说明 |
|--------|------|------|------|
| start | int | 是 | 起始页码，从1开始 |
| pageSize | int | 是 | 每页数量，如100 |
| encodeFlag | int | 是 | 编码标志，0=不编码 |

**请求头**:
```
Cookie: hwid_account=your_account_token
Accept: application/json
```

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
  "totalCount": 2,
  "list": [
    {
      "id": "1795250237237907968",
      "deviceName": "xiaobai-device-D7C074A10F",
      "udid": "D7C074A10F19DE713784339184E77ADD5178AAE6C56ACB663D5A52A117CED653",
      "deviceType": 4,
      "createTime": "2025-10-12 15:21:45.344",
      "updateTime": "2025-10-12 15:21:45.344",
      "status": 0
    },
    {
      "id": "1644441656348398016",
      "deviceName": "xiaobai-device-1275466917",
      "udid": "1275466917094175C5EA1495416BC102C1ACBB5BC7C3F184E2E8C044C66ED2F1",
      "deviceType": 4,
      "createTime": "2025-03-18 13:32:01.557",
      "updateTime": "2025-03-18 13:32:01.557",
      "status": 0
    }
  ]
}
```

**响应字段说明**:
- `ret`: 返回状态对象
- `totalCount`: 设备总数
- `list`: 设备列表数组
  - `id`: 设备唯一ID
  - `deviceName`: 设备名称
  - `udid`: 设备唯一设备标识符（64位十六进制字符串）
  - `deviceType`: 设备类型（4=真机设备）
  - `createTime`: 创建时间
  - `updateTime`: 更新时间
  - `status`: 设备状态（0=正常）

**作用**: 获取当前用户添加的所有调试设备列表

**使用示例**:
```bash
curl -X GET "https://connect-api.cloud.huawei.com/api/cps/device-manage/v1/device/list?start=1&pageSize=100&encodeFlag=0" \
  -H "Cookie: hwid_account=your_account_token" \
  -H "Accept: application/json"
```

---

## 6. Provision 配置文件管理 API

### 6.1 添加Provision配置文件

**接口名称**: 添加测试Provision配置文件

**请求URL**: `POST https://connect-api.cloud.huawei.com/api/cps/provision-manage/v1/ide/test/provision/add`

**请求方法**: POST

**请求参数**: 通过JSON请求体传递

**请求头**:
```
Cookie: hwid_account=your_account_token
Content-Type: application/json
Accept: application/json
```

**请求体**:
```json
{
  "provisionName": "xiaobai-debug_com_baitude_myapplication",
  "aclPermissionList": [],
  "deviceList": [
    "1795250237237907968",
    "1644441656348398016"
  ],
  "certList": [
    "1795252540640612864"
  ],
  "packageName": "com.baitude.myapplication"
}
```

**请求体参数说明**:
- `provisionName`: Provision配置文件名称
- `aclPermissionList`: ACL权限列表（可为空数组）
- `deviceList`: 设备ID列表，从设备列表API获取
- `certList`: 证书ID列表，从证书列表API获取
- `packageName`: 应用包名

**响应数据结构**:
```json
{
  "ret": {
    "code": 0,
    "msg": "OK"
  },
  "provisionFileUrl": "https://nsp-appgallery-agcfs-drcn.obs.cn-north-2.myhuaweicloud.cn/CN/2025103012/1761829082096-5a055b28-2873-4969-8d8a-7e5a6797dcea.p7b?X-Amz-Algorithm=AWS4-HMAC-SHA256&X-Amz-Credential=HPUA2A528IZVZQHJJCBL%2F20251030%2Fcn-north-2%2Fs3%2Faws4_request&X-Amz-Date=20251030T125802Z&X-Amz-Expires=300&X-Amz-SignedHeaders=host&response-content-disposition=attachment%3Bfilename%3Dxiaobai-debug_com_baitude_myapplication.p7b&X-Amz-Signature=1f8f86d331724dc0426ed9e471b38afe663f43947dc393bca174ad7c5f000dcd"
}
```

**响应字段说明**:
- `ret`: 返回状态对象
- `provisionFileUrl`: Provision配置文件下载URL（临时URL，有效期5分钟）

**作用**: 创建一个测试用的Provision配置文件，将证书、设备和应用包名绑定在一起，生成.p7b文件供应用签名使用

**Provision配置文件说明**:
- Provision配置文件是HarmonyOS应用签名的必要文件
- 它将调试证书、测试设备和应用包名关联起来
- 只有在Provision文件中注册的设备才能安装调试应用
- 配置文件有效期通常与证书一致

**使用示例**:
```bash
curl -X POST "https://connect-api.cloud.huawei.com/api/cps/provision-manage/v1/ide/test/provision/add" \
  -H "Cookie: hwid_account=your_account_token" \
  -H "Content-Type: application/json" \
  -H "Accept: application/json" \
  -d '{
    "provisionName": "my_app_provision",
    "aclPermissionList": [],
    "deviceList": ["1795250237237907968"],
    "certList": ["1795252540640612864"],
    "packageName": "com.example.myapp"
  }'

# 下载Provision文件
curl -o myapp.p7b "返回的provisionFileUrl"
```

---

## 7. 应用管理 API

### 7.1 重新申请对象URL

**接口名称**: 重新申请对象URL

**请求URL**: `POST https://connect-api.cloud.huawei.com/api/amis/app-manage/v1/objects/url/reapply`

**请求方法**: POST

**请求参数**: 通过JSON请求体传递

**请求头**:
```
Cookie: hwid_account=your_account_token
Content-Type: application/json
Accept: application/json
```

**请求体**:
```json
{
  "objectIds": [
    "CN/2025101215/1760282779900-9c3a8aaf-fc46-4fa7-aafd-79cd0f90b428.cer"
  ]
}
```

**请求体参数说明**:
- `objectIds`: 对象ID数组，通常是证书的objectId

**响应数据结构**:
```json
{
  "ret": {
    "code": 0,
    "msg": "OK"
  },
  "objects": [
    {
      "objectId": "CN/2025101215/1760282779900-9c3a8aaf-fc46-4fa7-aafd-79cd0f90b428.cer",
      "url": "https://nsp-appgallery-agcfs-drcn.obs.cn-north-2.myhuaweicloud.cn/CN/2025101215/1760282779900-9c3a8aaf-fc46-4fa7-aafd-79cd0f90b428.cer?X-Amz-Algorithm=AWS4-HMAC-SHA256&...",
      "validTime": 300
    }
  ]
}
```

**响应字段说明**:
- `ret`: 返回状态对象
- `objects`: 对象URL列表
  - `objectId`: 对象ID
  - `url`: 临时下载URL
  - `validTime`: URL有效时间（秒），通常为300秒（5分钟）

**作用**: 为存储在云端的对象（如证书文件）重新生成临时下载URL

**使用示例**:
```bash
curl -X POST "https://connect-api.cloud.huawei.com/api/amis/app-manage/v1/objects/url/reapply" \
  -H "Cookie: hwid_account=your_account_token" \
  -H "Content-Type: application/json" \
  -H "Accept: application/json" \
  -d '{
    "objectIds": ["CN/2025101215/1760282779900-9c3a8aaf-fc46-4fa7-aafd-79cd0f90b428.cer"]
  }'

# 下载证书文件
curl -o certificate.cer "返回的url"
```

---

## 8. 协议相关 API

### 8.1 查询协议内容

**接口名称**: 查询协议内容

**请求URL**: `GET https://cn.devecostudio.huawei.com/authrouter/agreement/v1/queryAgreementContent`

**请求方法**: GET

**请求参数**:
| 参数名 | 类型 | 必填 | 说明 |
|--------|------|------|------|
| language | string | 是 | 语言代码，如"zh-CN" |

**请求头**:
```
Cookie: hwid_account=your_account_token
Accept: application/json
```

**请求示例**:
```
GET https://cn.devecostudio.huawei.com/authrouter/agreement/v1/queryAgreementContent?language=zh-CN
```

**响应数据结构**:
```json
{
  "code": 200,
  "success": true,
  "body": {
    "agreementList": [
      {
        "agrType": 10349,
        "title": "开发者服务协议",
        "content": "协议内容HTML..."
      }
    ]
  }
}
```

**响应字段说明**:
- `code`: 响应状态码
- `success`: 是否成功
- `body`: 响应主体
  - `agreementList`: 协议列表
    - `agrType`: 协议类型ID
    - `title`: 协议标题
    - `content`: 协议内容（HTML格式）

**作用**: 获取服务协议的内容，用于显示给用户

**使用示例**:
```bash
curl -X GET "https://cn.devecostudio.huawei.com/authrouter/agreement/v1/queryAgreementContent?language=zh-CN" \
  -H "Cookie: hwid_account=your_account_token" \
  -H "Accept: application/json"
```

---

### 8.2 查询协议记录

**接口名称**: 查询协议签署记录

**请求URL**: `GET https://cn.devecostudio.huawei.com/authrouter/agreement/v1/queryAgreementRecord`

**请求方法**: GET

**请求参数**:
| 参数名 | 类型 | 必填 | 说明 |
|--------|------|------|------|
| agrType | int | 是 | 协议类型ID（如10349、681） |

**请求头**:
```
Cookie: hwid_account=your_account_token
Accept: application/json
```

**请求示例**:
```
GET https://cn.devecostudio.huawei.com/authrouter/agreement/v1/queryAgreementRecord?agrType=10349
```

**响应数据结构**:
```json
{
  "code": 200,
  "success": true,
  "body": {
    "agrType": 10349,
    "accepted": true,
    "acceptTime": 1761829350000
  }
}
```

**响应字段说明**:
- `code`: 响应状态码
- `success`: 是否成功
- `body`: 响应主体
  - `agrType`: 协议类型ID
  - `accepted`: 是否已签署
  - `acceptTime`: 签署时间戳（毫秒）

**作用**: 查询用户是否已经签署特定的服务协议

**使用示例**:
```bash
curl -X GET "https://cn.devecostudio.huawei.com/authrouter/agreement/v1/queryAgreementRecord?agrType=10349" \
  -H "Cookie: hwid_account=your_account_token" \
  -H "Accept: application/json"
```

---

### 8.3 IDE授权申请

**接口名称**: IDE授权申请

**请求URL**: `GET https://cn.devecostudio.huawei.com/console/DevEcoIDE/apply`

**请求方法**: GET

**请求参数**:
| 参数名 | 类型 | 必填 | 说明 |
|--------|------|------|------|
| port | int | 是 | 本地端口号 |
| appid | string | 是 | 应用ID，如"1007" |
| code | string | 是 | 授权码 |

**请求头**:
```
Cookie: hwid_account=your_account_token
Accept: text/html
```

**请求示例**:
```
GET https://cn.devecostudio.huawei.com/console/DevEcoIDE/apply?port=3904&appid=1007&code=20698961dd4f420c8b44f49010c6f0cc
```

**响应数据**: HTML页面（授权确认页面）

**作用**: IDE通过浏览器发起授权申请，获取用户授权后返回token到本地IDE

**使用说明**:
1. IDE本地启动一个HTTP服务监听指定端口
2. 打开浏览器访问此URL
3. 用户登录并授权
4. 服务器将token回调到本地IDE的HTTP服务

---

## 9. 完整工作流程示例

### 9.1 首次登录流程

```bash
# 1. 通过浏览器访问IDE授权页面（需要用户手动登录）
# https://cn.devecostudio.huawei.com/console/DevEcoIDE/apply?port=3904&appid=1007&code=YOUR_CODE

# 2. 登录成功后获取tempToken
curl -X GET "https://cn.devecostudio.huawei.com/authrouter/auth/api/temptoken" \
  -H "Cookie: hwid_account=YOUR_TOKEN" \
  -c cookies.txt

# 3. 验证tempToken
curl -X GET "https://cn.devecostudio.huawei.com/authrouter/auth/api/temptoken/check?site=CN&tempToken=YOUR_TEMP_TOKEN&appid=1007&version=0.0.0" \
  -b cookies.txt

# 4. 检查JWT Token是否有效
curl -X GET "https://cn.devecostudio.huawei.com/authrouter/auth/api/jwToken/check" \
  -b cookies.txt
```

---

### 9.2 创建签名配置流程

```bash
# 1. 获取证书列表
curl -X POST "https://connect-api.cloud.huawei.com/api/cps/harmony-cert-manage/v1/cert/list" \
  -H "Cookie: hwid_account=YOUR_TOKEN" \
  -H "Content-Type: application/json" \
  -d '{}' > cert_list.json

# 2. 如果没有证书，则添加证书
curl -X POST "https://connect-api.cloud.huawei.com/api/cps/harmony-cert-manage/v1/cert/add" \
  -H "Cookie: hwid_account=YOUR_TOKEN" \
  -H "Content-Type: application/x-www-form-urlencoded" \
  --data-urlencode "certType=1" \
  --data-urlencode "csr=$(cat your_csr.csr)" \
  --data-urlencode "certName=my_debug_cert.cer" > new_cert.json

# 3. 下载证书文件
CERT_OBJECT_ID=$(jq -r '.harmonyCert.certObjectId' new_cert.json)
curl -X POST "https://connect-api.cloud.huawei.com/api/amis/app-manage/v1/objects/url/reapply" \
  -H "Cookie: hwid_account=YOUR_TOKEN" \
  -H "Content-Type: application/json" \
  -d "{\"objectIds\": [\"$CERT_OBJECT_ID\"]}" > cert_url.json

CERT_URL=$(jq -r '.objects[0].url' cert_url.json)
curl -o debug_cert.cer "$CERT_URL"

# 4. 获取设备列表
curl -X GET "https://connect-api.cloud.huawei.com/api/cps/device-manage/v1/device/list?start=1&pageSize=100&encodeFlag=0" \
  -H "Cookie: hwid_account=YOUR_TOKEN" > device_list.json

# 5. 创建Provision配置文件
CERT_ID=$(jq -r '.harmonyCert.id' new_cert.json)
DEVICE_ID=$(jq -r '.list[0].id' device_list.json)

curl -X POST "https://connect-api.cloud.huawei.com/api/cps/provision-manage/v1/ide/test/provision/add" \
  -H "Cookie: hwid_account=YOUR_TOKEN" \
  -H "Content-Type: application/json" \
  -d "{
    \"provisionName\": \"my_app_provision\",
    \"aclPermissionList\": [],
    \"deviceList\": [\"$DEVICE_ID\"],
    \"certList\": [\"$CERT_ID\"],
    \"packageName\": \"com.example.myapp\"
  }" > provision.json

# 6. 下载Provision文件
PROVISION_URL=$(jq -r '.provisionFileUrl' provision.json)
curl -o myapp.p7b "$PROVISION_URL"

echo "签名配置完成！"
echo "证书文件: debug_cert.cer"
echo "配置文件: myapp.p7b"
```

---

## 10. 错误码说明

### 10.1 通用错误码

| 错误码 | 说明 | 解决方案 |
|--------|------|---------|
| 0 | 成功 | - |
| 401 | 未授权 | 检查Cookie中的认证信息是否有效 |
| 403 | 禁止访问 | 检查用户权限或实名认证状态 |
| 404 | 资源不存在 | 检查请求的资源ID是否正确 |
| 500 | 服务器内部错误 | 稍后重试或联系技术支持 |

### 10.2 业务错误码

错误信息通常在响应的`ret.msg`字段中返回，需要根据具体的错误消息进行处理。

---

## 11. 注意事项

### 11.1 安全注意事项

1. **Cookie安全**: Cookie中的认证信息非常重要，需要妥善保管，不要泄露
2. **临时URL有效期**: Provision文件和证书文件的下载URL有效期通常为5分钟（300秒），需要及时下载
3. **CSR私钥保管**: 生成CSR时的私钥需要安全保存，用于后续的应用签名

### 11.2 使用限制

1. **调试设备数量**: 调试设备列表有数量限制，需要合理管理
2. **证书有效期**: 调试证书通常有效期为6个月到1年，需要在过期前更新
3. **Provision配置**: 一个应用包名需要对应一个Provision配置文件

### 11.3 最佳实践

1. **定期检查Token**: 在每次API调用前检查JWT Token是否有效
2. **缓存机制**: 对于设备列表、证书列表等信息可以进行适当缓存，减少API调用
3. **错误重试**: 对于网络错误或临时性错误，可以实现重试机制
4. **日志记录**: 记录关键操作的日志，便于问题排查

---

## 12. 技术支持

如有问题，请访问华为开发者联盟官网：
- 开发者中心: https://developer.huawei.com
- 技术文档: https://developer.harmonyos.com
- 开发者论坛: https://developer.huawei.com/consumer/cn/forum

---

**文档版本**: v1.0  
**更新日期**: 2025-10-30  
**适用范围**: HarmonyOS DevEco Studio API
