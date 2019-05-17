#version 450 core

layout(location=0) in vec4 iPosition;
layout(location=1) in vec2 iTexcoord0;

layout(location=0) out vec4 oPosition;
layout(location=1) out vec2 oTexcoord0;

void main()
{
    oPosition = iPosition;
    oTexcoord0 = iTexcoord0;
}