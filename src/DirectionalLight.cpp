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
	XMVECTOR lightDir = XMVector3Normalize(XMLoadFloat3(&_direction));

	XMVECTOR sceneCenter = XMLoadFloat3(&_sceneCenter);
	XMVECTOR lightPos = sceneCenter - lightDir;

	XMVECTOR up = XMVectorSet(0, 1, 0, 1);
	XMMATRIX lightView = XMMatrixLookAtLH(lightPos, sceneCenter, up);

	
	XMMATRIX lightProj = XMMatrixOrthographicLH(_orthoWidth, _orthoHeight, _nearPlane, _farPlane);

	XMMATRIX lightViewProj = lightProj * lightView;

	XMStoreFloat4x4(&_lightViewProjMatrix, lightViewProj);
}

void DirectionalLight::CreateShadowMapResource(int32_t resolution)
{
	_vp.TopLeftX = 0;
	_vp.TopLeftY = 0;
	_vp.Width = static_cast<float>(resolution);
	_vp.Height = static_cast<float>(resolution);
	_vp.MinDepth = 0.0f;
	_vp.MaxDepth = 1.0f;

	_scissor.left = 0;
	_scissor.top = 0;
	_scissor.right = resolution;
	_scissor.bottom = resolution;

	D3D12_RESOURCE_DESC depthResourceDesc = {};
	depthResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthResourceDesc.Alignment = 0;
	depthResourceDesc.Width = resolution;
	depthResourceDesc.Height = resolution;
	depthResourceDesc.DepthOrArraySize = 1;
	depthResourceDesc.MipLevels = 1;
	depthResourceDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	depthResourceDesc.SampleDesc.Count = 1;
	depthResourceDesc.SampleDesc.Quality = 0;
	depthResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthResourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
	depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
	depthOptimizedClearValue.DepthStencil.Stencil = 0;

	D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	ThrowIfFailed(D3D12Core::GraphicsDevice::device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&depthResourceDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&depthOptimizedClearValue,
		IID_PPV_ARGS(&_directionalShadowMapBuffer)
	));

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

	_directionalShadowMapDSVCPUHandle = DescriptorAllocator::DSV::Allocate();
	D3D12Core::GraphicsDevice::device->CreateDepthStencilView(_directionalShadowMapBuffer.Get(), &dsvDesc, _directionalShadowMapDSVCPUHandle);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;

	_directionalShadowMapSRVCPUHandle = DescriptorAllocator::CBVSRVUAV::Allocate();
	D3D12Core::GraphicsDevice::device->CreateShaderResourceView(_directionalShadowMapBuffer.Get(), &srvDesc, _directionalShadowMapSRVCPUHandle);
}

void DirectionalLight::CreateCBV(unsigned long long size, D3D12_CPU_DESCRIPTOR_HANDLE& handle, MSWRL::ComPtr<ID3D12Resource>& buffer, uint8_t*& mappedPtr)
{
	handle = DescriptorAllocator::CBVSRVUAV::Allocate();

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
	ImGui::DragFloat("width", &_orthoWidth, 0.01f);
	ImGui::DragFloat("height", &_orthoHeight, 0.01f);
	ImGui::DragFloat("near", &_nearPlane, 0.01f);
	ImGui::DragFloat("far", &_farPlane, 0.01f);
	ImGui::DragFloat3("focuscenter", &_sceneCenter.x, 0.01f);

	{
		windowName = "ShadowMap";
		ImGui::Begin(windowName.c_str());
		auto gpuHandle = DescriptorAllocator::CBVSRVUAV::GetGPUHandle(_directionalShadowMapSRVCPUHandle);
		ImTextureID texID = (ImTextureID)gpuHandle.ptr;

		ImGui::Image(
			texID,
			ImVec2(300, 300),
			ImVec2(0, 0),
			ImVec2(1, 1),
			ImVec4(1, 1, 1, 1),
			ImVec4(0, 0, 0, 0)
		);
		ImGui::End();
	}

	ImGui::End();
}