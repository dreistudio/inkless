#pragma once

const char WEB_UI_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Strebergarten</title>
<script src="https://cdn.tailwindcss.com"></script>
<style>
  .confetti { position: fixed; pointer-events: none; z-index: 50; }
  .confetti span {
    position: absolute; display: block; width: 8px; height: 8px;
    animation: fall 2s ease-out forwards;
  }
  @keyframes fall {
    0%   { transform: translateY(0) rotate(0deg); opacity: 1; }
    100% { transform: translateY(100vh) rotate(720deg); opacity: 0; }
  }
  .slide { display: none; }
  .slide.active { display: block; }
  .arrow-btn {
    position: absolute; top: 50%; transform: translateY(-50%);
    width: 44px; height: 44px; border-radius: 50%;
    display: flex; align-items: center; justify-content: center;
    background: rgba(0,0,0,0.06); color: #555; font-size: 20px;
    border: none; cursor: pointer; transition: background 0.2s;
    -webkit-tap-highlight-color: transparent;
  }
  .arrow-btn:hover { background: rgba(0,0,0,0.12); }
  .mode-icon {
    width: 48px; height: 48px; border-radius: 50%;
    display: flex; align-items: center; justify-content: center;
    font-size: 22px; cursor: pointer; transition: all 0.2s;
    border: 3px solid transparent; opacity: 0.5;
    -webkit-tap-highlight-color: transparent;
  }
  .mode-icon.active { opacity: 1; border-color: currentColor; transform: scale(1.1); }
  .mode-icon:hover { opacity: 0.8; }
  .history-item {
    border-bottom: 1px solid #e5e7eb; padding: 12px 0; cursor: pointer;
  }
  .history-item:last-child { border-bottom: none; }
  .history-item:hover { background: #f9fafb; }
  .history-content { display: none; }
  .history-item.open .history-content { display: block; }
</style>
</head>
<body class="bg-gray-100 min-h-screen flex items-center justify-center p-4">
<div class="w-full max-w-md">

  <!-- Mode icons -->
  <div class="flex justify-center items-center gap-3 mb-4">
    <div class="mode-icon active bg-teal-100 text-teal-700" onclick="goTo(0)" title="Your Plot">&#127793;</div>
    <div class="mode-icon bg-amber-100 text-amber-700" onclick="goTo(1)" title="Harvest">&#127805;</div>
    <div class="mode-icon bg-pink-100 text-pink-700" onclick="goTo(2)" title="Talk Well Behind My Back">&#128144;</div>
    <div class="mode-icon bg-emerald-100 text-emerald-700" onclick="goTo(3)" title="If You Were a Plant">&#129716;</div>
    <div class="w-px h-8 bg-gray-300 mx-1"></div>
    <div class="mode-icon bg-violet-100 text-violet-700" onclick="goTo(4)" title="Day End">&#127769;</div>
    <div class="mode-icon bg-gray-200 text-gray-700" onclick="goTo(5)" title="Print">&#9998;</div>
    <div class="mode-icon bg-red-100 text-red-700" onclick="goTo(6)" title="Debug">&#128295;</div>
  </div>

  <div class="relative">
    <button class="arrow-btn" style="left: -56px;" onclick="nav(-1)">&larr;</button>
    <button class="arrow-btn" style="right: -56px;" onclick="nav(1)">&rarr;</button>

  <div class="bg-white rounded-2xl shadow-lg p-6">

    <div id="status" class="mb-4 text-center text-sm text-gray-500 hidden"></div>
    <div id="serverBadge" class="mb-2 text-center text-xs text-gray-400">checking printer...</div>

    <div style="min-height: 280px;">

      <!-- Slide 0: Your Plot -->
      <div class="slide active" data-slide="0">
        <h2 class="text-lg font-semibold text-teal-700 mb-1 text-center">Your Plot</h2>
        <pre class="text-center text-teal-600 text-xs leading-tight mb-3 select-none">  .  *  .     .  '  .     .  .  *
 [_____]     [_____]     [_____]
 |/////|     |/////|     |/////|</pre>
        <p class="text-gray-500 text-sm text-center mb-3">What brought you here today?</p>
        <input id="plotInput" type="text" placeholder="e.g. I need help shaping a product idea..."
          class="w-full border border-gray-300 rounded-lg p-3 text-gray-800 focus:ring-2 focus:ring-teal-400 focus:border-transparent mb-3">
        <button onclick="generatePlot()"
          class="gen-btn w-full bg-teal-100 text-teal-700 py-3 rounded-lg font-medium hover:bg-teal-200 transition-colors">
          Plan My Plot
        </button>
      </div>

      <!-- Slide 1: Harvest -->
      <div class="slide" data-slide="1">
        <h2 class="text-lg font-semibold text-amber-700 mb-1 text-center">Harvest</h2>
        <pre class="text-center text-amber-600 text-xs leading-tight mb-2 select-none">   \  |  /
    \ | /
  ---***---
    / | \
   /  |  \
   \_____/
   |     |
   |~~~~~|
   |_____|</pre>
        <p class="text-gray-500 text-sm text-center mb-3">What are you taking with you?</p>
        <input id="harvestInput" type="text" placeholder="e.g. a clear next step, a new perspective on..."
          class="w-full border border-gray-300 rounded-lg p-3 text-gray-800 focus:ring-2 focus:ring-amber-400 focus:border-transparent mb-3">
        <button onclick="generateHarvest()"
          class="gen-btn w-full bg-amber-100 text-amber-700 py-3 rounded-lg font-medium hover:bg-amber-200 transition-colors">
          Print My Harvest
        </button>
      </div>

      <!-- Slide 2: Talk Well Behind My Back -->
      <div class="slide" data-slide="2">
        <h2 class="text-lg font-semibold text-pink-700 mb-1 text-center">Talk Well Behind My Back</h2>
        <pre class="text-center text-pink-600 text-xs leading-tight mb-2 select-none">      *  *  *
    *  * * *  *
     *  * *  *
      * * * *
      '\|//'
        ||
        ||
       /  \
      '----'</pre>
        <p class="text-gray-500 text-sm text-center mb-3">A bouquet of praise for someone</p>
        <input id="behindBackName" type="text" placeholder="Name + what they do..."
          class="w-full border border-gray-300 rounded-lg p-3 text-gray-800 focus:ring-2 focus:ring-pink-400 focus:border-transparent mb-3">
        <button onclick="talkWellBehindBack()"
          class="gen-btn w-full bg-pink-100 text-pink-700 py-3 rounded-lg font-medium hover:bg-pink-200 transition-colors">
          Pick Their Flowers
        </button>
      </div>

      <!-- Slide 3: If you were a plant -->
      <div class="slide" data-slide="3">
        <h2 class="text-lg font-semibold text-emerald-700 mb-1 text-center">If You Were a Plant</h2>
        <pre class="text-center text-emerald-600 text-xs leading-tight mb-2 select-none">     v/v\v
      \___/
        V __
        |/_/
        |'
   .--''!''--.
  |'--._____.--'|
   \           /
    \         /
     '-...-'</pre>
        <p class="text-gray-500 text-sm text-center mb-3">Find out which plant matches you</p>
        <input id="plantName" type="text" placeholder="Name + what they do..."
          class="w-full border border-gray-300 rounded-lg p-3 text-gray-800 focus:ring-2 focus:ring-emerald-400 focus:border-transparent mb-3">
        <button onclick="ifYouWereAPlant()"
          class="gen-btn w-full bg-emerald-100 text-emerald-700 py-3 rounded-lg font-medium hover:bg-emerald-200 transition-colors">
          Find My Plant
        </button>
      </div>

      <!-- Slide 4: Day End -->
      <div class="slide" data-slide="4">
        <h2 class="text-lg font-semibold text-violet-700 mb-1 text-center">Day End</h2>
        <pre class="text-center text-violet-600 text-xs leading-tight mb-2 select-none">    *  .  *  .  *
   . ` . ` . ` . `
  ~~~~~~~~~~~~~~~~
      \_______/
       |     |
       |  ~  |
       |_____|</pre>
        <p class="text-gray-500 text-sm text-center mb-3">What did you make progress on today?</p>
        <input id="dayEndInput" type="text" placeholder="e.g. finalized the pitch deck, had a great brainstorm..."
          class="w-full border border-gray-300 rounded-lg p-3 text-gray-800 focus:ring-2 focus:ring-violet-400 focus:border-transparent mb-3">
        <button onclick="generateDayEnd()"
          class="gen-btn w-full bg-violet-100 text-violet-700 py-3 rounded-lg font-medium hover:bg-violet-200 transition-colors">
          Print Shutdown Receipt
        </button>
      </div>

      <!-- Slide 5: Free print -->
      <div class="slide" data-slide="5">
        <h2 class="text-lg font-semibold text-gray-800 mb-1 text-center">Print</h2>
        <p class="text-gray-500 text-sm text-center mb-4">Type anything</p>
        <form id="printForm">
          <textarea id="message" name="message" rows="3" maxlength="200"
            class="w-full border border-gray-300 rounded-lg p-3 text-gray-800 focus:ring-2 focus:ring-gray-400 focus:border-transparent resize-none"
            placeholder="What do you want to print?"></textarea>
          <div class="text-right text-xs text-gray-400 mt-1 mb-3">
            <span id="charCount">0</span>/200
          </div>
          <button type="submit" id="submitBtn"
            class="gen-btn w-full bg-gray-800 text-white py-3 rounded-lg font-medium hover:bg-gray-700 transition-colors disabled:opacity-50">
            Print
          </button>
        </form>
        <div class="flex gap-2 mt-3">
          <button onclick="doAction('/feed')"
            class="flex-1 bg-gray-200 text-gray-700 py-2 rounded-lg text-sm hover:bg-gray-300 transition-colors">Feed</button>
          <button onclick="doAction('/cut')"
            class="flex-1 bg-gray-200 text-gray-700 py-2 rounded-lg text-sm hover:bg-gray-300 transition-colors">Cut</button>
        </div>
      </div>

      <!-- Slide 6: Debug -->
      <div class="slide" data-slide="6">
        <h2 class="text-lg font-semibold text-red-700 mb-1 text-center">Debug</h2>
        <p class="text-gray-500 text-sm text-center mb-3">Test individual printer functions</p>

        <label class="flex items-center gap-2 mb-3 cursor-pointer select-none">
          <input type="checkbox" id="globalDebug" class="cursor-pointer">
          <span class="text-sm text-gray-600 font-medium">Debug mode (preview only, don't print)</span>
        </label>

        <div class="grid grid-cols-3 gap-2 mb-3">
          <button onclick="debugTest('/print/test', 'Test Pat')"
            class="bg-red-50 text-red-700 py-2 rounded-lg text-xs font-medium hover:bg-red-100 transition-colors">Test Pat</button>
          <button onclick="debugTest('/print/logo/raster', 'Logo Row')"
            class="bg-red-50 text-red-700 py-2 rounded-lg text-xs font-medium hover:bg-red-100 transition-colors">Logo (Row)</button>
          <button onclick="debugTest('/print/logo/inverted', 'Logo Inv')"
            class="bg-red-50 text-red-700 py-2 rounded-lg text-xs font-medium hover:bg-red-100 transition-colors">Logo (Inv)</button>
          <button onclick="debugTestQR()"
            class="bg-red-50 text-red-700 py-2 rounded-lg text-xs font-medium hover:bg-red-100 transition-colors">QR Code</button>
          <button onclick="debugTestText()"
            class="bg-red-50 text-red-700 py-2 rounded-lg text-xs font-medium hover:bg-red-100 transition-colors">Text</button>
          <button onclick="debugTestSpacing()"
            class="bg-red-50 text-red-700 py-2 rounded-lg text-xs font-medium hover:bg-red-100 transition-colors">Spacing</button>
          <button onclick="debugTestWeight()"
            class="bg-red-50 text-red-700 py-2 rounded-lg text-xs font-medium hover:bg-red-100 transition-colors">Weight</button>
          <button onclick="debugTest('/feed', 'Feed')"
            class="bg-gray-100 text-gray-700 py-2 rounded-lg text-xs font-medium hover:bg-gray-200 transition-colors">Feed</button>
          <button onclick="debugTest('/cut', 'Cut')"
            class="bg-gray-100 text-gray-700 py-2 rounded-lg text-xs font-medium hover:bg-gray-200 transition-colors">Cut</button>
          <button onclick="clearDebugLog()"
            class="bg-gray-100 text-gray-700 py-2 rounded-lg text-xs font-medium hover:bg-gray-200 transition-colors">Clear Log</button>
        </div>

        <div class="flex items-center justify-between mb-1">
          <h3 class="text-xs font-semibold text-gray-500 uppercase tracking-wide">Request Log</h3>
          <span id="debugLogCount" class="text-xs text-gray-400">0 entries</span>
        </div>
        <div id="debugLog" class="border border-gray-200 rounded-lg bg-gray-50 max-h-64 overflow-y-auto font-mono text-xs"></div>

        <div class="mt-3">
          <div class="flex items-center justify-between mb-1">
            <button onclick="toggleHistory()" class="text-xs text-gray-500 hover:text-gray-700 underline">History</button>
            <button onclick="exportHistory()" class="text-xs text-gray-400 hover:text-gray-600">Export JSON</button>
          </div>
          <div id="historyPanel" class="hidden">
            <div class="flex items-center justify-between mb-1">
              <span id="historyCount" class="text-xs text-gray-400"></span>
              <button onclick="clearHistory()" class="text-xs text-red-400 hover:text-red-600">Clear all</button>
            </div>
            <div id="historyList" class="max-h-48 overflow-y-auto border border-gray-200 rounded-lg bg-gray-50"></div>
          </div>
        </div>
      </div>

    </div>

    <!-- Debug preview (only shown in debug mode) -->
    <pre id="debugPreview" class="hidden mt-3 p-3 bg-gray-50 border border-gray-200 rounded-lg text-xs text-gray-700 whitespace-pre-wrap font-mono max-h-60 overflow-y-auto"></pre>

  </div>
  </div>
</div>

<script>
const statusEl = document.getElementById('status');
const slides = document.querySelectorAll('.slide');
const icons = document.querySelectorAll('.mode-icon');
let current = 0;

function goTo(idx) {
  if (!Number.isInteger(idx) || idx < 0 || idx >= slides.length || idx >= icons.length) return;
  if (current < 0 || current >= slides.length || current >= icons.length) return;
  slides[current].classList.remove('active');
  icons[current].classList.remove('active');
  current = idx;
  slides[current].classList.add('active');
  icons[current].classList.add('active');
  const input = slides[current].querySelector('input, textarea');
  if (input) input.focus();
}

function nav(dir) {
  goTo((current + dir + slides.length) % slides.length);
}

document.addEventListener('keydown', (e) => {
  if (e.target.tagName === 'TEXTAREA') return;
  if (e.key === 'ArrowLeft') nav(-1);
  if (e.key === 'ArrowRight') nav(1);
});

// --- Free print ---
const form = document.getElementById('printForm');
const msg = document.getElementById('message');
const charCount = document.getElementById('charCount');
const submitBtn = document.getElementById('submitBtn');

msg.addEventListener('input', () => charCount.textContent = msg.value.length);
msg.addEventListener('keydown', (e) => {
  if (e.key === 'Enter' && !e.shiftKey) { e.preventDefault(); form.dispatchEvent(new Event('submit')); }
});

form.addEventListener('submit', async (e) => {
  e.preventDefault();
  const text = msg.value.trim();
  if (!text) return;
  submitBtn.disabled = true;
  submitBtn.textContent = isDebug() ? 'Previewing...' : 'Printing...';
  hidePreview();
  try {
    if (await printOrPreview(text)) {
      saveToHistory('Print', {message: text}, text);
      if (!isDebug()) showStatus('Printed!', 'text-green-600');
      confetti(); msg.value = ''; charCount.textContent = '0';
    }
    else showStatus('Print failed', 'text-red-500');
  } catch (err) { showStatus('Connection error', 'text-red-500'); }
  submitBtn.disabled = false;
  submitBtn.textContent = 'Print';
});

async function doAction(path) {
  try { await fetch(path, {method: 'POST'}); showStatus('Done', 'text-green-600'); }
  catch (err) { showStatus('Error', 'text-red-500'); }
}

// --- Status + confetti ---
function showStatus(text, cls, autoHide) {
  statusEl.textContent = text;
  statusEl.className = 'mb-4 text-center text-sm ' + cls;
  statusEl.classList.remove('hidden');
  if (autoHide !== false) setTimeout(() => statusEl.classList.add('hidden'), 2000);
}

function confetti() {
  const container = document.createElement('div');
  container.className = 'confetti';
  document.body.appendChild(container);
  const colors = ['#f44336','#e91e63','#9c27b0','#3f51b5','#2196f3','#4caf50','#ff9800'];
  for (let i = 0; i < 40; i++) {
    const span = document.createElement('span');
    span.style.left = Math.random() * 100 + 'vw';
    span.style.background = colors[Math.floor(Math.random() * colors.length)];
    span.style.animationDelay = Math.random() * 0.5 + 's';
    span.style.width = (4 + Math.random() * 6) + 'px';
    span.style.height = span.style.width;
    span.style.borderRadius = Math.random() > 0.5 ? '50%' : '0';
    container.appendChild(span);
  }
  setTimeout(() => container.remove(), 2500);
}

// --- Debug mode ---
function isDebug() { return document.getElementById('globalDebug').checked; }

function showPreview(text) {
  const el = document.getElementById('debugPreview');
  el.textContent = text;
  el.classList.remove('hidden');
}

function hidePreview() {
  document.getElementById('debugPreview').classList.add('hidden');
}

function sanitize(text) {
  return text
    // Smart quotes → straight
    .replace(/[\u2018\u2019\u201A\u2032]/g, "'")
    .replace(/[\u201C\u201D\u201E\u2033]/g, '"')
    // Dashes
    .replace(/[\u2013\u2014]/g, '-')
    // Ellipsis
    .replace(/\u2026/g, '...')
    // Spaces
    .replace(/[\u00A0\u2002\u2003\u2009]/g, ' ')
    // Bullets
    .replace(/\u2022/g, '-')
    // Accented vowels
    .replace(/[\u00C0-\u00C5]/g, 'A').replace(/[\u00E0-\u00E5]/g, 'a')
    .replace(/[\u00C8-\u00CB]/g, 'E').replace(/[\u00E8-\u00EB]/g, 'e')
    .replace(/[\u00CC-\u00CF]/g, 'I').replace(/[\u00EC-\u00EF]/g, 'i')
    .replace(/[\u00D2-\u00D6]/g, 'O').replace(/[\u00F2-\u00F6]/g, 'o')
    .replace(/[\u00D9-\u00DC]/g, 'U').replace(/[\u00F9-\u00FC]/g, 'u')
    .replace(/[\u00D1]/g, 'N').replace(/[\u00F1]/g, 'n')
    .replace(/[\u00C7]/g, 'C').replace(/[\u00E7]/g, 'c')
    .replace(/\u00DF/g, 'ss')
    .replace(/[\u00C6]/g, 'AE').replace(/[\u00E6]/g, 'ae')
    .replace(/[\u00D8]/g, 'O').replace(/[\u00F8]/g, 'o')
    // Strip any remaining non-ASCII
    .replace(/[^\x00-\x7F]/g, '');
}

// Every slip goes through inkless-server -> the gateway (caps, hold, log), never straight to
// the printer. logo: true prints the Strebergarten logo above the text; qr: a link below it.
async function printOrPreview(text, logo, qr) {
  if (isDebug()) {
    showPreview(text);
    showStatus('Preview (not printed)', 'text-amber-600');
    return true;
  }
  const res = await fetch(getServerUrl() + '/api/print', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ text: sanitize(text), logo: logo ? 'strebergarten' : null, qr: qr || null })
  });
  if (!res.ok) {
    const err = await res.json().catch(() => ({}));
    lastPrintError = err.error?.message || ('print failed ' + res.status);
  }
  return res.ok;
}
let lastPrintError = '';
function printFailed() { showStatus('Print failed: ' + (lastPrintError || 'unknown'), 'text-red-500'); }

