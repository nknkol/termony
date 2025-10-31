# HarmonyOS API 快速参考卡片

## 🚀 快速查找

### 认证类 API

#### 获取临时Token
```bash
GET https://cn.devecostudio.huawei.com/authrouter/auth/api/temptoken
```
**返回**: `{"tempToken": "..."}`

#### 验证临时Token
```bash
GET /authrouter/auth/api/temptoken/check?site=CN&tempToken=XXX&appid=1007&version=0.0.0
```
**返回**: `{"status": true}`

#### 检查JWT Token
```bash
GET https://cn.devecostudio.huawei.com/authrouter/auth/api/jwToken/check
```
**返回**: `{"status": true, "userInfo": {...}}`

---

### 证书类 API

#### 获取证书列表
```bash
POST https://connect-api.cloud.huawei.com/api/cps/harmony-cert-manage/v1/cert/list
Content-Type: application/json
Body: {}
```
**返回**: `{"ret": {"code": 0}, "certList": [...]}`

#### 添加证书
```bash
POST /api/cps/harmony-cert-manage/v1/cert/add
Content-Type: application/x-www-form-urlencoded
Body: certType=1&csr=...&certName=xxx.cer
```
**返回**: `{"ret": {"code": 0}, "harmonyCert": {...}}`

#### 删除证书
```bash
DELETE /api/cps/harmony-cert-manage/v1/cert/delete
Content-Type: application/json
Body: {"id": "证书ID"}
```
**返回**: `{"ret": {"code": 0}}`

---

### 设备类 API

#### 获取设备列表
```bash
GET /api/cps/device-manage/v1/device/list?start=1&pageSize=100&encodeFlag=0
```
**返回**: 
```json
{
  "ret": {"code": 0},
  "totalCount": 2,
  "list": [
    {
      "id": "设备ID",
      "deviceName": "设备名称",
      "udid": "设备UDID"
    }
  ]
}
```

---

### Provision 类 API

#### 创建Provision配置
```bash
POST /api/cps/provision-manage/v1/ide/test/provision/add
Content-Type: application/json
Body: {
  "provisionName": "配置名称",
  "aclPermissionList": [],
  "deviceList": ["设备ID1", "设备ID2"],
  "certList": ["证书ID"],
  "packageName": "com.example.app"
}
```
**返回**: `{"ret": {"code": 0}, "provisionFileUrl": "下载链接"}`

---

### 文件下载类 API

#### 重新申请下载URL
```bash
POST /api/amis/app-manage/v1/objects/url/reapply
Content-Type: application/json
Body: {"objectIds": ["对象ID"]}
```
**返回**: 
```json
{
  "ret": {"code": 0},
  "objects": [{
    "objectId": "对象ID",
    "url": "临时下载链接",
    "validTime": 300
  }]
}
```

---

## 🔄 常用流程

### 完整签名流程（首次配置）
```
1. 检查Token (/jwToken/check)
2. 获取证书列表 (/cert/list)
3. [可选] 删除旧证书 (/cert/delete)
4. 添加新证书 (/cert/add)
5. 获取证书URL (/objects/url/reapply)
6. 下载证书 (curl下载)
7. 获取设备列表 (/device/list)
8. 创建Provision (/provision/add)
9. 下载Provision (curl下载)
```

### 快速签名流程（已有证书）
```
1. 获取设备列表 (/device/list)
2. 创建Provision (/provision/add) - 使用已有证书ID
3. 下载Provision (curl下载)
```

---

## 🎯 错误码速查

| 错误码 | 含义 | 解决方案 |
|--------|------|---------|
| 0 | 成功 | - |
| 401 | 未授权 | 刷新Token |
| 403 | 禁止访问 | 检查实名认证 |
| 404 | 资源不存在 | 检查ID是否正确 |
| 500 | 服务器错误 | 稍后重试 |

---

## 📋 必需请求头

```bash
Cookie: hwid_account=YOUR_TOKEN
Accept: application/json
User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64)
```

对于POST/DELETE请求，还需要：
```bash
Content-Type: application/json
# 或
Content-Type: application/x-www-form-urlencoded  # 添加证书时使用
```

---

## 💡 Tips

1. **Token刷新**: 遇到401错误时，调用 `/jwToken/check` 刷新
2. **URL时效**: 下载链接仅5分钟有效，需立即下载
3. **重试机制**: 网络错误时建议重试3次
4. **设备UDID**: 通过 `hdc shell bm get --udid` 获取
5. **CSR生成**: 使用OpenSSL生成证书签名请求

---

## 🔍 常见查询

### 如何生成CSR？
```bash
# 生成私钥
openssl ecparam -name prime256v1 -genkey -noout -out private.key

# 生成CSR
openssl req -new -key private.key -out request.csr \
  -subj "/C=/ST=/L=/O=/OU=/CN=DebugKey"
```

### 如何下载文件？
```bash
# 获取URL
curl -X POST "https://connect-api.cloud.huawei.com/api/amis/app-manage/v1/objects/url/reapply" \
  -H "Cookie: $COOKIE" \
  -H "Content-Type: application/json" \
  -d '{"objectIds": ["对象ID"]}' | jq -r '.objects[0].url'

# 下载文件
curl -o output.file "下载URL"
```

### 如何验证Cookie有效性？
```bash
curl -s "https://cn.devecostudio.huawei.com/authrouter/auth/api/jwToken/check" \
  -H "Cookie: $COOKIE" | jq '.status'
# 返回 true 表示有效
```

---

## 📞 联系支持

- **开发者中心**: https://developer.huawei.com
- **技术论坛**: https://developer.huawei.com/consumer/cn/forum
- **官方文档**: https://developer.harmonyos.com

---

**快速参考版本**: v1.0  
**适用范围**: HarmonyOS DevEco Studio API  
**数据来源**: 真实抓包分析
