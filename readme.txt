VideoPlayer
===========

视频播放器：后端（无 C++，无 Qt）负责目录入库 / HTTP 搜索 / WebSocket 推送；
前端（Qt5 + FFmpeg）负责搜索、播放、下载、播放列表、本地上传等。

仓库：https://github.com/fanghuarumeng/VideoPlayer


一、环境要求（Windows）
----------------------

1. Visual Studio 2019（含 C++ 桌面开发 / MSVC x64）
2. CMake >= 3.16
3. Qt 5.12.x（msvc2017_64），仅编译客户端需要
4. 客户端还需要把 FFmpeg **shared** SDK 放到 third_party/ffmpeg/
   （include / lib / bin），说明见 config/ffmpeg-vendor.txt

服务端依赖已 vendored：sqlite、cpp-httplib、websocketpp、asio、nlohmann/json、googletest。


二、一键编译
------------

在仓库根目录执行：

  scripts\build.bat
  scripts\build.bat server
  scripts\build.bat client
  scripts\build.bat all D:\Qt\Qt5.12.11\5.12.11\msvc2017_64

或 PowerShell：

  powershell -ExecutionPolicy Bypass -File scripts\build.ps1
  powershell -ExecutionPolicy Bypass -File scripts\build.ps1 -Target server
  powershell -ExecutionPolicy Bypass -File scripts\build.ps1 -Target client -QtPrefix D:/Qt/Qt5.12.11/5.12.11/msvc2017_64

产物：

  build-server\Release\VideoPlayerServer.exe
  build-client\Release\VideoPlayerApp.exe


三、手动编译（等价命令）
------------------------

服务端（无需 Qt）：

  cmake -S . -B build-server -DBUILD_SERVER=ON -DBUILD_CLIENT=OFF
  cmake --build build-server --config Release --target VideoPlayerServer

客户端：

  cmake -S . -B build-client -G "Visual Studio 16 2019" -A x64 ^
    -DBUILD_SERVER=OFF -DBUILD_CLIENT=ON ^
    -DCMAKE_PREFIX_PATH=D:/Qt/Qt5.12.11/5.12.11/msvc2017_64
  cmake --build build-client --config Release --target VideoPlayerApp


四、运行
--------

先启动后端（在仓库根目录，以便相对路径配置生效）：

  build-server\Release\VideoPlayerServer.exe --config config\server.json.example

再启动前端：

  build-client\Release\VideoPlayerApp.exe --config config\client.json.example

默认端口：

  HTTP API   http://127.0.0.1:8080
  WebSocket  ws://127.0.0.1:8081/ws

示例：

  curl "http://127.0.0.1:8080/api/v1/videos/search?q=&limit=20"
  curl -X POST "http://127.0.0.1:8080/api/v1/ingest/run"

更多说明：

  config\run-server.txt
  config\run-client.txt
  config\server.json.example
  config\client.json.example
  config\authorized_catalog.example.json


五、目录结构（简要）
--------------------

  src\common\video_player\     前后端共用 DTO / JSON
  src\server\video_player\     后端
  src\client\video_player\     前端（含 QML 玻璃态播放器）
  src\client\resources\        QML / 图标等资源
  tests\                       GTest
  scripts\                     编译与 CI 脚本
  third_party\                 第三方源码 / FFmpeg SDK
  config\                      运行配置示例


六、CI（仅服务端）
------------------

  powershell -ExecutionPolicy Bypass -File scripts\ci-server.ps1

GitHub Actions：.github\workflows\server-ci.yml（BUILD_CLIENT=OFF）


七、常见问题
------------

1) 找不到 Qt
   用 -QtPrefix 指向 msvc kit 根目录（含 lib\cmake\Qt5）。

2) 客户端链接报缺 libav*
   按 config\ffmpeg-vendor.txt 放置 shared FFmpeg，不要用 essentials 仅 exe 包。

3) 改名后旧 build 目录报错
   删除 build-server / build-client 后重新跑 scripts\build.bat。

4) windeployqt 报 Unable to patch Qt5Core.dll
   先关掉正在运行的 VideoPlayerApp（脚本也会自动结束占用进程），再编译。

5) Archive.org 入库失败
   当前 cpp-httplib 默认未开 OpenSSL；可用 authorized catalog 离线演示。