// --- History (localStorage) ---
function getHistory() {
  try { return JSON.parse(localStorage.getItem('strebergarten_history') || '[]'); }
  catch { return []; }
}

function saveToHistory(mode, inputs, output) {
  const history = getHistory();
  history.unshift({
    mode,
    inputs,
    output,
    timestamp: new Date().toISOString()
  });
  // Keep last 200 entries
  if (history.length > 200) history.length = 200;
  localStorage.setItem('strebergarten_history', JSON.stringify(history));
}

function toggleHistory() {
  const panel = document.getElementById('historyPanel');
  if (panel.classList.contains('hidden')) {
    panel.classList.remove('hidden');
    renderHistory();
  } else {
    panel.classList.add('hidden');
  }
}

function escapeHtml(str) {
  return String(str)
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;')
    .replace(/'/g, '&#39;')
    .replace(/\//g, '&#x2F;');
}

function renderHistory() {
  const list = document.getElementById('historyList');
  const history = getHistory();
  if (!history.length) {
    list.innerHTML = '<p class="text-xs text-gray-400 p-3 text-center">No receipts yet</p>';
    return;
  }
  list.innerHTML = history.map((item, i) => {
    const date = new Date(item.timestamp);
    const dateStr = date.toLocaleDateString('de-DE', {day:'numeric',month:'short'}) + ' ' + date.toLocaleTimeString('de-DE', {hour:'2-digit',minute:'2-digit'});
    const inputSummary = typeof item.inputs === 'string' ? item.inputs : Object.values(item.inputs).filter(Boolean).join(', ');
    return '<div class="history-item px-3" onclick="this.classList.toggle(\'open\')">' +
      '<div class="flex justify-between items-center">' +
        '<span class="text-xs font-medium text-gray-700">' + escapeHtml(item.mode) + '</span>' +
        '<span class="text-xs text-gray-400">' + dateStr + '</span>' +
      '</div>' +
      '<p class="text-xs text-gray-500 truncate mt-0.5">' + escapeHtml(inputSummary.substring(0, 60)) + '</p>' +
      '<pre class="history-content mt-2 p-2 bg-white rounded text-xs text-gray-600 whitespace-pre-wrap font-mono">' + escapeHtml(item.output) + '</pre>' +
      '<button class="history-content mt-1 w-full text-xs bg-gray-200 text-gray-700 py-1.5 rounded-lg hover:bg-gray-300" onclick="event.stopPropagation();reprintHistory(' + i + ')">Reprint</button>' +
    '</div>';
  }).join('');
}

async function reprintHistory(index) {
  const history = getHistory();
  if (!history[index]) return;
  const text = history[index].output;
  try {
    if (await printOrPreview(text, true)) showStatus('Reprinted!', 'text-green-600');
    else showStatus('Reprint failed: ' + lastPrintError, 'text-red-500');
  } catch (err) { showStatus('Reprint failed: ' + err.message, 'text-red-500'); }
}

function clearHistory() {
  if (!confirm('Clear all history?')) return;
  localStorage.removeItem('strebergarten_history');
  renderHistory();
}

function exportHistory() {
  const data = localStorage.getItem('strebergarten_history') || '[]';
  const blob = new Blob([data], {type: 'application/json'});
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = 'strebergarten-history-' + new Date().toISOString().slice(0,10) + '.json';
  a.click();
  URL.revokeObjectURL(url);
}

// --- inkless-server (AI + printing, on the studio Pi) ---
// The firmware hands us a hostname (.local) and an IP fallback: phones that cannot resolve
// mDNS use the IP. Whichever answers /status first is used for the rest of the visit.
let serverUrl = null;
async function probeServer() {
  const candidates = [window.INKLESS_SERVER, window.INKLESS_SERVER_FALLBACK].filter(Boolean);
  for (const url of candidates) {
    try {
      const ctrl = new AbortController(); const t = setTimeout(() => ctrl.abort(), 4000);
      const r = await fetch(url + '/status', {signal: ctrl.signal}); clearTimeout(t);
      if (r.ok) { serverUrl = url; const d = await r.json().catch(() => ({})); updateServerBadge(d); return; }
    } catch (e) { /* try the next */ }
  }
  updateServerBadge(null);
}
function updateServerBadge(d) {
  const el = document.getElementById('serverBadge'); if (!el) return;
  if (!d) { el.textContent = 'inkless-server not reachable'; el.className = 'text-xs text-red-500'; return; }
  el.textContent = d.printer === 'online' ? (d.hold ? 'printer on hold' : 'printer ready') : 'printer offline';
  el.className = 'text-xs ' + (d.printer === 'online' && !d.hold ? 'text-green-600' : 'text-red-500');
}
probeServer();
function getServerUrl() {
  if (serverUrl) return serverUrl;
  if (!window.INKLESS_SERVER) throw new Error('inkless-server not configured');
  return window.INKLESS_SERVER;  // not probed yet: try the hostname and let the error speak
}

// --- Simple generate (Haiku) ---
async function generate(prompt, mode, inputs, printLogoFirst) {
  if (!prompt || !prompt.trim()) return;
  const genBtns = document.querySelectorAll('.gen-btn');
  genBtns.forEach(b => b.disabled = true);
  showStatus('Generating...', 'text-teal-600', false);
  hidePreview();
  try {
    const res = await fetch(getServerUrl() + '/api/generate', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        model: 'claude-haiku-4-5-20251001',
        max_tokens: 500,
        system: 'You are a receipt printer at Strebergarten.studio, a design and product studio that helps people grow their ideas. Generate short content suitable for a 58mm thermal receipt. Maximum 32 characters per line. No markdown formatting. Just the content, nothing else.',
        messages: [{ role: 'user', content: prompt.trim() }]
      })
    });
    if (!res.ok) { const err = await res.json().catch(() => ({})); throw new Error(err.error?.message || 'API error ' + res.status); }
    const data = await res.json();
    const text = data.content[0].text;
    if (await printOrPreview(text, printLogoFirst)) {
      saveToHistory(mode || 'Generate', inputs || {prompt}, text);
      if (!isDebug()) showStatus('Printed!', 'text-green-600');
      confetti();
    }
    else printFailed();
  } catch (err) {
    if (err.message === 'inkless-server not configured' || err.message === 'Failed to fetch')
      showStatus('AI unavailable - inkless-server not reachable', 'text-red-500');
    else showStatus(err.message || 'Generation failed', 'text-red-500');
  }
  genBtns.forEach(b => b.disabled = false);
}

