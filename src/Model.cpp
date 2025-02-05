#include "Model.h"
Model::Model(MSWRL::ComPtr<ID3D12Device> device, std::vector<Vertex> vertices, std::vector<uint32_t> indices, DirectX::XMFLOAT4X4 modelMatrix)
{
	_mesh = Mesh(device, vertices, indices);
	// calc aabb
	_modelMatrix = modelMatrix;
}

void Model::DrawModel(MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList)
{
	_mesh.BindMeshData(commandList);
	// bind model Matrix here
}
