# HDC (OpenHarmony Device Connector) 用法总结

HDC 是 OpenHarmony 设备的连接器，一个用于在开发主机（PC）和 OpenHarmony 设备之间进行通信和调试的命令行工具。

## 1\. 架构简介

HDC 主要由三个组件构成，它们协同工作以实现设备通信：

1.  **HDC 客户端 (`hdc`)**: 用户在 PC 上直接运行的命令行工具。它负责解析用户输入的命令（例如 `hdc list targets`）。
2.  **HDC 服务器 (`hdc_server`)**: 一个在 PC 上运行的后台服务进程。客户端将命令发送给服务器，服务器负责管理所有与 OpenHarmony 设备的连接会话。
3.  **HDC 守护进程 (`hdcd`)**: 一个在 OpenHarmony 设备上运行的后台进程。它负责接收并执行来自 PC 端服务器的命令（如执行 shell 命令、收发文件等）。

## 2\. 命令语法

```bash
hdc [全局选项] <命令> [命令参数]
```

## 3\. 全局选项

这些选项适用于所有 `hdc` 命令：

  * `-h` / `help [verbose]`: 打印 hdc 帮助信息。`verbose` 会显示更多命令。
  * `-v` / `version`: 打印 hdc 的版本号。
  * `-t <connectkey>`: 指定要连接的目标设备。`connectkey` 通常是设备的 IP:PORT。
  * `-s <[ip:]port>`: 设置 hdc 服务器的监听地址和端口。
  * `-l[0-5]`: 设置运行时日志级别，0 为关闭，5 为最详细。

## 4\. 主要命令

以下是 `hdc` 支持的主要命令，基于 `translate.cpp` 中的帮助信息总结。

### 4.1 会话管理 (服务器命令)

这些命令用于管理 PC 上的 `hdc_server` 进程和设备连接。

  * `start [-r]`: 启动 `hdc_server` 服务。如果带 `-r` 参数，将重启服务。
  * `kill [-r]`: 停止 `hdc_server` 服务。如果带 `-r` 参数，将重启服务。
  * `list targets [-v]`: 列出所有已连接设备的状态。`-v` 显示详细信息。
  * `discover`: 通过局域网广播发现（基于 TCP）正在监听的设备。
  * `tconn <key>`: 通过 `key` 连接设备。对于 TCP 连接，`key` 的格式为 `ip:port`。
  * `wait`: 等待任一设备连接成功。
  * `checkserver`: 检查客户端和服务端的版本是否匹配。

### 4.2 设备控制 (守护进程命令) (无效)

这些命令在设备端的 `hdcd` 上执行。

  * `target mount`: 将设备的 `/system` 和 `/vendor` 分区设置为读写状态。
  * `target boot [MODE]`: 重启设备到指定模式。
  * `smode [-r]`: 以 root 权限重启 `hdcd` 守护进程。`-r` 参数用于取消 root 权限。
  * `tmode port [port]`: 重启设备，并使其在指定的 TCP 端口上监听 hdc 连接。

### 4.3 Shell 与调试

  * `shell [COMMAND...]`: 在设备上执行 shell 命令。如果未提供 `COMMAND`，则进入交互式 shell。
  * `hilog [-h]`: 抓取并显示设备日志。`-h` 查看更多帮助。
  * `bugreport [FILE]`: 生成设备诊断报告。如果指定了 `FILE`，报告将保存到该文件。
  * `jpid`: 列出设备上正在运行的 Java 进程 ID (JDWP)。
  * `track-jpid`: 跟踪设备上的 Java 调试进程。

### 4.4 文件传输

  * `file send [option] <local> <remote>`: 将本地文件或目录发送到设备。

  * `file recv [option] <remote> <local>`: 从设备拉取文件或目录到本地。

    **选项 (Options):**

      * `-a`: 保持文件的时间戳。
      * `-sync`: 仅当本地文件比设备上的文件新时才传输（同步）。
      * `-z`: 压缩传输（使用 LZ4 压缩）。
      * `-m`: 同步文件和目录的模式（权限）。

### 4.5 应用管理

  * `install [-r|-s] <src>`: 安装应用。`<src>` 可以是单个或多个 `.hap` / `.hsp` 包，也可以是包含这些包的目录。
      * 如果 `<src>` 是目录，hdc 会先将其打包成 tar 文件再发送。
      * `-r`: 替换已存在的应用。
      * `-s`: 作为共享包 (Shared Bundle) 安装。
  * `uninstall [-k|-s] <package>`: 卸载应用。
      * `-k`: 卸载但保留数据和缓存目录。
      * `-s`: 移除共享包。

### 4.6 端口转发

  * `fport <localnode> <remotenode>`: **正向转发**。将 PC 本地的流量转发到设备的指定节点。

  * `rport <remotenode> <localnode>`: **反向转发**。将设备本地的流量转发到 PC 的指定节点。

  * `fport ls`: 列出所有已建立的转发和反向转发任务。

  * `fport rm <taskstr>`: 移除一个指定的转发任务。

    **节点 (`node`) 格式:**

      * `tcp:<port>`: TCP 端口。
      * `localfilesystem:<path>`: Unix 域套接字路径。
      * `jdwp:<pid>`: 指定设备上的 Java 进程 ID (仅限 remote node)。

### 4.7 安全

  * `keygen <FILE>`: 生成用于 hdc 认证的 RSA 公钥/私钥对。私钥保存在 `FILE`，公钥保存在 `FILE.pub`。