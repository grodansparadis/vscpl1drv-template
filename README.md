# vscpl1drv-template

![License](https://img.shields.io/badge/license-MIT-blue.svg)
[![Linux](https://github.com/grodansparadis/vscpl1drv-template/actions/workflows/build-linux.yml/badge.svg?branch=master)](https://github.com/grodansparadis/vscpl1drv-template/actions/workflows/build-linux.yml?query=branch%3Amaster+is%3Asuccess)
[![Raspberry Pi arm64](https://github.com/grodansparadis/vscpl1drv-template/actions/workflows/build-raspberry-pi-arm64.yml/badge.svg?branch=master)](https://github.com/grodansparadis/vscpl1drv-template/actions/workflows/build-raspberry-pi-arm64.yml?query=branch%3Amaster+is%3Asuccess)
[![Raspberry Pi armhf](https://github.com/grodansparadis/vscpl1drv-template/actions/workflows/build-raspberry-pi-armhf.yml/badge.svg?branch=master)](https://github.com/grodansparadis/vscpl1drv-template/actions/workflows/build-raspberry-pi-armhf.yml?query=branch%3Amaster+is%3Asuccess)
[![macOS](https://github.com/grodansparadis/vscpl1drv-template/actions/workflows/build-macos.yml/badge.svg?branch=master)](https://github.com/grodansparadis/vscpl1drv-template/actions/workflows/build-macos.yml?query=branch%3Amaster+is%3Asuccess)
[![Windows](https://github.com/grodansparadis/vscpl1drv-template/actions/workflows/build-windows.yml/badge.svg?branch=master)](https://github.com/grodansparadis/vscpl1drv-template/actions/workflows/build-windows.yml?query=branch%3Amaster+is%3Asuccess)
[![Project Status: Active](https://www.repostatus.org/badges/latest/active.svg)](https://www.repostatus.org/#active)

The starting point for a new VSCP Level I driver — a
[CANAL](https://grodansparadis.github.io/vscp-doc-canal) driver that hosts
such as the VSCP daemon, VSCP Works, vscp-cli and the VSCP simulator load to
talk to a CAN bus or anything that carries CAN frames.

It is a complete, tested driver, not an empty skeleton:

* the whole CANAL API (1.0.6) plus the optional CAN FD functions (1.1),
* a driver object with a worker thread, receive queue, blocking calls with
  time-outs, filter/mask, statistics and status,
* a parsed configuration string and flags, described to VSCP Works by
  `CanalGetDriverInfo`,
* it works as a **loopback driver**: sent frames come back, and a test frame
  can be received at an interval,
* a test program that loads the built driver like a host and checks every
  function,
* CMake (VSCP fetched with FetchContent), CI with builds for Linux, Raspberry
  Pi 32/64-bit, macOS and Windows, a man page and documentation.

## Make a new driver

```bash
python3 tools/new_driver.py mydevice ../vscpl1drv-mydevice
cd ../vscpl1drv-mydevice && git init
```

Everything is renamed (library, files, class, macros). Then put the code
for your hardware in the five `hw*` functions at the end of
`src/mydevice-driver.cpp`. See [Make a new driver](docs/creating-a-driver.md).

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
sudo cmake --install build      # Linux: /var/lib/vscp/drivers/level1
```

Windows: `cmake -S . -B build -G "Visual Studio 17 2022" -A x64` and
`cmake --build build --config Release`. See [Build](docs/build.md).

## Configuration

```
interval;id;queue-size
```

Milliseconds between test frames (0 = none), the CAN id of the test frame
(default 0x123) and the size of the receive queue (default 1000). Flags: bit 0
turns the loopback off, bit 1 opens the interface for CAN FD. See
[Configuration and flags](docs/configure.md).

```bash
vscp-dump -i canal --driver build/vscpl1drv-template.so --driver-config "200;0x123"
```

## Download

Click a badge above to see the successful runs and download a build from a
run page: `vscpl1drv-template-linux-x86_64`, `-linux-arm64`, `-linux-armhf`,
`-macos-arm64`, `-macos-x86_64`, `-windows-x64` and `-windows-x86`. Each has a
`SHA256SUMS-<artifact>` file.

## Documentation

The [docs](docs/README.md) folder (a docsify site): overview, making a new
driver, configuration, how the driver works, the CANAL API, building and
testing. `man vscpl1drv-template` after installing.

## License

MIT, see [LICENSE](LICENSE). Changes are listed in
[README_CHANGES.md](README_CHANGES.md).

Copyright (C) 2000-2026 Ake Hedman, Grodans Paradis AB.
