#pragma once

#include "Node.h"
#include "TransformNode.h"
#include "Model.h"
#include <glm\gtc\matrix_transform.hpp>

class GeometryNode : public Node
{
	Model model;
	Shader* shader;
	glm::vec3 modelRotation = glm::vec3(0.0f);


public:
	GeometryNode() :  Node()
	{
		type = nt_GeometryNode;
	}

	GeometryNode(const std::string& name) : Node(name, nt_GeometryNode)
	{

	}

	GeometryNode(const std::string& name, const std::string& path) : Node(name, nt_GeometryNode)
	{
		model.LoadModel(path);
	}

	void LoadFromFile(const std::string& path)
	{
		model.LoadModel(path);
	}

	void SetShader(Shader* s)
	{
		shader = s;
	}

	void SetModelRotation(const glm::vec3& rotation)
	{
		modelRotation = rotation;
	}


	void Traverse()
	{
		glm::mat4 transform = TransformNode::GetTransformMatrix(); 


		transform = glm::rotate(transform, glm::radians(modelRotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
		transform = glm::rotate(transform, glm::radians(modelRotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
		transform = glm::rotate(transform, glm::radians(modelRotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

 
		shader->setMat4("model", transform);
		glm::mat3 normalMat = glm::transpose(glm::inverse(transform));
		shader->setMat3("normalMat", normalMat);
		model.Draw(*shader);
	}
};
