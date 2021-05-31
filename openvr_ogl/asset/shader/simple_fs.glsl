#version 330 core
out vec4 FragColor;

in vec3 Normal;
in vec2 TexCoord;
in vec3 Position;

// texture samplers
uniform sampler2D diffuseTexture;
uniform vec3 cameraPosition;

void main()
{
	vec3 baseColor = texture(diffuseTexture, TexCoord).xyz;

	// ambient
	float ambient = 0.3;

	// diffuse
	vec3 lightDirection = normalize(vec3(-0.6, -1.0, -0.8));
	float diffuse = max(dot(-lightDirection, Normal), 0.0);

	// specular
	float shininess = 32.0;
	vec3 lightColor = vec3(1.0);

	vec3 viewDirection = normalize(cameraPosition - Position);
	vec3 reflectDirection = reflect(lightDirection, Normal);
	vec3 halfwayDirection = normalize(-lightDirection + Normal);
	float specular = pow(max(dot(halfwayDirection, Normal), 0.0), shininess);

	FragColor = vec4(baseColor * ambient + baseColor * lightColor * diffuse + lightColor * specular, 1.0);
}