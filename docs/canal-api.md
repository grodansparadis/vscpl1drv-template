# The CANAL API

How the template implements each CANAL function. The full specification is
at <https://grodansparadis.github.io/vscp-doc-canal>.

| Function | Template |
| :------- | :------- |
| `long CanalOpen(const char *pDevice, unsigned long flags)` | Parses the [configuration string](configure.md) and flags, opens the hardware. Returns a handle > 0, or 0 on failure. |
| `int CanalClose(long handle)` | Closes the interface. `CANAL_ERROR_NOT_OPEN` for an unknown handle. |
| `unsigned long CanalGetLevel(long handle)` | `CANAL_LEVEL_STANDARD` (1), 0 for an unknown handle. |
| `int CanalSend(long handle, PCANALMSG msg)` | Sends a classic frame. `CANAL_ERROR_CAN_ID` for an id out of range, `CANAL_ERROR_MTU` for more than 8 data bytes. |
| `int CanalBlockingSend(long handle, PCANALMSG msg, unsigned long timeout)` | As `CanalSend`; the time-out is passed to the driver for hardware that can wait for room. |
| `int CanalReceive(long handle, PCANALMSG msg)` | Next frame, or `CANAL_ERROR_FIFO_EMPTY`. |
| `int CanalBlockingReceive(long handle, PCANALMSG msg, unsigned long timeout)` | Waits for a frame. `CANAL_ERROR_TIMEOUT` after `timeout` ms; 0 waits forever. `CANAL_ERROR_NOT_OPEN` when the interface is closed while waiting. |
| `int CanalDataAvailable(long handle)` | Number of frames in the queue (classic frames). |
| `int CanalGetStatus(long handle, PCANALSTATUS status)` | Channel state (`CANAL_STATUS_ACTIVE`, `..._RECIVE_BUFFER_FULL`, `..._STOPPED`) and the last error. |
| `int CanalGetStatistics(long handle, PCANALSTATISTICS stat)` | Frames and bytes sent and received, overruns. |
| `int CanalSetFilter(long handle, unsigned long filter)` | See [filter and mask](configure.md#filter-and-mask). |
| `int CanalSetMask(long handle, unsigned long mask)` | See [filter and mask](configure.md#filter-and-mask). |
| `int CanalSetBaudrate(long handle, unsigned long baudrate)` | Calls `hwSetBaudrate`. The loopback accepts any value. |
| `unsigned long CanalGetVersion(void)` | The CANAL version of `canal.h`: major in the most significant byte, then minor and sub version. |
| `unsigned long CanalGetDllVersion(void)` | The driver version from `src/version.h`: year, month, patch (high byte), patch (low byte). |
| `const char *CanalGetVendorString(void)` | `DRIVER_VENDOR` in `canal-api.cpp`. |
| `const char *CanalGetDriverInfo(void)` | The XML in `src/driverinfo.h`. |

## CAN FD (optional, CANAL 1.1)

| Function | Template |
| :------- | :------- |
| `int CanalSendFD(long handle, PCANALMSGFD msg)` | Sends a classic or FD frame. FD frames need flag bit 1 at open (`CANAL_ERROR_FD_SUPPORT` otherwise) and a valid FD length. |
| `int CanalBlockingSendFD(long handle, PCANALMSGFD msg, unsigned long timeout)` | As `CanalSendFD`. |
| `int CanalReceiveFD(long handle, PCANALMSGFD msg)` | Next frame, classic or FD. |
| `int CanalBlockingReceiveFD(long handle, PCANALMSGFD msg, unsigned long timeout)` | Waits for a classic or FD frame. |

A host that does not know about CAN FD looks up only the standard functions
and gets only classic frames. A driver without CAN FD leaves the four
functions out.

## Error codes

The functions return the `CANAL_ERROR_...` codes from `canal.h`. The ones
the template uses:

| Code | When |
| :--- | :--- |
| `CANAL_ERROR_SUCCESS` (0) | OK |
| `CANAL_ERROR_NOT_OPEN` (33) | Unknown handle, or closed while waiting |
| `CANAL_ERROR_PARAMETER` (34) | Null pointer, FD flag in a classic call, invalid configuration |
| `CANAL_ERROR_FIFO_EMPTY` (8) | No frame (non-blocking receive) |
| `CANAL_ERROR_TIMEOUT` (32) | No frame within the time-out |
| `CANAL_ERROR_CAN_ID` (5) | Id too large for a standard or extended frame |
| `CANAL_ERROR_MTU` (39) | Too many data bytes, or not a valid CAN FD length |
| `CANAL_ERROR_FD_SUPPORT` (40) | FD frame on an interface not opened for FD |
| `CANAL_ERROR_TRM_FULL` (21) | The hardware can not take more frames now |
| `CANAL_ERROR_HARDWARE` (13) | The hardware could not be opened |
