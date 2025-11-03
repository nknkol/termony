import requests
import webbrowser
import threading
import time
import json
import base64  # 用于解码
from http.server import BaseHTTPRequestHandler, HTTPServer
from urllib.parse import urlparse, parse_qs
from datetime import datetime
import os 
import subprocess # <-- (V19 新增) 用于调用 keytool

# =============================================================================
# 0. 导入 (V19 - 移除了 cryptography 的打包器)
# =============================================================================
try:
    from cryptography import x509
    from cryptography.x509.oid import NameOID
    from cryptography.hazmat.primitives import hashes
    from cryptography.hazmat.primitives.asymmetric import rsa, ec 
    from cryptography.hazmat.primitives import serialization
    # (V19 移除) 不再需要 pkcs12 和 load_pem_private_key
except ImportError:
    print("[!] 错误：未找到 'cryptography' 库。")
    print("    请运行: pip install cryptography")
    exit(1)


# =============================================================================
# 1. API 客户端类 (保持 V11 不变)
# =============================================================================

class HuaweiApiClient:
    """
    一个封装了所有华为 DevEco API 调用的客户端。
    (V19: 此类保持 V11 不变)
    """
    def __init__(self):
        self.session = requests.Session()
        self.base_url_auth = "https://cn.devecostudio.huawei.com"
        self.base_url_api = "https://connect-api.cloud.huawei.com"
        
        self.user_info = None
        self.access_token = None
        self.user_id = None
        self.team_id = None 

    def _get_auth_headers(self, content_type="application/json; charset=utf-8"):
        if not self.access_token or not self.user_id:
            raise Exception("客户端未认证。请先执行登录流程。")
        return {
            "user-agent": "Dart/3.7 (dart:io)", "uid": self.user_id,
            "oauth2token": self.access_token, "teamid": self.team_id,
            "accept-encoding": "gzip", "content-type": content_type, 
            "Host": "connect-api.cloud.huawei.com"
        }

    # --- API 1 & 2: (保持 V11 不变) ---
    def api_1_exchange_token_for_jwt(self, temp_token):
        print("\n[+] API 1: 正在使用 tempToken 换取 JWT (GET)...")
        url = f"{self.base_url_auth}/authrouter/auth/api/temptoken/check"
        params = { "site": "CN", "tempToken": temp_token, "appid": "1007", "version": "0.0.0" }
        headers = { "user-agent": "Dart/3.7 (dart:io)", "host": "cn.devecostudio.huawei.com", "content-type": "application/json; charset=utf-8" }
        try:
            response = self.session.get(url, params=params, headers=headers)
            if response.status_code == 200:
                jwt_token = response.text
                print(f"[+] API 1: 成功获取JWT！ (JWT: {jwt_token[:40]}...)")
                return jwt_token
            print(f"[!] API 1: 失败。状态码: {response.status_code}, 响应: {response.text}")
            return None
        except requests.RequestException as e:
            print(f"[!] API 1: 请求错误: {e}")
            return None

    def api_2_get_access_token(self, jwt_token):
        print("\n[+] API 2: 正在使用 JWT 换取 AccessToken (GET)...")
        url = f"{self.base_url_auth}/authrouter/auth/api/jwToken/check"
        headers = { "user-agent": "Dart/3.7 (dart:io)", "host": "cn.devecostudio.huawei.com", "content-type": "application/json; charset=utf-8", "jwttoken": jwt_token, "refresh": "false" }
        try:
            response = self.session.get(url, headers=headers)
            if response.status_code == 200:
                data = response.json()
                if data.get("status") == True and "userInfo" in data:
                    self.user_info = data["userInfo"]
                    self.access_token = self.user_info.get("accessToken")
                    self.user_id = self.user_info.get("userId")
                    self.team_id = self.user_id
                    print(f"[+] API 2: 成功获取 AccessToken！")
                    print(f"    UserID: {self.user_id}")
                    print(f"    NickName: {self.user_info.get('nickName')}")
                    return self.user_info
            print(f"[!] API 2: 失败。响应: {response.text}")
            return None
        except requests.RequestException as e:
            print(f"[!] API 2: 请求错误: {e}")
            return None
    
    # --- API 3, 4, 5, 6, 8: (保持 V13 不变) ---
    def api_3_get_cert_list(self):
        """API 3: 获取证书列表"""
        print(f"\n[+] API 3: 正在获取证书列表 (POST, Form-Urlencoded, 空正文)...")
        url = f"{self.base_url_api}/api/cps/harmony-cert-manage/v1/cert/list"
        headers = self._get_auth_headers(content_type="application/x-www-form-urlencoded; charset=UTF-8")
        try:
            response = self.session.post(url, headers=headers, data=None)
            if response.status_code != 200:
                print(f"[!] API 3: HTTP 失败。状态码: {response.status_code}, 响应: {response.text}")
                return None
            data = response.json()
            ret = data.get("ret", {})
            if str(ret.get("code")) == "0":
                certs = data.get('certList', [])
                print(f"[+] API 3: 成功获取证书列表。 (找到 {len(certs)} 个证书)")
                return data
            else:
                print(f"[!] API 3: 业务失败。 Code: {ret.get('code')}, Msg: {ret.get('msg')}")
                return None
        except requests.RequestException as e:
            print(f"[!] API 3: 请求错误: {e}")
            return None

    def api_4_delete_cert(self, cert_id: str):
        """API 4: 删除指定ID的证书 (V13 修正 - DELETE, JSON)"""
        print(f"\n[+] API 4: 正在删除证书 (ID: {cert_id}) (DELETE, JSON)...")
        url = f"{self.base_url_api}/api/cps/harmony-cert-manage/v1/cert/delete"
        headers = self._get_auth_headers(content_type="application/json")
        data = {"certIds": [cert_id]} 
        try:
            response = self.session.delete(url, headers=headers, json=data) 
            if response.status_code != 200:
                print(f"[!] API 4: HTTP 失败。状态码: {response.status_code}, 响应: {response.text}")
                return False
            data = response.json()
            ret = data.get("ret", {})
            if str(ret.get("code")) == "0":
                print(f"[+] API 4: 成功删除证书 (ID: {cert_id})。")
                return True
            else:
                print(f"[!] API 4: 业务失败。 Code: {ret.get('code')}, Msg: {ret.get('msg')}")
                return False
        except requests.RequestException as e:
            print(f"[!] API 4: 请求错误: {e}")
            return False

    def api_5_add_cert(self, cert_name: str, csr_pem_string: str, cert_type: int):
        """API 5: 上传CSR以添加新证书"""
        print(f"\n[+] API 5: 正在为 '{cert_name}' (type={cert_type}) 添加新证书 (POST, Form-Urlencoded)...")
        url = f"{self.base_url_api}/api/cps/harmony-cert-manage/v1/cert/add"
        headers = self._get_auth_headers(content_type="application/x-www-form-urlencoded; charset=UTF-8")
        data = { "certName": cert_name, "csr": csr_pem_string, "certType": cert_type }
        try:
            response = self.session.post(url, headers=headers, data=data)
            if response.status_code != 200:
                print(f"[!] API 5: HTTP 失败。状态码: {response.status_code}, 响应: {response.text}")
                return None
            data = response.json()
            ret = data.get("ret", {})
            if str(ret.get("code")) == "0":
                new_cert = data.get("harmonyCert")
                print(f"[+] API 5: 成功添加新证书！ (New ID: {new_cert.get('id')})")
                return new_cert 
            else:
                print(f"[!] API 5: 业务失败。 Code: {ret.get('code')}, Msg: {ret.get('msg')}")
                return None
        except requests.RequestException as e:
            print(f"[!] API 5: 请求错误: {e}")
            return None

    def api_6_get_device_list(self):
        """API 6: 获取已注册的设备列表"""
        print(f"\n[+] API 6: 正在获取设备列表 (GET)...")
        url = f"{self.base_url_api}/api/cps/device-manage/v1/device/list"
        headers = self._get_auth_headers()
        del headers['content-type'] 
        params = {"start": 1, "pageSize": 100, "encodeFlag": 0}
        try:
            response = self.session.get(url, headers=headers, params=params)
            if response.status_code != 200:
                print(f"[!] API 6: HTTP 失败。状态码: {response.status_code}, 响应: {response.text}")
                return None
            data = response.json()
            ret = data.get("ret", {})
            if str(ret.get("code")) == "0":
                devices = data.get('list', [])
                print(f"[+] API 6: 成功获取设备列表。 (找到 {len(devices)} 个设备)")
                return devices 
            else:
                print(f"[!] API 6: 业务失败。 Code: {ret.get('code')}, Msg: {ret.get('msg')}")
                return None
        except requests.RequestException as e:
            print(f"[!] API 6: 请求错误: {e}")
            return None
            
    def api_7_add_provision(self, profile_name: str, cert_id: str, device_ids: list, package_name: str):
        """API 7 (V11): 创建 Provision Profile"""
        print(f"\n[+] API 7: 正在为 '{profile_name}' 创建 Provision Profile (POST, JSON)...")
        url = f"{self.base_url_api}/api/cps/provision-manage/v1/ide/test/provision/add"
        headers = self._get_auth_headers(content_type="application/json")
        data = {
            "provisionName": profile_name, "aclPermissionList": [],
            "deviceList": device_ids, "certList": [cert_id],
            "packageName": package_name
        }
        try:
            response = self.session.post(url, headers=headers, json=data)
            if response.status_code != 200:
                print(f"[!] API 7: HTTP 失败。状态码: {response.status_code}, 响应: {response.text}")
                return None
            data = response.json()
            ret = data.get("ret", {})
            if str(ret.get("code")) == "0":
                provision_url = data.get("provisionFileUrl")
                print(f"[+] API 7: 成功创建 Provision！")
                return provision_url 
            else:
                print(f"[!] API 7: 业务失败。 Code: {ret.get('code')}, Msg: {ret.get('msg')}")
                return None
        except requests.RequestException as e:
            print(f"[!] API 7: 请求错误: {e}")
            return None

    def api_8_add_device(self, udid: str, device_name: str, device_type: int = 4):
            """API 8: 注册一个新设备 (V20 修正 - POST, JSON)"""
            print(f"\n[+] API 8: 正在注册新设备 '{device_name}' (UDID: {udid[:10]}...) (POST, JSON)...")
            url = f"{self.base_url_api}/api/cps/device-manage/v1/device/add"
            
            # (V20 修正) Content-Type 是 application/json
            headers = self._get_auth_headers(content_type="application/json; charset=utf-8")
            
            # (V20 修正) 请求体是 JSON
            data = {
                "deviceName": device_name,
                "udid": udid,
                "deviceType": device_type
            }
            
            try:
                # (V20 修正) 使用 'json=data'
                response = self.session.post(url, headers=headers, json=data)
                
                if response.status_code != 200:
                    print(f"[!] API 8: HTTP 失败。状态码: {response.status_code}, 响应: {response.text}")
                    return False

                data = response.json()
                ret = data.get("ret", {})
                if str(ret.get("code")) == "0":
                    print(f"[+] API 8: 成功添加设备 '{device_name}'。")
                    return True
                else:
                    print(f"[!] API 8: 业务失败。 Code: {ret.get('code')}, Msg: {ret.get('msg')}")
                    return False
            except requests.RequestException as e:
                print(f"[!] API 8: 请求错误: {e}")
                return False

    def api_9_get_download_url(self, source_url: str):
        """API 9 (V11): 获取 .cer 或 .p7b 的临时下载 URL"""
        print(f"\n[+] API 9: 正在获取下载 URL (POST, Form-Urlencoded)...")
        url = f"{self.base_url_api}/api/amis/app-manage/v1/objects/url/reapply"
        headers = self._get_auth_headers(content_type="application/x-www-form-urlencoded; charset=UTF-8")
        data = {"sourceUrls": source_url}
        try:
            response = self.session.post(url, headers=headers, data=data)
            if response.status_code != 200:
                print(f"[!] API 9: HTTP 失败。状态码: {response.status_code}, 响应: {response.text}")
                return None
            data = response.json()
            ret = data.get("ret", {})
            if str(ret.get("code")) == "0" and "urlsInfo" in data and data["urlsInfo"]:
                new_url = data["urlsInfo"][0].get("newUrl")
                print(f"[+] API 9: 成功获取下载 URL。")
                return new_url
            else:
                print(f"[!] API 9: 业务失败。 Code: {ret.get('code')}, Msg: {ret.get('msg')}")
                return None
        except requests.RequestException as e:
            print(f"[!] API 9: 请求错误: {e}")
            return None

    def api_10_download_file(self, download_url: str, save_path: str):
        """API 10 (V11): 从 OBS URL 下载文件"""
        print(f"[+] API 10: 正在下载文件到 {save_path}...")
        try:
            response = self.session.get(download_url, stream=True)
            if response.status_code == 200:
                with open(save_path, "wb") as f:
                    for chunk in response.iter_content(chunk_size=8192):
                        f.write(chunk)
                print(f"[+] API 10: 下载成功！")
                return True
            else:
                print(f"[!] API 10: 下载失败。状态码: {response.status_code}")
                return False
        except requests.RequestException as e:
            print(f"[!] API 10: 请求错误: {e}")
            return False


