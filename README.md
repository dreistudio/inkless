# Inkless Possibilities

A wireless thermal receipt printer powered by an ESP32 and a QR204 58mm thermal print mechanism. Print text, images, QR codes, and AI-generated receipts over WiFi from a browser or the command line.

Built for [Strebergarten](https://strebergarten.studio), our innovation studio. The AI modes are garden-themed because that's us. Yours will be different and uniquely yours. You won't get around cringey puns in this project...

**[Read the full build story →](https://strebergarten.studio/How-a-cheap-receipt-printer-can-print-kindness-and-tell-you-which-plant-you-are)**

## What it does

- **Web UI** at `http://printer.local` — type a message, hit print
- **AI modes** — generate prints using Claude (affirmations, haikus, personalized notes, end-of-day rituals)
- **CLI** (`rp` command) — print text, images, QR codes, notes, and todos from the terminal
- **HTTP API** — full ESC/POS control over WiFi

## Hardware

| Part | Notes |
|------|-------|
| ESP32 dev board | Any ESP32-WROOM-32 variant |
| QR204 58mm thermal printer | 384 dots/line, 203 DPI, ESC/POS, 5–9V ([datasheet](https://probots.co.in/technical_data/QR204%20-Thermal%20printer%20_Datasheet%20&%20User%20manual.pdf)) |
| 58mm thermal paper rolls | Standard receipt paper |
| 5V power supply | Shared or separate for printer; USB for ESP32 |

### Wiring

```
ESP32 GPIO17 (TX2) ──→ QR204 RX
ESP32 GND          ──→ QR204 GND
5V supply           ──→ QR204 VH (printer power)
```

The printer UART runs at **9600 baud** (8N1).

## Setup

> **Note:** This project was built for a specific hardware setup and hasn't been tested as a standalone kit yet. The instructions below describe the general flow, but you may need to adapt things. If you run into issues, open an issue (or more likely to get a quick response, ask your LLM-of-choice about it.)

### 1. Configure WiFi

```bash
cp firmware/include/secrets.h.example firmware/include/secrets.h
# Edit secrets.h with your WiFi credentials
```

### 2. Flash firmware

Requires [PlatformIO](https://platformio.org/install/cli). Connect the ESP32 via USB:

```bash
cd firmware && pio run -t upload
```

On success the printer prints a startup receipt with its IP address.

### 3. AI features (optional)

The AI modes require **inkless-server**, a lightweight proxy that holds your Anthropic API key:

```bash
cd inkless-server
cp .env.example .env
# Edit .env — add your ANTHROPIC_API_KEY
pip install -e .
python -m uvicorn server:app --host 0.0.0.0 --port 8100
```

Then add the server URL to `secrets.h`:

```cpp
#define INKLESS_SERVER_URL "http://YOUR_IP:8100"
```

Without inkless-server, all direct print features still work — only AI generation is unavailable.

### 4. CLI (optional)

```bash
cd cli && pip install -e .
rp status
```

## Project structure

```
firmware/          ESP32 firmware (PlatformIO, C++)
inkless-server/    AI proxy (Python, FastAPI)
cli/               Command-line tool (Python, Typer)
```

## Make it yours

| What | Where |
|------|-------|
| AI modes & prompts | `firmware/src/web_ui.h` |
| AI provider | `inkless-server/server.py` (swap Claude for OpenAI, Ollama, etc.) |
| Look & feel | `firmware/src/web_ui.h` (single HTML page, Tailwind CSS) |
| CLI print templates | `cli/src/receipt_printer/formatting.py` |

The ESC/POS driver, web server, and CLI are generic — they work with any ESC/POS thermal printer.

## Acknowledgments

- **[Project Scribe](https://github.com/UrbanCircles/scribe/tree/main)** by UrbanCircles (MIT). The concept of upside-down printing with reversed line order comes from Scribe (no code was directly copied). [3D model on MakerWorld.](https://makerworld.com/en/models/1577165-project-scribe#profileId-1670812)
- **[Scribe Evolution](https://github.com/Pharkie/scribe-evolution/releases/tag/v0.2.0)** by Adam Knowles.
- **[Josh Pham's receipt project](https://jpham.space/receipt)**, a friend's hack that got me inspired.
- **[Good Hang podcast](https://open.spotify.com/show/1z20EiwuKoDiftKxMVLde1)**, where "Talk Well Behind Your Back" feature came from.

## License

[CC BY-NC-SA 4.0](https://creativecommons.org/licenses/by-nc-sa/4.0/). See [LICENSE](LICENSE). Non-commercial use only.
