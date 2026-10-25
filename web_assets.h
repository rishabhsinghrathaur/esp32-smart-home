#pragma once
// Embedded web assets, kept out of the main firmware file.
// Icons are in app_icons.h.

const char SVG_ICON[] PROGMEM = R"rawliteral(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 512 512">
  <defs>
    <linearGradient id="bg" x1="0%" y1="0%" x2="100%" y2="100%">
      <stop offset="0%" stop-color="#151d30"/>
      <stop offset="100%" stop-color="#0b0f19"/>
    </linearGradient>
    <linearGradient id="roof" x1="0%" y1="0%" x2="100%" y2="100%">
      <stop offset="0%" stop-color="#38bdf8"/>
      <stop offset="100%" stop-color="#0284c7"/>
    </linearGradient>
  </defs>
  <rect width="512" height="512" rx="110" fill="url(#bg)"/>
  <rect x="12" y="12" width="488" height="488" rx="100" fill="none" stroke="#38bdf8" stroke-width="8" stroke-opacity="0.3"/>
  <path d="M256 90 L80 230 L120 230 L120 410 L210 410 L210 300 L302 300 L302 410 L392 410 L392 230 L432 230 Z" fill="url(#roof)"/>
  <circle cx="256" cy="220" r="38" fill="#22c55e"/>
  <path d="M246 205 L246 235 M231 220 L261 220" stroke="#ffffff" stroke-width="6" stroke-linecap="round"/>
</svg>)rawliteral";

const char MANIFEST_JSON[] PROGMEM = R"rawliteral({
  "id": "/",
  "name": "Smart Home Pro",
  "short_name": "SmartHome",
  "description": "ESP32 4-Channel Home Automation Dashboard",
  "start_url": "/",
  "scope": "/",
  "display": "standalone",
  "background_color": "#0b0f19",
  "theme_color": "#0b0f19",
  "orientation": "portrait-primary",
  "icons": [
    {
      "src": "/icon-192.png",
      "sizes": "192x192",
      "type": "image/png",
      "purpose": "any"
    },
    {
      "src": "/icon-512.png",
      "sizes": "512x512",
      "type": "image/png",
      "purpose": "any"
    },
    {
      "src": "/icon-512.png",
      "sizes": "512x512",
      "type": "image/png",
      "purpose": "maskable"
    }
  ]
})rawliteral";

