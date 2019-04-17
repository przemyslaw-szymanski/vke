#version 450 core

layout(location=0) in vec4 iPosition;

void main()
{
    gl_Position = iPosition;
}