# =============================================================================
# 2. 本地服务器类 (保持 V11 不变)
# =============================================================================

class LocalCallbackServer:
    def __init__(self, port):
        self.port = port
        self.temp_token = None
        self._server_instance = None
        self._server_thread = None

    def _get_handler(self):
        server_instance_ref = self 
        class OAuthCallbackHandler(BaseHTTPRequestHandler):
            def do_POST(self):
                try:
                    content_length = int(self.headers.get('Content-Length', 0))
                    post_data_raw = self.rfile.read(content_length)
                    post_data_str = post_data_raw.decode('utf-8')
                    form_data = parse_qs(post_data_str)
                    if "tempToken" in form_data:
                        server_instance_ref.temp_token = form_data["tempToken"][0]
                        print(f"\n[+] 本地服务器: 成功 (POST) 捕获 tempToken！")
                        self.send_response(200)
                        self.send_header("Content-type", "text/html")
                        self.end_headers()
                        self.wfile.write(b"<html><head><meta charset='utf-8'></head><body><h1>\xE7\x99\xBB\xE5\xBD\x95\xE6\x88\x90\E5\x8A\x9F\xEF\xBC\x81</h1><p>Token\xE5\xB7\xB2\xE8\x8E\xB7\E5\x8F\x96\xEF\xBC\x8C\E8\xAF\xB7\E8\xBF\x94\E5\x9B\x9E\E8\x84\x9A\E6\x9C\xAC\E3\x80\x82</p></body></html>")
                    else:
                        self.send_response(400)
                except Exception as e:
                    print(f"[!] 本地服务器: 处理POST时出错: {e}")
                    self.send_response(500)
                finally:
                    threading.Thread(target=server_instance_ref._server_instance.shutdown).start()
            def do_GET(self):
                self.send_response(200)
                self.end_headers()
                self.wfile.write(b"Server is running. Waiting for POST callback.")
        return OAuthCallbackHandler

    def start_and_wait_for_token(self):
        try:
            server_address = ('localhost', self.port)
            self._server_instance = HTTPServer(server_address, self._get_handler())
            print(f"[+] 本地服务器: 已启动 (POST 模式)，正在 http://localhost:{self.port}/ 等待回调...")
            self._server_thread = threading.Thread(target=self._server_instance.serve_forever)
            self._server_thread.daemon = True
            self._server_thread.start()
            print(f"[+] 浏览器: 正在打开登录页面: {AUTHORIZATION_URL}")
            webbrowser.open(AUTHORIZATION_URL)
            self._server_thread.join()
            return self.temp_token
        except OSError as e:
            if e.errno == 48:
                print(f"[!] 本地服务器: 错误：端口 {self.port} 已被占用。")
            else:
                print(f"[!] 本地服务器: 启动失败: {e}")
            return None
        except Exception as e:
            print(f"[!] 本地服务器: 启动失败: {e}")
            return None