// --- Your Plot ---
function generatePlot() {
  const input = document.getElementById('plotInput');
  const text = input.value.trim();
  if (!text) return;
  input.value = '';

  let prompt = 'Someone just arrived at Strebergarten.studio (a design & product studio by DREi Studio) for a meeting. They said:\n\n"' + text + '"\n\n';
  prompt += 'Write them a plot plan receipt that sharpens what they said. This receipt is a physical anchor for their meeting — they\'ll hold onto it and glance at it to stay on track. Don\'t give advice — be a mirror that makes their thinking crisper. Read between the lines. Format:\n\n';
  prompt += 'PLOT #(random 3-digit number)\n';
  prompt += 'STREBERGARTEN.STUDIO\n';
  prompt += '---\n';
  prompt += 'Hold onto this.\n';
  prompt += 'Glance at it mid-meeting\n';
  prompt += 'to stay on track.\n';
  prompt += '---\n';
  prompt += 'Seed: (restate their goal more\n  crisply — what are they really\n  asking for?)\n';
  prompt += 'Water: (name the kind of help\n  that would actually move this\n  forward — be specific)\n';
  prompt += 'Harvest check: (a concrete\n  thing they can point to when\n  leaving to know it worked)\n';
  prompt += '---\n';
  prompt += 'If anything feels vague, gently challenge it in a short P.S. (e.g. "P.S. \'feedback\' on what exactly?")\n\n';
  prompt += 'Keep it to 32 chars per line max. Be warm but sharp. No markdown.';

  generate(prompt, 'Your Plot', {input: text}, true);
}

