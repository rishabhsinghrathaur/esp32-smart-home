#pragma once
// Wi-Fi setup page served by the captive portal / /wifi route.

const char WIFI_SETUP_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Wi-Fi Setup - Smart Home</title>
  <link rel="icon" type="image/svg+xml" href="/icon.svg">
  <link rel="apple-touch-icon" href="/apple-touch-icon.png">
  <style>
    :root {
      --bg: #0b0f19; --surface: #151d30; --border: rgba(255,255,255,0.08);
      --accent: #38bdf8; --text: #f8fafc; --muted: #94a3b8;
      --ok: #22c55e; --warn: #f59e0b; --danger: #ef4444;
    }
    * { box-sizing: border-box; margin: 0; padding: 0; font-family: system-ui, -apple-system, "Segoe UI", Roboto, sans-serif; -webkit-tap-highlight-color: transparent; }
    body { background: var(--bg); color: var(--text); min-height: 100vh; padding: 24px 14px; display: flex; justify-content: center; }
    .wrap { width: 100%; max-width: 520px; }
    header { text-align: center; margin-bottom: 18px; }
    header h1 { color: var(--accent); font-size: 1.65rem; letter-spacing: -0.5px; }
    header p { color: var(--muted); font-size: 0.85rem; margin-top: 6px; }
    .card { background: var(--surface); border: 1px solid var(--border); border-radius: 18px; padding: 20px; margin-bottom: 14px; }
    .status { display: flex; gap: 10px; align-items: flex-start; font-size: 0.86rem; line-height: 1.5; }
    .dot { width: 10px; height: 10px; border-radius: 50%; background: var(--warn); margin-top: 5px; flex-shrink: 0; box-shadow: 0 0 10px var(--warn); }
    .dot.online { background: var(--ok); box-shadow: 0 0 10px var(--ok); }
    .muted { color: var(--muted); }
    label { display: block; font-size: 0.82rem; color: var(--muted); font-weight: 600; margin: 15px 0 6px; }
    input[type="text"], input[type="password"] {
      width: 100%; background: #0b0f19; border: 1px solid var(--border); border-radius: 10px;
      padding: 11px 12px; color: var(--text); font-size: 0.92rem; outline: none;
    }
    input[type="text"]:focus, input[type="password"]:focus { border-color: var(--accent); }
    .row { display: flex; gap: 8px; }
    .row > input { flex: 1; }
    button, .btn {
      border: none; border-radius: 10px; padding: 11px 14px; font-weight: 700; font-size: 0.88rem;
      cursor: pointer; background: var(--accent); color: #0b0f19; white-space: nowrap;
    }
    button.secondary { background: transparent; color: var(--text); border: 1px solid var(--border); }
    button.danger { background: rgba(239,68,68,0.15); color: #f87171; border: 1px solid rgba(239,68,68,0.35); }
    button.full { width: 100%; margin-top: 18px; padding: 13px; font-size: 0.95rem; }
    .hint { font-size: 0.76rem; color: var(--muted); margin-top: 6px; line-height: 1.45; }
    .nets { display: flex; flex-direction: column; gap: 7px; margin-top: 8px; max-height: 190px; overflow-y: auto; }
    .net { display: flex; justify-content: space-between; gap: 10px; background: #0b0f19; border: 1px solid var(--border); border-radius: 9px; padding: 9px 11px; font-size: 0.83rem; cursor: pointer; }
    .net:hover { border-color: var(--accent); }
    .net span:last-child { color: var(--muted); white-space: nowrap; }
    .static-box { display: none; margin-top: 4px; }
    .static-box.show { display: block; }
    .checkline { display: flex; align-items: center; gap: 8px; margin-top: 14px; font-size: 0.86rem; color: var(--text); }
    .checkline input { width: auto; }
    .split { display: grid; grid-template-columns: 1fr 1fr; gap: 8px; }
    @media (max-width: 420px) { .split { grid-template-columns: 1fr; } }
    .msg { font-size: 0.82rem; margin-top: 10px; color: var(--warn); }
    .actions { display: flex; gap: 8px; margin-top: 16px; flex-wrap: wrap; }
    .actions a, .actions form { flex: 1; }
    .actions .btn, .actions button { width: 100%; }
    a { color: var(--accent); text-decoration: none; }
    footer { text-align: center; color: var(--muted); font-size: 0.78rem; margin-top: 18px; line-height: 1.6; }
  </style>
</head>
<body>
  <div class="wrap">
    <header>
      <h1>Wi-Fi Setup</h1>
      <p>Smart Home Pro &bull; ESP32 4-Channel Controller</p>
    </header>

    <div class="card">
      <div class="status">
        <div class="dot" id="status-dot"></div>
        <div>
          <div id="status-title">Checking network...</div>
          <div class="muted" id="status-detail"></div>
        </div>
      </div>
    </div>

    <div class="card">
      <form method="POST" action="/save_wifi" autocomplete="off">
        <label for="ssid">Wi-Fi network name (2.4 GHz only)</label>
        <div class="row">
          <input id="ssid" name="ssid" type="text" list="networks" maxlength="32" placeholder="Your home Wi-Fi" required>
          <button type="button" class="secondary" onclick="scan()">Scan</button>
        </div>
        <datalist id="networks"></datalist>
        <div class="nets" id="net-list"><div class="hint">Tap Scan to list nearby 2.4 GHz networks.</div></div>
        <p class="hint">The ESP32 cannot see 5 GHz networks. If your router uses one name for both bands, split them or use the 2.4 GHz name.</p>

        <label for="pass">Wi-Fi password</label>
        <div class="row">
          <input id="pass" name="pass" type="password" maxlength="64" placeholder="Leave blank for an open network">
          <button type="button" class="secondary" onclick="togglePass()">Show</button>
        </div>

        <div class="checkline">
          <input type="checkbox" id="static" name="static" value="1" onchange="toggleStatic()">
          <label for="static" style="margin:0;color:var(--text);font-weight:600;">Use a fixed (static) IP address</label>
        </div>
        <div class="static-box" id="static-box">
          <p class="hint">Use this only if you know your network. A wrong static IP can make the device unreachable.</p>
          <div class="split">
            <div><label for="ip">IP address</label><input id="ip" name="ip" type="text" placeholder="192.168.1.100"></div>
            <div><label for="gw">Gateway / router</label><input id="gw" name="gw" type="text" placeholder="192.168.1.1"></div>
            <div><label for="mask">Subnet mask</label><input id="mask" name="mask" type="text" placeholder="255.255.255.0"></div>
            <div><label for="dns">DNS (optional)</label><input id="dns" name="dns" type="text" placeholder="192.168.1.1"></div>
          </div>
        </div>

        <button type="submit" class="full">Save &amp; Connect</button>
        <div class="msg" id="msg"></div>
      </form>
    </div>

    <div class="card">
      <div class="hint" style="margin:0 0 12px;">
        After saving, the device restarts and joins your Wi-Fi. Reconnect this phone to your home Wi-Fi,
        then open <strong id="dash-url">http://smarthome.local</strong> or the IP shown in your router.
        Voice control needs internet; local controls do not.
      </div>
      <div class="actions">
        <a class="btn" href="/" id="dash-link">Open Dashboard</a>
        <form method="POST" action="/forget_wifi" onsubmit="return confirm('Forget the saved Wi-Fi and restart into the setup hotspot?');">
          <button type="submit" class="danger">Forget Wi-Fi</button>
        </form>
      </div>
    </div>

    <footer>
      Setup hotspot: <strong>SmartHome-Setup</strong> (open) &bull; Fallback hotspot: <strong>SmartHome-Fallback</strong> (default password <strong>smarthome1234</strong>; change it in Settings).<br>
      Only 2.4 GHz Wi-Fi is supported by the ESP32.
    </footer>
  </div>

  <script>
    async function loadInfo() {
      try {
        const r = await fetch('/wifi_info', { cache: 'no-store' });
        const d = await r.json();
        const dot = document.getElementById('status-dot');
        const title = document.getElementById('status-title');
        const detail = document.getElementById('status-detail');
        if (d.connected) {
          dot.classList.add('online');
          title.innerText = 'Connected to ' + d.ssid;
          detail.innerText = 'IP ' + d.ip + ' • Signal ' + d.rssi + '% • ' + d.hostname + '.local';
        } else if (d.mode === 'AP_SETUP') {
          title.innerText = 'Setup hotspot active';
          detail.innerText = 'Join "SmartHome-Setup", then choose your Wi-Fi below.';
        } else {
          title.innerText = d.mode === 'AP_FALLBACK' ? 'Fallback hotspot active' : 'Not connected';
          detail.innerText = 'Home Wi-Fi unavailable. You can still configure it below.';
        }
        if (!d.connected && d.configured) {
          const err = (d.lastErrorName || '').toUpperCase();
          let hint = '';
          if (err.indexOf('NO_AP_FOUND') >= 0) hint = ' Network not found — check 2.4 GHz and the SSID.';
          else if (err.indexOf('AUTH') >= 0 || err.indexOf('HANDSHAKE') >= 0) hint = ' Authentication failed — check the password and WPA2 mode.';
          if (err && err !== 'UNKNOWN') detail.innerText += ' Last error: ' + err + '.' + hint;
        }
        if (d.ssid) document.getElementById('ssid').value = d.ssid;
        document.getElementById('static').checked = !!d.useStatic;
        if (d.useStatic) {
          document.getElementById('ip').value = d.ipcfg || '';
          document.getElementById('gw').value = d.gw || '';
          document.getElementById('mask').value = d.mask || '';
          document.getElementById('dns').value = d.dns || '';
        }
        toggleStatic();
        const dash = document.getElementById('dash-link');
        if (d.connected && d.mode !== 'AP_SETUP') {
          document.getElementById('dash-url').innerText = 'http://' + d.hostname + '.local';
          dash.href = '/';
          dash.innerText = 'Open Dashboard';
          dash.style.opacity = '1';
          dash.style.pointerEvents = 'auto';
        } else {
          dash.href = '#';
          dash.innerText = d.connected ? 'Reconnect phone to home Wi-Fi' : 'Dashboard after Wi-Fi';
          dash.style.opacity = '0.45';
          dash.style.pointerEvents = 'none';
        }
      } catch (e) {
        document.getElementById('status-title').innerText = 'Could not read device status.';
      }
    }

    async function scan() {
      const list = document.getElementById('net-list');
      list.innerHTML = '<div class="hint">Scanning... this can take a few seconds.</div>';
      try {
        const r = await fetch('/scan', { cache: 'no-store' });
        const d = await r.json();
        if (!d.networks || !d.networks.length) {
          list.innerHTML = '<div class="hint">No networks found. Make sure your 2.4 GHz Wi-Fi is on, then scan again.</div>';
          return;
        }
        const datalist = document.getElementById('networks');
        datalist.innerHTML = '';
        list.innerHTML = '';
        d.networks.forEach(n => {
          const opt = document.createElement('option');
          opt.value = n.ssid;
          datalist.appendChild(opt);
          const row = document.createElement('div');
          row.className = 'net';
          const nm = document.createElement('span');
          nm.textContent = n.ssid + (n.secure ? ' 🔒' : '');
          const sig = document.createElement('span');
          sig.textContent = n.rssi + ' dBm';
          row.appendChild(nm); row.appendChild(sig);
          row.onclick = () => { document.getElementById('ssid').value = n.ssid; };
          list.appendChild(row);
        });
      } catch (e) {
        list.innerHTML = '<div class="hint">Scan failed. You can still type the network name below.</div>';
      }
    }

    function togglePass() {
      const p = document.getElementById('pass');
      p.type = p.type === 'password' ? 'text' : 'password';
    }

    function toggleStatic() {
      document.getElementById('static-box').classList.toggle('show', document.getElementById('static').checked);
    }

    document.getElementById('dash-link').href = '/';
    loadInfo();
  </script>
</body>
</html>
)rawliteral";
