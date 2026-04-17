MESHLET_GROUP_IN_SUBTILE_COUNT = 128
TASK_THREADGROUP_SIZE = 32
MESH_WORKGROUP_SIZE = 32
MESH_THREADGROUP_SIZE = 32
VERTEX_COUNT_PER_ROW = 8
VERTEX_DISTANCE = 0.0714285970
MESHLET_COUNT_PER_ROW = 64
TOTAL_PRIMITIVE_COUNT = 98
TOTAL_VERTEX_COUNT = 8*8
SUBTILE_MESHLET_COUNT = 4096
SUBTILE_SIZE = 32
SUBTILE_WORKGROUP_SIZE = 128
MESHLET_SIZE = 0.571428597
MESHLET_DISTANCE = 0.5
SUBTILE_IN_ROW_COUNT = 32
MESH_GROUP_SHAPE_WIDTH = 8
MESH_GROUP_SHAPE_HEIGHT = 4




class float2:
    def __init__(this, x=0, y=0):
        this.x =x
        this.y = y
    def __str__(this)->str:
        return f"({this.x}, {this.y})"

def Map1DTo2D(idx:int, width:int, height:int) -> float2:
    return float2(int(idx % width), int(idx / width))

class STileData:
    def __init__(this):
        return

class SSubTileData:
    def __init__(this):
        return
    
class SMeshletGroupInfo:
    def __init__(this):
        this.meshletSize = 0
        this.indexInSubTileXY:float2 = float2(0,0)
        this.meshletDistance = 0
        this.worldPositionXZ:float2 = float2(0,0)
        this.size:float2 = float2(0,0)
        return
    def __str__(this)->str:
        return f"{this.indexInSubTileXY} -> {this.worldPositionXZ} -> {this.size}"

class SSubTileInfo:
    def __init__(this, pos:float2):
        this.worldPositionXZ:float2 = pos
        this.lod:int = 1
        this.workgroupSize:int = SUBTILE_WORKGROUP_SIZE
        this.tileId:int = 0
        
        return
    def GetMeshletGroupIndex(this, gid:int):
        return gid % MESHLET_GROUP_IN_SUBTILE_COUNT

    def GetMeshletGroup(this, gid:int) -> SMeshletGroupInfo:
        meshletGroupInRowCount = MESHLET_COUNT_PER_ROW / MESH_GROUP_SHAPE_WIDTH
        meshletDistance = MESHLET_DISTANCE * this.lod

        Info:SMeshletGroupInfo = SMeshletGroupInfo()
        Info.meshletSize = this.lod * MESHLET_SIZE
        Info.groupIndex = this.GetMeshletGroupIndex(gid)
        Info.indexInSubTileXY:float2 = Map1DTo2D(Info.groupIndex, meshletGroupInRowCount, MESHLET_COUNT_PER_ROW)
        Info.size.x = Info.meshletSize * MESH_GROUP_SHAPE_WIDTH
        Info.size.y = Info.meshletSize * MESH_GROUP_SHAPE_HEIGHT

        xOffset = Info.indexInSubTileXY.x * Info.meshletSize * MESH_GROUP_SHAPE_WIDTH
        yOffset = Info.indexInSubTileXY.y * Info.meshletSize * MESH_GROUP_SHAPE_HEIGHT
        
        Info.meshletDistance = meshletDistance
        Info.worldPositionXZ:float2 = float2(this.worldPositionXZ.x, this.worldPositionXZ.y)
        Info.worldPositionXZ.x += xOffset
        Info.worldPositionXZ.y += yOffset
        
        return Info
    
pos:float2 = float2(0,0)
SubTileInfo = SSubTileInfo(pos)
for gid in range(0, 33):
    MeshGroup = SubTileInfo.GetMeshletGroup(gid)
    
    print(gid, MeshGroup)
