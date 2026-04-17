import os
import math
import numpy as np

def dot(furstumVec:list, centerPos:list, c:int=4) -> float:
    ret:float = 0
    for i in range(0, c):
        ret += furstumVec[i] * centerPos[i]
    return ret

def CreateFrustum(fov:float, aspect:float, near:float, far:float) -> list:
    frustum:list = []
    taa:float = math.tan(fov * 3.1415/180.0)
    y:float = 1.0 / taa
    x:float = aspect * 1.0 / taa
    c:float = -(far + near) / (far - near)
    d:float = -(2.0 * far * near) / (far - near)

    return frustum

planeNames = [
    "left",
    "right",
    "bottom",
    "top",
    "near",
    "far"
]



objects = [
    [0.00, -31.50, 0.41, 0.58],
    [0.50, -31.50, 0.41, 0.58],
    [1.00, -31.50, 0.41, 0.58],
    [1.50, -31.50, 0.41, 0.58],
    [2.00, -31.50, 0.41, 0.58],
    [2.50, -31.50, 0.41, 0.58],
    [3.00, -31.50, 0.41, 0.58],
    [3.50, -31.50, 0.41, 0.58],
    [4.00, -31.50, 0.41, 0.58],
    [4.50, -31.50, 0.41, 0.58],
    [5.00, -31.50, 0.41, 0.58],
    [5.50, -31.50, 0.41, 0.58],
    [6.00, -31.50, 0.41, 0.58],
    [6.50, -31.50, 0.41, 0.58],
    [7.00, -31.50, 0.41, 0.58],
    [7.50, -31.50, 0.41, 0.58],
    [8.00, -31.50, 0.41, 0.58],
    [8.50, -31.50, 0.41, 0.58],
    [9.00, -31.50, 0.41, 0.58],
    [9.50, -31.50, 0.41, 0.58],
    [10.00, -31.50, 0.41, 0.58],
    [10.50, -31.50, 0.41, 0.58],
    [11.00, -31.50, 0.41, 0.58],
    [11.50, -31.50, 0.41, 0.58],
    [12.00, -31.50, 0.41, 0.58],
    [12.50, -31.50, 0.41, 0.58],
    [13.00, -31.50, 0.41, 0.58],
    [13.50, -31.50, 0.41, 0.58],
    [14.00, -31.50, 0.41, 0.58],
    [14.50, -31.50, 0.41, 0.58],
    [15.00, -31.50, 0.41, 0.58],
    [15.50, -31.50, 0.41, 0.58],
]

def VecSelect(v1:list, v2:list, c:list) -> list:
#      XMVECTORU32 Result = { { {
#          (V1.vector4_u32[0] & ~Control.vector4_u32[0]) | (V2.vector4_u32[0] & Control.vector4_u32[0]),
#          (V1.vector4_u32[1] & ~Control.vector4_u32[1]) | (V2.vector4_u32[1] & Control.vector4_u32[1]),
#          (V1.vector4_u32[2] & ~Control.vector4_u32[2]) | (V2.vector4_u32[2] & Control.vector4_u32[2]),
#          (V1.vector4_u32[3] & ~Control.vector4_u32[3]) | (V2.vector4_u32[3] & Control.vector4_u32[3]),
#      } } };
#  return Result.v;
    return [ v2[0], v2[1], v2[2], 0.0 ]

def QuatMul(q1:list, q2:list) -> list:
#     XMVECTORF32 Result = { { {
#         (Q2.vector4_f32[3] * Q1.vector4_f32[0]) + (Q2.vector4_f32[0] * Q1.vector4_f32[3]) + (Q2.vector4_f32[1] * Q1.vector4_f32[2]) - (Q2.vector4_f32[2] * Q1.vector4_f32[1]),
#         (Q2.vector4_f32[3] * Q1.vector4_f32[1]) - (Q2.vector4_f32[0] * Q1.vector4_f32[2]) + (Q2.vector4_f32[1] * Q1.vector4_f32[3]) + (Q2.vector4_f32[2] * Q1.vector4_f32[0]),
#         (Q2.vector4_f32[3] * Q1.vector4_f32[2]) + (Q2.vector4_f32[0] * Q1.vector4_f32[1]) - (Q2.vector4_f32[1] * Q1.vector4_f32[0]) + (Q2.vector4_f32[2] * Q1.vector4_f32[3]),
#         (Q2.vector4_f32[3] * Q1.vector4_f32[3]) - (Q2.vector4_f32[0] * Q1.vector4_f32[0]) - (Q2.vector4_f32[1] * Q1.vector4_f32[1]) - (Q2.vector4_f32[2] * Q1.vector4_f32[2])
#     } } };
# return Result.v;
    return [
        q2[3] * q1[0] + q2[0] * q1[3] + q2[1] * q1[2] - q2[2] * q1[1],
        q2[3] * q1[1] - q2[0] * q1[2] + q2[1] * q1[3] + q2[2] * q1[0],
        q2[3] * q1[2] + q2[0] * q1[1] - q2[1] * q1[0] + q2[2] * q1[3],
        q2[3] * q1[3] - q2[0] * q1[0] - q2[1] * q1[1] - q2[2] * q1[2]
    ]

def QuatConjugate(q:list) -> list:
#     XMVECTORF32 Result = { { {
#         -Q.vector4_f32[0],
#         -Q.vector4_f32[1],
#         -Q.vector4_f32[2],
#         Q.vector4_f32[3]
#     } } };
# return Result.v;
    return [ -q[0], -q[1], -q[2], q[3] ]

