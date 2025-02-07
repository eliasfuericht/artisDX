#include "Model.h"
Model::Model(MSWRL::ComPtr<ID3D12Device> device, std::vector<Vertex> vertices, std::vector<uint32_t> indices, DirectX::XMFLOAT4X4 modelMatrix)
{
	_mesh = Mesh(device, vertices, indices);
	// calc aabb
	_modelMatrix = modelMatrix;
	CreateModelMatrixBuffer(device);
}

void Model::DrawModel(MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList)
{
	_mesh.BindMeshData(commandList);

	// Update model matrix buffer
	memcpy(_mappedUniformBuffer, &_modelMatrix, sizeof(_modelMatrix));

	// Bind model matrix buffer directly (instead of using descriptor heap)
	commandList->SetGraphicsRootConstantBufferView(1, _modelMatrixBuffer->GetGPUVirtualAddress());
}

void Model::CreateModelMatrixBuffer(MSWRL::ComPtr<ID3D12Device> device)
{
	D3D12_HEAP_PROPERTIES heapProps;
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProps.CreationNodeMask = 1;
	heapProps.VisibleNodeMask = 1;

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.NumDescriptors = 1;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	ThrowIfFailed(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&_modelMatrixBufferHeap)));

	D3D12_RESOURCE_DESC uboResourceDesc;
	uboResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	uboResourceDesc.Alignment = 0;
	uboResourceDesc.Width = (sizeof(_modelMatrix) + 255) & ~255;
	uboResourceDesc.Height = 1;
	uboResourceDesc.DepthOrArraySize = 1;
	uboResourceDesc.MipLevels = 1;
	uboResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	uboResourceDesc.SampleDesc.Count = 1;
	uboResourceDesc.SampleDesc.Quality = 0;
	uboResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	uboResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	ThrowIfFailed(device->CreateCommittedResource(
		&heapProps, D3D12_HEAP_FLAG_NONE, &uboResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&_modelMatrixBuffer)));
	_modelMatrixBufferHeap->SetName(L"Model Matrix Buffer Upload Resource Heap");

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = _modelMatrixBuffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = (sizeof(_modelMatrix) + 255) & ~255; // CB size is required to be 256-byte aligned.

	D3D12_CPU_DESCRIPTOR_HANDLE cbvHandle(_modelMatrixBufferHeap->GetCPUDescriptorHandleForHeapStart());
	cbvHandle.ptr = cbvHandle.ptr + device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 0;

	device->CreateConstantBufferView(&cbvDesc, cbvHandle);

	// Map once and keep the pointer
	D3D12_RANGE readRange = { 0, 0 };
	ThrowIfFailed(_modelMatrixBuffer->Map(0, &readRange, reinterpret_cast<void**>(&_mappedUniformBuffer)));

	// Copy initial model matrix data
	memcpy(_mappedUniformBuffer, &_modelMatrix, sizeof(_modelMatrix));
}

void Model::Transform(FLOAT x)
{
	DirectX::XMStoreFloat4x4(&_modelMatrix, DirectX::XMMatrixTranslation(x, 0.0, 0.0));
}
