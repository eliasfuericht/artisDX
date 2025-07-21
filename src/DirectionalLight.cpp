#include "DirectionalLight.h"

DirectionalLight::DirectionalLight(float x, float y, float z, float enableShadowMap, int32_t shadowMapResolution)
{
	_direction = XMFLOAT3(x, y, z);

	BuildLightProjMatrix();

	if(enableShadowMap)
		CreateShadowMapResource(shadowMapResolution);

	CreateCBV(sizeof(XMFLOAT3), _dLightDirectionCPUHandle, _dLightDirectionBufferResource, _mappedDirectionPtr);
	CreateCBV(sizeof(XMFLOAT4X4), _dLightLVPCPUHandle, _dLightLVPBufferResource, _mappedLVPPtr);
	CreateCBV(sizeof(XMFLOAT4X4), _dLightSTCPUHandle, _dLightSTBufferResource, _mappedSTPtr);
}

void DirectionalLight::BuildLightProjMatrix()
{
	/*
	// 1) Normalize light direction
	XMVECTOR lightDir = XMVector3Normalize(XMLoadFloat3(&_direction));

	// 2) Choose a light "camera" position
	XMVECTOR sceneCenter = XMVectorZero(); // or your camera frustum center
	XMVECTOR lightPos = (sceneCenter - lightDir) * 100.0f;

	// 3) Build View matrix
	XMVECTOR up = XMVectorSet(0, 1, 0, 0);
	XMMATRIX lightView = XMMatrixLookAtLH(lightPos, sceneCenter, up);

	// 4) Build Ortho Projection
	float orthoWidth = 50.0f;
	float orthoHeight = 50.0f;
	float nearPlane = 0.1f;
	float farPlane = 200.0f;
	XMMATRIX lightProj = XMMatrixOrthographicLH(orthoWidth, orthoHeight, nearPlane, farPlane);

	// 5) Combine
	XMMATRIX lightViewProj = lightView * lightProj;

	// Store for shader usage
	XMStoreFloat4x4(&_lightViewProjMatrix, XMMatrixTranspose(lightViewProj));
	*/

	// Simple debug ortho matrix that should always cover [-1..1]
	XMMATRIX debugView = XMMatrixIdentity();

	// Very simple ortho projection
	float orthoWidth = 1.0f;
	float orthoHeight = 1.0f;
	float nearPlane = 0.01f;
	float farPlane = 10.0f;

	XMMATRIX debugProj = XMMatrixOrthographicLH(orthoWidth, orthoHeight, nearPlane, farPlane);

	XMMATRIX debugViewProj = debugView * debugProj;

	XMStoreFloat4x4(&_lightViewProjMatrix, XMMatrixTranspose(debugViewProj));
}

void DirectionalLight::CreateShadowMapResource(int32_t resolution)
{
	// Heap properties for creating the texture (GPU read/write)
	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProps.CreationNodeMask = 1;
	heapProps.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC depthResourceDesc = {};
	depthResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthResourceDesc.Alignment = 0;
	depthResourceDesc.Width = D3D12Core::Swapchain::width;
	depthResourceDesc.Height = D3D12Core::Swapchain::height;
	depthResourceDesc.DepthOrArraySize = 1;
	depthResourceDesc.MipLevels = 1;
	depthResourceDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthResourceDesc.SampleDesc.Count = 1;
	depthResourceDesc.SampleDesc.Quality = 0;
	depthResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthResourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
	depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
	depthOptimizedClearValue.DepthStencil.Stencil = 0;

	ThrowIfFailed(D3D12Core::GraphicsDevice::device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&depthResourceDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthOptimizedClearValue,
		IID_PPV_ARGS(&_directionalShadowMapBuffer)
	));

	// Create the DSV Heap
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	ThrowIfFailed(D3D12Core::GraphicsDevice::device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&_directionalShadowMapHeap)));

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

	D3D12Core::GraphicsDevice::device->CreateDepthStencilView(_directionalShadowMapBuffer.Get(), &dsvDesc, _directionalShadowMapHeap->GetCPUDescriptorHandleForHeapStart());
}

void DirectionalLight::CreateCBV(unsigned long long size, D3D12_CPU_DESCRIPTOR_HANDLE& handle, MSWRL::ComPtr<ID3D12Resource>& buffer, uint8_t*& mappedPtr)
{
	handle = DescriptorAllocator::Resource::Allocate();

	const uint32_t bufferSize = (size + 255) & ~255;

	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
	CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize);

	D3D12Core::GraphicsDevice::device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&buffer));

	CD3DX12_RANGE readRange(0, 0);
	buffer->Map(0, &readRange, reinterpret_cast<void**>(&mappedPtr));

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = buffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = bufferSize;

	D3D12Core::GraphicsDevice::device->CreateConstantBufferView(&cbvDesc, handle);
}

void DirectionalLight::UpdateBuffer()
{
	memcpy(_mappedDirectionPtr, &_direction, sizeof(XMFLOAT3));

	BuildLightProjMatrix();
	memcpy(_mappedLVPPtr, &_lightViewProjMatrix, sizeof(XMFLOAT4X4));
}

void DirectionalLight::DrawGUI()
{
	std::string windowName = "DirectionalLight";
	ImGui::Begin(windowName.c_str());

	ImGui::DragFloat3("Direction", &_direction.x, 0.01f);

	ImGui::End();
}