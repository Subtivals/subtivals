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

document.addEventListener("DOMContentLoaded", () => {
  // Kick it off
  connect();
});

function show(msg) {
  document.querySelector("#main").innerHTML = msg;
}

function showError(msg) {
  show(`❌ ${msg}`);
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
    } else if (type === "error") {
      showError(String(content ?? "Unknown error"));
    } else if (type === "ok") {
      show("✅ Connected");
    } else if (type === "movie-started") {
      document.title = payload.title;
    } else if (type === "clear") {
      document.title = "Remote Subtivals";
      show("");
    } else if (type === "add-subtitle") {
      show(content ?? "");
    } else if (type === "rem-subtitle") {
      show("");
    } else {
      console.error("Unknown event type", type, content);
    }
  };
}
