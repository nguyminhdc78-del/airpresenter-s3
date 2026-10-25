// Click-to-identify: calls onPick(hit or null) when the user clicks (not drags) the canvas.
import * as THREE from 'three';

const DRAG_TOLERANCE_PX = 6;

export function listenForPicks(renderer, camera, getMeshes, accept, onPick) {
  const raycaster = new THREE.Raycaster();
  let down = null;
  renderer.domElement.addEventListener('pointerdown', (e) => { down = [e.clientX, e.clientY]; });
  renderer.domElement.addEventListener('pointerup', (e) => {
    if (!down || Math.hypot(e.clientX - down[0], e.clientY - down[1]) > DRAG_TOLERANCE_PX) return;
    const rect = renderer.domElement.getBoundingClientRect();
    const ndc = new THREE.Vector2(((e.clientX - rect.left) / rect.width) * 2 - 1,
      -((e.clientY - rect.top) / rect.height) * 2 + 1);
    raycaster.setFromCamera(ndc, camera);
    onPick(raycaster.intersectObjects(getMeshes(), false).find(accept) ?? null);
  });
}
