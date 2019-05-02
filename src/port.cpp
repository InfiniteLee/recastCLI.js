#include <stdlib.h>
#include <emscripten/bind.h>
#include "../recast/Recast.h"
#include "../recast/InputGeom.h"
#include "../recast/MeshLoaderObj.h"
#include "build.h"

emscripten::val loadArray(emscripten::val vs, emscripten::val fs) {
    std::vector<float> vs_vec = emscripten::vecFromJSArray<float>(vs);
    std::vector<int> fs_vec = emscripten::vecFromJSArray<int>(fs);
    return emscripten::val(recast_loadArray(vs_vec.data(), vs_vec.size(), fs_vec.data(), fs_vec.size()));
}

NavMeshResult lastResult;

int buildNavmesh(
    float cellSize,
    float cellHeight,
    float agentHeight,
    float agentRadius,
    float agentMaxClimp,
    float agentMaxSlope,
    float regionMinSize,
    float regionMergeSize,
    float edgeMaxLen,
    float edgeMaxError,
    float vertsPerPoly,
    float detailSampleDist,
    float detailSampleMaxError
) {
    lastResult = build(
        cellSize,
        cellHeight,
        agentHeight,
        agentRadius,
        agentMaxClimp,
        agentMaxSlope,
        regionMinSize,
        regionMergeSize,
        edgeMaxLen,
        edgeMaxError,
        vertsPerPoly,
        detailSampleDist,
        detailSampleMaxError
    );

    return (int)lastResult.status;
}

emscripten::val getMeshes() {
    return emscripten::val(emscripten::typed_memory_view(lastResult.polyMeshDetail->nmeshes * 4, lastResult.polyMeshDetail->meshes));
}

emscripten::val getVerts() {
    return emscripten::val(emscripten::typed_memory_view(lastResult.polyMeshDetail->nverts * 3, lastResult.polyMeshDetail->verts));
}

emscripten::val getTris() {
    return emscripten::val(emscripten::typed_memory_view(lastResult.polyMeshDetail->ntris * 4, lastResult.polyMeshDetail->tris));
}

void freeNavMesh() {
    rcFreePolyMeshDetail(lastResult.polyMeshDetail);
    lastResult.polyMeshDetail = 0;
}

EMSCRIPTEN_BINDINGS(recast)
{
    emscripten::function("loadArray", &loadArray);
    emscripten::function("build", &buildNavmesh);
    emscripten::function("getMeshes", &getMeshes);
    emscripten::function("getVerts", &getVerts);
    emscripten::function("getTris", &getTris);
    emscripten::function("freeNavMesh", &freeNavMesh);
}