// Old v1 firmware registered a service worker. Keep this endpoint so browsers
// that still have it installed fetch a version that unregisters itself.
const char SW_CLEANUP[] PROGMEM = R"rawliteral(
self.addEventListener('install', (e) => { self.skipWaiting(); });
self.addEventListener('activate', (e) => { self.registration.unregister(); });
)rawliteral";

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no">
  <title>Smart Home Pro</title>
  
  <link rel="manifest" href="/manifest.json">
  <meta name="theme-color" content="#0b0f19">
  <meta name="mobile-web-app-capable" content="yes">
  <meta name="apple-mobile-web-app-capable" content="yes">
  <meta name="apple-mobile-web-app-status-bar-style" content="black-translucent">
  <meta name="apple-mobile-web-app-title" content="SmartHome">
  <link rel="icon" type="image/svg+xml" href="/icon.svg">
  <link rel="icon" type="image/png" sizes="192x192" href="/icon-192.png">
  <link rel="apple-touch-icon" href="/apple-touch-icon.png">

  <style>
    :root {
      --bg: #0b0f19;
      --surface: #151d30;
      --surface-border: rgba(255, 255, 255, 0.08);
      --accent: #38bdf8;
      --accent-glow: rgba(56, 189, 248, 0.3);
      --text: #f8fafc;
      --text-muted: #94a3b8;
      --on: #22c55e;
      --on-glow: rgba(34, 197, 94, 0.35);
      --off: #475569;
      --danger: #ef4444;
      --warning: #f59e0b;
      --purple: #a855f7;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; font-family: system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; -webkit-tap-highlight-color: transparent; }
    body { background-color: var(--bg); color: var(--text); min-height: 100vh; padding: 20px 12px; display: flex; flex-direction: column; align-items: center; }
    .container { width: 100%; max-width: 680px; }
    
    header { text-align: center; margin-bottom: 16px; position: relative; }
    header h1 { font-size: 1.8rem; font-weight: 800; color: var(--accent); text-shadow: 0 0 25px var(--accent-glow); letter-spacing: -0.5px; }
    header p { color: var(--text-muted); font-size: 0.88rem; margin-top: 4px; }

    /* Install Banner */
    .install-banner { display: flex; background: linear-gradient(135deg, rgba(56,189,248,0.15) 0%, rgba(168,85,247,0.15) 100%); border: 1px solid rgba(56,189,248,0.4); border-radius: 14px; padding: 12px 16px; margin-bottom: 16px; align-items: center; justify-content: space-between; gap: 10px; }
    .install-text { font-size: 0.86rem; font-weight: 600; color: var(--text); display: flex; align-items: center; gap: 10px; }
    .install-icon { width: 32px; height: 32px; flex-shrink: 0; }
    .btn-install { background: var(--accent); color: #0b0f19; font-weight: 700; border: none; padding: 8px 14px; border-radius: 8px; cursor: pointer; font-size: 0.82rem; white-space: nowrap; }

    /* Top Stats Bar */
    .top-bar { display: grid; grid-template-columns: repeat(auto-fit, minmax(130px, 1fr)); gap: 8px; margin-bottom: 16px; }
    .stat-chip { background: var(--surface); border: 1px solid var(--surface-border); border-radius: 10px; padding: 8px 12px; font-size: 0.8rem; display: flex; align-items: center; gap: 8px; }
    .stat-dot { width: 8px; height: 8px; border-radius: 50%; background: var(--on); box-shadow: 0 0 8px var(--on); }
    .stat-val { font-weight: 600; color: var(--text); }

    /* Navigation Tabs */
    .tab-nav { display: flex; background: var(--surface); border-radius: 12px; padding: 4px; margin-bottom: 20px; border: 1px solid var(--surface-border); }
    .tab-btn { flex: 1; padding: 10px; border: none; background: transparent; color: var(--text-muted); font-weight: 700; font-size: 0.9rem; border-radius: 8px; cursor: pointer; transition: all 0.2s; display: flex; align-items: center; justify-content: center; gap: 8px; }
    .tab-btn.active { background: var(--accent); color: #0b0f19; box-shadow: 0 0 15px var(--accent-glow); }

    .tab-content { display: none; }
    .tab-content.active { display: block; }

    /* Master Controls */
    .master-row { display: grid; grid-template-columns: 1fr 1fr; gap: 10px; margin-bottom: 20px; }
    .m-btn { border: none; padding: 12px; border-radius: 12px; font-weight: 700; font-size: 0.95rem; cursor: pointer; display: flex; align-items: center; justify-content: center; gap: 8px; transition: all 0.2s; }
    .m-btn.all-on { background: rgba(34, 197, 94, 0.15); color: #4ade80; border: 1px solid rgba(34, 197, 94, 0.3); }
    .m-btn.all-on:hover { background: rgba(34, 197, 94, 0.25); box-shadow: 0 0 15px rgba(34, 197, 94, 0.2); }
    .m-btn.all-off { background: rgba(239, 68, 68, 0.15); color: #f87171; border: 1px solid rgba(239, 68, 68, 0.3); }
    .m-btn.all-off:hover { background: rgba(239, 68, 68, 0.25); box-shadow: 0 0 15px rgba(239, 68, 68, 0.2); }

    /* Appliance Cards Grid */
    .grid { display: grid; grid-template-columns: 1fr 1fr; gap: 16px; margin-bottom: 24px; }
    @media (max-width: 540px) { .grid { grid-template-columns: 1fr; } }
    
    .card { background: var(--surface); border-radius: 18px; border: 1px solid var(--surface-border); padding: 18px; display: flex; flex-direction: column; justify-content: space-between; transition: all 0.25s cubic-bezier(0.4, 0, 0.2, 1); position: relative; }
    .card.is-on { border-color: rgba(56, 189, 248, 0.45); box-shadow: 0 0 25px var(--accent-glow); background: linear-gradient(145deg, var(--surface) 0%, rgba(56, 189, 248, 0.08) 100%); }
    
    .card-top { display: flex; justify-content: space-between; align-items: center; margin-bottom: 12px; }
    .icon-box { width: 44px; height: 44px; border-radius: 12px; background: rgba(255, 255, 255, 0.04); display: flex; align-items: center; justify-content: center; color: var(--off); transition: all 0.3s; cursor: pointer; }
    .card.is-on .icon-box { background: var(--accent); color: #0b0f19; box-shadow: 0 0 16px var(--accent); }
    .icon-box svg, .icon-opt svg, .history-app-title svg { fill: none; stroke: currentColor; stroke-width: 2; stroke-linecap: round; stroke-linejoin: round; }
    .icon-box svg { width: 22px; height: 22px; }
    .net-banner { display: flex; align-items: center; justify-content: space-between; gap: 10px; background: rgba(245, 158, 11, 0.12); border: 1px solid rgba(245, 158, 11, 0.35); color: #fbbf24; border-radius: 12px; padding: 10px 14px; margin-bottom: 16px; font-size: 0.82rem; font-weight: 600; }
    .net-banner a { color: #fbbf24; white-space: nowrap; font-weight: 700; }
    @media (display-mode: standalone) { #pwa-banner { display: none !important; } }
    
    .gear-btn { background: none; border: none; color: var(--text-muted); cursor: pointer; padding: 6px; border-radius: 8px; transition: color 0.2s; }
    .gear-btn:hover { color: var(--text); background: rgba(255, 255, 255, 0.06); }
    .gear-btn svg { width: 18px; height: 18px; }

    /* Switch Area */
    .switch-row { display: flex; justify-content: space-between; align-items: center; cursor: pointer; user-select: none; margin: 10px 0; }
    .card-details { flex-grow: 1; }
    .card-ch { font-size: 0.72rem; color: var(--text-muted); text-transform: uppercase; letter-spacing: 0.6px; font-weight: 600; }
    .card-title { font-size: 1.1rem; font-weight: 700; color: var(--text); margin-top: 2px; }
    .card-state { font-size: 0.8rem; font-weight: 600; color: var(--off); margin-top: 2px; }
    .card.is-on .card-state { color: var(--accent); }

    .toggle-track { width: 48px; height: 26px; background: #26334d; border-radius: 14px; position: relative; transition: background 0.3s; flex-shrink: 0; }
    .card.is-on .toggle-track { background: var(--accent); }
    .toggle-thumb { width: 20px; height: 20px; background: #fff; border-radius: 50%; position: absolute; top: 3px; left: 3px; transition: transform 0.25s; box-shadow: 0 2px 5px rgba(0,0,0,0.3); }
    .card.is-on .toggle-thumb { transform: translateX(22px); }

    /* Badges */
    .timer-badge { display: inline-flex; align-items: center; gap: 6px; background: rgba(245, 158, 11, 0.15); color: #fbbf24; border: 1px solid rgba(245, 158, 11, 0.3); border-radius: 8px; padding: 4px 8px; font-size: 0.75rem; font-weight: 600; margin-top: 8px; }
    .runtime-mini { font-size: 0.72rem; color: var(--text-muted); margin-top: 6px; display: flex; align-items: center; gap: 4px; }

    /* History & Analytics */
    .analytics-summary { display: grid; grid-template-columns: 1fr 1fr; gap: 12px; margin-bottom: 20px; }
    .summary-card { background: var(--surface); border: 1px solid var(--surface-border); border-radius: 14px; padding: 14px; }
    .summary-label { font-size: 0.78rem; color: var(--text-muted); text-transform: uppercase; font-weight: 600; }
    .summary-val { font-size: 1.4rem; font-weight: 800; color: var(--accent); margin-top: 4px; }

    .history-card { background: var(--surface); border: 1px solid var(--surface-border); border-radius: 16px; padding: 18px; margin-bottom: 16px; }
    .history-card-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 14px; }
    .history-app-title { font-size: 1.05rem; font-weight: 700; color: var(--text); display: flex; align-items: center; gap: 8px; }
    
    .progress-track { height: 10px; background: #0b0f19; border-radius: 5px; overflow: hidden; margin-top: 8px; }
    .progress-bar-fill { height: 100%; background: linear-gradient(90deg, var(--accent), var(--purple)); border-radius: 5px; transition: width 0.3s; }
    
    .history-meta-row { display: flex; justify-content: space-between; font-size: 0.82rem; color: var(--text-muted); margin-top: 8px; }
    .history-meta-val { font-weight: 700; color: var(--text); }

    /* Activity Log */
    .log-box { background: var(--surface); border: 1px solid var(--surface-border); border-radius: 16px; padding: 18px; margin-top: 20px; }
    .log-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 14px; }
    .log-list { list-style: none; display: flex; flex-direction: column; gap: 10px; }
    .log-item { display: flex; justify-content: space-between; align-items: center; padding: 10px 12px; background: #0b0f19; border-radius: 10px; border: 1px solid var(--surface-border); font-size: 0.83rem; }
    .log-left { display: flex; align-items: center; gap: 10px; }
    .log-badge { padding: 3px 8px; border-radius: 6px; font-size: 0.72rem; font-weight: 700; }
    .badge-on { background: rgba(34, 197, 94, 0.2); color: #4ade80; border: 1px solid rgba(34, 197, 94, 0.4); }
    .badge-off { background: rgba(239, 68, 68, 0.2); color: #f87171; border: 1px solid rgba(239, 68, 68, 0.4); }
    .log-duration { color: var(--accent); font-weight: 600; }

    /* Modal Styling */
    .modal-overlay { position: fixed; inset: 0; background: rgba(0, 0, 0, 0.75); backdrop-filter: blur(5px); display: none; justify-content: center; align-items: center; z-index: 100; padding: 16px; }
    .modal { background: var(--surface); border: 1px solid var(--surface-border); border-radius: 20px; padding: 24px; width: 100%; max-width: 440px; max-height: 90vh; overflow-y: auto; }
    .modal-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 18px; }
    .modal-header h3 { font-size: 1.25rem; color: var(--accent); }
    .close-btn { background: none; border: none; color: var(--text-muted); cursor: pointer; font-size: 1.4rem; }

    .form-group { margin-bottom: 16px; }
    .form-group label { display: block; font-size: 0.85rem; color: var(--text-muted); margin-bottom: 6px; font-weight: 600; }
    .form-group input, .form-group select { width: 100%; background: #0b0f19; border: 1px solid var(--surface-border); border-radius: 10px; padding: 10px 12px; color: var(--text); font-size: 0.9rem; outline: none; }
    .form-group input:focus, .form-group select:focus { border-color: var(--accent); }

    .icon-picker { display: grid; grid-template-columns: repeat(4, 1fr); gap: 8px; margin-top: 6px; }
    .icon-opt { padding: 10px; border-radius: 10px; background: #0b0f19; border: 1px solid var(--surface-border); display: flex; justify-content: center; align-items: center; cursor: pointer; color: var(--text-muted); }
    .icon-opt.selected { border-color: var(--accent); color: var(--accent); background: rgba(56, 189, 248, 0.1); }
    .icon-opt svg { width: 20px; height: 20px; }

    .timer-quick-btns { display: flex; gap: 8px; margin-top: 8px; }
    .t-quick { flex: 1; padding: 8px; background: #0b0f19; border: 1px solid var(--surface-border); border-radius: 8px; color: var(--text); cursor: pointer; font-size: 0.8rem; }
    .t-quick:hover { border-color: var(--accent); }

    .btn-save { width: 100%; padding: 12px; border: none; border-radius: 10px; background: var(--accent); color: #0b0f19; font-weight: 700; cursor: pointer; margin-top: 12px; font-size: 0.95rem; }
    .btn-cancel { width: 100%; padding: 10px; border: 1px solid var(--surface-border); border-radius: 10px; background: transparent; color: var(--text-muted); font-weight: 600; cursor: pointer; margin-top: 8px; font-size: 0.85rem; }

    /* Step guide inside PWA Assistant modal */
    .step-box { background: #0b0f19; border-radius: 12px; padding: 14px; margin-bottom: 12px; border: 1px solid var(--surface-border); }
    .step-title { font-weight: 700; color: var(--accent); font-size: 0.9rem; margin-bottom: 6px; display: flex; align-items: center; gap: 6px; }
    .step-desc { font-size: 0.85rem; color: var(--text-muted); line-height: 1.4; }

    footer { text-align: center; color: var(--text-muted); font-size: 0.82rem; margin-top: 20px; display: flex; flex-direction: column; gap: 8px; }
    .footer-links { display: flex; justify-content: center; gap: 16px; flex-wrap: wrap; }
    .footer-link { color: var(--accent); text-decoration: none; font-weight: 600; display: inline-flex; align-items: center; gap: 4px; }
    .footer-link:hover { text-decoration: underline; }
  </style>
</head>
<body>
  <div class="container">
    <header>
      <h1>Smart Home Pro</h1>
      <p>4-Channel Dual Relay Automation System</p>
    </header>

    <div class="net-banner" id="net-banner" style="display: none;">
      <span id="net-banner-text">Hotspot mode</span>
      <a href="/wifi">Wi-Fi setup</a>
    </div>

    <!-- PWA Install Banner -->
    <div class="install-banner" id="pwa-banner">
      <div class="install-text">
        <img src="/icon.svg" class="install-icon" alt="App Icon">
        <span>Install App / Add Shortcut to Home Screen</span>
      </div>
      <button class="btn-install" onclick="openInstallModal()">Add to Screen</button>
    </div>

    <!-- Telemetry Bar -->
    <div class="top-bar">
      <div class="stat-chip">
        <div class="stat-dot" id="wifi-dot"></div>
        <span>Wi-Fi: <span class="stat-val" id="sync-stat">Connecting</span></span>
      </div>
      <div class="stat-chip">
        <span>📶 Signal: <span class="stat-val" id="wifi-rssi">--%</span></span>
      </div>
      <div class="stat-chip">
        <span>🕒 Clock: <span class="stat-val" id="sys-time">--:--</span></span>
      </div>
      <div class="stat-chip">
        <span>⏱️ Up: <span class="stat-val" id="sys-uptime">--</span></span>
      </div>
      <div class="stat-chip">
        <span>🎙️ Voice: <span class="stat-val" id="cloud-stat">Not set up</span></span>
      </div>
    </div>

    <!-- Navigation Tabs -->
    <div class="tab-nav">
      <button class="tab-btn active" onclick="switchTab('control')">
        <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M18.36 6.64a9 9 0 1 1-12.73 0"></path><line x1="12" y1="2" x2="12" y2="12"></line></svg>
        Controls
      </button>
      <button class="tab-btn" onclick="switchTab('history')">
        <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M12 20v-6M6 20V10M18 20V4"></path></svg>
        Usage & History
      </button>
    </div>

    <!-- TAB 1: CONTROLS -->
    <div class="tab-content active" id="tab-control">
      <div class="master-row">
        <button class="m-btn all-on" onclick="setAll(1)">
          <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M18.36 6.64a9 9 0 1 1-12.73 0"></path><line x1="12" y1="2" x2="12" y2="12"></line></svg>
          All Power ON
        </button>
        <button class="m-btn all-off" onclick="setAll(0)">
          <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="9"></circle><line x1="9" y1="9" x2="15" y2="15"></line><line x1="15" y1="9" x2="9" y2="15"></line></svg>
          All Power OFF
        </button>
      </div>

      <div class="grid" id="relay-grid"></div>
    </div>

    <!-- TAB 2: USAGE & HISTORY -->
    <div class="tab-content" id="tab-history">
      <div class="analytics-summary">
        <div class="summary-card">
          <div class="summary-label">Today's Total Runtime</div>
          <div class="summary-val" id="sum-today">0m</div>
        </div>
        <div class="summary-card">
          <div class="summary-label">Total Switch Cycles</div>
          <div class="summary-val" id="sum-switches">0 times</div>
        </div>
      </div>

      <div id="history-cards-container"></div>

      <div class="log-box">
        <div class="log-header">
          <h3 style="font-size: 1.1rem; color: var(--accent);">Activity Timeline (Recent Events)</h3>
          <button class="t-quick" style="padding: 4px 10px;" onclick="resetAnalytics()">Reset Stats</button>
        </div>
        <ul class="log-list" id="log-list">
          <li style="color: var(--text-muted); font-size: 0.85rem; text-align: center; padding: 12px;">No activity logged yet.</li>
        </ul>
      </div>
    </div>

    <footer>
      <div class="footer-links">
        <a href="javascript:void(0)" onclick="openInstallModal()" class="footer-link">
          <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="5" y="2" width="14" height="20" rx="2" ry="2"></rect><line x1="12" y1="18" x2="12.01" y2="18"></line></svg>
          Add App Shortcut
        </a>
        <a href="/wifi" class="footer-link">
          <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M5 12.55a11 11 0 0 1 14.08 0"></path><path d="M1.42 9a16 16 0 0 1 21.16 0"></path><path d="M8.53 16.11a6 6 0 0 1 6.95 0"></path><line x1="12" y1="20" x2="12.01" y2="20"></line></svg>
          Wi-Fi &amp; Network
        </a>
        <a href="/update" class="footer-link">
          <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M21 15v4a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2v-4"></path><polyline points="17 8 12 3 7 8"></polyline><line x1="12" y1="3" x2="12" y2="15"></line></svg>
          OTA Firmware Update
        </a>
        <a href="javascript:void(0)" onclick="openSettingsModal()" class="footer-link">
          <svg width="15" height="15" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="3"></circle><path d="M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 0 1 0 2.83 2 2 0 0 1-2.83 0l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 0 1-2 2 2 2 0 0 1-2-2v-.09A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 0 1-2.83 0 2 2 0 0 1 0-2.83l.06-.06a1.65 1.65 0 0 0-.33 1.82V9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 0 1 2 2 2 2 0 0 1-2 2h-.09a1.65 1.65 0 0 0-1.51 1z"></path></svg>
          Config & Timezone
        </a>
      </div>
      <span>ESP32 Smart Home &bull; IP: <span id="esp-ip">...</span> &bull; <a class="footer-link" href="/" id="esp-url">smarthome.local</a> &bull; FW <span id="fw-ver">2.0.0</span></span>
    </footer>
  </div>

  <!-- PWA Install / Home Screen Assistant Modal -->
  <div class="modal-overlay" id="install-modal">
    <div class="modal">
      <div class="modal-header">
        <h3>📲 Add App to Home Screen</h3>
        <button class="close-btn" onclick="closeInstallModal()">&times;</button>
      </div>

      <div style="text-align: center; margin-bottom: 16px;">
        <img src="/icon.svg" style="width: 72px; height: 72px; border-radius: 16px; box-shadow: 0 4px 16px var(--accent-glow);" alt="App Logo">
        <h4 style="margin-top: 8px; color: var(--text);">Smart Home Pro</h4>
        <p style="font-size: 0.8rem; color: var(--text-muted);">Quick 1-Click Access with Logo</p>
      </div>

      <div id="pwa-native-box" style="display: none; margin-bottom: 14px;">
        <button class="btn-save" id="btn-trigger-pwa">Install Automatically</button>
      </div>

      <div class="step-box">
        <div class="step-title">🤖 For Android (Chrome / Brave / Edge):</div>
        <div class="step-desc">
          1. Tap Chrome's <strong>Top-Right Menu (⋮ 3 Dots)</strong>.<br>
          2. Tap <strong>"Add to Home screen"</strong> or <strong>"Install app"</strong>.<br>
          3. Tap <strong>Add / Install</strong> &mdash; It will open in Fullscreen without browser URL bar!
        </div>
      </div>

      <div class="step-box">
        <div class="step-title">🍏 For iPhone & iPad (Safari):</div>
        <div class="step-desc">
          1. Tap the <strong>Share Button</strong> (square with arrow at bottom).<br>
          2. Scroll down and select <strong>"Add to Home Screen"</strong>.<br>
          3. Tap <strong>Add</strong> in the top right.
        </div>
      </div>

      <button class="btn-cancel" onclick="closeInstallModal()">Got it!</button>
    </div>
  </div>

  <!-- Channel Config & Timer Modal -->
  <div class="modal-overlay" id="config-modal">
    <div class="modal">
      <div class="modal-header">
        <h3 id="modal-title">Edit Appliance</h3>
        <button class="close-btn" onclick="closeModal()">&times;</button>
      </div>

      <div class="form-group">
        <label>Appliance Name</label>
        <input type="text" id="edit-name" placeholder="e.g. Living Room Chandelier" maxlength="25">
      </div>

      <div class="form-group">
        <label>Select Icon</label>
        <div class="icon-picker" id="icon-picker"></div>
      </div>

      <div class="form-group">
        <label>⏱️ Auto-Off Countdown Timer (Minutes)</label>
        <input type="number" id="edit-timer" placeholder="0 = Cancel / Disabled" min="0" max="1440">
        <div class="timer-quick-btns">
          <button class="t-quick" onclick="setTimerQuick(5)">5m</button>
          <button class="t-quick" onclick="setTimerQuick(15)">15m</button>
          <button class="t-quick" onclick="setTimerQuick(30)">30m</button>
          <button class="t-quick" onclick="setTimerQuick(60)">1 hr</button>
          <button class="t-quick" onclick="setTimerQuick(0)">Off</button>
        </div>
      </div>

      <div class="form-group">
        <label>📅 Daily Automation Schedule</label>
        <div style="display: flex; gap: 8px; align-items: center; margin-bottom: 8px;">
          <input type="checkbox" id="sched-enable" style="width: auto;">
          <span style="font-size: 0.85rem;">Enable Schedule</span>
        </div>
        <div style="display: grid; grid-template-columns: 1fr 1fr; gap: 8px;">
          <div>
            <label style="font-size: 0.75rem;">Turn ON At</label>
            <input type="time" id="sched-on">
          </div>
          <div>
            <label style="font-size: 0.75rem;">Turn OFF At</label>
            <input type="time" id="sched-off">
          </div>
        </div>
      </div>

      <div class="form-group">
        <label>⚡ Power-On Default State</label>
        <select id="edit-power-mode">
          <option value="2">Restore Last State (Recommended)</option>
          <option value="0">Always Power OFF</option>
          <option value="1">Always Power ON</option>
        </select>
      </div>

      <div class="form-group">
        <label>⏱️ Inching / Pulse Mode (Auto OFF in Seconds)</label>
        <input type="number" id="edit-inching" placeholder="0 = Disabled, e.g. 5 for pulse" min="0" max="3600">
      </div>

      <button class="btn-save" onclick="saveRelayConfig()">Save Changes</button>
      <button class="btn-cancel" onclick="closeModal()">Cancel</button>
    </div>
  </div>

  <!-- Global Settings Modal -->
  <div class="modal-overlay" id="settings-modal">
    <div class="modal">
      <div class="modal-header">
        <h3>System Configuration</h3>
        <button class="close-btn" onclick="closeSettingsModal()">&times;</button>
      </div>

      <div class="form-group">
        <label>Timezone Offset in Hours (e.g. 5.5 for India, -5 for EST, 0 for UTC)</label>
        <input type="number" id="cfg-tz" step="0.5" value="5.5">
      </div>

      <div class="form-group">
        <label>Network</label>
        <div class="step-box" style="margin-bottom: 0;">
          <div class="step-desc" id="net-info">Loading...</div>
          <div style="margin-top: 10px;"><a href="/wifi" class="footer-link">Open Wi-Fi setup</a></div>
        </div>
      </div>

      <div class="form-group">
        <label>Hotspot / OTA Password (8–63 characters)</label>
        <input type="password" id="cfg-ap-pass" placeholder="Leave blank to keep current" maxlength="63">
        <div style="font-size: 0.75rem; color: var(--text-muted); margin-top: 6px;">Used for the fallback hotspot and web/Arduino OTA. Takes full effect after a reboot.</div>
      </div>

      <div class="form-group" style="margin-top: 20px;">
        <label>System Maintenance</label>
        <button class="m-btn all-off" style="width: 100%; margin-bottom: 8px;" onclick="restartESP()">
          🔄 Reboot ESP32
        </button>
      </div>

      <button class="btn-save" onclick="saveSystemConfig()">Save Settings</button>
      <button class="btn-cancel" onclick="closeSettingsModal()">Close</button>
    </div>
  </div>

  <script>
    let deferredPrompt = null;
    window.addEventListener('beforeinstallprompt', (e) => {
      e.preventDefault();
      deferredPrompt = e;
      const box = document.getElementById('pwa-native-box');
      if (box) box.style.display = 'block';
    });

    document.getElementById('btn-trigger-pwa')?.addEventListener('click', async () => {
      if (deferredPrompt) {
        deferredPrompt.prompt();
        const { outcome } = await deferredPrompt.userChoice;
        if (outcome === 'accepted') {
          closeInstallModal();
          document.getElementById('pwa-banner').style.display = 'none';
        }
        deferredPrompt = null;
      }
    });

    function openInstallModal() { document.getElementById('install-modal').style.display = 'flex'; }
    function closeInstallModal() { document.getElementById('install-modal').style.display = 'none'; }

    const ICONS = {
      lightbulb: `<svg viewBox="0 0 24 24"><path d="M9 18h6"></path><path d="M10 22h4"></path><path d="M15.09 14c.18-.98.65-1.74 1.41-2.5A4.65 4.65 0 0 0 18 8 6 6 0 0 0 6 8c0 1 .23 2.23 1.5 3.5A4.61 4.61 0 0 1 8.91 14"></path></svg>`,
      fan: `<svg viewBox="0 0 24 24"><path d="M12 12c-2-2.5-5.5-2.5-8 0s-2.5 5.5 0 8 5.5 2.5 8 0"></path><path d="M12 12c2.5-2 2.5-5.5 0-8s-5.5-2.5-8 0-2.5 5.5 0 8"></path><path d="M12 12c2 2.5 5.5 2.5 8 0s2.5-5.5 0-8-5.5-2.5-8 0"></path><circle cx="12" cy="12" r="2"></circle></svg>`,
      lamp: `<svg viewBox="0 0 24 24"><circle cx="12" cy="12" r="5"></circle><line x1="12" y1="1" x2="12" y2="3"></line><line x1="12" y1="21" x2="12" y2="23"></line><line x1="4.22" y1="4.22" x2="5.64" y2="5.64"></line><line x1="18.36" y1="18.36" x2="19.78" y2="19.78"></line><line x1="1" y1="12" x2="3" y2="12"></line><line x1="21" y1="12" x2="23" y2="12"></line></svg>`,
      power: `<svg viewBox="0 0 24 24"><path d="M18.36 6.64a9 9 0 1 1-12.73 0"></path><line x1="12" y1="2" x2="12" y2="12"></line></svg>`,
      tv: `<svg viewBox="0 0 24 24"><rect x="2" y="7" width="20" height="15" rx="2" ry="2"></rect><polyline points="17 2 12 7 7 2"></polyline></svg>`,
      ac: `<svg viewBox="0 0 24 24"><path d="M4 14.899A7 7 0 1 1 15.71 8h1.79a4.5 4.5 0 0 1 2.5 8.242"></path><path d="M12 12v9"></path><path d="m8 17 4 4 4-4"></path></svg>`,
      water: `<svg viewBox="0 0 24 24"><path d="M12 2.69l5.66 5.66a8 8 0 1 1-11.31 0z"></path></svg>`,
      door: `<svg viewBox="0 0 24 24"><path d="M3 21h18"></path><path d="M18 21V3H6v18"></path><path d="M14 12v.01"></path></svg>`
    };

    let appData = {
      states: [false, false, false, false],
      names: ["Relay 1", "Relay 2", "Relay 3", "Relay 4"],
      icons: ["lightbulb", "fan", "lamp", "power"],
      pins: [23, 22, 21, 19],
      timers: [0, 0, 0, 0],
      timerMins: [0, 0, 0, 0],
      inchings: [0, 0, 0, 0],
      powerModes: [2, 2, 2, 2],
      schedules: [],
      todaySecs: [0, 0, 0, 0],
      totalSecs: [0, 0, 0, 0],
      switches: [0, 0, 0, 0],
      logs: [],
      ip: "",
      mode: "STA",
      mdns: false,
      cloud: 0,
      ssid: "",
      apSsid: "",
      hostname: "smarthome",
      rssi: 0,
      time: "--:--",
      uptime: "",
      tz: 5.5,
      fw: "2.0.0",
      connected: false
    };

    let activeEditChannel = 0;
    let selectedIconKey = "lightbulb";

    function escapeHtml(value) {
      return String(value === null || value === undefined ? '' : value).replace(/[&<>"']/g, ch => ({
        '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;'
      }[ch]));
    }

    function switchTab(tab) {
      document.querySelectorAll('.tab-btn').forEach(b => b.classList.remove('active'));
      document.querySelectorAll('.tab-content').forEach(c => c.classList.remove('active'));
      if (tab === 'control') {
        document.querySelectorAll('.tab-btn')[0].classList.add('active');
        document.getElementById('tab-control').classList.add('active');
      } else {
        document.querySelectorAll('.tab-btn')[1].classList.add('active');
        document.getElementById('tab-history').classList.add('active');
      }
    }

    function formatTimeFriendly(totalSeconds) {
      if (!totalSeconds || totalSeconds <= 0) return '0 mins';
      const h = Math.floor(totalSeconds / 3600);
      const m = Math.floor((totalSeconds % 3600) / 60);
      const s = totalSeconds % 60;
      if (h > 0) return `${h}h ${m}m`;
      if (m > 0) return `${m}m ${s}s`;
      return `${s}s`;
    }

    function format2Digits(n) { return (n < 10 ? '0' : '') + n; }

    function renderCards() {
      const grid = document.getElementById('relay-grid');
      grid.innerHTML = appData.names.map((name, idx) => {
        const iconSvg = ICONS[appData.icons[idx]] || ICONS.lightbulb;
        const timerSecs = appData.timers[idx] || 0;
        let timerBadgeHtml = '';
        if (timerSecs > 0) {
          const m = Math.floor(timerSecs / 60);
          const s = timerSecs % 60;
          const isPulse = (appData.timerMins[idx] || 0) === 0;
          timerBadgeHtml = `<div class="timer-badge">
            <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="10"></circle><polyline points="12 6 12 12 16 14"></polyline></svg>
            ${isPulse ? 'Pulse off' : 'Off'} in ${m}m ${s < 10 ? '0' : ''}${s}s
          </div>`;
        }

        const sched = appData.schedules[idx];
        let schedBadgeHtml = '';
        if (sched && sched.enabled) {
          schedBadgeHtml = `<div class="timer-badge" style="background: rgba(56,189,248,0.15); color: #38bdf8; border-color: rgba(56,189,248,0.3);">
            📅 ${format2Digits(sched.onH)}:${format2Digits(sched.onM)} &rarr; ${format2Digits(sched.offH)}:${format2Digits(sched.offM)}
          </div>`;
        }

        const todayTimeStr = formatTimeFriendly(appData.todaySecs[idx]);
        const pin = appData.pins[idx] !== undefined ? appData.pins[idx] : '--';

        return `
        <div class="card ${appData.states[idx] ? 'is-on' : ''}" id="card-${idx}">
          <div class="card-top">
            <div class="icon-box" onclick="toggleRelay(${idx})">
              ${iconSvg}
            </div>
            <button class="gear-btn" onclick="openModal(${idx})" title="Edit Name, Icon & Timers">
              <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="3"></circle><path d="M19.4 15a1.65 1.65 0 0 0 .33 1.82l.06.06a2 2 0 0 1 0 2.83 2 2 0 0 1-2.83 0l-.06-.06a1.65 1.65 0 0 0-1.82-.33 1.65 1.65 0 0 0-1 1.51V21a2 2 0 0 1-2 2 2 2 0 0 1-2-2v-.09A1.65 1.65 0 0 0 9 19.4a1.65 1.65 0 0 0-1.82.33l-.06.06a2 2 0 0 1-2.83 0 2 2 0 0 1 0-2.83l.06-.06a1.65 1.65 0 0 0 .33-1.82 1.65 1.65 0 0 0-1.51-1H3a2 2 0 0 1-2-2 2 2 0 0 1 2-2h.09A1.65 1.65 0 0 0 4.6 9a1.65 1.65 0 0 0-.33-1.82l-.06-.06a2 2 0 0 1 0-2.83 2 2 0 0 1 2.83 0l.06.06a1.65 1.65 0 0 0 1.82.33H9a1.65 1.65 0 0 0 1-1.51V3a2 2 0 0 1 2-2 2 2 0 0 1 2 2v.09a1.65 1.65 0 0 0 1 1.51 1.65 1.65 0 0 0 1.82-.33l.06-.06a2 2 0 0 1 2.83 0 2 2 0 0 1 0 2.83l-.06.06a1.65 1.65 0 0 0-.33 1.82V9a1.65 1.65 0 0 0 1.51 1H21a2 2 0 0 1 2 2 2 2 0 0 1-2 2h-.09a1.65 1.65 0 0 0-1.51 1z"></path></svg>
            </button>
          </div>

          <div class="switch-row" onclick="toggleRelay(${idx})">
            <div class="card-details">
              <div class="card-ch">CH ${idx + 1} &bull; Pin ${pin}</div>
              <div class="card-title">${escapeHtml(name)}</div>
              <div class="card-state">${appData.states[idx] ? 'POWER ON' : 'POWER OFF'}</div>
            </div>
            <div class="toggle-track">
              <div class="toggle-thumb"></div>
            </div>
          </div>

          <div class="runtime-mini">
            <svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="10"></circle><polyline points="12 6 12 12 16 14"></polyline></svg>
            Today's Usage: <strong>${todayTimeStr}</strong>
          </div>

          <div style="display: flex; gap: 6px; flex-wrap: wrap;">
            ${timerBadgeHtml}
            ${schedBadgeHtml}
          </div>
        </div>`;
      }).join('');
    }

    function renderHistory() {
      let sumTodaySecs = 0;
      let sumSwitches = 0;
      appData.todaySecs.forEach(s => sumTodaySecs += s);
      appData.switches.forEach(c => sumSwitches += c);

      document.getElementById('sum-today').innerText = formatTimeFriendly(sumTodaySecs);
      document.getElementById('sum-switches').innerText = sumSwitches + ' times';

      const maxToday = Math.max(...appData.todaySecs, 3600);

      const container = document.getElementById('history-cards-container');
      container.innerHTML = appData.names.map((name, idx) => {
        const iconSvg = ICONS[appData.icons[idx]] || ICONS.lightbulb;
        const todaySec = appData.todaySecs[idx];
        const totalSec = appData.totalSecs[idx];
        const pct = Math.min(100, Math.round((todaySec / maxToday) * 100));

        return `
        <div class="history-card">
          <div class="history-card-header">
            <div class="history-app-title">
              <span style="color: var(--accent);">${iconSvg}</span>
              ${escapeHtml(name)} (CH ${idx + 1})
            </div>
            <span class="log-badge ${appData.states[idx] ? 'badge-on' : 'badge-off'}">
              ${appData.states[idx] ? 'CURRENTLY ON' : 'OFF'}
            </span>
          </div>

          <div class="history-meta-row">
            <span>Today's Active Time</span>
            <span class="history-meta-val">${formatTimeFriendly(todaySec)}</span>
          </div>

          <div class="progress-track">
            <div class="progress-bar-fill" style="width: ${pct}%;"></div>
          </div>

          <div class="history-meta-row" style="margin-top: 12px;">
            <span>All-Time Total Runtime: <strong style="color:var(--text);">${formatTimeFriendly(totalSec)}</strong></span>
            <span>Switch Cycles: <strong style="color:var(--text);">${appData.switches[idx]}x</strong></span>
          </div>
        </div>`;
      }).join('');

      const logList = document.getElementById('log-list');
      if (appData.logs && appData.logs.length > 0) {
        logList.innerHTML = appData.logs.map(lg => {
          const isTurnOn = lg.action === "ON";
          const durHtml = (!isTurnOn && lg.duration > 0) ? `<span class="log-duration">Ran for ${formatTimeFriendly(lg.duration)}</span>` : '';
          const logName = appData.names[lg.relay] || ('Relay ' + (lg.relay + 1));
          return `
          <li class="log-item">
            <div class="log-left">
              <span class="log-badge ${isTurnOn ? 'badge-on' : 'badge-off'}">${escapeHtml(lg.action)}</span>
              <div>
                <strong>${escapeHtml(logName)}</strong>
                <div style="font-size: 0.72rem; color: var(--text-muted);">Via ${escapeHtml(lg.trigger)} &bull; ${escapeHtml(lg.time)}</div>
              </div>
            </div>
            <div>${durHtml}</div>
          </li>`;
        }).join('');
      } else {
        logList.innerHTML = `<li style="color: var(--text-muted); font-size: 0.85rem; text-align: center; padding: 12px;">No activity logged yet.</li>`;
      }
    }

    function updateTelemetry(data) {
      document.getElementById('sync-stat').innerText =
        data.connected ? 'Connected' : (data.mode === 'AP_SETUP' ? 'Hotspot setup' : 'Hotspot');
      const dot = document.getElementById('wifi-dot');
      if (dot) {
        dot.style.background = data.connected ? 'var(--on)' : 'var(--warning)';
        dot.style.boxShadow = data.connected ? '0 0 8px var(--on)' : '0 0 8px var(--warning)';
      }
      document.getElementById('wifi-rssi').innerText = data.rssi + '%';
      document.getElementById('sys-time').innerText = data.time;
      document.getElementById('sys-uptime').innerText = data.uptime;
      document.getElementById('esp-ip').innerText = data.ip;

      const urlEl = document.getElementById('esp-url');
      if (urlEl) {
        urlEl.innerText = data.mdns ? ('http://' + data.hostname + '.local') : ('http://' + data.ip);
        urlEl.href = 'http://' + data.ip;
      }
      const fwEl = document.getElementById('fw-ver');
      if (fwEl) fwEl.innerText = data.fw || '2.0.0';

      const cloudEl = document.getElementById('cloud-stat');
      if (cloudEl) {
        cloudEl.innerText = data.cloud === 1 ? 'Online' : (data.cloud === 2 ? 'Offline' : 'Not set up');
      }

      const banner = document.getElementById('net-banner');
      if (banner) {
        const showBanner = (data.mode === 'AP_SETUP' || data.mode === 'AP_FALLBACK') && !data.connected;
        banner.style.display = showBanner ? 'flex' : 'none';
        if (showBanner) {
          document.getElementById('net-banner-text').innerText =
            data.mode === 'AP_SETUP'
              ? 'Setup hotspot active. Configure Wi-Fi to go online.'
              : 'Hotspot mode. Home Wi-Fi is unavailable, but local control still works.';
        }
      }
    }

    async function fetchStatus() {
      try {
        const res = await fetch('/status', { cache: 'no-store' });
        if (!res.ok) throw new Error('status');
        const data = await res.json();
        appData = data;
        updateTelemetry(data);
        renderCards();
        renderHistory();
      } catch (e) {
        const s = document.getElementById('sync-stat');
        if (s) s.innerText = 'Reconnecting...';
      }
    }

    async function post(url) {
      return fetch(url, { method: 'POST', cache: 'no-store', headers: { 'Accept': 'application/json' } });
    }

    async function toggleRelay(idx) {
      appData.states[idx] = !appData.states[idx];
      renderCards();
      try {
        const res = await post(`/toggle?relay=${idx}`);
        const data = await res.json();
        appData.states[idx] = data.state;
        fetchStatus();
      } catch (e) {
        fetchStatus();
      }
    }

    async function setAll(state) {
      try {
        const res = await post(`/all?state=${state}`);
        const data = await res.json();
        if (data.states) appData.states = data.states;
        fetchStatus();
      } catch (e) {}
    }

    function openModal(idx) {
      activeEditChannel = idx;
      document.getElementById('modal-title').innerText = `Edit Relay ${idx + 1}`;
      document.getElementById('edit-name').value = appData.names[idx];
      selectedIconKey = appData.icons[idx] || 'lightbulb';
      document.getElementById('edit-timer').value = appData.timerMins[idx] || '';
      document.getElementById('edit-power-mode').value = appData.powerModes[idx];
      document.getElementById('edit-inching').value = appData.inchings[idx] || 0;

      const sched = appData.schedules[idx] || { enabled: false, onH: 18, onM: 0, offH: 22, offM: 0 };
      document.getElementById('sched-enable').checked = sched.enabled;
      document.getElementById('sched-on').value = `${format2Digits(sched.onH)}:${format2Digits(sched.onM)}`;
      document.getElementById('sched-off').value = `${format2Digits(sched.offH)}:${format2Digits(sched.offM)}`;

      renderIconPicker();
      document.getElementById('config-modal').style.display = 'flex';
    }

    function closeModal() { document.getElementById('config-modal').style.display = 'none'; }

    function openSettingsModal() {
      document.getElementById('cfg-tz').value = appData.tz ?? 5.5;
      document.getElementById('cfg-ap-pass').value = '';

      const info = document.getElementById('net-info');
      const modeText = appData.connected
        ? ('Connected to ' + escapeHtml(appData.ssid))
        : (appData.mode === 'AP_SETUP' ? 'Setup hotspot (no home Wi-Fi yet)'
          : (appData.mode === 'AP_FALLBACK' ? 'Fallback hotspot' : 'Not connected'));
      const mdnsText = appData.mdns ? ('http://' + escapeHtml(appData.hostname) + '.local') : 'mDNS unavailable';
      const cloudText = appData.cloud === 1 ? 'Online' : (appData.cloud === 2 ? 'Offline' : 'Not configured');
      if (info) {
        info.innerHTML = `${modeText}<br>IP: ${escapeHtml(appData.ip)} &bull; ${mdnsText}<br>Voice: ${cloudText}`;
      }
      document.getElementById('settings-modal').style.display = 'flex';
    }

    function closeSettingsModal() { document.getElementById('settings-modal').style.display = 'none'; }

    function renderIconPicker() {
      const picker = document.getElementById('icon-picker');
      picker.innerHTML = Object.keys(ICONS).map(k => `
        <div class="icon-opt ${k === selectedIconKey ? 'selected' : ''}" onclick="selectIcon('${k}')">
          ${ICONS[k]}
        </div>
      `).join('');
    }

    function selectIcon(key) {
      selectedIconKey = key;
      renderIconPicker();
    }

    function setTimerQuick(mins) {
      document.getElementById('edit-timer').value = mins;
    }

    async function saveRelayConfig() {
      const name = document.getElementById('edit-name').value.trim() || `Relay ${activeEditChannel + 1}`;
      const timerMins = parseInt(document.getElementById('edit-timer').value) || 0;
      const powerMode = document.getElementById('edit-power-mode').value;
      const inching = parseInt(document.getElementById('edit-inching').value) || 0;

      const schedEnable = document.getElementById('sched-enable').checked ? 1 : 0;
      const onParts = document.getElementById('sched-on').value.split(':');
      const offParts = document.getElementById('sched-off').value.split(':');
      const onH = onParts[0] || 0, onM = onParts[1] || 0;
      const offH = offParts[0] || 0, offM = offParts[1] || 0;

      const url = `/save_config?relay=${activeEditChannel}&name=${encodeURIComponent(name)}&icon=${selectedIconKey}&timer=${timerMins}&power=${powerMode}&inching=${inching}&sched=${schedEnable}&onH=${onH}&onM=${onM}&offH=${offH}&offM=${offM}`;

      closeModal();
      await post(url);
      fetchStatus();
    }

    async function saveSystemConfig() {
      const tz = document.getElementById('cfg-tz').value;
      const pass = document.getElementById('cfg-ap-pass').value;
      closeSettingsModal();
      await post(`/set_tz?tz=${encodeURIComponent(tz)}`);
      if (pass.trim().length) {
        await post(`/set_ap_pass?pass=${encodeURIComponent(pass.trim())}`);
      }
      fetchStatus();
    }

    async function resetAnalytics() {
      if (confirm("Reset all usage statistics and activity logs?")) {
        await post('/reset_history');
        fetchStatus();
      }
    }

    async function restartESP() {
      if (confirm("Are you sure you want to reboot the ESP32?")) {
        await post('/reboot');
        alert("Reboot command sent. Please wait 10 seconds...");
        setTimeout(() => location.reload(), 10000);
      }
    }

    if (window.matchMedia('(display-mode: standalone)').matches || window.navigator.standalone) {
      const banner = document.getElementById('pwa-banner');
      if (banner) banner.style.display = 'none';
    }

    fetchStatus();
    setInterval(fetchStatus, 2000);
  </script>
</body>
</html>
)rawliteral";

const char OTA_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Firmware Update - Smart Home</title>
  <style>
    body { background-color: #0b0f19; color: #f8fafc; font-family: system-ui, sans-serif; display: flex; justify-content: center; align-items: center; min-height: 100vh; margin: 0; padding: 16px; }
    .card { background: #151d30; border-radius: 18px; padding: 28px; max-width: 440px; width: 100%; border: 1px solid rgba(255,255,255,0.08); box-shadow: 0 10px 30px rgba(0,0,0,0.5); }
    h2 { color: #38bdf8; margin-bottom: 8px; }
    p { color: #94a3b8; font-size: 0.88rem; margin-bottom: 20px; }
    .upload-box { border: 2px dashed #475569; border-radius: 12px; padding: 24px; text-align: center; margin-bottom: 20px; background: rgba(11,15,25,0.5); cursor: pointer; }
    input[type="file"] { width: 100%; color: #94a3b8; font-size: 0.85rem; }
    .progress-bar { height: 8px; background: #334155; border-radius: 4px; overflow: hidden; margin-top: 16px; display: none; }
    .progress-fill { height: 100%; width: 0%; background: #38bdf8; transition: width 0.2s; }
    .btn { width: 100%; padding: 12px; border: none; border-radius: 10px; background: #38bdf8; color: #0b0f19; font-weight: 700; cursor: pointer; }
    .back-btn { display: block; text-align: center; margin-top: 16px; color: #94a3b8; text-decoration: none; font-size: 0.85rem; }
    #status-msg { margin-top: 12px; font-size: 0.85rem; text-align: center; }
  </style>
</head>
<body>
  <div class="card">
    <h2>Over-The-Air Firmware Update</h2>
    <p>Upload a compiled <code>.bin</code> firmware file to update your ESP32 wirelessly.</p>
    <form id="upload-form" method="POST" action="/update" enctype="multipart/form-data">
      <div class="upload-box">
        <input type="file" name="update" id="file" accept=".bin" required>
      </div>
      <div class="progress-bar" id="p-bar"><div class="progress-fill" id="p-fill"></div></div>
      <div id="status-msg"></div>
      <button type="submit" class="btn" id="up-btn" style="margin-top: 16px;">Flash Firmware</button>
    </form>
    <a href="/" class="back-btn">&larr; Back to Dashboard</a>
  </div>
  <script>
    const form = document.getElementById('upload-form');
    form.onsubmit = function(e) {
      e.preventDefault();
      const file = document.getElementById('file').files[0];
      if (!file) return;
      const formData = new FormData();
      formData.append('update', file);
      document.getElementById('p-bar').style.display = 'block';
      const msg = document.getElementById('status-msg');
      msg.innerText = 'Uploading & Flashing...';
      const xhr = new XMLHttpRequest();
      xhr.open('POST', '/update', true);
      xhr.upload.onprogress = function(e) {
        if (e.lengthComputable) {
          const pct = Math.round((e.loaded / e.total) * 100);
          document.getElementById('p-fill').style.width = pct + '%';
          msg.innerText = `Uploading: ${pct}%`;
        }
      };
      xhr.onload = function() {
        if (xhr.status === 200) {
          msg.style.color = '#22c55e';
          msg.innerText = 'Update Successful! Rebooting ESP32...';
          setTimeout(() => window.location.href = '/', 6000);
        } else {
          msg.style.color = '#ef4444';
          msg.innerText = 'Update Failed: ' + xhr.responseText;
        }
      };
      xhr.send(formData);
    };
  </script>
</body>
</html>
)rawliteral";

