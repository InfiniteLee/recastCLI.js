import test from "ava";
import * as THREE from "three";
import Recast from "../dist/recast";

const defaultParams = {
  cellSize: 0.166,
  cellHeight: 0.1,
  agentHeight: 1.7,
  agentRadius: 0.5,
  agentMaxClimb: 0.3,
  agentMaxSlope: 45,
  regionMinSize: 1,
  regionMergeSize: 20,
  edgeMaxLen: 12,
  edgeMaxError: 1,
  vertsPerPoly: 3,
  detailSampleDist: 16,
  detailSampleMaxError: 1
};

test("Export Recast API", async t => {
  const recast = await new Recast().ready;
  t.is(typeof recast.loadArray, "function");
  t.is(typeof recast.build, "function");
  t.is(typeof recast.getMeshes, "function");
  t.is(typeof recast.getVerts, "function");
  t.is(typeof recast.getTris, "function");
  t.is(typeof recast.freeNavMesh, "function");
});

test("Build Nav Mesh", async t => {
  const recast = await new Recast().ready;
  
  const geometry  = new THREE.BoxBufferGeometry(10, 10, 10);

  t.is(1, recast.loadArray(geometry.attributes.position.array, geometry.index.array));

  const status = recast.build(
    defaultParams.cellSize,
    defaultParams.cellHeight,
    defaultParams.agentHeight,
    defaultParams.agentRadius,
    defaultParams.agentMaxClimb,
    defaultParams.agentMaxSlope,
    defaultParams.regionMinSize,
    defaultParams.regionMergeSize,
    defaultParams.edgeMaxLen,
    defaultParams.edgeMaxError,
    defaultParams.vertsPerPoly,
    defaultParams.detailSampleDist,
    defaultParams.detailSampleMaxError
  );

  t.is(0, status);

  const meshes = recast.getMeshes();
  t.true(meshes.length > 0);

  const verts = recast.getVerts();
  t.true(verts.length > 0);

  const tris = recast.getTris();
  t.true(tris.length > 0);
});