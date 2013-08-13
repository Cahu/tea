#version 330

float scale = 1.0/10.0;

uniform mat4 MVP;

layout (location = 0) in vec3 p1;


void main()
{
    gl_Position = MVP*vec4(p1, 1);
}
