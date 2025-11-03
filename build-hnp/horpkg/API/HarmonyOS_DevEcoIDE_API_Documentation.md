# **DevEco IDE/小白助手 认证与签名 API 文档**

本文档基于对 DevEco IDE 客户端网络请求的分析总结而成，涵盖了从身份认证到证书、设备及 Profile 管理的完整 API 流程。

## **1\. 认证流程 (Authentication Flow)**

客户端通过一个 OAuth 2.0 授权码流程的变体（使用 tempToken）来获取长效的 accessToken。

1. **启动本地服务器**：客户端（如 DevEco IDE）在本地启动一个 HTTP 服务器（例如 http://localhost:3569）以侦听回调。  
2. **打开浏览器**：客户端打开系统默认浏览器，访问华为的统一认证入口，并在 URL 中附带本地服务器端口、appid 和一个临时的 code。  
   * https://cn.devecostudio.huawei.com/console/DevEcoIDE/apply?port=3569\&appid=1007\&code=...  
3. **用户登录**：用户在浏览器中完成华为帐号登录。  
4. **浏览器回调 (POST)**：登录成功后，华为服务器会指示浏览器向客户端的本地服务器（步骤1中启动的）发送一个 POST 请求（例如 POST http://localhost:3569/callback）。此请求的正文 (form data) 中包含一个一次性的 tempToken。  
5. **交换 JWT (API 1\)**：客户端（Dart 应用）收到 tempToken 后，立即调用 temptoken/check API，将其交换为一个 JWT。  
6. **交换 AccessToken (API 2\)**：客户端立即使用上一步获得的 JWT，调用 jwToken/check API，换取最终的 accessToken、userId 和用户信息。  
7. **调用业务 API**：客户端现在可以使用 accessToken 和 userId 调用所有需要授权的业务 API（如 cert/list, device/add 等）。

## **2\. 通用请求头 (Common Headers)**

所有对 https://connect-api.cloud.huawei.com 的业务 API 调用（API 3-10）都必须包含以下认证头：

* uid: string \- 用户的 userId (来自 API 2)。  
* oauth2token: string \- 用户的 accessToken (来自 API 2)。  
* teamid: string \- 用户的 teamId (通常与 userId 相同)。  
* User-Agent: string \- 客户端标识 (例如 Dart/3.7 (dart:io))。  
* Content-Type: string \- **非常重要**，根据 API 不同而变化 (见下文)。

## **3\. API 端点详情**

### **A. 认证 API (Host: cn.devecostudio.huawei.com)**

#### **API 1: GET /authrouter/auth/api/temptoken/check**

* **描述**：使用从浏览器回调中获取的 tempToken 换取 JWT。  
* **方法**：GET  
* **认证**：否  
* **查询参数 (Query Params)**：  
  * site: "CN"  
  * tempToken: (来自步骤 4 的 tempToken)  
  * appid: "1007"  
  * version: "0.0.0"  
* **成功响应 (200 OK)**：  
  * Content-Type: text/plain  
  * 响应体是一个原始的 JWT 字符串。

#### **API 2: GET /authrouter/auth/api/jwToken/check**

* **描述**：使用 JWT 换取最终的 accessToken 和用户信息。  
* **方法**：GET  
* **认证**：在请求头中提供 JWT。  
* **请求头 (Headers)**：  
  * jwttoken: (来自 API 1 的 JWT 字符串)  
* **成功响应 (200 OK)**：  
  * Content-Type: application/json  
  * **Body**: {"status": true, "userInfo": {"name": ..., "userId": "...", "accessToken": "DgEAA...", ...}}

### **B. 业务 API (Host: connect-api.cloud.huawei.com)**

#### **API 3: POST /api/cps/harmony-cert-manage/v1/cert/list**

* **描述**：获取当前团队的所有证书列表。  
* **方法**：POST  
* **认证**：是 (见通用请求头)  
* **请求体 (Content-Type: application/x-www-form-urlencoded)**：  
  * 空 (Empty Body)。  
* **成功响应 (200 OK)**：  
  * Content-Type: application/json  
  * **Body**: {"ret":{"code":0,"msg":"OK"}, "certList": \[{"id": "...", "certName": "...", "certType": 1, "expireTime": ...}, ...\]}

