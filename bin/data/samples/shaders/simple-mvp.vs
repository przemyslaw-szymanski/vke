#version 450 core

layout(set=0, binding=0) uniform Camera
{
    mat4    ViewProj;
};

layout(set=0, binding=1) uniform Model
{
    mat4    Transform;
};

layout(location=0) in vec3 iPosition;

void main()
{
    gl_Position = ViewProj * vec4( iPosition, 1.0 );
}