// --- Harvest ---
function getSeason() {
  const m = new Date().getMonth();
  if (m >= 2 && m <= 4) return 'Spring';
  if (m >= 5 && m <= 7) return 'Summer';
  if (m >= 8 && m <= 10) return 'Autumn';
  return 'Winter';
}

async function generateHarvest() {
  const input = document.getElementById('harvestInput');
  const text = input.value.trim();
  if (!text) return;

  const genBtns = document.querySelectorAll('.gen-btn');
  genBtns.forEach(b => b.disabled = true);
  showStatus('Cooking up your recipe...', 'text-amber-600', false);
  hidePreview();

  const now = new Date();
  const date = now.toLocaleDateString('en-GB', {day: 'numeric', month: 'long', year: 'numeric'});
  const time = now.toLocaleTimeString('en-GB', {hour: '2-digit', minute: '2-digit'});
  const season = getSeason();
  const year = now.getFullYear();

  // Build the header + harvest
  let header = '';
  header += 'strebergarten by DREi Studio\n';
  header += date + ', ' + time + '\n';
  header += 'Season: ' + season + ' ' + year + '\n';
  header += '--------------------------------\n';
  header += 'Your harvest:\n';
  // Format as bulleted list (split on commas, newlines, or treat as single item)
  const items = text.split(/[,\n]+/).map(s => s.trim()).filter(Boolean);
  items.forEach(item => { header += '- ' + item + '\n'; });
  header += '--------------------------------\n';

  // Ask AI for the recipe
  let prompt = 'Someone is leaving Strebergarten.studio (a design & product studio by DREi Studio) after a visit. Here is what they harvested today:\n\n';
  prompt += '"' + text + '"\n\n';
  prompt += 'These are their ingredients. Write a short RECIPE for what to cook with them — a playful, concrete suggestion for what to do next with what they learned/gained. Use the cooking metaphor:\n\n';
  prompt += 'Recipe: (a fun name for the dish)\n';
  prompt += 'Ingredients: (reword their\n  takeaways as ingredients)\n';
  prompt += 'Method: (2-3 concrete steps\n  for what to do with these\n  in the next week)\n';
  prompt += 'Serves: (who benefits)\n';
  prompt += 'Best before: (a playful\n  expiry nudge)\n\n';
  prompt += 'Keep it to 32 chars per line max. Be warm, playful, concrete. No markdown. Just the recipe, nothing else.';

  try {
    const res = await fetch(getServerUrl() + '/api/generate', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        model: 'claude-haiku-4-5-20251001',
        max_tokens: 500,
        system: 'You are a receipt printer at Strebergarten.studio. Generate short content suitable for a 58mm thermal receipt. Maximum 32 characters per line. No markdown formatting. Just the content, nothing else.',
        messages: [{ role: 'user', content: prompt }]
      })
    });
    if (!res.ok) { const err = await res.json().catch(() => ({})); throw new Error(err.error?.message || 'API error ' + res.status); }
    const data = await res.json();
    const recipe = data.content[0].text;

    let footer = '\n--------------------------------\n';
    footer += 'Notes:\n';
    footer += '\n\n\n';
    footer += '--------------------------------\n';
    footer += 'Thanks for visiting\n';
    footer += 'Strebergarten. Keep growing.\n';

    const fullReceipt = header + recipe + footer;

    if (await printOrPreview(fullReceipt, true, 'https://strebergarten.studio')) {
      if (!isDebug()) showStatus('Printed!', 'text-green-600');
      saveToHistory('Harvest', {input: text}, fullReceipt);
      confetti();
      input.value = '';
    } else {
      printFailed();
    }
  } catch (err) {
    if (err.message === 'inkless-server not configured' || err.message === 'Failed to fetch')
      showStatus('AI unavailable - inkless-server not reachable', 'text-red-500');
    else showStatus(err.message || 'Generation failed', 'text-red-500');
  }

  genBtns.forEach(b => b.disabled = false);
}

