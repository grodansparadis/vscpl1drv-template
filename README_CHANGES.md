# Changes in vscpl1drv-template

Every change that goes into a release is listed here, newest release first.
Add each change under **Unreleased** when it is made. When a release is
tagged, rename **Unreleased** to the version and date, and start a new
**Unreleased** section above it.

## Unreleased

- Build against the latest vscp master (d75d4f5c6)
- Rewritten as a complete, tested template: all CANAL 1.0.6 functions plus the optional CAN FD functions, a loopback driver object (worker thread, receive queue, blocking calls, filter/mask, statistics), a parsed configuration string and CanalGetDriverInfo XML, a test program that loads the driver like a host, tools/new_driver.py to make a new driver, CMake with VSCP fetched by FetchContent, CI and builds for Linux, Raspberry Pi 32/64-bit, macOS and Windows, a man page and docs. The old Visual C++ 6 project and the vscp submodule are removed.
- Updated copyright (88b1376)
- Added submodule (b957b1b)
- Transfered from main repository (165f997)
- Initial commit (8699722)

No release has been tagged yet.