# =============================================================================
# 3. (V19 重写) 辅助函数: 使用 keytool 生成 .p12 和 .csr
# =============================================================================

def generate_files_and_get_csr(common_name: str, p12_filename: str, csr_filename: str, p12_password: str):
    """
    (V19) 使用 keytool 生成 .p12 (含私钥) 和 .csr
    """
    print(f"\n[+] 辅助函数 (keytool): 正在为 '{common_name}' 生成 .p12 和 .csr...")

    # 1. (keytool 步骤 1) 生成 p12 (含私钥 + 自签占位证书)
    # dname 遵循您 keytool 流程中的示例
    dname = f"C=CN, ST=Beijing, L=Beijing, O=YourOrg, OU=Mobile, CN={common_name}"
    
    cmd_genkey = [
        "keytool", "-genkeypair",
        "-alias", CERT_ALIAS, # 使用全局变量 "horpkg"
        "-keystore", p12_filename,
        "-storetype", "PKCS12",
        "-keyalg", "EC",
        "-keysize", "256",
        "-sigalg", "SHA256withECDSA",
        "-dname", dname,
        "-validity", "3650",
        "-storepass", p12_password,
        "-keypass", p12_password # 保持一致
    ]
    
    print(f"    1. 正在执行 (genkeypair): {' '.join(cmd_genkey)}")
    try:
        result_gen = subprocess.run(cmd_genkey, capture_output=True, text=True, check=True)
    except FileNotFoundError:
        print(f"[!] 辅助函数 (keytool): 致命错误！ 'keytool' 命令未找到。")
        print(f"    ... 请确保 Java Development Kit (JDK) 已安装并在系统 PATH 中。")
        return None
    except subprocess.CalledProcessError as e:
        print(f"[!] 辅助函数 (keytool): genkeypair 失败！")
        print(f"    Stderr: {e.stderr}")
        return None
        
    print(f"[+] 辅助函数: 临时的 {p12_filename} (含私钥) 已创建。")

    # 2. (keytool 步骤 2) 生成 CSR
    cmd_certreq = [
        "keytool", "-certreq",
        "-alias", CERT_ALIAS,
        "-keystore", p12_filename,
        "-storetype", "PKCS12",
        "-file", csr_filename,
        "-sigalg", "SHA256withECDSA",
        "-storepass", p12_password
    ]

    print(f"    2. 正在执行 (certreq): {' '.join(cmd_certreq)}")
    try:
        result_req = subprocess.run(cmd_certreq, capture_output=True, text=True, check=True)
    except subprocess.CalledProcessError as e:
        print(f"[!] 辅助函数 (keytool): certreq 失败！")
        print(f"    Stderr: {e.stderr}")
        return None
        
    print(f"[+] 辅助函数: {csr_filename} 已创建。")

    # 3. 读取 CSR 文件内容
    try:
        with open(csr_filename, "r") as f:
            csr_pem_string = f.read()
        return csr_pem_string
    except IOError as e:
        print(f"[!] 辅助函数: 读取 {csr_filename} 失败: {e}")
        return None


