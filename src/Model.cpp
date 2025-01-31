#include "Model.h"
Model::Model(MSWRL::ComPtr<ID3D12Device> device, std::vector<VertexAdvanced> vertices, std::vector<uint32_t> indices)
{
	_mesh = Mesh(device, vertices, indices);
	// calc aabb
}

void Model::DrawModel(MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList)
{
	_mesh.DrawMesh(commandList);
}
