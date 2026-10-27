"use strict";
const TOKEN = new URLSearchParams(location.search).get("token") || "";
const $ = (s) => document.querySelector(s);
const el = (tag, attrs = {}, ...kids) => {
  const e = document.createElement(tag);
  for (const [k, v] of Object.entries(attrs)) {
	if (k === "class") e.className = v;
	else if (k.startsWith("on")) e.addEventListener(k.slice(2), v);
	else if (v !== false && v != null) e.setAttribute(k, v === true ? "" : v);
  }
  for (const k of kids) if (k != null) e.append(k);
  return e;
};

async function api(path, body) {
  const opt = { headers: { "X-DevTool-Token": TOKEN } };
  if (body !== undefined) { opt.method = "POST"; opt.body = JSON.stringify(body); opt.headers["Content-Type"] = "application/json"; }
  const r = await fetch(path, opt);
  const data = await r.json().catch(() => ({}));
  if (!r.ok) throw new Error(data.message || data.error || r.statusText);
  return data;
}

let model = null;
let ui = { presets: [], configs: ["Debug"], samples: [], overrides: {}, extra: [] };

function banner(text, error) {
  const b = $("#banner");
  b.textContent = text || "";
  b.className = "banner" + (error ? " error" : "") + (text ? "" : " hidden");
}

function save() { api("/api/state", ui).catch(() => {}); updateButtons(); }

// ------------------------------------------------------------------ rendering
function render() {
  const e = model.environment;
  let host = `${model.host} | ${model.repo}`;
  if (e.visualStudio) host += ` | VS ${e.visualStudio.version}`;
  if (e.wsl) host += ` | WSL: ${e.wsl.userReady ? e.wsl.distroName : "not ready"}`;
  $("#hostInfo").textContent = host;
  const px = model.proxy;
  $("#proxyInfo").textContent = px.source === "direct" ? "Proxy: direct" : `Proxy (${px.source}): ${px.https || px.http}`;
  if (model.setupResume.pending)
	banner(model.setupResume.restartRequired ? "Setup paused for a restart. Run Setup again to continue." :
	  "Setup is waiting for a manual step. Open Setup to continue.");

  const groups = {};
  for (const p of model.presets) (groups[p.group] ||= []).push(p);
  const gc = $("#presetGroups"); gc.replaceChildren();
  for (const g of ["Windows", "Linux", "Android", ...Object.keys(groups).filter(k => !["Windows","Linux","Android"].includes(k))]) {
	const list = groups[g] || [];
	const box = el("div", { class: "box" }, el("h2", {}, g));
	if (!list.length) box.append(el("div", { class: "hint" }, g === "Android" ? "No Android presets defined yet." : "No presets."));
	for (const p of list) {
	  const st = p.state || {};
	  const badge = st.generated ? el("span", { class: "badge " + (st.stale ? "stale" : "ok"), title: st.stale ? "Presets/options changed since last generate" : "Generated" }, st.stale ? "stale" : "generated")
		: (st.ok === false ? el("span", { class: "badge fail" }, "gen failed") : null);
	  const cb = el("input", { type: "checkbox", disabled: !p.available, checked: ui.presets.includes(p.name),
		onchange: (ev) => { toggle(ui.presets, p.name, ev.target.checked); save(); } });
	  const openSln = p.isVS && st.generated && st.solution
		? el("a", { class: "badge link", title: st.solution,
			onclick: (ev) => { ev.preventDefault(); ev.stopPropagation(); act(() => api("/api/open", { presets: [p.name] })); } }, "open solution")
		: null;
	  box.append(el("label", { class: p.available ? "" : "disabled", title: p.reason || p.name }, cb,
		`${p.label}${p.mode === "wsl" ? " (WSL)" : ""}`, badge, openSln));
	}
	const rhis = [...new Set(list.flatMap(p => p.rhis || []))];
	if (rhis.length) {
	  const defs = [...new Set(list.map(p => p.defaultRhi).filter(Boolean))];
	  ui.rhi ||= {};
	  const rs = el("select", { onchange: (ev) => { if (ev.target.value) ui.rhi[g] = ev.target.value; else delete ui.rhi[g]; save(); } },
		...rhis.map(r => defs.length === 1 && r === defs[0]
		  ? el("option", { value: "" }, `${r} - default for platform`)
		  : el("option", { value: r }, r)));
	  if (defs.length !== 1) rs.prepend(el("option", { value: "" }, `${defs.join("/") || "?"} - default for platform`));
	  rs.value = rhis.includes(ui.rhi[g]) && !(defs.length === 1 && ui.rhi[g] === defs[0]) ? ui.rhi[g] : "";
	  box.append(el("label", { class: "rhi", title: "VKE_RENDER_SYSTEM passed to presets of this platform" }, "Default RHI", rs));
	}
	gc.append(box);
  }

  const cc = $("#configs"); cc.replaceChildren();
  for (const c of model.configs)
	cc.append(el("label", {}, el("input", { type: "checkbox", checked: ui.configs.includes(c),
	  onchange: (ev) => { toggle(ui.configs, c, ev.target.checked); save(); } }), c));

  const sc = $("#samples"); sc.replaceChildren();
  for (const s of model.samples)
	sc.append(el("label", { title: s.excludeFromAll ? "Excluded from ALL" : "" }, el("input", { type: "checkbox", checked: ui.samples.includes(s.target),
	  onchange: (ev) => { toggle(ui.samples, s.target, ev.target.checked); save(); } }), s.name));

  renderOptions(); renderExtra(); updateButtons();
}