# =============================================================================
# 4. (V19 重写) 证书管理工作流 (检查 .p12)
# =============================================================================

def run_certificate_workflow(client: HuaweiApiClient, cert_name: str, cert_type: int, p12_filename: str, csr_filename: str, p12_password: str):
    """
    (V19)
    1. 检查服务器证书。
    2. (关键) 如果证书有效，检查本地 .p12 (Keystore) 是否存在。
    3. 如果 .p12 不存在，强制删除服务器证书并重新创建。
    """
    print(f"\n[+] === 开始证书管理工作流 (目标: '{cert_name}', 类型: {cert_type}) ===")
    
    list_response_data = client.api_3_get_cert_list()
    
    if not list_response_data:
        print("[!] 证书流: 获取证书列表失败 (API调用失败)。工作流终止。")
        return None

    found_cert = None
    for cert in list_response_data.get("certList", []):
        if cert.get("certName") == cert_name and cert.get("certType") == cert_type:
            found_cert = cert
            break

    needs_new_cert = False
    valid_cert_object = None 
    
    if found_cert:
        cert_id = found_cert.get("id")
        expire_time_ms = found_cert.get("expireTime", 0)
        expire_time = datetime.fromtimestamp(expire_time_ms / 1000)
        
        print(f"[+] 证书流: 找到证书 '{cert_name}' (ID: {cert_id})。")
        print(f"    过期时间: {expire_time}")

        if expire_time < datetime.now():
            print(f"[!] 证书流: 证书 '{cert_name}' 已于 {expire_time} 过期。")
            if client.api_4_delete_cert(cert_id):
                needs_new_cert = True
            else:
                print("[!] 证书流: 删除过期证书失败，流程终止。")
                return None
        else:
            # (V19 修正) 检查 .p12 (keystore) 是否存在，而不是 .key
            if not os.path.exists(p12_filename):
                print(f"[!] 证书流: 证书 '{cert_name}' 在服务器上有效，但本地密钥库 '{p12_filename}' 缺失！")
                print(f"[!] 证书流: 将强制删除服务器证书以创建新密钥库...")
                
                if client.api_4_delete_cert(cert_id):
                    needs_new_cert = True
                    valid_cert_object = None
                else:
                    print(f"[!] 证书流: 尝试删除服务器证书失败。流程终止。")
                    return None
            else:
                print(f"[+] 证书流: 证书 '{cert_name}' 仍然有效，且本地密钥库 {p12_filename} 已找到。")
                valid_cert_object = found_cert
            
    else:
        print(f"[!] 证书流: 未在列表中找到名为 '{cert_name}' 的 {cert_type} 型证书。")
        needs_new_cert = True

    if needs_new_cert:
        print(f"[+] 证书流: 需要为 '{cert_name}' 创建一个新证书。")
        
        # (V19 修正) 调用 keytool 辅助函数
        csr_pem = generate_files_and_get_csr(
            common_name=cert_name, 
            p12_filename=p12_filename, 
            csr_filename=csr_filename,
            p12_password=p12_password
        )
        
        if not csr_pem:
            print("[!] 证书流: (keytool) 生成 CSR/P12 失败。流程终止。")
            return None
        
        new_cert = client.api_5_add_cert(cert_name, csr_pem, cert_type)
        if new_cert:
            print(f"[+] 证书流: 成功创建新证书！ ID: {new_cert.get('id')}")
            valid_cert_object = new_cert
        else:
            print("[!] 证书流: 创建新证书失败。")
            return None

    print("[+] === 证书管理工作流结束 ===")
    return valid_cert_object