// --- Streaming + web search helper with clarification loop ---
async function generateWithSearch(inputId, systemPrompt, statusColor, statusSearching, statusWriting, mode) {
  const input = document.getElementById(inputId);
  const name = input.value.trim();
  if (!name) return;

  const genBtns = document.querySelectorAll('.gen-btn');
  genBtns.forEach(b => b.disabled = true);
  showStatus(statusSearching.replace('{}', name), statusColor, false);
  hidePreview();

  try {
    const res = await fetch(getServerUrl() + '/api/generate', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        model: 'claude-sonnet-4-6',
        max_tokens: 1024,
        stream: true,
        system: systemPrompt,
        tools: [{ type: 'web_search_20250305', name: 'web_search', max_uses: 3 }],
        messages: [{ role: 'user', content: name }]
      })
    });

    if (!res.ok) { const err = await res.json().catch(() => ({})); throw new Error(err.error?.message || 'API error ' + res.status); }

    const reader = res.body.getReader();
    const decoder = new TextDecoder();
    let buffer = '';
    let fullText = '';
    let searchCount = 0;

    while (true) {
      const { done, value } = await reader.read();
      if (done) break;
      buffer += decoder.decode(value, { stream: true });
      const lines = buffer.split('\n');
      buffer = lines.pop();

      for (const line of lines) {
        if (!line.startsWith('data: ')) continue;
        const json = line.slice(6);
        if (json === '[DONE]') continue;
        let event;
        try { event = JSON.parse(json); } catch { continue; }

        if (event.type === 'content_block_start') {
          if (event.content_block?.type === 'server_tool_use') {
            searchCount++;
            showStatus('Searching the web (' + searchCount + ')...', statusColor, false);
          } else if (event.content_block?.type === 'text') {
            showStatus(statusWriting, statusColor, false);
          }
        } else if (event.type === 'content_block_delta') {
          if (event.delta?.type === 'text_delta') fullText += event.delta.text;
        }
      }
    }

    if (!fullText) throw new Error('No text in response');

    // Check if the model needs clarification (response starts with ?)
    if (fullText.trimStart().startsWith('?')) {
      const clarification = fullText.trimStart().slice(1).trim();
      showStatus(clarification, 'text-amber-600', false);
      input.value = name + ', ';
      input.focus();
      genBtns.forEach(b => b.disabled = false);
      return;
    }

    showStatus(isDebug() ? 'Preview ready' : 'Printing...', statusColor, false);
    if (await printOrPreview(fullText, true)) {
      saveToHistory(mode, {name}, fullText);
      if (!isDebug()) showStatus('Printed!', 'text-green-600');
      confetti(); input.value = '';
    }
    else printFailed();
  } catch (err) {
    if (err.message === 'inkless-server not configured' || err.message === 'Failed to fetch')
      showStatus('AI unavailable - inkless-server not reachable', 'text-red-500');
    else showStatus(err.message || 'Generation failed', 'text-red-500');
  }

  genBtns.forEach(b => b.disabled = false);
}