function renderOptions() {
  const f = $("#optFilter").value.toLowerCase();
  const tb = $("#options tbody"); tb.replaceChildren();
  for (const o of model.options) {
	if (f && !o.name.toLowerCase().includes(f) && !o.description.toLowerCase().includes(f)) continue;
	const sel = el("select", { onchange: (ev) => { if (ev.target.value) ui.overrides[o.name] = ev.target.value; else delete ui.overrides[o.name]; save(); } },
	  el("option", { value: "" }, "Default"), el("option", { value: "ON" }, "ON"), el("option", { value: "OFF" }, "OFF"));
	sel.value = ui.overrides[o.name] || "";
	const defCls = { ON: "def-on", OFF: "def-off" }[String(o.default).toUpperCase()] || "";
	const defTitle = o.presetValues ? "Set by preset: " + o.presetValues.join(" / ") : "";
	const row = el("tr", {}, el("td", {}, o.name), el("td", {}, o.description), el("td", { class: defCls, title: defTitle }, o.default), el("td", {}, sel));
	tb.append(row);
  }
}

function renderExtra() {
  const c = $("#extra"); c.replaceChildren();
  ui.extra.forEach((x, i) => {
	const upd = () => save();
	c.append(el("div", {},
	  el("input", { value: x.name, placeholder: "NAME", oninput: (ev) => { x.name = ev.target.value; upd(); } }), " = ",
	  el("input", { value: x.value, placeholder: "value", oninput: (ev) => { x.value = ev.target.value; upd(); } }), " ",
	  el("button", { class: "small", onclick: () => { ui.extra.splice(i, 1); renderExtra(); save(); } }, "Remove")));
  });
}

function toggle(arr, v, on) { const i = arr.indexOf(v); if (on && i < 0) arr.push(v); if (!on && i >= 0) arr.splice(i, 1); }

function selected() { return model.presets.filter(p => ui.presets.includes(p.name) && p.available); }

function updateButtons() {
  if (!model) return;
  const sel = selected();
  $("#btnGenerate").disabled = !sel.length;
  const gen = sel.filter(p => p.state && p.state.generated);
  $("#btnBuild").disabled = !gen.length || !ui.configs.length;
  $("#btnBuild").title = gen.length < sel.length ? "Some selected presets are not generated and will be skipped." : "";
}

// ------------------------------------------------------------------ tasks
const fmt = (s) => { s = Math.round(s); return `${Math.floor(s / 60)}:${String(s % 60).padStart(2, "0")}`; };
let lastStates = "";
let lostConnection = false;
async function pollTasks() {
  try {
	const d = await api("/api/tasks");
	const tb = $("#tasks tbody"); tb.replaceChildren();
	for (const t of d.tasks.slice().reverse()) {
	  tb.append(el("tr", {}, el("td", {}, String(t.id)), el("td", {}, t.title),
		el("td", { class: "state-" + t.state, title: t.error || "" }, t.state + (t.exitCode != null && t.state === "failed" ? ` (${t.exitCode})` : "")),
		el("td", {}, fmt(t.elapsed)),
		el("td", { class: "actions" },
		  el("a", { onclick: () => act(() => api("/api/tasks/openlog", { id: t.id })) }, "open log"),
		  t.kind === "build" ? el("a", { onclick: () => act(() => api("/api/tasks/openbin", { id: t.id })) }, "open") : null,
		  ["waiting", "running"].includes(t.state)
			? el("a", { onclick: () => act(() => api("/api/tasks/cancel", { id: t.id })) }, "cancel")
			: el("a", { onclick: () => act(() => api("/api/tasks/remove", { id: t.id })) }, "delete"))));
	}
	const states = d.tasks.map(t => t.id + t.state).join();
	if (states !== lastStates) {
	  const finished = lastStates && d.tasks.some(t => !["waiting", "running"].includes(t.state));
	  lastStates = states;
	  if (finished) await refreshModel();
	}
	if (lostConnection) { lostConnection = false; banner(""); await refreshModel(); }
  } catch (e) {
	lostConnection = true;
	banner("Lost connection to DevTool server (" + e.message + "). Retrying... Restart it with devtool.cmd / devtool.sh if it was closed.", true);
  }
  setTimeout(pollTasks, lostConnection ? 5000 : 1500);
}

// ------------------------------------------------------------------ actions
async function act(fn) { try { banner(""); await fn(); } catch (e) { banner(e.message, true); } }

