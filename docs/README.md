# vscpl1drv-template

The starting point for a new VSCP Level I driver, a *CANAL driver*. A CANAL
driver is a shared library (`.so`, `.dylib` or `.dll`) with a small, fixed
set of C functions — the [CANAL API](https://grodansparadis.github.io/vscp-doc-canal)
— that hosts such as the VSCP daemon, VSCP Works, vscp-cli and the VSCP
simulator load at run time to talk to a CAN bus or anything that carries CAN
frames.

The template is not an empty skeleton. It is a complete driver that builds
and passes its tests on Linux (also Raspberry Pi), macOS and Windows:

* All CANAL 1.0.6 functions, plus the optional CAN FD functions of CANAL 1.1.
* A driver object with a worker thread, a receive queue, blocking calls with
  time-outs, CANAL filter/mask, statistics and status.
* A configuration string and flags that are parsed and checked, and the
  `CanalGetDriverInfo` XML that describes them to VSCP Works.
* It works as a **loopback driver**: sent frames are received again, and a
  test frame can be received at an interval. Hosts can be tested with it
  without hardware.
* A test program that loads the built library the way a host does and checks
  every function.
* CMake build that fetches the VSCP headers with FetchContent, CI for Linux,
  Raspberry Pi (32/64-bit), macOS (arm64/x86_64) and Windows (x64/x86),
  downloadable builds, a man page and these docs.
* `tools/new_driver.py` copies the template to a new driver with everything
  renamed.

## Where to start

1. [Make a new driver](creating-a-driver.md) from the template.
2. Read [how the driver works](design.md) and replace the five `hw*`
   functions with code for your hardware.
3. Change the [configuration string and flags](configure.md) to what your
   hardware needs.
4. [Build](build.md) and [test](testing.md) it.

## Files

| File | What it is |
| :--- | :--------- |
| `src/canal-api.cpp` | The exported CANAL functions. They look up the driver object for a handle and call it. |
| `src/template-driver.h/.cpp` | The driver object. The `HARDWARE` section at the end is what a new driver replaces. |
| `src/driverinfo.h` | The XML returned by `CanalGetDriverInfo`. |
| `src/version.h` | The driver version (year.month.patch), also returned by `CanalGetDllVersion`. |
| `src/vscpl1drv-template.def` | The export list of the Windows DLL. |
| `tests/test-driver.cpp` | Loads the built driver and tests the CANAL API. |
| `tools/new_driver.py` | Makes a new driver from the template. |
| `cmake/dependencies.cmake` | Fetches the VSCP sources (for `canal.h`). |
| `man/vscpl1drv-template.1.md` | The man page, converted with pandoc. |

## License

MIT. See the [LICENSE](https://github.com/grodansparadis/vscpl1drv-template/blob/master/LICENSE) file.