// --- If you were a plant ---
function ifYouWereAPlant() {
  generateWithSearch('plantName',
    "You are a receipt printer. The user will give you a person's name (and optionally context about who they are). Use web_search to learn about them and their work. If you find too many people with that name, respond with ONLY a line starting with '?' followed by a short clarification question. Otherwise — even if you only found a little — work with what you have. If you found enough to identify them: match them to a specific real plant species (not a common obvious one). Start with a small ASCII art drawing of that plant (5-8 lines, max 32 chars wide, using | / \\\\ _ . * ~ ( ) { }). Then their first name, then 'You are a [plant name].' Then a warm, poetic explanation of why. If you only found fragments or themes about them: write a short, beautiful poem for them instead, inspired by whatever threads you did find. Start with a small ASCII art plant, then their name, then the poem. Either way, the output is a keepsake. Maximum 32 characters per line. No markdown. Just the content.",
    'text-emerald-600', 'Researching {}...', 'Finding your plant...', 'If You Were a Plant');
}

// --- Talk Well Behind My Back ---
function talkWellBehindBack() {
  generateWithSearch('behindBackName',
    "You are a receipt printer. The user will give you a person's name (and optionally context about who they are). Use the web_search tool to find out about them and their work. If you find too many people with that name, respond with ONLY a line starting with '?' followed by a short clarification question. Otherwise — even if you only found a little — work with what you have. If you found enough to identify them: arrange a bouquet of praise. Start with a small ASCII art bouquet (5-8 lines, max 32 chars wide). Then their first name, then 'A bouquet for you.' Then warm, specific praise referencing real things they have done or made. If you only found fragments or themes about them: write a short, beautiful poem for them instead, woven from whatever threads you did find. Start with a small ASCII art bouquet, then their name, then the poem. Either way — no explanations, no caveats. This is a keepsake they take with them. Maximum 32 characters per line. No markdown. Just the content.",
    'text-pink-600', 'Looking up {}...', 'Arranging bouquet...', 'Talk Well Behind My Back');
}

