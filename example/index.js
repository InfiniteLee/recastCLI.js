import * as THREE from "three";
import { OrbitControls } from "three/examples/jsm/controls/OrbitControls";
import { BufferGeometryUtils } from "three/examples/jsm/utils/BufferGeometryUtils";
import RecastWorker from "./recast.worker";

const canvas = document.getElementById("canvas");

const renderer = new THREE.WebGLRenderer({ antialias: true, canvas });
renderer.setPixelRatio(window.devicePixelRatio);
renderer.setSize(window.innerWidth, window.innerHeight);
renderer.shadowMap.enabled = true;
renderer.shadowMap.type = THREE.BasicShadowMap;

const scene = new THREE.Scene();

scene.add(new THREE.AmbientLight(0x404040));

const directionalLight = new THREE.DirectionalLight(0xffffff, 1);
directionalLight.position.set(1, 10, 0);
directionalLight.castShadow = true;
directionalLight.shadow.camera.near = 1;
directionalLight.shadow.camera.far = 100;
directionalLight.shadow.camera.right = 150;
directionalLight.shadow.camera.left = -150;
directionalLight.shadow.camera.top	= 150;
directionalLight.shadow.camera.bottom = -150;
directionalLight.shadow.mapSize.width = 1024;
directionalLight.shadow.mapSize.height = 1024;
scene.add(directionalLight);

const directionalLightHelper = new THREE.DirectionalLightHelper(directionalLight, 5);
scene.add(directionalLightHelper);

const camera = new THREE.PerspectiveCamera(75, window.innerWidth / window.innerHeight, 0.1, 1000);
camera.position.set(0, 2, 5);
camera.lookAt(0, 0, 0);
scene.add(camera);

const plane = new THREE.Mesh(
  new THREE.PlaneBufferGeometry(10, 10, 10),
  new THREE.MeshStandardMaterial({
    color: 0xffffff,
    roughness: 0.9,
    metalness: 0.01
  })
);
plane.castShadow = true;
plane.receiveShadow = true;
plane.rotation.set(-Math.PI / 2, 0, 0);
scene.add(plane);

const cube1 = new THREE.Mesh(
  new THREE.BoxBufferGeometry(1, 0.1, 2, 1, 1, 1),
  new THREE.MeshStandardMaterial({
    color: 0xffffff,
    roughness: 0.9,
    metalness: 0.01
  })
);
cube1.castShadow = true;
cube1.receiveShadow = true;
cube1.position.set(-2, 0.5, 1.2);
cube1.rotation.set(Math.PI / 5, 0, 0);
scene.add(cube1);

const cube2 = new THREE.Mesh(
  new THREE.BoxBufferGeometry(5, 0.1, 5),
  new THREE.MeshStandardMaterial({
    color: 0xffffff,
    roughness: 0.9,
    metalness: 0.01
  })
);
cube2.castShadow = true;
cube2.receiveShadow = true;
cube2.position.set(-2, 1, -2);
scene.add(cube2);

const sphere = new THREE.Mesh(
  new THREE.SphereBufferGeometry(1),
  new THREE.MeshStandardMaterial({
    color: 0xffffff,
    roughness: 0.9,
    metalness: 0.01
  })
);
sphere.castShadow = true;
sphere.receiveShadow = true;
sphere.position.set(2, 0, 2);
scene.add(sphere);

const controls = new OrbitControls(camera, renderer.domElement);
controls.target.set(0, 0, 0);
controls.update();

function render() {
  renderer.render(scene, camera);
}

renderer.setAnimationLoop(render);

