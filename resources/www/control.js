const MIN_DELAY_MS = 500; // ms
const MAX_DELAY_MS = 10000; // ms
const JITTER = 0.25; // 25% jitter
const PING_INTERVAL_MS = 5000;

let ws = null;

// Backoff settings
let reconnectAttempts = 0;
let reconnectTimer = null;
let intentionalClose = false;

let pingInterval = null;
let waitingForPong = false;

let totalDuration = null;
let state = null;

document.addEventListener("DOMContentLoaded", () => {
  // Kick it off
  connect();

  // Bind buttons to WS actions
  for (const action of ["play", "subDelay", "addDelay"]) {
    document.getElementById(`btn-${action}`).addEventListener("click", () => {
      if (!ws || ws.readyState !== WebSocket.OPEN) {
        return;
      }
      const sendAction = (action === "play" && state == "PLAYING") ? "pause" : action;
      console.log(`Sending ${sendAction}`)
      ws.send(JSON.stringify({action: sendAction}));
    });
  }
});

function show(msg) {
  document.querySelector("#main").innerHTML = msg;
}

function showError(msg) {
  show(`❌ ${msg}`);
}

function ts2tc(p_ts) {
    const sign = p_ts >= 0 ? "+" : "-";
    const ms = Math.abs(p_ts);

    const hours = Math.floor(ms / 3600000);
    const minutes = Math.floor((ms % 3600000) / 60000);
    const seconds = Math.floor((ms % 60000) / 1000);

    // Pad with zeros if needed
    const hh = String(hours).padStart(2, '0');
    const mm = String(minutes).padStart(2, '0');
    const ss = String(seconds).padStart(2, '0');

    return sign + `${hh}:${mm}:${ss}`;
}

function showProgress(elapsed, total) {
  const elapsedHuman = ts2tc(elapsed);
  const percent = totalDuration ? (elapsed / totalDuration * 100) : 0;
  document.getElementById("bar").style.width = `${percent}%`;
  document.getElementById("elapsed").innerHTML = elapsedHuman;
}

function parseHash(hash) {
  // Parse hash → { wsPort }
  const raw = (hash || "").replace(/^#/, "").trim();
  if (!raw) return {};
  const asParams = new URLSearchParams(raw);
  const wsPortStr = asParams.get("wsPort");
  const wsPort = /^\d+$/.test(wsPortStr || "") ? Number(wsPortStr) : undefined;
  return { wsPort };
}

function nextDelay() {
  const base = Math.min(
    MAX_DELAY_MS,
    MIN_DELAY_MS * Math.pow(2, reconnectAttempts)
  );
  const rand = base * JITTER * (Math.random() * 2 - 1); // +/- jitter
  return Math.max(250, Math.round(base + rand));
}

function scheduleReconnect(reason) {
  if (intentionalClose) return;
  if (!navigator.onLine) {
    showError("Offline");
    return; // wait for 'online' event
  }
  const delay = nextDelay();
  reconnectAttempts++;
  showError(`${reason}. Reconnecting in ${Math.round(delay / 1000)}s…`);
  clearTimeout(reconnectTimer);
  reconnectTimer = setTimeout(connect, delay);
}

function sendPing() {
  if (!ws || ws.readyState !== WebSocket.OPEN || waitingForPong) return;

  waitingForPong = true;
  ws.send(JSON.stringify({
    action: "ping",
    content: Date.now(),
  }));
}

// Auto-reconnect when the network comes back
window.addEventListener("online", () => {
  if (!intentionalClose && (!ws || ws.readyState === WebSocket.CLOSED)) {
    connect();
  }
});

// Optional: attempt again when the tab becomes visible
document.addEventListener("visibilitychange", () => {
  if (
    document.visibilityState === "visible" &&
    !intentionalClose &&
    (!ws || ws.readyState === WebSocket.CLOSED)
  ) {
    connect();
  }
});

function connect() {
  const { wsPort } = parseHash(location.hash);
  const scheme = location.protocol === "https:" ? "wss" : "ws";
  const port = wsPort ?? 8765;
  const wsUrl = `${scheme}://${location.hostname}:${port}/`;

  clearTimeout(reconnectTimer);
  show("Connecting…");

  const passphrase = window.prompt("Passphrase?");

  try {
    ws = new WebSocket(wsUrl);
  } catch (e) {
    scheduleReconnect("Connection error");
    return;
  }

  ws.onopen = () => {
    // Reset backoff
    reconnectAttempts = 0;
    // Send handshake
    ws.send(JSON.stringify({
      action: "auth",
      content: passphrase,
    }));
    // Clear status
    show("");
    // Start pinging every X seconds
    clearInterval(pingInterval);
    pingInterval = setInterval(sendPing, PING_INTERVAL_MS);
  };

  ws.onerror = () => {
    // Let onclose drive the reconnect so we don’t double schedule
    showError("Connection error");
  };

  ws.onclose = () => {
    clearInterval(pingInterval);
    if (!intentionalClose) scheduleReconnect("Disconnected");
  };

  ws.onmessage = (ev) => {
    console.debug("Received message:", ev.data);
    let payload;
    try {
      payload = JSON.parse(ev.data);
    } catch {
      showError("Invalid data");
      return;
    }

    if (
      !payload ||
      typeof payload !== "object" ||
      typeof payload["event-type"] !== "string"
    ) {
      showError("Invalid data");
      return;
    }

    const type = payload["event-type"];
    const content = payload.content;

    if (type === "pong" && content) {
      const rtt = Date.now() - content;
      console.log(`Ping RTT: ${rtt} ms`);
      waitingForPong = false;
      document.getElementById("ping").innerHTML = `Ping: ${rtt} msec`;
    } else if (type === "error") {
      showError(String(content ?? "Unknown error"));
    } else if (type === "ok") {
      show("✅ Connected");
    } else if (type === "state-info") {
      document.title = payload.title;
      totalDuration = payload.totalDuration;

      state = payload.state;

      const humanDelay = {
        100: "1/10 sec",
        250: "1/4 sec",
        500: "1/2 sec",
        1000: "1 sec",
      }[payload.delayMsec] || `${payload.delayMsec / 1000} sec`;
      document.getElementById("subDelay").innerHTML = `-${humanDelay}`;
      document.getElementById("addDelay").innerHTML = `+${humanDelay}`;

      if (state === "PLAYING") {
        document.getElementById("btn-play").setAttribute('aria-pressed','true');
        document.getElementById("playIcon").innerHTML = "❙❙";
        document.getElementById("playHint").innerHTML = "Pause";
      } else {
        document.getElementById("btn-play").setAttribute('aria-pressed','false');
        document.getElementById("playIcon").innerHTML = "▶";
        document.getElementById("playHint").innerHTML = "Play";
      }

      document.getElementById("movieTitle").innerHTML = payload.title || "No movie loaded";
      document.getElementById("presetName").innerHTML = `${payload.presetName}`;

    } else if (type === "play-pulse") {
      showProgress(payload.elapsed, totalDuration);
    } else if (type === "clear") {
      document.getElementById("subtitle").innerHTML = "";
    } else if (type === "add-subtitle") {
      document.getElementById("subtitle").innerHTML = content;
    } else if (type === "rem-subtitle") {
      document.getElementById("subtitle").innerHTML = "";
    } else {
      console.error("Unknown event type", type, content);
    }
  };
}
