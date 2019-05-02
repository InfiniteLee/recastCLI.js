#include "stdio.h"
#include "../recast/Recast.h"
#include "math.h"
#include "build.h"
#include "../recast/InputGeom.h"
#include "../recast/MeshLoaderObj.h"
#include <string.h>

// #define DEBUG

rcConfig m_cfg;
class rcContext *m_ctx = new rcContext;
class InputGeom *m_geom = new InputGeom;
rcHeightfield *m_solid;
unsigned char *m_triareas;

bool m_keepInterResults =				true;
bool m_filterLowHangingObstacles =		true;
bool m_filterLedgeSpans =				true;
bool m_filterWalkableLowHeightSpans =	true;

rcCompactHeightfield *m_chf;
int m_partitionType;

rcContourSet *m_cset;

rcPolyMesh *m_pmesh;		// Navigation mesh 数据
rcPolyMeshDetail *m_dmesh;	// Navigation mesh detail 数据

int recast_loadArray(const float *v, const int vl, const int *f, const int fl) {
	if (!m_geom->loadArray(m_ctx, v, vl, f, fl)) {
		return 0;
	}

	return 1;
}

NavMeshResult build(
    float 	cellSize,
    float 	cellHeight,
    float 	agentHeight,
    float 	agentRadius,
    float 	agentMaxClimp,
    float 	agentMaxSlope,
	float 	regionMinSize,
	float 	regionMergeSize,
	float 	edgeMaxLen,
	float 	edgeMaxError,
	float 	vertsPerPoly,
	float 	detailSampleDist,
	float 	detailSampleMaxError
)
{
	if(cellSize == 0)
		cellSize = CFG_CELL_SIZE;
	if(cellHeight == 0)
		cellHeight = CFG_CELL_HEIGHT;

	if(agentHeight == 0)
		agentHeight = CFG_AGENT_HEIGHT;
	if(agentRadius == 0)
		agentRadius = CFG_AGENT_RADIUS;
	if(agentMaxClimp == 0)
		agentMaxClimp = CFG_AGENT_MAX_CLIMP;
	if(agentMaxSlope == 0)
		agentMaxSlope = CFG_AGENT_MAX_SLOPE;

	if(regionMinSize == 0)
		regionMinSize = CFG_REGION_MIN_SIZE;
	if(regionMergeSize == 0)
		regionMergeSize = CFG_REGION_MERGE_SIZE;

	if(edgeMaxLen == 0)
		edgeMaxLen = CFG_EDGE_MAX_LEN;
	if(edgeMaxError == 0)
		edgeMaxError = CFG_EDGE_MAX_ERROR;
	if(vertsPerPoly == 0)
		vertsPerPoly = CFG_VERTS_PER_POLY;

	if(detailSampleDist == 0)
		detailSampleDist = CFG_DETAIL_SAMPLE_DIST;
	if(detailSampleMaxError == 0)
		detailSampleMaxError = CFG_DETAIL_SAMPLE_MAX_ERROR;

	if (!m_geom->getMesh()) {
		return { NAVMESH_NO_MESH_ERROR, nullptr};
	}
	// 模型包围盒
	const float *bmin = m_geom->getNavMeshBoundsMin();
	const float *bmax = m_geom->getNavMeshBoundsMax();
	// 顶点数据
	const float *verts = m_geom->getMesh()->getVerts();
	const int nverts = m_geom->getMesh()->getVertCount();
	// 三角形索引数据
	const int *tris = m_geom->getMesh()->getTris();
	const int ntris = m_geom->getMesh()->getTriCount();

#ifdef DEBUG
    printf("Verts\t: %d\n", nverts);
	printf("Tris\t: %d\n", ntris);
	printf("bmin\t: %f\n", *bmin);
	printf("bmax\t: %f\n", *bmax);
#endif

	//
	// Step 1. Initialize build config.
	//
	memset(&m_cfg, 0, sizeof(m_cfg));
	// Init build configuration from GUI

	// 体素大小
	m_cfg.cs = cellSize;   // Cell Size
	m_cfg.ch = cellHeight; // Cell Height

	// Agent
	m_cfg.walkableHeight = (int)ceilf(agentHeight / m_cfg.ch);
	m_cfg.walkableRadius = (int)ceilf(agentRadius / m_cfg.cs);
	m_cfg.walkableClimb = (int)floorf(agentMaxClimp / m_cfg.ch);
	m_cfg.walkableSlopeAngle = (float)agentMaxSlope;


	// Polygonization
	m_cfg.maxEdgeLen = (int)(edgeMaxLen / m_cfg.cs);
	m_cfg.maxSimplificationError = (float)edgeMaxError;
	m_cfg.maxVertsPerPoly = (int)vertsPerPoly;

	// Region
	m_cfg.minRegionArea = (int)rcSqr(regionMinSize);	 // Note: area = size*size
	m_cfg.mergeRegionArea = (int)rcSqr(regionMergeSize); // Note: area = size*size

	// Detail mesh
	m_cfg.detailSampleDist = (float)detailSampleDist < 0.9f ? 0 : m_cfg.cs * detailSampleDist;
	m_cfg.detailSampleMaxError = (float)m_cfg.ch * detailSampleMaxError;

	// Set the area where the navigation will be build.
	// Here the bounds of the input mesh are used, but the
	// area could be specified by an user defined box, etc.

	// 开辟构建空间
	// 获取NavMeshBoundsMin 和 NavMeshBoundsMax 参数
	// 根据包围盒计算网格大小
	rcVcopy(m_cfg.bmin, bmin);
	rcVcopy(m_cfg.bmax, bmax);
	// 计算网格数量
	rcCalcGridSize(m_cfg.bmin, m_cfg.bmax, m_cfg.cs, &m_cfg.width, &m_cfg.height);

	// Reset build times gathering.
	// 重置构建计时
	m_ctx->resetTimers();

	// Start the build process.
	// 开始计时
	m_ctx->startTimer(RC_TIMER_TOTAL);

#ifdef DEBUG
    // 打 log
	m_ctx->log(RC_LOG_PROGRESS, "Building navigation:");
	m_ctx->log(RC_LOG_PROGRESS, " - %d x %d cells", m_cfg.width, m_cfg.height);
	m_ctx->log(RC_LOG_PROGRESS, " - %.1fK verts, %.1fK tris", nverts / 1000.0f, ntris / 1000.0f);
#endif

	//
	// Step 2. Rasterize input polygon soup.
	// 第二步: 光栅化输入的多边形
	//

	// Allocate voxel heightfield where we rasterize our input data to.
	// 在 Heap 动态分配一块空间 用来存储光栅化之后的体素数据
	m_solid = rcAllocHeightfield();
	if (!m_solid)
	{
		// 如果为 Null, 则 Heap 的空间不够分配 solid 了
		return { NAVMESH_OUT_OF_MEMORY, nullptr};
	}

	// 分配二维网格空间, 每个格子是一个链表
	if (!rcCreateHeightfield(m_ctx, *m_solid, m_cfg.width, m_cfg.height, m_cfg.bmin, m_cfg.bmax, m_cfg.cs, m_cfg.ch))
	{
		return { NAVMESH_HEIGHTFIELD_ERROR, nullptr};
	}

	// Allocate array that can hold triangle area types.
	// If you have multiple meshes you need to process, allocate
	// and array which can hold the max number of triangles you need to process.
	// 分配 Heap 空间存储三角形数据
	m_triareas = new unsigned char[ntris];
	if (!m_triareas)
	{
		return { NAVMESH_OUT_OF_MEMORY, nullptr};
	}

	// Find triangles which are walkable based on their slope and rasterize them.
	// If your input data is multiple meshes, you can transform them here, calculate
	// the are type for each of the meshes and rasterize them.
	memset(m_triareas, 0, ntris * sizeof(unsigned char)); // 清空数据
	// 根据倾斜度查找可行走的三角形
	// 如果三角形可行走 则将三角形ID标记为 RC_WALKABLE_AREA
	rcMarkWalkableTriangles(m_ctx, m_cfg.walkableSlopeAngle, verts, nverts, tris, ntris, m_triareas);
	// 光栅化三角形, 转换成体素
	if (!rcRasterizeTriangles(m_ctx, verts, nverts, tris, m_triareas, ntris, *m_solid, m_cfg.walkableClimb))
	{
		return { NAVMESH_RASTERIZE_TRIANGLES_ERROR, nullptr};
	}

	// 是否 Keep Itermediate Results
	// m_triareas中的数据已经存贮到了span中, m_triareas可以删除了
	if (!m_keepInterResults)
	{
		delete[] m_triareas;
		m_triareas = 0;
	}

	//
	// Step 3. Filter walkables surfaces.
	// 第三步: 过滤可走表面
	//

	// Once all geoemtry is rasterized, we do initial pass of filtering to
	// remove unwanted overhangs caused by the conservative rasterization
	// as well as filter spans where the character cannot possibly stand.
	// 一旦完成所有几何面的光栅化, 我们移除掉因“保守光栅化”引起的的无用的悬崖, 以及角色不可能站立的位置

	// 在walkableClimp范围内, 允许从低一级span过渡到当前span, 比如楼梯
	if (m_filterLowHangingObstacles)
		rcFilterLowHangingWalkableObstacles(m_ctx, m_cfg.walkableClimb, *m_solid);
	// 过滤掉峭壁, 将峭壁两侧的span设置为不可走
	// 判断高度差是否大于玩家攀爬高度
	if (m_filterLedgeSpans)
		rcFilterLedgeSpans(m_ctx, m_cfg.walkableHeight, m_cfg.walkableClimb, *m_solid);
	// 过滤掉上下间隔太小的span
	if (m_filterWalkableLowHeightSpans)
		rcFilterWalkableLowHeightSpans(m_ctx, m_cfg.walkableHeight, *m_solid);

	//
	// Step 4. Partition walkable surface to simple regions.
	// 第四步: 把可行走表面划分为简单区域
	//

	// Compact the heightfield so that it is faster to handle from now on.
	// This will result more cache coherent data as well as the neighbours
	// between walkable cells will be calculated.
	// 简化高度场
	m_chf = rcAllocCompactHeightfield();
	if (!m_chf)
	{
		return { NAVMESH_OUT_OF_MEMORY, nullptr};
	}
	// Builds a compact heightfield representing open space, from a heightfield representing solid space.
	if (!rcBuildCompactHeightfield(m_ctx, m_cfg.walkableHeight, m_cfg.walkableClimb, *m_solid, *m_chf))
	{
		return { NAVMESH_COMPACT_HEIGHTFIELD_ERROR, nullptr};
	}

	// 是否保留原数据
	if (!m_keepInterResults)
	{
		rcFreeHeightField(m_solid);
		m_solid = 0;
	}

	// Erode the walkable area by agent radius.
	// 通过 Agent 半径, 收紧可走区域
	if (!rcErodeWalkableArea(m_ctx, m_cfg.walkableRadius, *m_chf))
	{
		return { NAVMESH_ERODE_WALKABLE_AREA_ERROR, nullptr};
	}

	// (Optional) Mark areas.
	// 可选, 标记行走标记
	const ConvexVolume *vols = m_geom->getConvexVolumes();
	for (int i = 0; i < m_geom->getConvexVolumeCount(); ++i)
		rcMarkConvexPolyArea(m_ctx, vols[i].verts, vols[i].nverts, vols[i].hmin, vols[i].hmax, (unsigned char)vols[i].area, *m_chf);

	// 划分高度场, 这样我们就可以用简单的算法去三角形化可走区域
	// 有三个选项:
	//      - Watershed (default)
	//      - Monotone
	//      - Layers
	// 三选一

	// Partition the heightfield so that we can use simple algorithm later to triangulate the walkable areas.
	// There are 3 martitioning methods, each with some pros and cons:
	// 1) Watershed partitioning
	//   - the classic Recast partitioning
	//   - creates the nicest tessellation
	//   - usually slowest
	//   - partitions the heightfield into nice regions without holes or overlaps
	//   - the are some corner cases where this method creates produces holes and overlaps
	//      - holes may appear when a small obstacles is close to large open area (triangulation can handle this)
	//      - overlaps may occur if you have narrow spiral corridors (i.e stairs), this make triangulation to fail
	//   * generally the best choice if you precompute the nacmesh, use this if you have large open areas
	// 2) Monotone partioning
	//   - fastest
	//   - partitions the heightfield into regions without holes and overlaps (guaranteed)
	//   - creates long thin polygons, which sometimes causes paths with detours
	//   * use this if you want fast navmesh generation
	// 3) Layer partitoining
	//   - quite fast
	//   - partitions the heighfield into non-overlapping regions
	//   - relies on the triangulation code to cope with holes (thus slower than monotone partitioning)
	//   - produces better triangles than monotone partitioning
	//   - does not have the corner cases of watershed partitioning
	//   - can be slow and create a bit ugly tessellation (still better than monotone)
	//     if you have large open areas with small obstacles (not a problem if you use tiles)
	//   * good choice to use for tiled navmesh with medium and small sized tiles

	if (m_partitionType == SAMPLE_PARTITION_WATERSHED)
	{
		// Prepare for region partitioning, by calculating distance field along the walkable surface.
		if (!rcBuildDistanceField(m_ctx, *m_chf))
		{
			return { NAVMESH_DISTANCE_FIELD_ERROR, nullptr};
		}

		// Partition the walkable surface into simple regions without holes.
		if (!rcBuildRegions(m_ctx, *m_chf, 0, m_cfg.minRegionArea, m_cfg.mergeRegionArea))
		{
			return { NAVMESH_REGIONS_ERROR, nullptr};
		}
	}
	// 单调分割
	else if (m_partitionType == SAMPLE_PARTITION_MONOTONE)
	{
		// Partition the walkable surface into simple regions without holes.
		// Monotone partitioning does not need distancefield.
		// 将可行走表面划分成简单地区, 无孔, 单调划分不需要距离范围
		if (!rcBuildRegionsMonotone(m_ctx, *m_chf, 0, m_cfg.minRegionArea, m_cfg.mergeRegionArea))
		{
			return { NAVMESH_MONOTONE_REGIONS_ERROR, nullptr};
		}
	}
	else // SAMPLE_PARTITION_LAYERS
	{
		// Partition the walkable surface into simple regions without holes.
		if (!rcBuildLayerRegions(m_ctx, *m_chf, 0, m_cfg.minRegionArea))
		{
			return { NAVMESH_LAYER_REGIONS_ERROR, nullptr};
		}
	}

	//
	// Step 5. Trace and simplify region contours.
	// 第五步: 跟踪并且生成简单轮廓
	//

	// Create contours.
	m_cset = rcAllocContourSet();
	if (!m_cset)
	{
		return { NAVMESH_OUT_OF_MEMORY, nullptr};
	}
	if (!rcBuildContours(m_ctx, *m_chf, m_cfg.maxSimplificationError, m_cfg.maxEdgeLen, *m_cset))
	{
		return { NAVMESH_CONTOURS_ERROR, nullptr};
	}

	//
	// Step 6. Build polygons mesh from contours.
	// 第六步: 从轮廓生成凸多边形网格
	//

	// Build polygon navmesh from the contours.
	m_pmesh = rcAllocPolyMesh();
	if (!m_pmesh)
	{
		return { NAVMESH_OUT_OF_MEMORY, nullptr};
	}

	if (!rcBuildPolyMesh(m_ctx, *m_cset, m_cfg.maxVertsPerPoly, *m_pmesh))
	{
		return { NAVMESH_POLY_MESH_ERROR, nullptr};
	}

	//
	// Step 7. Create detail mesh which allows to access approximate height on each polygon.
	// 第七步: 创建细节网格, 生成高度细节
	// 在这最后的阶段，凸多边形网格会被Delaunay三角化算法三角化，可以增加高度的细节。
	//

	m_dmesh = rcAllocPolyMeshDetail();
	if (!m_dmesh)
	{
		return { NAVMESH_OUT_OF_MEMORY, nullptr};
	}

	if (!rcBuildPolyMeshDetail(m_ctx, *m_pmesh, *m_chf, m_cfg.detailSampleDist, m_cfg.detailSampleMaxError, *m_dmesh))
	{
		return { NAVMESH_DETAIL_MESH_ERROR, nullptr};
	}

	if (!m_keepInterResults)
	{
		rcFreeCompactHeightfield(m_chf);
		m_chf = 0;
		rcFreeContourSet(m_cset);
		m_cset = 0;
		rcFreePolyMesh(m_pmesh);
		m_pmesh = 0;
	}

	return { NAVMESH_BUILD_SUCCESS, m_dmesh };
}
