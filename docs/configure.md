# Configuration and flags

A host opens a CANAL driver with two values: a **configuration string** and
**flags**. Both are passed to `CanalOpen(const char *pDevice, unsigned long
flags)`. In the VSCP daemon they are `config` and `flags` in the driver's
entry, in vscp-cli `--driver-config` and `--flags`.

## Configuration string

A semicolon separated list. All parts are optional; numbers are decimal or
hex with `0x`.

```
interval;id;queue-size
```

| Part | Range | Default | Description |
| :--- | :---- | :------ | :---------- |
| `interval` | 0-3600000 | 0 | Milliseconds between test frames. 0 sends none. The test frame has one data byte that counts up. |
| `id` | 0-0x1fffffff | 0x123 | CAN id of the test frame. Ids above 0x7ff are sent as extended (29-bit) frames. |
| `queue-size` | 1-1000000 | 1000 | Frames the receive queue holds. Frames that arrive when it is full are counted as overruns. |

Examples:

| String | Means |
| :----- | :---- |
| *(empty)* | Loopback only |
| `1000` | A test frame with id 0x123 every second |
| `500;0x0c0a0300` | A test frame every 500 ms with an extended id |
| `;;100` | Loopback with a receive queue of 100 frames |

An invalid string (not a number, out of range) makes `CanalOpen` fail and
return 0.

## Flags

| Bit | Value | Description |
| :-: | :---: | :---------- |
| 0 | 1 | Do not loop sent frames back. |
| 1 | 2 | Open the interface for CAN FD frames. Without it `CanalSendFD` of an FD frame returns `CANAL_ERROR_FD_SUPPORT`. |

## Filter and mask

`CanalSetFilter` and `CanalSetMask` select the received frames by id. A frame
is received when

```
((id XOR filter) AND mask) == 0
```

that is, the bits that are one in the mask must be equal in the id and the
filter. The default mask 0 receives everything. Example: filter 0x100 and
mask 0x7ff receive only id 0x100.

## Driver information

`CanalGetDriverInfo` returns an XML description of the configuration string
and the flags (`src/driverinfo.h`). VSCP Works uses it to show a dialog for
them. The format is described in the
[CANAL specification](https://grodansparadis.github.io/vscp-doc-canal/#/canalgetdriverinfo).
Keep it in step with `parseConfig()` when you change the configuration.

## Changing the configuration

For a new driver, change:

1. `CTemplateDriver::parseConfig()` in `src/template-driver.cpp`,
2. the flags (`TEMPLATE_FLAG_...`) in `src/template-driver.h`,
3. `src/driverinfo.h`,
4. this page and the man page.

Use `setError()` in `parseConfig()` to tell what is wrong; hosts can read it
with `CanalGetStatus` (`lasterrorstr`).
