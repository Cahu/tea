#version 330

layout (location = 0) in vec2 p1;

float scale = 1.0/10.0;

void main()
{
    gl_Position = vec4(p1, 0.0, 1/scale);
}
