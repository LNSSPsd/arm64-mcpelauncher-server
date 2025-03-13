# arm64-mcpelauncher-server

An unstable hacky Minecraft Bedrock Edition server that runs on arm64 devices (e.g. Raspberry Pi)

NOTE: This branch is for Minecraft **v1.21.60.28** (latest version as of March 12<sup>th</sup>, 2025),  
for Minecraft v1.21.2.02, [click here](https://github.com/LNSSPsd/arm64-mcpelauncher-server/tree/minecraft-v1.21.2.02).
for Minecraft v1.20.81, [click here](https://github.com/LNSSPsd/arm64-mcpelauncher-server/tree/minecraft-v1.20.81.01).

Some configurations might not actually work.

![img](screenshot.png)

The image above shows a Minecraft server running on Raspberry Pi 4 device.  
(Only terminal window for raspi, the Minecraft window is on `x86_64` machine)

This project is based on [mcpelauncher](https://github.com/minecraft-linux/mcpelauncher-manifest) project.  
Some ideas of the class structures came from [LeviLamina](https://github.com/LiteLDev/LeviLamina).

Build instruction:
```
mkdir -p build
cd build
CC=clang CXX=clang++ cmake ..
make -j12
cp server.properties build/mcpelauncher-server/
mkdir build/mcpelauncher-server/data # or preferred data path
# the server should be at build/mcpelauncher-server
# download arm64-v8a version of .apk file on mcpelauncher ui settings-versions-download apk
# and unpack game to build/mcpelauncher-server/game (or customized path) before use, the directory should contain 'lib/' and 'assets/'
```