#### **API 4: DELETE /api/cps/harmony-cert-manage/v1/cert/delete**

* **描述**：删除一个或多个证书。  
* **方法**：DELETE  
* **认证**：是 (见通用请求头)  
* **请求体 (Content-Type: application/json)**：  
  * **Body**: {"certIds": \["1806...464"\]} (一个包含证书 ID 字符串的数组)  
* **成功响应 (200 OK)**：  
  * Content-Type: application/json  
  * **Body**: {"ret":{"code":0,"msg":"OK"}}

#### **API 5: POST /api/cps/harmony-cert-manage/v1/cert/add**

* **描述**：上传一个 CSR 以创建新证书。  
* **方法**：POST  
* **认证**：是 (见通用请求头)  
* **请求体 (Content-Type: application/x-www-form-urlencoded)**：  
  * certName: "horpkg"  
  * csr: (PEM 格式的 CSR 字符串)  
  * certType: 1 (代表 Debug 证书)  
* **成功响应 (200 OK)**：  
  * Content-Type: application/json  
  * **Body**: {"ret":{"code":0,"msg":"OK"}, "harmonyCert": {"id": "...", "certName": "...", "certObjectId": "CN/2025...cer", ...}}

#### **API 6: GET /api/cps/device-manage/v1/device/list**

* **描述**：获取当前团队的所有已注册设备。  
* **方法**：GET  
* **认证**：是 (见通用请求头)  
* **查询参数 (Query Params)**：  
  * start: 1  
  * pageSize: 100  
  * encodeFlag: 0  
* **成功响应 (200 OK)**：  
  * Content-Type: application/json  
  * **Body**: {"ret":{"code":0,"msg":"OK"}, "totalCount": 2, "list": \[{"id": "...", "deviceName": "...", "udid": "..."}, ...\]}

#### **API 7: POST /api/cps/provision-manage/v1/ide/test/provision/add**

* **描述**：创建一个新的 Provision Profile (即 .p7b 文件)。  
* **方法**：POST  
* **认证**：是 (见通用请求头)  
* **请求体 (Content-Type: application/json)**：  
  * **Body**: {"provisionName": "horpkg-profile", "aclPermissionList": \[\], "deviceList": \["1795...968"\], "certList": \["1811...152"\], "packageName": "com.example.horpkgapp"}  
  * deviceList 和 certList 需要的是 id 字符串数组。  
* **成功响应 (200 OK)**：  
  * Content-Type: application/json  
  * **Body**: {"ret":{"code":0,"msg":"OK"}, "provisionFileUrl": "https://nsp-appgallery...p7b?..."} (一个 OBS 链接)

#### **API 8: POST /api/cps/device-manage/v1/device/add**

* **描述**：注册一个新设备到团队。  
* **方法**：POST  
* **认证**：是 (见通用请求头)  
* **请求体 (Content-Type: application/json)**：  
  * **Body**: {"deviceName": "horpkg-device", "udid": "D7C07...ED653", "deviceType": 4}  
* **成功响应 (200 OK)**：  
  * Content-Type: application/json  
  * **Body**: {"ret":{"code":0,"msg":"OK"}}

#### **API 9: POST /api/amis/app-manage/v1/objects/url/reapply**

* **描述**：获取一个用于下载 .cer 或 .p7b 文件的临时（预签名）OBS URL。  
* **方法**：POST  
* **认证**：是 (见通用请求头)  
* **请求体 (Content-Type: application/x-www-form-urlencoded)**：  
  * sourceUrls: (从 API 5 返回的 certObjectId 或从 API 7 返回的 provisionFileUrl)  
* **成功响应 (200 OK)**：  
  * Content-Type: application/json  
  * **Body**: {"ret":{"code":0,"msg":"success."}, "urlsInfo": \[{"sourceUrl": "...", "newUrl": "https://nsp-appgallery..."}\]}

#### **API 10: GET (Dynamic OBS URL)**

* **描述**：下载由 API 9 提供的文件。  
* **方法**：GET  
* **认证**：否 (URL 是预签名的)。  
* **成功响应 (200 OK)**：  
  * 响应体是原始文件数据（.cer 或 .p7b）。