# =============================================================================
# 5. (V19 重写) 辅助函数：使用 keytool 导入 .cer
# =============================================================================

def keytool_import_cert(p12_filename: str, cer_filename: str, p12_password: str):
    print(f"\n[+] 辅助函数 (keytool): 正在将 {cer_filename} 导入 {p12_filename}...")

    cmd_import = [
        "keytool", "-importcert",
        "-alias", CERT_ALIAS,
        "-keystore", p12_filename,
        "-storetype", "PKCS12",
        "-file", cer_filename,
        "-trustcacerts",
        "-storepass", p12_password,
        "-noprompt",
    ]
    print(f"    3. 正在执行 (importcert): {' '.join(cmd_import)}")

    try:
        proc = subprocess.run(cmd_import, capture_output=True, text=True)
        out, err = proc.stdout or "", proc.stderr or ""
        text = (out + "\n" + err)

        # 1) 首选以返回码判断
        if proc.returncode == 0:
            print("[+] 辅助函数: keytool importcert 返回码=0，判定成功。")
            # 2) 兼容不同 JDK 本地化输出，提示一下关键信息（它常在 stderr）
            if ("installed in keystore" in text) or ("已安装在密钥库中" in text):
                print("[+] 辅助函数: 证书回复已安装在密钥库中。")
            else:
                print("[i] 辅助函数: keytool 输出（可能在 stderr）：")
                print(text.strip())
            print(f"[+] 辅助函数: 最终的 P12 文件已准备就绪 -> {p12_filename}")
            return True

        # returncode != 0 才算失败
        print("[!] 辅助函数 (keytool): importcert 失败！")
        print("    Stdout:", out.strip())
        print("    Stderr:", err.strip())
        return False

    except FileNotFoundError:
        print("[!] 辅助函数 (keytool): 致命错误！未找到 keytool。请确认 JDK 已安装并在 PATH 中。")
        return False

