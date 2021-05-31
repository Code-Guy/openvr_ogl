#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec3 Normal;
out vec2 TexCoord;
out vec3 Position;

uniform mat4 model;
uniform mat4 viewProj;

void main()
{
	gl_Position = viewProj * model * vec4(aPos, 1.0f);
	Normal = (model * vec4(aNormal, 0.0f)).xyz;
	TexCoord = vec2(aTexCoord.x, aTexCoord.y);
	Position = (model * vec4(aPos, 1.0f)).xyz;
}