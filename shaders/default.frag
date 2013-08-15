#version 330

smooth in vec4 FragColor;

out vec4 FinalColor;

void main()
{
   FinalColor = FragColor;
   //FinalColor = vec4(1.0, 1.0, 1.0, 1.0);
}
