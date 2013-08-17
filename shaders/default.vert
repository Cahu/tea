#version 330

float scale = 1.0;

uniform mat4 MP;
uniform mat4 MVP;

// our VBOs
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;

// pass the collor to the fragment shader
uniform vec4 Color;
smooth out vec4 FragColor;

// lighting test
vec3 LightPos = vec3(0.0, 0.0, 10.0);
vec4 LightCol = vec4(1.0, 1.0, 1.0, 1.0);
vec4 Ambiant  = vec4(0.2, 0.2, 0.2, 1.0);

void main()
{
    gl_Position = MVP * vec4(position, 1/scale);

	vec3  dirvec           = LightPos - vec3(MP * vec4(position, 1/scale));
	float incidence_cosine = clamp(dot(normalize(dirvec), normal), 0, 1);

	FragColor = Color*(Ambiant+LightCol*incidence_cosine*20/length(dirvec));
}