def verify_keystore(p12_filename: str, p12_password: str):
    cmd = [
        "keytool", "-list", "-v",
        "-keystore", p12_filename, "-storetype", "PKCS12",
        "-alias", CERT_ALIAS, "-storepass", p12_password
    ]
    proc = subprocess.run(cmd, capture_output=True, text=True)
    print("\n[+] 验证 keystore：")
    # keytool 可能把提示写到 stderr，所以两边都打印一份
    if proc.stdout:
        print(proc.stdout.strip())
    if proc.stderr and proc.stderr.strip():
        print(proc.stderr.strip())

# =============================================================================
# 6. (V19 重写) 设备、Profile 和文件下载工作流
# =============================================================================

def run_download_and_package_workflow(client: HuaweiApiClient, cert_object: dict, 
                                        target_udid: str, target_device_name: str, 
                                        profile_name: str, profile_type: int, package_name: str,
                                        cer_filename: str, profile_filename: str, 
                                        p12_filename: str, p12_password: str):
    """
    (V19) 检查/添加设备，下载 .cer, 创建/下载 .p7b, 并调用 keytool 导入 .cer
    """
    print(f"\n[+] === 开始下载与打包工作流 (目标设备 UDID: {target_udid[:10]}...) ===")
    
    if not target_udid:
        print("[!] 工作流: 错误！未在脚本中设置 TARGET_DEVICE_UUID。")
        return
        
    cert_id = cert_object.get("id")
    cert_object_id = cert_object.get("certObjectId") 
    
    if not cert_id or not cert_object_id:
        print(f"[!] 工作流: 证书对象无效。缺少 'id' 或 'certObjectId'。")
        return

    # --- 步骤 1: 下载 .CER 文件 ---
    print(f"[+] 工作流: 步骤 1/4 - 正在下载 .cer 文件...")
    cer_download_url = client.api_9_get_download_url(source_url=cert_object_id)
    if not cer_download_url:
        print("[!] 工作流: 获取 .cer 下载 URL 失败。")
        return
    if not client.api_10_download_file(cer_download_url, cer_filename):
        print("[!] 工作流: 下载 .cer 文件失败。")
        return

    # --- 步骤 2: 检查/添加设备 ---
    print(f"\n[+] 工作流: 步骤 2/4 - 正在检查设备...")
    device_list = client.api_6_get_device_list()
    if device_list is None:
        print("[!] 工作流: 获取设备列表失败。")
        return

    target_device_id = None
    for device in device_list:
        if device.get("udid") == target_udid:
            print(f"[+] 工作流: 找到已注册的目标设备: {device.get('deviceName')}")
            target_device_id = device.get("id")
            break
    
    if not target_device_id:
        print(f"[!] 工作流: 目标设备 {target_udid[:10]}... 未注册。")
        if client.api_8_add_device(target_udid, target_device_name, device_type=4):
            print("[+] 工作流: 重新获取设备列表以查找新 ID...")
            time.sleep(1) 
            device_list = client.api_6_get_device_list()
            if device_list:
                for device in device_list:
                    if device.get("udid") == target_udid:
                        target_device_id = device.get("id")
                        break
        if not target_device_id:
            print(f"[!] 工作流: 添加设备失败，或未能找到新添加的设备 ID。流程终止。")
            return
            
    print(f"[+] 工作流: 准备就绪 (CertID: {cert_id}, DeviceID: {target_device_id})")

    # --- 步骤 3: 创建和下载 Profile (.p7b) ---
    print(f"\n[+] 工作流: 步骤 3/4 - 正在创建和下载 .p7b (Profile) 文件...")
    provision_file_url = client.api_7_add_provision(
        profile_name=profile_name,
        cert_id=cert_id,
        device_ids=[target_device_id], 
        package_name=package_name
    )
    if not provision_file_url:
        print("[!] 工作流: 创建 Provision Profile 失败 (API 7)。")
        return
        
    p7b_download_url = client.api_9_get_download_url(source_url=provision_file_url)
    if not p7b_download_url:
        print("[!] 工作流: 获取 .p7b 下载 URL 失败 (API 9)。")
        return
    if not client.api_10_download_file(p7b_download_url, profile_filename):
        print("[!] 工作流: 下载 .p7b 文件失败 (API 10)。")
        return
    print(f"[+] 工作流: 签名文件 (.p7b) 已保存到 -> {profile_filename}")

    # --- (V19) 步骤 4: 打包 .p12 (使用 keytool) ---
    print(f"\n[+] 工作流: 步骤 4/4 - 正在打包 .p12 文件 (使用 keytool)...")
    ok = keytool_import_cert(
        p12_filename=p12_filename,
        cer_filename=cer_filename,
        p12_password=p12_password
    )

    # ✅ 导入成功才做校验
    if ok:
        verify_keystore(p12_filename=p12_filename, p12_password=p12_password)
    else:
        print("[!] 工作流: 导入 .cer 失败，跳过 keystore 校验。")

    print("[+] === 下载与打包工作流结束 ===")