def Sub(v1:list, v2:list) -> list:
    return [ v1[0] - v2[0], v1[1] - v2[1], v1[2] - v2[2], v1[3] - v2[3] ]

def InverseRotate(v:list, rotQuat:list) -> list:
    a:list = VecSelect([1.0, 1.0, 1.0, 0.0], v, [1.0, 1.0, 1.0, 0.0])
    r:list = QuatMul(rotQuat, a)
    q:list = QuatConjugate(rotQuat)
    return QuatMul(r, q)

def calcCenter(object:list, size:float) -> list:
    halfSize:float = size / 2
    return [ object[0] + halfSize, object[1], object[2] - halfSize, 1 ]

def calcDistance(a:list, b:list) -> float:
    return math.dist(a, b)

def calcRadius(obj:list, center:list) -> float:
    dist:float = calcDistance(center, obj)
    return dist

vCenter = [15.79, 0.0, -31.21, 0.0]
vOrigin =  [0.0, 0.0, 0.0, 0.0]
s = Sub(vCenter, vOrigin)
orientation:list = [ 0.0, 0.7, 0.0, -0.7 ]
v = InverseRotate( s, orientation )

def Dist(plane:list, point:list) -> float:
    return plane[0]*point[0] + plane[1]*point[1] + plane[2]*point[2] + plane[3]

class Obj:
    def __init__(this, pos:list, size:float):
        this.pos:list = pos
        this.pos.append(1)
        this.size:float = size
        this.center:list = calcCenter(pos, size)
        this.radius:float = calcRadius(pos, this.center)

def InFrustum(frustum:list, obj:Obj, doPrint:bool = False) ->bool:
    o:list = obj.pos
    size:float = obj.size
    center:list = obj.center
    radius:float = obj.radius
    #center = InverseRotate( Sub( center, vOrigin ), orientation)
    ret:bool = True
    for f in range(0, len(frustum)):
        d:float = dot(frustum[f], center, 4)
        dist:float = ( d - radius )
        outFrustum:bool = d < -radius
        intersects:bool = d < radius
        res = intersects or outFrustum == False 
        if(res == False):
            ret = False
        if(doPrint):
            print(f"\t{center} x {frustum[f]}{planeNames[f]} = ({round(dist,2)}) {round(d, 2)} < {round(radius,2)} = outFrustum={outFrustum} / intersects={intersects}")
    if(doPrint):
        print("\n")
    return ret

def sphere_in_frustum(planes, center, radius):
    """
    planes: list of tuples (normal, d), where normal is np.array([x, y, z]), d is float
    center: np.array([x, y, z])
    radius: float
    Returns: 'outside', 'intersect', or 'inside'
    """
    result = 'inside'
    for normal, d in planes:
        distance = np.dot(normal, center) + d
        if distance < -radius:
            return 'outside'
        elif distance < radius:
            result = 'intersect'
    return result

# Example usage:
# Each plane: (normal, d), where plane equation is normal.x + d = 0
# For a real camera, extract these from the view-projection matrix.
planes = [
    (np.array([1, 0, 0]), -1),   # left
    (np.array([-1, 0, 0]), -1),  # right
    (np.array([0, 1, 0]), -1),   # bottom
    (np.array([0, -1, 0]), -1),  # top
    (np.array([0, 0, 1]), -1),   # near
    (np.array([0, 0, -1]), -10)  # far
]
center = np.array([0, 0, -5])
radius = 1

print(sphere_in_frustum(planes, center, radius))

dx=[
[-0.802381, 0.42201, -0.42201, -47.848],
[0.802381, 0.42201, -0.42201, -70.3147],
[0, -0.273036, -0.962004, -72.9286],
[0, 0.962004, 0.273036, -23.527],
[0, 0.707107, -0.707107, -97.995],
[0, -0.707107, 0.707107, -900.899]
]

mg=[
    [0.802381, 0, -0.596812, 0],
[-0.802381, 0, -0.596812, 0],
[0, 0.873305, -0.487174, 0],
[0, -0.873305, -0.487174, 0],
[0, 0, -1, 0.50025],
[0, 0, 1, -1]
]

me=[
    [0.802381, -0.42201, 0.42201, 47.848],
[-0.802381, -0.42201, 0.42201, 70.3147],
[0, 0.273036, 0.962004, 72.9286],
[0, -0.962004, -0.273036, 23.527],
[0, -0.707107, 0.707107, 97.9949],
[0, 0.707107, -0.707107, 901.019]
]
f = me

InFrustum(f, Obj([64,0,-64], 32), doPrint=True)
#InFrustum(minigraph, [0, 20, 0.41, 0.58])
#InFrustum(gribb, [0, 20, 0.41, 0.58])
InFrustum(f, Obj([32,0,-64], 32), doPrint=True)
InFrustum(dx, Obj([0,0,-32], 32))
InFrustum(dx, Obj([-32,0,0], 32), True)
#InFrustum(frustum, [0.0, 0.5, 0.41, 0.58])
#InFrustum(frustum, [0.0, -0.7, 0.11, 0.58])

tilePos = [-1024, 0]
passed:list = []
for y in range(0,32):
    for x in range(0,32):
        pos = [ tilePos[0] + x * 32, 0, tilePos[1] - y * 32 ]
        obj = Obj(pos, 32)
        idx = x + y*32
        if(idx==31):
            do = False
        inFrust = InFrustum(dx, obj)
        print(f"{idx} obj pos: {pos} = {inFrust}")
        if(inFrust):
            passed.append(obj)
print("in frustum")
for p in passed:
    print(f"{p.pos}")
exit()