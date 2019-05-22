#version 450 core

layout(location=0) in vec4 iPosition;
layout(location=1) in vec2 iTexcoord0;

layout(location=0) out vec2 oTexcoord0;

void main()
{
    oTexcoord0 = iTexcoord0;
    gl_Position = iPosition;
}