// Renderer, camera, lights and theme-aware background for the enclosure viewer.
// The model group is in OpenSCAD coordinates (mm, z-up), centred on the origin.
import * as THREE from 'three';
import { OrbitControls } from 'three/addons/controls/OrbitControls.js';

export const cssToken = (name) => getComputedStyle(document.documentElement).getPropertyValue(name).trim();

export function createViewerScene(stage, caseSize, startCamera) {
  const renderer = new THREE.WebGLRenderer({ antialias: true });
  renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
  renderer.localClippingEnabled = true;
  stage.prepend(renderer.domElement);

  const scene = new THREE.Scene();
  const camera = new THREE.PerspectiveCamera(32, 1, 1, 4000);
  camera.position.set(...startCamera);
  const controls = new OrbitControls(camera, renderer.domElement);
  controls.enableDamping = true;

  scene.add(new THREE.HemisphereLight(0xffffff, 0x8a92a0, 1.5));
  const sun = new THREE.DirectionalLight(0xffffff, 1.7);
  sun.position.set(-120, 230, -90);
  const fill = new THREE.DirectionalLight(0xffffff, 0.7);
  fill.position.set(160, 70, 170);
  scene.add(sun, fill);

  // OpenSCAD is z-up, three.js is y-up: rotate once and centre the case on the origin.
  const pivot = new THREE.Group();
  pivot.rotation.x = -Math.PI / 2;
  const model = new THREE.Group();
  model.position.set(0, -caseSize.length / 2, -caseSize.height / 2);
  pivot.add(model);
  scene.add(pivot);

  // Section view keeps x <= constant (world x is the case's width axis).
  const sectionPlane = new THREE.Plane(new THREE.Vector3(-1, 0, 0), 0);
  const edgeMaterial = new THREE.LineBasicMaterial();
  const clippedEdgeMaterial = new THREE.LineBasicMaterial({ clippingPlanes: [sectionPlane] });

  let grid = null;
  function applyTheme() {
    renderer.setClearColor(new THREE.Color(cssToken('--stage')));
    edgeMaterial.color.set(cssToken('--edge'));
    clippedEdgeMaterial.color.set(cssToken('--edge'));
    if (grid) scene.remove(grid);
    grid = new THREE.GridHelper(340, 34, cssToken('--grid'), cssToken('--grid'));
    grid.position.y = -caseSize.height / 2 - 20;
    scene.add(grid);
  }
  applyTheme();
  matchMedia('(prefers-color-scheme: dark)').addEventListener('change', applyTheme);

  new ResizeObserver(() => {
    const width = stage.clientWidth;
    const height = Math.max(stage.clientHeight, 1);
    renderer.setSize(width, height, false);
    camera.aspect = width / height;
    camera.updateProjectionMatrix();
  }).observe(stage);

  return { renderer, scene, camera, controls, model, sectionPlane, edgeMaterial, clippedEdgeMaterial };
}
