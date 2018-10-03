#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <emscripten/bind.h>
#include "../recast/Recast.h"
#include "../recast/InputGeom.h"
#include "../recast/MeshLoaderObj.h"
#include "build.h"

void loadArray(emscripten::val vs, emscripten::val fs) {
  std::vector<float> vs_vec = emscripten::vecFromJSArray<float>(vs);
  std::vector<int> fs_vec = emscripten::vecFromJSArray<int>(fs);
  recast_loadArray(vs_vec.data(), vs_vec.size(), fs_vec.data(), fs_vec.size());
}

void loadContent(std::string content) {
  recast_loadContent(content.c_str());
}

emscripten::val buildNavmesh(
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
  std::string result(build(
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
  ));
  return emscripten::val(result);
}

EMSCRIPTEN_BINDINGS(recast)
{
  emscripten::function("build", &buildNavmesh);
  emscripten::function("loadContent", &loadContent);
  emscripten::function("loadArray", &loadArray);
}
