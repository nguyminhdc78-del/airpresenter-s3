// 3D viewer for the AirPresenter enclosure: loads one STL per piece (exported from
// OpenSCAD in assembled position) and offers closed / open / exploded / section views,
// per-piece visibility, see-through shells and click-to-identify.
import * as THREE from 'three';
import { STLLoader } from 'three/addons/loaders/STLLoader.js';
import { CASE_SIZE, MODES, PIECES } from './viewer-piece-catalog.js';
import { createViewerScene, cssToken } from './viewer-scene-setup.js';
import { listenForPicks } from './viewer-pointer-picking.js';

const stage = document.getElementById('stage');
const statusEl = document.getElementById('status');
const infoEl = document.getElementById('info');
const listEl = document.getElementById('part-list');
const cutInput = document.getElementById('cut');
const cutValue = document.getElementById('cut-value');
const reducedMotion = matchMedia('(prefers-reduced-motion: reduce)').matches;
const { renderer, scene, camera, controls, model, sectionPlane, edgeMaterial, clippedEdgeMaterial } =
  createViewerScene(stage, CASE_SIZE, MODES.closed.camera);

// ---------- pieces ----------
const parts = new Map();  // id -> { piece, mesh, edges, row }
let mode = 'closed';
let selected = null;

// One group per layer, so a view can move or flip a whole layer (e.g. the lid).
const layers = {};
for (const name of ['bottom', 'parts', 'top', 'caps']) {
  layers[name] = new THREE.Group();
  layers[name].userData.pose = { x: 0, z: 0, flip: false };
  model.add(layers[name]);
}

function addPiece(piece, geometry) {
  const material = new THREE.MeshStandardMaterial({
    color: piece.color, roughness: 0.6, metalness: 0.05, side: THREE.DoubleSide,
  });
  const mesh = new THREE.Mesh(geometry, material);
  mesh.userData.id = piece.id;
  let edges = null;
  if (piece.id !== 'caps' && piece.id !== 'tact_switches') {  // engraved text = edge noise
    edges = new THREE.LineSegments(new THREE.EdgesGeometry(geometry, 25), edgeMaterial);
    mesh.add(edges);
  }
  layers[piece.layer].add(mesh);
  parts.get(piece.id).mesh = mesh;
  parts.get(piece.id).edges = edges;
}

function buildList() {
  for (const piece of PIECES) {
    const row = document.createElement('li');
    const toggle = Object.assign(document.createElement('input'), {
      type: 'checkbox', checked: true, id: `show-${piece.id}`,
    });
    toggle.setAttribute('aria-label', `Hiện ${piece.name}`);
    toggle.addEventListener('change', () => {
      const part = parts.get(piece.id);
      if (part.mesh) part.mesh.visible = toggle.checked;
    });
    const swatch = document.createElement('span');
    swatch.className = 'swatch';
    swatch.style.background = `#${piece.color.toString(16).padStart(6, '0')}`;
    const name = Object.assign(document.createElement('button'), { type: 'button', textContent: piece.name });
    name.addEventListener('click', () => select(piece.id));
    row.append(toggle, swatch, name);
    listEl.append(row);
    parts.set(piece.id, { piece, mesh: null, edges: null, row });
  }
}

async function loadPieces() {
  const loader = new STLLoader();
  let done = 0;
  try {
    await Promise.all(PIECES.map(async (piece) => {
      addPiece(piece, await loader.loadAsync(`models/${piece.id}.stl`));
      statusEl.textContent = `Đang tải mô hình ${++done}/${PIECES.length}…`;
    }));
    statusEl.hidden = true;
    const linked = location.hash.slice(1);  // e.g. #exploded opens that view directly
    setMode(MODES[linked] ? linked : 'closed');
  } catch (err) {
    statusEl.classList.add('error');
    statusEl.textContent = 'Không tải được mô hình. Hãy mở trang qua server '
      + '(python3 -m http.server trong thư mục viewer), không mở file trực tiếp.';
    console.error(err);
  }
}

