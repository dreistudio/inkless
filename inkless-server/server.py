"""inkless-server: AI proxy for the Inkless receipt printer.

Every slip leaves through the inkless gateway (the same door the `inkless` command and the
robot use), so its caps, hold switch and log apply to the AI modes too. This server never
calls the printer's own LAN API. It holds the Anthropic key; the gateway holds none.
"""

import json
import logging
import os
import time
from contextlib import asynccontextmanager
from datetime import datetime
from typing import Literal, Optional, Union

import anthropic
import httpx
import uvicorn
from dotenv import load_dotenv
from collections import deque
from fastapi import FastAPI, Request
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import HTMLResponse, JSONResponse, StreamingResponse
from pydantic import BaseModel

load_dotenv()

log = logging.getLogger("inkless-server")

ANTHROPIC_API_KEY = os.environ.get("ANTHROPIC_API_KEY", "")
GATEWAY_URL = os.environ.get("INKLESS_GATEWAY_URL", "http://127.0.0.1:8110").rstrip("/")
GATEWAY_TOKEN = os.environ.get("INKLESS_TOKEN", "").strip()
GATEWAY_HEADERS = {"Authorization": f"Bearer {GATEWAY_TOKEN}", "X-Inkless-From": "inkless-server"}
SERVER_PORT = int(os.environ.get("INKLESS_SERVER_PORT", "8100"))

# In-memory log of recent prints (newest first, capped at 50)
recent_prints: deque = deque(maxlen=50)

# /api/generate is called from visitors' browsers, so it must not be a general-purpose
# door to the API key: only the models the web UI uses, with its output ceilings, its one
# tool, a bounded request size, and a bounded number of calls per hour.
GENERATE_MODELS = {"claude-haiku-4-5-20251001": 500, "claude-sonnet-4-6": 1024}  # model → max_tokens ceiling
GENERATE_MAX_BODY = 20_000  # bytes
GENERATE_MAX_SEARCHES = 3
GENERATE_PER_HOUR = int(os.environ.get("GENERATE_PER_HOUR", "60"))
generate_calls: deque = deque()


def generate_refusal(raw: bytes, body: dict) -> Optional[JSONResponse]:
    """Why this request is not served, or None if it is within what the web UI does."""
    def refuse(code, message):
        return JSONResponse(status_code=code, content={"error": {"message": message}})
    if len(raw) > GENERATE_MAX_BODY:
        return refuse(413, "Request too large")
    if body.get("model", "claude-haiku-4-5-20251001") not in GENERATE_MODELS:
        return refuse(400, "Model not allowed")
    if not isinstance(body.get("max_tokens", 500), int):
        return refuse(400, "max_tokens must be a number")
    for tool in body.get("tools") or []:
        uses = tool.get("max_uses") if isinstance(tool, dict) else None
        if not (isinstance(uses, int) and uses <= GENERATE_MAX_SEARCHES
                and str(tool.get("type", "")).startswith("web_search_")):
            return refuse(400, "Tool not allowed")
    while generate_calls and time.time() - generate_calls[0] > 3600:
        generate_calls.popleft()
    if len(generate_calls) >= GENERATE_PER_HOUR:
        return refuse(429, "Too many requests this hour")
    generate_calls.append(time.time())
    return None

