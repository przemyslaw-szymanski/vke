#version 450 
#include "inc.h"

void main()
{
	vec3 v = vec3(0);
	gl_Position = vec4(v.x,v.z,v.y,0);
}