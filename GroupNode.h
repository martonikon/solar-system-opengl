#pragma once

#include "Node.h"
#include <vector>

class GroupNode : public Node
{
protected:
	std::vector<Node*> children;

public:
	GroupNode() : Node()
	{
		type = nt_GroupNode;
	}
	GroupNode(const std::string& name) : Node(name, nt_GroupNode)
	{

	}

	void AddChild(Node* node)
	{
		children.push_back(node);
	}

	unsigned int GetChildCount()
	{
		return children.size();
	}

	Node* GetChild(unsigned int ix)
	{
		return children[ix];
	}

	virtual void Traverse()
	{
		for (unsigned int i = 0; i < children.size(); i++)
		{
			children[i]->Traverse();
		}
	}
};
