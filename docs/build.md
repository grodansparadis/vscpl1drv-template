# Build

CMake 3.16 or later and a C++17 compiler are needed. The VSCP sources (for
`canal.h`) are fetched by CMake with FetchContent, pinned to a commit and
checked with SHA256 — see `cmake/dependencies.cmake`. The driver has no other
dependencies. pandoc is needed for the man page (optional).

## Linux and Raspberry Pi

```bash
sudo apt install build-essential cmake git pandoc
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
sudo cmake --install build
```

The driver is installed in `/var/lib/vscp/drivers/level1`, where the VSCP
daemon looks for Level I drivers. Set `-DDRIVER_INSTALL_DIR=...` for another
place. The man page goes to `share/man/man1`.

Packages: `cd build && cpack` makes a `.deb` and a `.tar.gz`.

## macOS

```bash
xcode-select --install
brew install cmake pandoc
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
cmake --install build --prefix ~/vscp
```

`-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"` builds a universal library.

## Windows

Visual Studio 2022 with *Desktop development with C++*:

```bat
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
cmake --install build --config Release --prefix C:\vscp
```

`-A Win32` builds a 32-bit DLL for 32-bit hosts.

## Using a local VSCP checkout

```bash
cmake -S . -B build -DFETCHCONTENT_SOURCE_DIR_VSCP=../vscp
```

## Ready built drivers

Every push builds the driver in GitHub Actions. Click a badge in the README
and download the artifact from a successful run:

| Artifact | Platform |
| :------- | :------- |
| `vscpl1drv-template-linux-x86_64` | Linux x86_64 (Debian 12, Ubuntu 22.04 and newer) |
| `vscpl1drv-template-linux-arm64` | Raspberry Pi OS 64-bit |
| `vscpl1drv-template-linux-armhf` | Raspberry Pi OS 32-bit |
| `vscpl1drv-template-macos-arm64`, `-macos-x86_64` | macOS |
| `vscpl1drv-template-windows-x64`, `-windows-x86` | Windows |

Each artifact has a `SHA256SUMS-<artifact>` file to check it with.
