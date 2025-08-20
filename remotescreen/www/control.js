// UI
const subtitleDiv = document.querySelector(".subtitle");
const show = (msg) => {
  if (subtitleDiv) subtitleDiv.innerHTML = msg;
};
const showError = (msg) => show(`❌ ${msg}`);

// Parse hash → { wsPort, uuid }
function parseHash(hash) {
  const raw = (hash || "").replace(/^#/, "").trim();
  if (!raw) return {};
  const asParams = new URLSearchParams(raw);
  const wsPortStr = asParams.get("wsPort");
  const uuid = asParams.get("uuid") || undefined;
  const wsPort = /^\d+$/.test(wsPortStr || "") ? Number(wsPortStr) : undefined;
  return { wsPort, uuid };
}

const { wsPort, uuid } = parseHash(location.hash);
const scheme = location.protocol === "https:" ? "wss" : "ws";
const port = wsPort ?? 8765;
const wsUrl = `${scheme}://${location.hostname}:${port}/`;

// Backoff settings
let ws = null;
let reconnectAttempts = 0;
let reconnectTimer = null;
let intentionalClose = false;

const minDelay = 500; // ms
const maxDelay = 10000; // ms
const jitter = 0.25; // 25% jitter

function nextDelay() {
  const base = Math.min(maxDelay, minDelay * Math.pow(2, reconnectAttempts));
  const rand = base * jitter * (Math.random() * 2 - 1); // +/- jitter
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

function connect() {
  if (!uuid) {
    showError("Missing uuid in URL");
    return;
  }
  clearTimeout(reconnectTimer);
  show("Connecting…");

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
    ws.send(JSON.stringify({ uuid }));
    // Clear status
    show("");
  };

  ws.onerror = () => {
    // Let onclose drive the reconnect so we don’t double schedule
    showError("Connection error");
  };

  ws.onclose = () => {
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
    const data = payload.data;

    if (type === "error") {
      showError(String(data ?? "Unknown error"));
    } else if (type === "ok") {
      show(data ?? "");
    } else if (type === "add-subtitle") {
      show(data ?? "");
    } else {
      show("");
    }
  };
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

// Kick it off
connect();
