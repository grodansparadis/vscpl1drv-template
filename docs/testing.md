# Test and use

## The tests

```bash
ctest --test-dir build --output-on-failure
```

| Test | What it checks |
| :--- | :------------- |
| `driver` | `tests/test-driver.cpp` loads the built library with `dlopen` (`LoadLibrary` on Windows), finds every CANAL function by name and checks them: versions, driver information, invalid handles and configurations, loopback, standard and extended ids, filter/mask, statistics, status, CAN FD, the no-loopback flag, test frames, queue overruns, closing while another thread waits, and several open interfaces. |
| `update_version` | The version updater used by the *Update version* workflow. |
| `new_driver` | `tools/new_driver.py` makes a correctly renamed copy. |

The `driver` test is the important one: it uses the library exactly like a
host, so a missing or wrongly exported function fails it.

## Try it with vscp-cli

[vscp-cli](https://github.com/grodansparadis/vscp-cli) can use any CANAL
driver with `-i canal`:

```bash
# A test frame every 200 ms
vscp-dump -i canal --driver build/vscpl1drv-template.so --driver-config "200;0x123"

# With an id that is a VSCP event (CLASS1.MEASUREMENT, temperature, priority 3)
vscp-dump -i canal --driver build/vscpl1drv-template.so --driver-config "1000;0x0c0a0600"
```

## Use it in the VSCP daemon

In `/etc/vscp/vscpd.json`:

```json
"drivers": {
  "level1": [
    {
      "enable": true,
      "name": "loopback",
      "config": "1000;0x123",
      "flags": 0,
      "translation": 0,
      "path": "/var/lib/vscp/drivers/level1/vscpl1drv-template.so",
      "guid": "FF:FF:FF:FF:FF:FF:FF:F5:00:00:00:00:00:00:00:00"
    }
  ]
}
```

## Use it in VSCP Works

Add a new *CANAL* connection and select the driver file. VSCP Works reads the
driver information (`CanalGetDriverInfo`) and shows the configuration
string items and flags described there.

## Use it in the VSCP simulator

```bash
vscpsim -i canal -c "/var/lib/vscp/drivers/level1/vscpl1drv-template.so;1000"
```
