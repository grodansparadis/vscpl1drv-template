# Make a new driver

## 1. Copy the template

```bash
git clone https://github.com/grodansparadis/vscpl1drv-template.git
cd vscpl1drv-template
python3 tools/new_driver.py mydevice ../vscpl1drv-mydevice
cd ../vscpl1drv-mydevice
git init
```

`new_driver.py` copies the template and renames everything:

| In the template | In the new driver |
| :-------------- | :---------------- |
| `vscpl1drv-template` (library, project, man page, `.def` file) | `vscpl1drv-mydevice` |
| `src/template-driver.h/.cpp` | `src/mydevice-driver.h/.cpp` |
| `CTemplateDriver` | `CMydeviceDriver` |
| `TEMPLATE_FLAG_...`, `TEMPLATE_DEFAULT_...` | `MYDEVICE_FLAG_...`, `MYDEVICE_DEFAULT_...` |

The new driver gets an empty `README_CHANGES.md` and version 0 of the
current month. The name must be lower case letters, digits and `-`.

Build it right away; it is still a working loopback driver:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## 2. Talk to your hardware

All hardware access is in five functions at the end of
`src/mydevice-driver.cpp`, in the section marked `HARDWARE`:

| Function | Called | Does |
| :------- | :----- | :--- |
| `hwOpen()` | by `CanalOpen`, after the configuration is parsed | Open the device (serial port, USB, socket, vendor library...). Return `false` on failure. |
| `hwClose()` | by `CanalClose` | Close the device. |
| `hwWrite(msg)` | by the send functions | Write one frame. Return `CANAL_ERROR_SUCCESS`, or `CANAL_ERROR_TRM_FULL` when the device can not take it now. |
| `hwRead(msg, timeout)` | in a loop by the worker thread | Wait at most `timeout` ms for a frame. Return `true` and fill in `flags`, `id`, `sizeData` and `data` when one arrived. |
| `hwSetBaudrate(rate)` | by `CanalSetBaudrate` | Set the bus speed, or return `CANAL_ERROR_NOT_SUPPORTED`. |

Remove the loopback members (`m_hwLoop`, `m_hwCv`, `m_hwMutex`,
`m_nextTest`, `m_testCounter`) from the header when they are no longer
used, and add what your hardware needs (a file descriptor, a device handle).

Everything else stays: the receive queue, the filter, statistics, status,
blocking calls with time-outs, closing while another thread waits, and the
CANAL functions in `canal-api.cpp`. See [how the driver works](design.md).

`hwRead` must return within `timeout` so the driver can be closed. With a
file descriptor use `poll()`/`select()` with the time-out; with a vendor
library use its read function with a time-out.

## 3. Configuration and flags

Change `parseConfig()` to the configuration string your driver needs (for
example a serial port and a bit rate), and the flags in the header. Then
update the documentation of them in three places:

* `src/driverinfo.h` — the XML that VSCP Works uses to show a dialog,
* `docs/configure.md`,
* the man page `man/vscpl1drv-mydevice.1.md`.

See [Configuration and flags](configure.md).

## 4. Names and texts

* `DRIVER_VENDOR` in `src/canal-api.cpp` — the vendor string.
* The description in `CMakeLists.txt` (`project(... DESCRIPTION ...)`) and
  the CPack vendor and contact.
* `README.md`, the docs and the man page.

## 5. CAN FD or not

The four `*FD` functions in `canal-api.cpp` are optional (CANAL 1.1). If your
hardware can not do CAN FD, remove them and their lines in the `.def` file.
Hosts then use the standard functions. Otherwise implement FD frames in
`hwWrite`/`hwRead`; the driver object already handles FD frame sizes, the FD
flag at open (bit 1) and keeps FD frames away from classic receivers.

## 6. Tests

`tests/test-driver.cpp` tests the template's loopback. Keep the parts that
test the API (exports, handles, versions, parameter checks) and change the
loopback parts to what your driver can do without hardware — or run them
against your hardware in a separate test.

## 7. Publish

Push to GitHub. The workflows build and test the driver on all platforms and
the README badges show the result. On every push to the default branch the
*Update version* workflow bumps the patch number in `src/version.h`.
