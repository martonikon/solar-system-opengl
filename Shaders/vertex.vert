#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;

//the normal matrix - transforming the normal vector for a vertex from local to world space 
//(no translation and avoiding the effects of non-uniform scale)
uniform mat3 normalMat;

void main()
{
	//the fragment position in world space (multiplied only with the model matrix and without w component)
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = normalMat * aNormal;  
	TexCoords = aTexCoords;

    gl_Position = proj * view * vec4(FragPos, 1.0);
}

