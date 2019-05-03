#include <stdlib.h>
#include <emscripten/bind.h>
#include "../recast/Recast.h"
#include "../recast/InputGeom.h"
#include "../recast/MeshLoaderObj.h"
#include "build.h"
#include <vector>
#include <array>

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

emscripten::val getHeightfieldHeight() {
    return emscripten::val(lastResult.heightfield ->height);
}

emscripten::val getHeightfieldWidth() {
    return emscripten::val(lastResult.heightfield ->width);
}

emscripten::val getHeightfieldMin() {
    return emscripten::val(emscripten::typed_memory_view(3, lastResult.heightfield ->bmin));
}

emscripten::val getHeightfieldMax() {
    return emscripten::val(emscripten::typed_memory_view(3, lastResult.heightfield ->bmax));
}

float* heightfieldVoxels;

emscripten::val getHeightfieldVoxels() {
    rcHeightfield* hf = lastResult.heightfield;

    const float* orig = hf->bmin;
	const float cs = hf->cs;
	const float ch = hf->ch;
	
	const int w = hf->width;
	const int h = hf->height;

    int voxelCount = 0;

    for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
            const rcSpan* s = hf->spans[x + y*w];
			while (s)
			{
                voxelCount++;
				s = s->next;
			}
		}
    }

    heightfieldVoxels = new float[voxelCount * 6];

    int voxelIndex = 0;
	
	for (int y = 0; y < h; ++y)
	{
		for (int x = 0; x < w; ++x)
		{
			float fx = orig[0] + x*cs;
			float fz = orig[2] + y*cs;
			const rcSpan* s = hf->spans[x + y*w];
			while (s)
			{
                heightfieldVoxels[voxelIndex * 6] = fx;
                heightfieldVoxels[voxelIndex * 6 + 1] = orig[1] + s->smin*ch;
                heightfieldVoxels[voxelIndex * 6 + 2] = fz;
                heightfieldVoxels[voxelIndex * 6 + 3] = fx+cs;
                heightfieldVoxels[voxelIndex * 6 + 4] = orig[1] + s->smax*ch;
                heightfieldVoxels[voxelIndex * 6 + 5] = fz + cs;
                voxelIndex++;
				s = s->next;
			}
		}
    }

    return emscripten::val(emscripten::typed_memory_view(voxelCount * 6, (float *)heightfieldVoxels));
}

void freeNavMesh() {
    rcFreePolyMeshDetail(lastResult.polyMeshDetail);
    lastResult.polyMeshDetail = 0;
    rcFreeHeightField(lastResult.heightfield);
	lastResult.heightfield = 0;
    delete heightfieldVoxels;
}

EMSCRIPTEN_BINDINGS(recast)
{
    emscripten::function("loadArray", &loadArray);
    emscripten::function("build", &buildNavmesh);
    emscripten::function("getMeshes", &getMeshes);
    emscripten::function("getVerts", &getVerts);
    emscripten::function("getTris", &getTris);
    emscripten::function("getHeightfieldWidth", &getHeightfieldWidth);
    emscripten::function("getHeightfieldHeight", &getHeightfieldHeight);
    emscripten::function("getHeightfieldMin", &getHeightfieldMin);
    emscripten::function("getHeightfieldMax", &getHeightfieldMax);
    emscripten::function("getHeightfieldVoxels", &getHeightfieldVoxels);
    emscripten::function("freeNavMesh", &freeNavMesh);
}