// --- Enter key handlers ---
document.getElementById('plotInput').addEventListener('keydown', (e) => {
  if (e.key === 'Enter') generatePlot();
});
document.getElementById('plantName').addEventListener('keydown', (e) => {
  if (e.key === 'Enter') ifYouWereAPlant();
});
document.getElementById('behindBackName').addEventListener('keydown', (e) => {
  if (e.key === 'Enter') talkWellBehindBack();
});
document.getElementById('harvestInput').addEventListener('keydown', (e) => {
  if (e.key === 'Enter') generateHarvest();
});
document.getElementById('dayEndInput').addEventListener('keydown', (e) => {
  if (e.key === 'Enter') generateDayEnd();
});

// --- Day End ---
async function generateDayEnd() {
  const input = document.getElementById('dayEndInput');
  const text = input.value.trim();
  if (!text) return;

  const genBtns = document.querySelectorAll('.gen-btn');
  genBtns.forEach(b => b.disabled = true);
  showStatus('Preparing your shutdown receipt...', 'text-violet-600', false);
  hidePreview();

  try {
    const res = await fetch(getServerUrl() + '/api/print/dayend', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ progress: text, debug: isDebug() })
    });
    if (!res.ok) { const err = await res.json().catch(() => ({})); throw new Error(err.error?.message || 'API error ' + res.status); }
    const data = await res.json();
    if (isDebug()) {
      showPreview(data.text);
      showStatus('Preview (not printed)', 'text-amber-600');
    } else {
      showStatus('Printed! Time to go home.', 'text-green-600');
      confetti();
    }
    saveToHistory('Day End', {progress: text}, data.text);
    input.value = '';
  } catch (err) {
    if (err.message === 'inkless-server not configured' || err.message === 'Failed to fetch')
      showStatus('AI unavailable - inkless-server not reachable', 'text-red-500');
    else showStatus(err.message || 'Generation failed', 'text-red-500');
  }

  genBtns.forEach(b => b.disabled = false);
}

// --- Debug log ---
const debugLogEntries = [];

function addDebugEntry(method, url, status, body, response, duration) {
  const entry = {
    time: new Date().toLocaleTimeString('de-DE', {hour:'2-digit',minute:'2-digit',second:'2-digit'}),
    method, url, status, body, response, duration
  };
  debugLogEntries.unshift(entry);
  if (debugLogEntries.length > 200) debugLogEntries.length = 200;
  renderDebugLog();
}

