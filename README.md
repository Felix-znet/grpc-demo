# grpc-demo — gRPC 对话式聊天示例

一个基于 gRPC **双向流式 RPC** 的对话式聊天程序:
- `src/server.cpp` — 服务端,监听 `0.0.0.0:50051`
- `src/client.cpp` — 客户端,连上后输入消息

两端都在各自终端里**随处输入、随时收发**,客户端发 → 服务端显示收到 → 服务端输入回复 → 客户端收到 → 客户端继续发,往复对话。输入 `exit` / `q` 退出。

## 特性:自动收录 src/ 下新增的 .cpp

CMakeLists.txt 里用 **`file(GLOB_RECURSE ... CONFIGURE_DEPENDS)`** 自动扫描 `src/`:
只要你在 `src/` 下添加新的 `.cpp` / `.h`,重新运行一次

```powershell
cmake --build build
```

或重新 configure 一次

```powershell
cmake -S . -B build
```

就会**自动把它写进构建、生成对应的可执行目标**,完全不需要手动改 CMakeLists.txt。
(建议重新 configure 以触发 `CONFIGURE_DEPENDS` 的 glob 重新扫描。)

## 依赖

已编译安装的 gRPC 位于 `D:/grpc-install`(包含 grpc、protobuf、abseil 等)。
若你的 grpc 装到别处,改 `CMakeLists.txt` 中的 `set(CMAKE_PREFIX_PATH "你的grpc安装路径" ...)`。

## 构建 / 运行

```powershell
# 1. 配置 (生成 VS 工程)
cmake -S . -B build

# 2. 编译
cmake --build build --config Release

# 3. 运行服务端 (终端 A)
.\build\Release\server.exe

# 4. 运行客户端 (终端 B)
.\build\Release\client.exe
```

### 对话操作
- **客户端终端**:输入消息回车即发给服务端,`q` 退出
- **服务端终端**:收到客户端消息会打印出来,然后输入内容回车即回复给客户端,`q` 退出
- 双方可以**任意交替发送**,gRPC 双向流保证实时收发

## 一键脚本 build.bat

在工程根目录(与该文件同级)双击或运行 `build.bat` 即可自动完成配置 + 编译。

| 命令 | 作用 |
|------|------|
| `build.bat` | 首次配置 + 编译 Release |
| `build.bat build` | 仅重新编译(自动纳入 `src/` 新增的 .cpp/.h) |
| `build.bat rebuild` | 清空 build 目录,完整重新配置 + 编译 |
| `build.bat server` | 编译后启动服务端 |
| `build.bat client` | 编译后启动客户端 |
| `build.bat run` | 编译后开两个窗口,一键起服务端和客户端 |