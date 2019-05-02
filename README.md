A fork of [RecastCLI](https://github.com/but0n/recastCLI.js) that builds a subset of its functionality in WebAssembly, so you can build a navigation mesh directly in a browser.

## Usage

```
npm install -S recast-wasm
```

```js
import Recast from "recast-wasm";

const recast = await Recast.ready();

const vertices = new Float32Array();
const faces = new Uint16Array();

recast.loadArray(vertices, faces);

const status = recast.build(
  0.166, // cellSize
  0.1, // cellHeight
  1.7, // agentHeight
  0.5, // agentRadius
  0.3, // agentMaxClimb
  45, // agentMaxSlope
  1, // regionMinSize
  20, // regionMergeSize
  12, // edgeMaxLen
  1, // edgeMaxError
  3, // vertsPerPoly
  16, // detailSampleDist
  1 // detailSampleMaxError
);

if (status !== 0) return;

const meshes = recast.getMeshes();
const verts = recast.getVerts();
const tris = recast.getTris();

// Use nav mesh data

recast.freeNavMesh();
```

See the `/example` folder for a more complete example.

## Building from source

[Install docker](https://www.docker.com/)

```
npm run build
```

## Running the example:

```
npm build
npm start
```

## Credits

https://github.com/recastnavigation/recastnavigation

https://github.com/but0n/recastCLI.js

https://github.com/mqp/recastCLI.js