DASHBOARD_HTML = """<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>inkless</title>
<style>
  * { box-sizing: border-box; margin: 0; padding: 0; }
  body { font-family: monospace; background: #f5f5f0; color: #111; padding: 2rem; max-width: 640px; margin: 0 auto; }
  h1 { font-size: 1.2rem; margin-bottom: 0.25rem; }
  .sub { color: #777; font-size: 0.85rem; margin-bottom: 2rem; }
  .status { display: inline-flex; align-items: center; gap: 0.4rem; font-size: 0.85rem; margin-bottom: 2rem; }
  .dot { width: 8px; height: 8px; border-radius: 50%; background: #4caf50; }
  .dot.err { background: #f44336; }
  section { margin-bottom: 2rem; }
  h2 { font-size: 0.85rem; text-transform: uppercase; letter-spacing: 0.08em; color: #777; margin-bottom: 0.75rem; }
  textarea { width: 100%; height: 120px; font-family: monospace; font-size: 0.9rem; padding: 0.75rem; border: 1px solid #ccc; border-radius: 4px; resize: vertical; background: #fff; }
  .row { display: flex; gap: 1rem; align-items: center; margin-top: 0.5rem; }
  label { font-size: 0.85rem; display: flex; align-items: center; gap: 0.3rem; cursor: pointer; }
  button { padding: 0.5rem 1.25rem; font-family: monospace; font-size: 0.9rem; background: #111; color: #fff; border: none; border-radius: 4px; cursor: pointer; }
  button:hover { background: #333; }
  .msg { font-size: 0.85rem; margin-top: 0.5rem; min-height: 1.2em; }
  .msg.ok { color: #4caf50; }
  .msg.err { color: #f44336; }
  .log { list-style: none; }
  .log li { display: flex; gap: 1rem; padding: 0.5rem 0; border-bottom: 1px solid #e5e5e0; font-size: 0.85rem; }
  .log .time { color: #999; flex-shrink: 0; }
  .log .text { flex: 1; white-space: pre-wrap; word-break: break-word; }
  .log .badge { flex-shrink: 0; font-size: 0.75rem; padding: 0.1rem 0.4rem; border-radius: 3px; align-self: flex-start; }
  .badge.printed { background: #e8f5e9; color: #2e7d32; }
  .badge.error { background: #ffebee; color: #c62828; }
  .empty { color: #aaa; font-size: 0.85rem; }
</style>
</head>
<body>
<h1>inkless</h1>
<p class="sub">strebergarten.studio</p>

<div class="status" id="statusLine">
  <span class="dot" id="statusDot"></span>
  <span id="statusText">checking…</span>
</div>

<section>
  <h2>Print</h2>
  <textarea id="printText" placeholder="Type your receipt text here…"></textarea>
  <div class="row">
    <label><input type="checkbox" id="logo"> logo</label>
    <button onclick="sendPrint()">Print</button>
  </div>
  <p class="msg" id="printMsg"></p>
</section>

<section>
  <h2>Recent prints</h2>
  <ul class="log" id="log"><li class="empty">No prints yet this session.</li></ul>
</section>

<script>
async function checkStatus() {
  try {
    const r = await fetch('/status');
    const d = await r.json();
    document.getElementById('statusDot').className = 'dot';
    document.getElementById('statusDot').className = d.printer === 'online' ? 'dot' : 'dot err';
    document.getElementById('statusText').textContent = 'online — gateway ' + d.gateway + ' — printer ' + d.printer + (d.hold ? ' — ON HOLD' : '');
  } catch {
    document.getElementById('statusDot').className = 'dot err';
    document.getElementById('statusText').textContent = 'offline';
  }
}

async function loadLog() {
  try {
    const r = await fetch('/api/recent');
    const items = await r.json();
    const ul = document.getElementById('log');
    if (!items.length) { ul.innerHTML = '<li class="empty">No prints yet this session.</li>'; return; }
    const esc = t => String(t).replace(/[&<>"']/g, c => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
    ul.innerHTML = items.map(i =>
      `<li><span class="time">${esc(i.time)}</span><span class="text">${esc(i.text)}</span><span class="badge ${esc(i.status)}">${esc(i.status)}</span></li>`
    ).join('');
  } catch {}
}

async function sendPrint() {
  const text = document.getElementById('printText').value.trim();
  const logo = document.getElementById('logo').checked;
  const msg = document.getElementById('printMsg');
  if (!text) { msg.textContent = 'Enter some text first.'; msg.className = 'msg err'; return; }
  msg.textContent = 'Printing…'; msg.className = 'msg';
  try {
    const r = await fetch('/api/print', { method: 'POST', headers: {'Content-Type':'application/json'}, body: JSON.stringify({text, logo}) });
    if (r.ok) { msg.textContent = 'Printed.'; msg.className = 'msg ok'; loadLog(); }
    else { const d = await r.json(); msg.textContent = 'Error: ' + (d.error?.message || r.status); msg.className = 'msg err'; loadLog(); }
  } catch(e) { msg.textContent = 'Network error.'; msg.className = 'msg err'; }
}

checkStatus();
loadLog();
setInterval(loadLog, 5000);
</script>
</body>
</html>"""

# Reusable async client (connection pooling across requests)
ai_client = anthropic.AsyncAnthropic(api_key=ANTHROPIC_API_KEY) if ANTHROPIC_API_KEY else None


@asynccontextmanager
async def lifespan(app: FastAPI):
    if not ANTHROPIC_API_KEY:
        log.warning("ANTHROPIC_API_KEY not set")
    if not GATEWAY_TOKEN:
        log.warning("INKLESS_TOKEN not set - nothing will print")
    log.info("Gateway: %s", GATEWAY_URL)
    log.info("Ready on port %d", SERVER_PORT)
    yield


