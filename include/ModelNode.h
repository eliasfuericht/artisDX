#pragma once 

#include "pch.h"

class ModelNode {
public:
	std::string name;
	int meshIndex = -1; // optional
	int parentIndex = -1; // -1 means root
	std::vector<int> children;

	XMFLOAT4X4 localMatrix = {}; // From GLTF, if present
	XMFLOAT3 translation = { 0,0,0 };
	XMFLOAT4 rotation = { 0,0,0,1 }; // Quaternion
	XMFLOAT3 scale = { 1,1,1 };

	XMFLOAT4X4 globalMatrix = {}; // computed during update
};