# inkless

Thermal receipt printer system for Strebergarten studio. The ESP32 drives a thermal printer; inkless-server is a FastAPI proxy that receives print requests and holds the Anthropic API key for AI-generated content.

## Architecture

```
ESP32 thermal printer (printer.local, firmware 1.1.0)
  └── serves the themed web UI (Your Plot, Harvest, If You Were a Plant…)
  └── the page asks inkless-server for AI text and sends every slip to inkless-server
      (hostname first, LAN-IP fallback for phones that cannot resolve .local)
  └── its own LAN API (/print/*, /update, OTA) is only used by the gateway and the debug panel

inkless-server (strebergarten-pi4:8100 — the studio Pi, always on; systemd unit inkless-server, user inklessai)
  └── POST /api/generate      — AI proxy for the web UI; bounded to its two models, 60 calls/hour
  └── POST /api/print         — text + optional logo/QR → forwarded to the gateway
  └── POST /api/print/dayend  — generate the shutdown receipt, print it through the gateway
  └── GET  /                  — dashboard with print log (in memory) + manual print form
  └── GET  /status            — gateway + printer + hold, as the gateway reports them
  └── holds the Anthropic key; never calls the printer directly

inkless-gateway (same Pi, :8110 on LAN + tailnet via `tailscale serve`) — the one door to the printer
  └── token on every request, 60 lines / 20 slips per hour, hold switch, log without slip text
  └── callers: the `inkless` command (any tailnet machine), inkless-server, the robot (studio LAN)
  └── source + runbook: ~/src/household-plugins/ops/inkless (private, never in this repo)

Luigi (robot) at github.com/dreistudio/reachy-host (private)
  └── prints through the gateway at the end of each visitor interaction
```

## Commands

- Flash OTA: `cd firmware && pio run -e ota -t upload` (readback: `curl printer.local/status` shows the new FW_VERSION)
- Flash USB: `cd firmware && pio run -t upload`
- PlatformIO project root is `firmware/`, not repo root
- `[env:esp32]` is pinned to `espressif32@6.13.0` (Arduino core 2.x). On core 3.x the web-server library
  asserts at boot (`tcp_alloc … Required to lock TCPIP core functionality`) and the board reboot-loops,
  printing garbage glyphs slip after slip — happened 2026-10-14 after an unpinned OTA; recovery was a USB flash.
- inkless-server runs on the Pi; deploy/update scripts in `~/src/household-plugins/ops/inkless/server/`.
  Local dev only: `cd inkless-server && INKLESS_GATEWAY_URL=<gateway> INKLESS_TOKEN=<token> ./start.sh`
- ESP32 points to inkless-server via `INKLESS_SERVER_URL` + `INKLESS_SERVER_URL_FALLBACK` in `firmware/include/secrets.h`

## Open Studio day startup

Nothing to start. The Pi runs the gateway and inkless-server around the clock.

1. **Printer (ESP32)** — power on; ~10 s to join WiFi. Verify at `http://printer.local` (the badge under the title says "printer ready").
2. **Luigi (robot)** — see `github.com/dreistudio/reachy-host`; it prints through the gateway.
3. If anything does not print: `inkless --status` (gateway / printer / hold / slips left), then `inkless --log`.

| What | URL |
|---|---|
| Themed experiences (Plot, Harvest…) | `http://printer.local` |
| Print log + manual print | `http://strebergarten-pi4.local:8100` |

## inkless-server config (/opt/inkless-server/.env on the Pi)

```
ANTHROPIC_API_KEY=...
INKLESS_TOKEN=...                      # the gateway's token
INKLESS_GATEWAY_URL=http://127.0.0.1:8110
INKLESS_SERVER_PORT=8100
GENERATE_PER_HOUR=60
```

## Prerequisites

- Install PlatformIO CLI or PlatformIO in VS Code so `pio` is available in your shell.
- Install Python if your PlatformIO setup does not already provide it.
- Ensure the USB serial driver for your ESP32 board is installed on your machine.
- Use the `firmware/` directory as the PlatformIO project root for all build and upload commands.
- For USB flashing, connect the board over USB and confirm the serial device is visible before uploading.
- For OTA flashing, make sure the board is already running firmware with OTA support and is reachable on the same network as your computer.

## Explanation

- `cd firmware && pio run -e ota -t upload`
	- Builds the firmware using the `ota` environment from `platformio.ini`.
	- Uploads over the network instead of over a USB cable.
	- Use this when the device is already deployed, powered on, and reachable via Wi‑Fi.

- `cd firmware && pio run -t upload`
	- Builds the default PlatformIO environment and uploads over the connected serial/USB port.
	- Use this for first-time flashing, recovery, or whenever OTA is unavailable.

In practice: USB is the safest path for initial setup and debugging. OTA is the convenient path once the device is already configured and online.

## Hardware/Setup

- Required hardware:
	- ESP32-based controller used by this receipt printer firmware.
	- Thermal receipt printer hardware wired to the configured UART pins.
	- USB cable for wired flashing and serial debugging.
	- Stable power for the board and printer.

- OTA setup requirements:
	- The device must already have a working firmware build that includes OTA upload support.
	- Wi‑Fi credentials in `firmware/include/secrets.h` must be valid.
	- Your computer and the device must be on the same local network unless your OTA setup explicitly supports routing across networks.
	- The device should be powered on, connected to Wi‑Fi, and discoverable/reachable by its configured network address.

## Troubleshooting

- Serial port problems:
	- Reconnect the USB cable and verify the board appears as a serial device.
	- Close any serial monitor or other tool that may already be using the port.
	- Reinstall or update the USB-to-serial driver if the port never appears.
	- If upload fails repeatedly, try a different USB cable or port.

- OTA upload problems:
	- Verify the board is on the same network and still reachable.
	- Check firewall settings if network uploads time out or cannot connect.
	- Confirm the OTA target configuration in `platformio.ini` matches the board's current IP/hostname.
	- If OTA is broken, fall back to USB flashing to restore a known-good build.

- PlatformIO build problems:
	- Run the command from `firmware/`, not the repository root.
	- Confirm PlatformIO is installed and `pio` is available on your PATH.
	- Rebuild after checking `platformio.ini`, board configuration, and secrets/config header changes.
	- If dependency or cache issues appear, clean the build artifacts and retry the upload.