app = FastAPI(title="inkless-server", lifespan=lifespan)

# Allow requests from the ESP32 web UI (any origin on local network)
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)


@app.get("/", response_class=HTMLResponse)
async def index():
    return HTMLResponse(content=DASHBOARD_HTML)


@app.get("/api/recent")
async def get_recent():
    return list(recent_prints)


async def gateway_status() -> dict:
    """What the gateway says about itself and the printer; 'unreachable' if it does not answer."""
    try:
        async with httpx.AsyncClient() as client:
            r = await client.get(f"{GATEWAY_URL}/v1/status", headers=GATEWAY_HEADERS, timeout=15.0)
            r.raise_for_status()
            return r.json()
    except (httpx.HTTPError, ValueError) as e:
        return {"gateway": "unreachable", "printer": "unknown", "error": type(e).__name__}


@app.get("/status")
async def status():
    g = await gateway_status()
    return {
        "status": "online",
        "gateway": g.get("gateway", "unreachable"),
        "printer": g.get("printer", "unknown"),
        "hold": g.get("hold", False),
        "slips_left_this_hour": g.get("slips_left_this_hour"),
        "api_key_set": bool(ANTHROPIC_API_KEY),
    }


@app.get("/api/printer")
async def printer_status():
    """Is the printer reachable, as seen by the gateway? Tells 'server down' from 'printer unplugged'."""
    g = await gateway_status()
    if g.get("printer") == "online":
        return {"status": "online"}
    return JSONResponse(status_code=502, content={"error": {"message": f"Printer unavailable: gateway {g.get('gateway')}, printer {g.get('printer')}"}})


@app.post("/api/generate")
async def generate(request: Request):
    """Proxy AI requests to Anthropic. Supports both streaming and non-streaming."""
    if not ai_client:
        return JSONResponse(
            status_code=503,
            content={"error": {"message": "ANTHROPIC_API_KEY not configured"}},
        )

    raw = await request.body()
    try:
        body = json.loads(raw)
        assert isinstance(body, dict)
    except Exception:
        return JSONResponse(
            status_code=400,
            content={"error": {"message": "Invalid JSON body"}},
        )

    refusal = generate_refusal(raw, body)
    if refusal:
        return refusal

    model = body.get("model", "claude-haiku-4-5-20251001")
    max_tokens = min(int(body.get("max_tokens", 500)), GENERATE_MODELS[model])
    system = body.get("system", "")
    messages = body.get("messages", [])
    stream = body.get("stream", False)
    tools = body.get("tools", None)

    if stream:
        async def event_stream():
            try:
                async with ai_client.messages.stream(
                    model=model,
                    max_tokens=max_tokens,
                    system=system,
                    messages=messages,
                    tools=tools if tools is not None else anthropic.NOT_GIVEN,
                ) as stream_resp:
                    async for event in stream_resp:
                        yield f"data: {event.model_dump_json()}\n\n"
            except Exception as e:
                log.exception("Streaming error")
                yield f"data: {json.dumps({'type': 'error', 'error': {'message': str(e)}})}\n\n"

        return StreamingResponse(
            event_stream(),
            media_type="text/event-stream",
            headers={"Cache-Control": "no-cache", "X-Accel-Buffering": "no"},
        )
    else:
        try:
            response = await ai_client.messages.create(
                model=model,
                max_tokens=max_tokens,
                system=system,
                messages=messages,
                tools=tools if tools is not None else anthropic.NOT_GIVEN,
            )
            return response.model_dump()
        except anthropic.RateLimitError as e:
            return JSONResponse(status_code=429, content={"error": {"message": str(e)}})
        except anthropic.AuthenticationError as e:
            return JSONResponse(status_code=401, content={"error": {"message": str(e)}})
        except Exception as e:
            log.exception("Generate error")
            return JSONResponse(status_code=500, content={"error": {"message": str(e)}})


# --- Printing: one slip = one gateway call ---

class GatewayRefused(Exception):
    """The gateway answered, but said no (hold, hourly cap, too long, bad token)."""
    def __init__(self, status: int, message: str):
        super().__init__(message)
        self.status = status


async def print_slip(text: str, logo: Optional[str] = None, qr: Optional[str] = None) -> dict:
    """One slip through the gateway. It shapes the text for the paper, applies the caps and logs; raises on refusal or printer trouble."""
    body = {"text": text}
    if logo:
        body["logo"] = logo
    if qr:
        body["qr"] = qr
    async with httpx.AsyncClient() as client:
        r = await client.post(f"{GATEWAY_URL}/v1/print", json=body, headers=GATEWAY_HEADERS, timeout=90.0)
    if r.status_code == 200:
        return r.json()
    try:
        j = r.json()
        detail = j.get("detail") or (j.get("error") or {}).get("message") or r.text
    except ValueError:
        detail = r.text
    if r.status_code == 502:
        raise httpx.HTTPError(f"printer unreachable ({detail})")
    raise GatewayRefused(r.status_code, str(detail))