# =============================================================================
# 7. 主执行入口 (V19 - 更新)
# =============================================================================

# --- 全局配置 ---
LOCAL_SERVER_PORT = 3569
AUTHORIZATION_URL = "https://cn.devecostudio.huawei.com/console/DevEcoIDE/apply?port=3569&appid=1007&code=20698961dd4f420c8b44f49010c6f0cc" # ⚠️ Code 可能已过期

# --- (V19) 您的设备和包名配置 ---
TARGET_DEVICE_UUID = "D7C074A10F19DE713784339184E77ADD5178AAE6C56ACB663D5A52A117CED653" # 例如 "D7C074A10F19DE713784339184E77ADD5178AAE6C56ACB663D5A52A117CED653"
TARGET_DEVICE_NAME = "horpkg-device" 
PACKAGE_NAME = "com.example.horpkgapp" 

# --- (V19) 文件和名称配置 ---
CERT_NAME = "horpkg"
CERT_ALIAS = "horpkg" # (V19 新增) keytool -alias
CERT_TYPE = 1 # 1 = Debug
PROFILE_NAME = "horpkg-profile"

# --- (V19) 最终输出文件配置 ---
# (V19 移除) KEY_FILENAME
CSR_FILENAME = "horpkg.csr" # (签名请求)
CER_FILENAME = "horpkg.cer" # (公钥证书)
PROFILE_FILENAME = "horpkg.p7b" # (Profile 签名文件)
P12_FILENAME = "horpkg.p12" # (P12 捆绑包)
P12_PASSWORD = "123456" # P12 文件的导出密码