function mergeMeshGeometries(meshes) {
  const geometries = [];

  for (const mesh of meshes) {
    let geometry = mesh.geometry;
    let attributes = geometry.attributes;

    if (!geometry.isBufferGeometry) {
      geometry = new THREE.BufferGeometry().fromGeometry(geometry);
      attributes = geometry.attributes;
    }

    if (!attributes.position || attributes.position.itemSize !== 3) return;

    if (geometry.index) geometry = geometry.toNonIndexed();

    const cloneGeometry = new THREE.BufferGeometry();
    cloneGeometry.addAttribute("position", geometry.attributes.position.clone());
    mesh.updateMatrixWorld();
    cloneGeometry.applyMatrix(mesh.matrixWorld);
    geometry = cloneGeometry;

    geometries.push(geometry);
  }

  if (geometries.length === 0) {
    return new THREE.BufferGeometry();
  }

  const geometry = BufferGeometryUtils.mergeBufferGeometries(geometries);

  const flippedGeometry = geometry.clone();

  const positions = flippedGeometry.attributes.position.array;
  for (let i = 0; i < positions.length; i += 9) {
    const x0 = positions[i];
    const y0 = positions[i + 1];
    const z0 = positions[i + 2];
    const offset = 6;
    positions[i] = positions[i + offset];
    positions[i + 1] = positions[i + offset + 1];
    positions[i + 2] = positions[i + offset + 2];
    positions[i + offset] = x0;
    positions[i + offset + 1] = y0;
    positions[i + offset + 2] = z0;
  }

  return BufferGeometryUtils.mergeBufferGeometries([geometry, flippedGeometry]);
}

const worker = new RecastWorker();

worker.addEventListener("error", event => {
  console.log(error);
});

worker.addEventListener("message", event => {
  if (event.error) {
    console.log(error);
  }

  const { verts, indices, heightfieldVoxels } = event.data;

  const geometry = new THREE.BufferGeometry();
  geometry.addAttribute("position", new THREE.Float32BufferAttribute(verts, 3));
  geometry.setIndex(new THREE.Uint16BufferAttribute(indices, 1));

  const material = new THREE.MeshBasicMaterial({ color: 0x0000ff });

  const mesh = new THREE.Mesh(geometry, material);

  scene.add(mesh);

  const voxelCount = heightfieldVoxels.length / 6;

  console.log(voxelCount);

  const voxelVerts = [];
  const voxelIndices = [];

  for (let i = 0; i < voxelCount; i++) {
    const minx = heightfieldVoxels[i * 6];
    const miny = heightfieldVoxels[i * 6 + 1];
    const minz = heightfieldVoxels[i * 6 + 2];
    const maxx = heightfieldVoxels[i * 6, + 3];
    const maxy = heightfieldVoxels[i * 6 + 4];
    const maxz = heightfieldVoxels[i * 6 + 5];

    voxelVerts.push(
      minx, miny, minz,
      maxx, miny, minz,
      maxx, miny, maxz,
      minx, miny, maxz,
      minx, maxy, minz,
      maxx, maxy, minz,
      maxx, maxy, maxz,
      minx, maxy, maxz,
    );

    const o = i * 8;

    voxelIndices.push(
      o + 7, o + 6, o + 5, o + 4,
      o + 0, o + 1, o + 2, o + 3,
      o + 1, o + 5, o + 6, o + 2,
      o + 3, o + 7, o + 4, o + 0,
      o + 2, o + 6, o + 7, o + 3,
      o + 0, o + 4, o + 5, o + 1,
    );
  }

  const voxelGeometry = new THREE.BufferGeometry();
  voxelGeometry.addAttribute("position", new THREE.Float32BufferAttribute(voxelVerts, 3));
  voxelGeometry.setIndex(new THREE.Uint16BufferAttribute(voxelIndices, 1));
  const voxelMaterial = new THREE.MeshBasicMaterial({ color: 0xcccccc });
  const voxelMesh = new THREE.Mesh(voxelGeometry, voxelMaterial);
  scene.add(voxelMesh);
});

const meshes = [];

scene.traverse(object => {
  if (object.isMesh) {
    meshes.push(object);
  }
});

const mergedGeometry = mergeMeshGeometries(meshes);

const verts = mergedGeometry.attributes.position.array;
const faces = new Int32Array(verts.length / 3);

for (let i = 0; i < faces.length; i++) {
  faces[i] = i;
}

worker.postMessage({
  faces,
  verts,
  params: {
    cellSize: 0.1,
    cellHeight: 0.1,
    regionMinSize: 0.5,
    agentRadius: 0.1,
    agentHeight: 0.7
  }
}, [verts.buffer, faces.buffer]);