// ---------- interactions ----------
function setMode(next) {
  mode = next;
  const view = MODES[next];
  history.replaceState(null, '', `#${next}`);
  for (const [name, group] of Object.entries(layers)) {
    group.userData.pose = { x: 0, z: 0, flip: false, ...view.layers[name] };
  }
  for (const { piece, mesh, edges } of parts.values()) {
    if (!mesh) continue;
    const clip = view.section && piece.shell;
    mesh.material.clippingPlanes = clip ? [sectionPlane] : [];
    if (edges) edges.material = clip ? clippedEdgeMaterial : edgeMaterial;
  }
  document.getElementById('section-controls').hidden = !view.section;
  for (const button of document.querySelectorAll('[data-mode]')) {
    button.setAttribute('aria-pressed', String(button.dataset.mode === next));
  }
  flyTo(view.camera, view.target);
}

function select(id) {
  if (selected) {
    const prev = parts.get(selected);
    prev.mesh?.material.emissive.setHex(0x000000);
    prev.row.classList.remove('selected');
  }
  selected = id;
  const heading = '<h2 class="label">Thông tin</h2>';
  if (!id) {
    infoEl.innerHTML = `${heading}<p class="muted">Bấm vào một chi tiết trên mô hình hoặc trong danh sách.</p>`;
    return;
  }
  const { piece, mesh, row } = parts.get(id);
  mesh?.material.emissive.set(cssToken('--accent')).multiplyScalar(0.35);
  row.classList.add('selected');
  const facts = piece.facts.map((fact) => `<li>${fact}</li>`).join('');
  infoEl.innerHTML = `${heading}<h3>${piece.name}</h3><ul>${facts}</ul>`;
}

function setSeeThrough(on) {
  for (const { piece, mesh } of parts.values()) {
    if (!piece.shell || !mesh) continue;
    Object.assign(mesh.material, { transparent: on, opacity: on ? 0.28 : 1, depthWrite: !on });
    mesh.material.needsUpdate = true;
  }
}

let flight = null;
function flyTo(position, target) {
  const to = new THREE.Vector3(...position);
  const toTarget = new THREE.Vector3(...target);
  if (reducedMotion) {
    camera.position.copy(to);
    controls.target.copy(toTarget);
    return;
  }
  flight = { from: camera.position.clone(), fromTarget: controls.target.clone(), to, toTarget, t: 0 };
}
controls.addEventListener('start', () => { flight = null; });  // the user takes over

// Clicking the model selects the piece under the pointer; see-through or cut-away
// shell surfaces are skipped so the parts behind them can be picked.
listenForPicks(renderer, camera,
  () => [...parts.values()].filter(({ mesh }) => mesh?.visible).map(({ mesh }) => mesh),
  ({ object, point }) => {
    const { piece } = parts.get(object.userData.id);
    if (piece.shell && document.getElementById('see-through').checked) return false;
    return !(piece.shell && MODES[mode].section && point.x > sectionPlane.constant);
  },
  (hit) => select(hit ? hit.object.userData.id : null));

for (const button of document.querySelectorAll('[data-mode]')) {
  button.addEventListener('click', () => setMode(button.dataset.mode));
}
document.getElementById('see-through').addEventListener('change', (e) => setSeeThrough(e.target.checked));
document.getElementById('reset-view').addEventListener('click', () => flyTo(MODES[mode].camera, MODES[mode].target));
cutInput.addEventListener('input', () => {
  sectionPlane.constant = Number(cutInput.value);
  cutValue.textContent = `x = ${cutInput.value} mm`;
});

// ---------- render loop ----------
function tick() {
  requestAnimationFrame(tick);
  if (flight) {
    flight.t = Math.min(1, flight.t + 0.035);
    const k = 1 - (1 - flight.t) ** 3;  // ease-out
    camera.position.lerpVectors(flight.from, flight.to, k);
    controls.target.lerpVectors(flight.fromTarget, flight.toTarget, k);
    if (flight.t >= 1) flight = null;
  }
  // Ease every layer toward its pose: offsets in mm, flip = turned over along the length.
  const ease = reducedMotion ? 1 : 0.12;
  for (const group of Object.values(layers)) {
    const { x, z, flip } = group.userData.pose;
    group.position.x += (x - group.position.x) * ease;
    group.position.z += (z - group.position.z) * ease;
    group.rotation.y += ((flip ? Math.PI : 0) - group.rotation.y) * ease;
  }
  controls.update();
  renderer.render(scene, camera);
}

buildList();
loadPieces();
tick();