if __name__ == "__main__":
    print("[+] === 脚本启动 ===")
    
    if not TARGET_DEVICE_UUID or not PACKAGE_NAME:
        print("[!] 致命错误: 请在 '主执行入口' (第 7 部分) 中设置 TARGET_DEVICE_UUID 和 PACKAGE_NAME 变量。")
        exit(1)
        
    # --- 步骤 1: 启动服务器并等待 tempToken ---
    server = LocalCallbackServer(port=LOCAL_SERVER_PORT)
    temp_token = server.start_and_wait_for_token()
    
    if not temp_token:
        print("\n[!] 致命错误: 未能获取 tempToken。脚本终止。")
        exit(1)
        
    # --- 步骤 2: 执行登录 API 流程 ---
    client = HuaweiApiClient()
    
    jwt = client.api_1_exchange_token_for_jwt(temp_token)
    if jwt:
        client.api_2_get_access_token(jwt)
    
    # --- 步骤 3: 执行证书管理工作流 ---
    if client.access_token:
        # (V19 修正) 
        valid_cert_object = run_certificate_workflow(
            client=client, 
            cert_name=CERT_NAME, 
            cert_type=CERT_TYPE, 
            p12_filename=P12_FILENAME, # (V19)
            csr_filename=CSR_FILENAME,
            p12_password=P12_PASSWORD  # (V19)
        )
        
        # --- (V19) 步骤 4: 执行下载和打包工作流 ---
        if valid_cert_object:
            run_download_and_package_workflow(
                client=client,
                cert_object=valid_cert_object,
                target_udid=TARGET_DEVICE_UUID,
                target_device_name=TARGET_DEVICE_NAME,
                profile_name=PROFILE_NAME,
                profile_type=CERT_TYPE,
                package_name=PACKAGE_NAME, 
                cer_filename=CER_FILENAME,
                profile_filename=PROFILE_FILENAME,
                p12_filename=P12_FILENAME,
                p12_password=P12_PASSWORD
            )
        else:
            print("\n[!] 错误: 未能获取有效的证书对象，无法继续创建 Profile。")
            
    else:
        print("\n[!] 错误: 未能获取 AccessToken，无法继续执行证书和 Profile 流程。")

    print("\n[+] === 脚本执行完毕 ===")
    print("\n[+] 最终文件已生成 (使用 keytool):")
    # (V19 移除) .key
    print(f"    1. 签名请求: {os.path.abspath(CSR_FILENAME)}")
    print(f"    2. 公钥证书: {os.path.abspath(CER_FILENAME)}")
    print(f"    3. Profile:   {os.path.abspath(PROFILE_FILENAME)}")
    print(f"    4. P12 密钥库: {os.path.abspath(P12_FILENAME)}")