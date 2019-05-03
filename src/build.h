#ifndef _BUILD_H_
#define _BUILD_H_

// Default setting

// Rasterization
#define CFG_CELL_SIZE   0.03
#define CFG_CELL_HEIGHT 0.02

// Agent
#define CFG_AGENT_HEIGHT    0.2
#define CFG_AGENT_RADIUS    0.03
#define CFG_AGENT_MAX_CLIMP 0.001
#define CFG_AGENT_MAX_SLOPE 45

// Region
#define CFG_REGION_MIN_SIZE     8
#define CFG_REGION_MERGE_SIZE   20

// Polygonization
#define CFG_EDGE_MAX_LEN    12
#define CFG_EDGE_MAX_ERROR  1.3
#define CFG_VERTS_PER_POLY  6

// Detail Mesh
#define CFG_DETAIL_SAMPLE_DIST      6
#define CFG_DETAIL_SAMPLE_MAX_ERROR 1

///
/// @brief 三角形化可走区域选项
///
enum SamplePartitionType
{
    SAMPLE_PARTITION_WATERSHED, ///< 默认选项
    SAMPLE_PARTITION_MONOTONE,
    SAMPLE_PARTITION_LAYERS,
};

extern bool m_keepInterResults; ///< 保留中间结果
extern bool m_filterLowHangingObstacles;
extern bool m_filterLedgeSpans;
extern bool m_filterWalkableLowHeightSpans;

///
/// @brief 读取地图
///
/// @param str 数据
/// @return int 读取状态
///
extern int recast_loadArray(const float *v, const int vl, const int *f, const int fl);

enum NavmeshStatusType {
    NAVMESH_BUILD_SUCCESS,
    NAVMESH_UNKNOWN_ERROR,
    NAVMESH_OUT_OF_MEMORY,
    NAVMESH_NO_MESH_ERROR,
    NAVMESH_HEIGHTFIELD_ERROR,
    NAVMESH_RASTERIZE_TRIANGLES_ERROR,
    NAVMESH_COMPACT_HEIGHTFIELD_ERROR,
    NAVMESH_ERODE_WALKABLE_AREA_ERROR,
    NAVMESH_DISTANCE_FIELD_ERROR,
    NAVMESH_REGIONS_ERROR,
    NAVMESH_MONOTONE_REGIONS_ERROR,
    NAVMESH_LAYER_REGIONS_ERROR,
    NAVMESH_CONTOURS_ERROR,
    NAVMESH_POLY_MESH_ERROR,
    NAVMESH_DETAIL_MESH_ERROR
};

struct NavMeshResult {
    NavmeshStatusType status;
    rcPolyMeshDetail *polyMeshDetail;
    rcHeightfield *heightfield;
};

///
/// @brief 构建 Navigation mesh
/// @param cellSize 网格长度, 用来体素化
/// @param cellHeight 网格高度, 体素高度
/// @param agentHeight 角色高度
/// @param agentRadius 角色碰撞半径
/// @param agentMaxClimp 角色最高攀爬高度
/// @param agentMaxSlope 角色最大坡度
/// @param regionMinSize 区域最大尺寸
/// @param regionMergeSize 区域合并尺寸
/// @param edgeMaxLen 边界最大长度
/// @param edgeMaxError 边界最多错误数
/// @param vertsPerPoly 多边形最大顶点数
/// @param detailSampleDist 细节样本距离
/// @param detailSampleMaxError 细节样本最大错误
/// @return int 构建状态
///
extern NavMeshResult build( float cellSize, float cellHeight, float agentHeight, float agentRadius, float agentMaxClimp, float agentMaxSlope, float regionMinSize, float regionMergeSize, float edgeMaxLen, float edgeMaxError, float vertsPerPoly, float detailSampleDist, float detailSampleMaxError);

#endif