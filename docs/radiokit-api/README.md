# RadioKit Remote Access API

The RadioKit Flutter companion app embeds a REST API for remote device control,
firmware code generation, and autonomous agent workflows. This directory archives
everything discovered from the running instance so no re-discovery is needed.

## Where the API lives (important!)

The API server runs **inside the RadioKit app on the Android phone**, not on a
development server:

- **Phone IP (current rig):** `10.0.0.6` (wlan0)
- **Port:** `7007` (listening on `0.0.0.0` inside the app process)
- **Server identity:** `GET /api/status` → `{"version":"1.0.0","port":7007,"localIp":"10.0.0.6","platform":"android","debug":true}`
- **Only reachable while the app process is running.**

### Reaching it from the dev machine

Direct `curl http://10.0.0.6:7007` **fails** (times out) — Android network
isolation blocks LAN inbound to the app port even though `ping` works. The
reliable path is an adb reverse tunnel:

```bash
adb forward tcp:17007 tcp:7007
curl http://127.0.0.1:17007/api/status          # works
curl http://127.0.0.1:17007/api/widgets         # etc.
```

If `adb devices` is empty, first: enable USB debugging + connect the phone.

## Files in this directory

| File | Content |
|------|---------|
| `README.md` | This operational index (reachability, workflows, cheat sheet) |
| `root.md` | Raw `GET /` documentation page served by the API |
| `api-schema.json` | Full endpoint schema (`GET /api/docs/api-schema`) |
| `skills/radiokit-remote.md` | Complete remote-control guide (connection, FS, OTA, widgets, agent patterns) |
| `skills/radiokit-firmware.md` | Firmware development guide |
| `skills/radiokit-widgets.md` | All widget types + value semantics |
| `skills/radiokit-transports.md` | BLE / Serial / WiFi / Cloud transports |
| `skills/radiokit-filesystem.md` | LittleFS operations guide |
| `skills/radiokit-ota.md` | OTA firmware update guide |

These mirror the server's own docs at `GET /api/docs/<skill>` — refresh them with:

```bash
adb forward tcp:17007 tcp:7007
for s in radiokit-firmware radiokit-widgets radiokit-transports \
         radiokit-filesystem radiokit-ota radiokit-remote; do
  curl -s http://127.0.0.1:17007/api/docs/$s > docs/radiokit-api/skills/$s.md
done
curl -s http://127.0.0.1:17007/api/docs/api-schema > docs/radiokit-api/api-schema.json
```

## Endpoint cheat sheet

| Operation | Method | Endpoint |
|-----------|--------|----------|
| Server status | GET | `/api/status` |
| BLE scan | POST | `/api/pair/scan` |
| Scan results | GET | `/api/pair/devices` |
| Connect | POST | `/api/connection/connect` |
| Connection state | GET | `/api/connection` |
| Disconnect | POST | `/api/connection/disconnect` |
| Reconnect | POST | `/api/connection/reconnect` |
| Switch transport | POST | `/api/connection/switch` |
| List widgets | GET | `/api/widgets` |
| Get widget | GET | `/api/widgets/<id>` |
| Set widget | PUT | `/api/widgets/<id>` |
| FS list | GET | `/api/fs/list?path=/` |
| FS info | GET | `/api/fs/info` |
| FS read | GET | `/api/fs/read?path=<file>` |
| FS write | POST | `/api/fs/write` |
| FS upload (chunked) | POST | `/api/fs/upload` |
| FS mkdir / delete / rename / format | POST | `/api/fs/mkdir` `/delete` `/rename` `/format` |
| OTA upload | POST | `/api/ota/upload` |
| OTA progress | GET | `/api/ota/progress` |
| Console log | GET / DELETE | `/api/console` |
| Multi-device | GET | `/api/devices` (+ per-device `/<id>/widgets`, `/<id>/fs/...`) |
| List designs | GET | `/api/designs` |
| Design JSON | GET | `/api/designs/<id>/json` |
| **Generate RADIOKIT.h** | GET | `/api/designs/<id>/header` (text/plain) |
| Save / delete design | POST / DELETE | `/api/designs` `/api/designs/<id>` |

## Standard workflow (drive the vehicle)

```bash
B=http://127.0.0.1:17007

# 1. scan for BLE devices, then list
curl -X POST $B/api/pair/scan
curl $B/api/pair/devices          # -> {"devices":[{"id":"<mac>","name":"RC_UI","rssi":..,"type":"ble"}]}

# 2. connect
curl -X POST $B/api/connection/connect -H 'Content-Type: application/json' \
  -d '{"id":"<mac>","type":"ble"}'
curl $B/api/connection            # poll until "connected":true, device.hasFs true

# 3. see widget ids/names
curl $B/api/widgets

# 4. set a widget value (values are int[]; semantics per widget type)
curl -X PUT $B/api/widgets/<id> -H 'Content-Type: application/json' -d '{"values":[75]}'
```

### Widget `values` semantics

| Widget type | `values` | Effect |
|-------------|----------|--------|
| Push button | `[1]` then `[0]` | Press then release |
| Toggle button | `[1]` / `[0]` | ON / OFF |
| Slider / knob / pedal | `[-100]` .. `[100]` | Position (pedals spring to min = released; knob springs to 0 = center) |
| Joystick | `[x, y]` | Both axes (-100..+100) |
| Multiple select (single) | `[index]` | Select item |
| Multiple select (multi) | `[bitmask]` | Toggle items |

## Error format

All endpoints return `{"error":"<code>","message":"..."}` on failure.
Common codes: `not_connected` (503-ish, nothing attached), `not_found`,
`invalid_params`, `connection_failed`, `ota_not_supported`, `ota_failed`.

## Notes / gotchas

- **BLE pairing from this rig:** the board advertises as the design's model name
  (RC_UI firmware sets `RadioKit.config.name`). Use the scan+connect flow above
  instead of UI-tapping the phone — it is deterministic.
- **Designs API** is how `src/RADIOKIT.h` was generated originally
  (`GET /api/designs/1785927365527/header`), and
  `docs/radiokit-rc-ui-design.json` is the archived design JSON.
- **Security:** LAN-only, no auth. Do not expose to the internet.
