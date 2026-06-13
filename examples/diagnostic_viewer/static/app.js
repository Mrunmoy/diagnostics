const state = {
  events: [],
  timer: null,
};

const ids = {
  connectionState: document.getElementById("connectionState"),
  healthText: document.getElementById("healthText"),
  activeCount: document.getElementById("activeCount"),
  confirmedCount: document.getElementById("confirmedCount"),
  storedBytes: document.getElementById("storedBytes"),
  identityList: document.getElementById("identityList"),
  dtcList: document.getElementById("dtcList"),
  eventLog: document.getElementById("eventLog"),
  refreshButton: document.getElementById("refreshButton"),
};

function addEvent(text) {
  const timestamp = new Date().toLocaleTimeString();
  state.events.unshift(`${timestamp} ${text}`);
  state.events = state.events.slice(0, 8);
  ids.eventLog.replaceChildren(
    ...state.events.map((entry) => {
      const item = document.createElement("li");
      item.textContent = entry;
      return item;
    }),
  );
}

async function fetchJson(url, options = {}) {
  const response = await fetch(url, {
    cache: "no-store",
    ...options,
  });
  if (!response.ok) {
    throw new Error(await response.text());
  }
  return response.json();
}

function renderIdentity(identity) {
  const fields = [
    ["Ecosystem", identity.ecosystem_id],
    ["Product", identity.product_id],
    ["Device Type", identity.device_type],
    ["Instance", identity.device_instance],
    ["Firmware Stage", identity.firmware_stage],
    ["Component", identity.firmware_component],
  ];

  const nodes = [];
  fields.forEach(([label, value]) => {
    const term = document.createElement("dt");
    const description = document.createElement("dd");
    term.textContent = label;
    description.textContent = value;
    nodes.push(term, description);
  });
  ids.identityList.replaceChildren(...nodes);
}

function badge(text, className = "") {
  const node = document.createElement("span");
  node.className = `badge ${className}`.trim();
  node.textContent = text;
  return node;
}

function renderDtcs(dtcs) {
  const cards = dtcs.map((dtc) => {
    const card = document.createElement("article");
    card.className = `dtc-card ${dtc.severity}`;

    const main = document.createElement("div");
    main.className = "dtc-main";

    const text = document.createElement("div");
    const title = document.createElement("div");
    const code = document.createElement("div");
    title.className = "dtc-title";
    title.textContent = dtc.title;
    code.className = "dtc-id";
    code.textContent = `${dtc.id} status=0x${dtc.status.toString(16).padStart(2, "0")}`;
    text.append(title, code);

    const clear = document.createElement("button");
    clear.type = "button";
    clear.textContent = "Clear";
    clear.disabled = !dtc.active && !dtc.confirmed && dtc.status === 0;
    clear.addEventListener("click", () => clearDtc(dtc.id));

    main.append(text, clear);

    const badges = document.createElement("div");
    badges.className = "dtc-badges";
    badges.append(
      badge(dtc.severity),
      badge(dtc.active ? "active" : "inactive", dtc.active ? "active" : ""),
      badge(dtc.confirmed ? "confirmed" : "unconfirmed", dtc.confirmed ? "confirmed" : ""),
      badge(`${dtc.occurrences} occurrences`),
    );

    card.append(main, badges);
    return card;
  });

  ids.dtcList.replaceChildren(...cards);
}

function render(snapshot) {
  const healthLabel =
    snapshot.summary.health === "service_required" ? "Service required" : "Nominal";

  ids.connectionState.textContent = "Connected";
  ids.healthText.textContent = healthLabel;
  ids.healthText.className = snapshot.summary.health.replace("_", "-");
  ids.activeCount.textContent = snapshot.summary.active_dtcs;
  ids.confirmedCount.textContent = snapshot.summary.confirmed_dtcs;
  ids.storedBytes.textContent = `${snapshot.summary.stored_bytes} bytes`;
  renderIdentity(snapshot.identity);
  renderDtcs(snapshot.dtcs);
}

async function refresh(manual = false) {
  try {
    const snapshot = await fetchJson("/api/snapshot");
    render(snapshot);
    if (manual) {
      addEvent("manual refresh complete");
    }
  } catch (error) {
    ids.connectionState.textContent = "Disconnected";
    addEvent(`poll failed: ${error.message.trim()}`);
  }
}

async function clearDtc(dtcId) {
  try {
    const snapshot = await fetchJson("/api/clear", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ dtc_id: dtcId }),
    });
    render(snapshot);
    addEvent(`clear requested for ${dtcId}`);
  } catch (error) {
    addEvent(`clear failed for ${dtcId}: ${error.message.trim()}`);
  }
}

ids.refreshButton.addEventListener("click", () => refresh(true));
refresh(true);
state.timer = window.setInterval(() => refresh(false), 1000);