def _record(text: str, status: str):
    recent_prints.appendleft({"time": datetime.now().strftime("%H:%M"), "text": text, "status": status})


def _print_error(e: Exception, text: str) -> JSONResponse:
    _record(text, "error")
    if isinstance(e, GatewayRefused):
        log.warning("Gateway refused: %s %s", e.status, e)
        return JSONResponse(status_code=e.status, content={"error": {"message": f"Gateway refused: {e}"}, "text": text})
    log.exception("Gateway/printer communication error")
    return JSONResponse(status_code=502, content={"error": {"message": f"Printer unavailable: {e}"}, "text": text})


class PrintRequest(BaseModel):
    text: str
    logo: Union[Literal["drei", "strebergarten"], bool, None] = None  # true = strebergarten
    qr: Optional[str] = None


@app.post("/api/print")
async def print_receipt(req: PrintRequest):
    """Print text (with optional logo and QR) through the gateway. Used by the printer's web UI and automations."""
    logo = "strebergarten" if req.logo is True else (req.logo or None)
    try:
        await print_slip(req.text, logo=logo, qr=req.qr)
    except (httpx.HTTPError, GatewayRefused) as e:
        return _print_error(e, req.text)
    _record(req.text, "printed")
    return {"status": "printed"}


class DayEndRequest(BaseModel):
    progress: str
    debug: bool = False


DAYEND_SYSTEM = (
    "You are a receipt printer at Strebergarten.studio, a design and product "
    "studio. You print end-of-day shutdown receipts — a small ritual to mark "
    "the transition from work to rest. Maximum 32 characters per line. No "
    "markdown. Just the content, nothing else."
)

DAYEND_PROMPT = """Someone is about to leave the studio for the day. Here's what they said they made progress on:

"{progress}"

Write them a short, warm shutdown receipt. Format:

CLOSING TIME
strebergarten.studio
--------------------------------
Today you:
(restate what they did in 2-3
crisp bullet points, each
starting with a verb)
--------------------------------
(a short, poetic 2-3 line
reflection — something that
honors the work without being
cheesy. Think: grounding, not
motivational poster.)
--------------------------------
Tomorrow is another plot.
Leave this here. Go home.

Keep it to 32 chars per line max. Be warm, grounded, honest. No markdown."""


@app.post("/api/print/dayend")
async def print_dayend(req: DayEndRequest):
    """Generate and print an end-of-day shutdown receipt."""
    if not ai_client:
        return JSONResponse(status_code=503, content={"error": "API key not configured"})

    now = datetime.now()
    date_str = now.strftime("%A, %d %B %Y")
    time_str = now.strftime("%H:%M")

    prompt = DAYEND_PROMPT.format(progress=req.progress)

    try:
        response = await ai_client.messages.create(
            model="claude-haiku-4-5-20251001",
            max_tokens=500,
            system=DAYEND_SYSTEM,
            messages=[{"role": "user", "content": prompt}],
        )
    except anthropic.RateLimitError as e:
        return JSONResponse(status_code=429, content={"error": {"message": str(e)}})
    except anthropic.AuthenticationError as e:
        return JSONResponse(status_code=401, content={"error": {"message": str(e)}})
    except Exception as e:
        log.exception("Day end generation error")
        return JSONResponse(status_code=500, content={"error": {"message": str(e)}})

    if not response.content or not hasattr(response.content[0], "text"):
        return JSONResponse(status_code=500, content={"error": {"message": "No text in AI response"}})

    receipt_body = response.content[0].text
    header = f"{date_str}, {time_str}\n"
    full_receipt = header + receipt_body

    if not req.debug:
        try:
            await print_slip(full_receipt, logo="strebergarten")
        except (httpx.HTTPError, GatewayRefused) as e:
            return _print_error(e, full_receipt)
        _record(full_receipt, "printed")

    return {"status": "printed", "text": full_receipt}


def main():
    reload = os.environ.get("INKLESS_DEV", "").lower() in ("1", "true")
    uvicorn.run("server:app", host="0.0.0.0", port=SERVER_PORT, reload=reload)


if __name__ == "__main__":
    main()