function renderDebugLog() {
  const el = document.getElementById('debugLog');
  const countEl = document.getElementById('debugLogCount');
  if (!el) return;
  countEl.textContent = debugLogEntries.length + ' entries';
  el.innerHTML = debugLogEntries.map(e => {
    const statusColor = e.status < 300 ? 'text-green-600' : 'text-red-600';
    const safeMethod = escapeHtml(e.method);
    const safeUrl = escapeHtml(e.url);
    const safeStatus = escapeHtml(String(e.status));
    let html = '<div class="px-2 py-1.5 border-b border-gray-200 last:border-0">';
    html += '<div class="flex justify-between items-center">';
    html += '<span class="font-semibold">' + safeMethod + ' <span class="text-gray-600">' + safeUrl + '</span></span>';
    html += '<span class="text-gray-400">' + e.time + '</span>';
    html += '</div>';
    html += '<div class="flex gap-2 mt-0.5">';
    html += '<span class="' + statusColor + '">' + safeStatus + '</span>';
    html += '<span class="text-gray-400">' + e.duration + 'ms</span>';
    html += '</div>';
    if (e.body) { const safeBody = escapeHtml(e.body); html += '<div class="text-gray-500 mt-0.5 truncate" title="' + safeBody + '">&#8594; ' + safeBody.substring(0, 80) + '</div>'; }
    if (e.response) { const safeResponse = escapeHtml(e.response); html += '<div class="text-gray-700 mt-0.5 truncate" title="' + safeResponse + '">&#8592; ' + safeResponse.substring(0, 80) + '</div>'; }
    html += '</div>';
    return html;
  }).join('');
}

function clearDebugLog() {
  debugLogEntries.length = 0;
  renderDebugLog();
}

// Intercept all fetch calls to log printer requests
const _origFetch = window.fetch;
window.fetch = async function(url, opts) {
  const urlStr = typeof url === 'string' ? url : url.toString();
  // Only log local printer requests (not Anthropic API)
  const isLocal = urlStr.startsWith('/') || urlStr.startsWith(location.origin);
  const method = opts?.method || 'GET';
  const bodyStr = opts?.body ? (typeof opts.body === 'string' ? opts.body : JSON.stringify(opts.body)) : null;
  const t0 = performance.now();

  try {
    const res = await _origFetch.apply(this, arguments);
    const duration = Math.round(performance.now() - t0);
    if (isLocal) {
      const clone = res.clone();
      clone.text().then(text => {
        addDebugEntry(method, urlStr, res.status, bodyStr, text, duration);
      }).catch(() => {
        addDebugEntry(method, urlStr, res.status, bodyStr, '<unavailable>', duration);
      });
    }
    return res;
  } catch (err) {
    const duration = Math.round(performance.now() - t0);
    if (isLocal) addDebugEntry(method, urlStr, 'ERR', bodyStr, err.message, duration);
    throw err;
  }
};

async function debugTest(path, label, method) {
  method = method || 'POST';
  try {
    const res = await fetch(path, {method});
    const data = await res.text();
    showStatus(label + ': ' + data, res.ok ? 'text-green-600' : 'text-red-500');
  } catch (err) {
    showStatus(label + ': ' + err.message, 'text-red-500');
  }
}

async function debugTestQR() {
  try {
    const res = await fetch('/print/qrcode', {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: JSON.stringify({data: 'https://strebergarten.studio', label: 'strebergarten.studio', size: 4})
    });
    showStatus('QR: ' + await res.text(), res.ok ? 'text-green-600' : 'text-red-500');
  } catch (err) { showStatus('QR: ' + err.message, 'text-red-500'); }
}

async function debugTestText() {
  try {
    const res = await fetch('/print/text', {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: JSON.stringify({text: 'Debug test ' + new Date().toLocaleTimeString(), bold: true, align: 1})
    });
    showStatus('Text: ' + await res.text(), res.ok ? 'text-green-600' : 'text-red-500');
  } catch (err) { showStatus('Text: ' + err.message, 'text-red-500'); }
}

async function debugTestSpacing() {
  const lines = 'Line spacing: ESC 3 6\n--------------------------------\nYour harvest:\n- a clear next step\n- a new perspective\n--------------------------------\nRecipe: Fresh Start Stew\n1. Simmer on Monday\n2. Serve by Friday\n--------------------------------\nStrebergarten. Keep growing.\n';
  try {
    const res = await fetch('/print/text', {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: JSON.stringify({text: lines})
    });
    showStatus('Spacing: ' + await res.text(), res.ok ? 'text-green-600' : 'text-red-500');
  } catch (err) { showStatus('Spacing: ' + err.message, 'text-red-500'); }
}

async function debugTestWeight() {
  try {
    await fetch('/print/text', {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: JSON.stringify({text: 'Font weight test\n--------------------------------\nNormal weight'})
    });
    await fetch('/print/text', {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: JSON.stringify({text: 'Bold weight', bold: true})
    });
    await fetch('/print/text', {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: JSON.stringify({text: 'Double size', font_width: 2, font_height: 2})
    });
    await fetch('/print/text', {
      method: 'POST',
      headers: {'Content-Type': 'application/json'},
      body: JSON.stringify({text: 'Bold + double', bold: true, font_width: 2, font_height: 2})
    });
    showStatus('Weight: printed', 'text-green-600');
  } catch (err) { showStatus('Weight: ' + err.message, 'text-red-500'); }
}

// --- Touch swipe ---
let touchStartX = 0;
document.addEventListener('touchstart', (e) => touchStartX = e.touches[0].clientX);
document.addEventListener('touchend', (e) => {
  const diff = e.changedTouches[0].clientX - touchStartX;
  if (Math.abs(diff) > 50) nav(diff < 0 ? 1 : -1);
});
</script>
</body>
</html>
)rawliteral";