$("#btnGenerate").onclick = () => act(async () => {
  await api("/api/generate", { presets: selected().map(p => p.name), overrides: ui.overrides, extra: ui.extra, rhi: ui.rhi || {} });
});
$("#btnBuild").onclick = () => act(async () => {
  const r = await api("/api/build", { presets: selected().map(p => p.name), configs: ui.configs, samples: ui.samples });
  if (r.skipped.length) banner("Skipped (not generated): " + r.skipped.join(", "));
});
$("#btnClear").onclick = () => act(() => api("/api/tasks/clear", {}));
$("#btnReload").onclick = () => act(async () => { model = await api("/api/reload", {}); render(); });
$("#btnAddExtra").onclick = () => { ui.extra.push({ name: "", value: "" }); renderExtra(); };
$("#optFilter").oninput = renderOptions;

// ------------------------------------------------------------------ setup dialog
let setupData = null;
$("#btnSetup").onclick = () => { $("#setupDlg").showModal(); setupCheck(); };
$("#btnSetupClose").onclick = () => $("#setupDlg").close();
$("#btnSetupRecheck").onclick = () => setupCheck();
$("#btnSetupGate").onclick = () => act(async () => { await api("/api/setup/gate", { id: "wsl-user" }); await refreshModel(true); setupCheck(); });
$("#btnSetupRun").onclick = () => act(async () => {
  const ids = [...document.querySelectorAll("#setupTable input:checked")].map(c => c.value);
  if (!ids.length) return;
  await api("/api/setup/run", { ids });
  $("#setupDlg").close();
});

async function setupCheck() {
  const tb = $("#setupTable tbody");
  tb.replaceChildren(el("tr", {}, el("td", { colspan: 6 }, "Loading...")));
  $("#setupProxy").replaceChildren(el("div", {}, "Checking network... ", el("a", { onclick: openProxy }, "Proxy settings")));
  api("/api/proxy/test").then(px => {
	const bad = px.results.filter(r => !r.ok);
	$("#setupProxy").replaceChildren(el("div", { class: bad.length ? "st-failed" : "st-ok" },
	  bad.length ? `Network check failed: ${bad.map(b => b.url + " - " + b.error).join("; ")}. ` : "Network OK. ",
	  el("a", { onclick: openProxy }, "Proxy settings")));
  }).catch(e => $("#setupProxy").replaceChildren(el("div", { class: "st-failed" }, e.message)));
  try {
	setupRender(await api("/api/setup/check?quick=1"));
	setupRender(await api("/api/setup/check"));
  } catch (e) { tb.replaceChildren(el("tr", {}, el("td", { colspan: 6, class: "st-failed" }, e.message))); }
}

function setupRender(d) {
  const tb = $("#setupTable tbody");
  setupData = d;
  {
	tb.replaceChildren();
	const prog = d.progress.steps || {};
	for (const x of d.dependencies) {
	  const pending = x.status !== "ok" && x.status !== "checking";
	  const runState = prog[x.id] ? prog[x.id].status : "";
	  tb.append(el("tr", {},
		el("td", {}, el("input", { type: "checkbox", value: x.id, checked: pending && x.status !== "blocked" && !x.gate, disabled: !pending || x.status === "blocked" || x.gate })),
		el("td", {}, x.name), el("td", {}, x.group),
		el("td", { class: "st-" + x.status }, x.status),
		el("td", {}, x.version), el("td", {}, x.detail)));
	}
	const gate = d.dependencies.find(x => x.gate && x.status === "missing");
	$("#btnSetupGate").classList.toggle("hidden", !gate);
  }
}

// ------------------------------------------------------------------ proxy dialog
function openProxy() { $("#pxResults").textContent = ""; $("#proxyDlg").showModal(); }
$("#proxyInfo").onclick = openProxy;
$("#btnPxClose").onclick = () => $("#proxyDlg").close();
$("#btnPxSave").onclick = () => act(async () => {
  await api("/api/proxy", { http: $("#pxHttp").value, https: $("#pxHttps").value, noProxy: $("#pxNo").value });
  await refreshModel();
});
$("#btnPxTest").onclick = () => act(async () => {
  $("#pxResults").textContent = "Testing...";
  const d = await api("/api/proxy/test");
  $("#pxResults").textContent = d.results.map(r => `${r.ok ? "OK  " : "FAIL"} ${r.url} ${r.error}`).join("\n");
});

// ------------------------------------------------------------------ init
async function refreshModel(reload) {
  model = reload ? await api("/api/reload", {}) : await api("/api/model");
  render();
}

(async function init() {
  if (!TOKEN) { banner("Missing access token. Start the DevTool using devtool.cmd / devtool.sh.", true); return; }
  try {
	model = await api("/api/model");
	const c = await api("/api/state");
	if (c.state) ui = Object.assign(ui, c.state);
	else ui.configs = model.configs.slice(0, 1);
	render();
	if (c.changed) banner("CMakePresets.json / CompileOptions.cmake / samples changed since last session; invalid selections were dropped.");
	pollTasks();
  } catch (e) { banner(e.message, true); }
})();
