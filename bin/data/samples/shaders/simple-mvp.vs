#version 450 core

layout(set=0, binding=0) uniform Camera
{
    mat4    ViewProj;
};

layout(set=0, binding=1) uniform Model
{
    mat4    Transform;
};

layout(location=0) in vec4 iPosition;

void main()
{
    gl_Position = ViewProj * iPosition;
}