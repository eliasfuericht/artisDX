#include "LightManager.h"

void LightManager::AddPointLight(float x, float y, float z)
{
	std::shared_ptr<PointLight> pLight = std::make_shared<PointLight>(x, y, z);
	pLight->RegisterWithGUI();
	_pLights.push_back(pLight);
}

void LightManager::AddDirectionalLight(float x, float y, float z, bool shadows, int shadowMapResolution)
{
	std::shared_ptr<DirectionalLight> dLight = std::make_shared<DirectionalLight>(x, y, z, shadows);
	dLight->RegisterWithGUI();
	_dLights.push_back(dLight);
	if (shadows)
	{
		CreateDShadowMapResource(shadowMapResolution);
	}
}

void LightManager::TransitionStartDLightShadowMap(std::shared_ptr<CommandContext> commandContext)
{
	for (auto& dS : _dShadowMaps)
	{
		commandContext->DeclareResource(dS.resource.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, "shadow map");
		commandContext->KeepAlive(dS.resource);
	}
}

void LightManager::TransitionEndDLightShadowMap(std::shared_ptr<CommandContext> commandContext)
{
	for (auto& dS : _dShadowMaps)
	{
		commandContext->UseResource(dS.resource.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		commandContext->KeepAlive(dS.resource);
	}
}

void LightManager::RenderDLightShadowMap(std::shared_ptr<CommandContext> commandContext)
{
	for (int i = 0; i < _dShadowMaps.size(); i++)
	{
		if (_dLights[i]->_shadowsEnabled)
		{
			commandContext->GetCommandList()->RSSetViewports(1, &_dShadowMaps[i].vp);
			commandContext->GetCommandList()->RSSetScissorRects(1, &_dShadowMaps[i].scissor);
			commandContext->UseResource(_dShadowMaps[i].resource.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE);
			commandContext->GetCommandList()->OMSetRenderTargets(0, nullptr, false, &_dShadowMaps[i].dsv);
			commandContext->GetCommandList()->ClearDepthStencilView(_dShadowMaps[i].dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, '\0', 0, nullptr);
		}
	}
}

void LightManager::BindDLightBuffers(std::shared_ptr<ShaderPass> shaderPass, std::shared_ptr<CommandContext> commandContext)
{
	if (_dLights.empty())
	{
		CreateEmptyDirectionalLightBuffers();
		commandContext->KeepAlive(_emptyDLightBuffer);
		commandContext->KeepAlive(_emptyDLightShadowMapBuffer);
		if (auto slot = shaderPass->GetRootParameterIndex("directionalLights"))
			commandContext->GetCommandList()->SetGraphicsRootDescriptorTable(slot.value(), DescriptorAllocator::CBVSRVUAV::GetGPUHandle(_emptyDLightSRV));
		if (auto slot = shaderPass->GetRootParameterIndex("dShadowMap"))
			commandContext->GetCommandList()->SetGraphicsRootDescriptorTable(slot.value(), DescriptorAllocator::CBVSRVUAV::GetGPUHandle(_emptyDLightShadowMapSRV));
		return;
	}

	if (auto slot = shaderPass->GetRootParameterIndex("directionalLights"))
	{
		const UINT frameIndex = D3D12Core::Swapchain::swapchain ? D3D12Core::Swapchain::swapchain->GetCurrentBackBufferIndex() : 0;
		DirectionalLightBuffer& buffer = _dLightBuffers[frameIndex];
		UpdateDirectionalLightBuffer(buffer);
		commandContext->KeepAlive(buffer.resource);
		commandContext->GetCommandList()->SetGraphicsRootDescriptorTable(slot.value(), DescriptorAllocator::CBVSRVUAV::GetGPUHandle(buffer.srv));
	}
	for (auto& dS : _dShadowMaps)
	{
		commandContext->KeepAlive(dS.resource);
	}

	if (auto slot = shaderPass->GetRootParameterIndex("dShadowMap"))
		commandContext->GetCommandList()->SetGraphicsRootDescriptorTable(slot.value(), DescriptorAllocator::CBVSRVUAV::GetGPUHandle(_dShadowMaps[0].srv));
}

bool LightManager::HasShadowLights() const
{
	return std::any_of(_dLights.begin(), _dLights.end(), [](const std::shared_ptr<DirectionalLight>& light) { return light->_shadowsEnabled; });
}

void LightManager::CreateEmptyDirectionalLightBuffers()
{
	if (_emptyDLightBuffer) return;
	const CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
	const CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(512);
	ThrowIfFailed(D3D12Core::GraphicsDevice::device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
		&desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&_emptyDLightBuffer)), "Empty directional light buffer creation failed");
	ThrowIfFailed(_emptyDLightBuffer->SetName(L"EmptyDirectionalLightConstants"));
	uint8_t* mapped = nullptr;
	const CD3DX12_RANGE readRange(0, 0);
	ThrowIfFailed(_emptyDLightBuffer->Map(0, &readRange, reinterpret_cast<void**>(&mapped)));
	memset(mapped, 0, 512);
	XMFLOAT4X4 identity;
	XMStoreFloat4x4(&identity, XMMatrixIdentity());
	memcpy(mapped + 256, &identity, sizeof(identity));
	_emptyDLightBuffer->Unmap(0, nullptr);

	_emptyDLightSRV = DescriptorAllocator::CBVSRVUAV::Allocate();
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc{};
	cbvDesc.BufferLocation = _emptyDLightBuffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = 256;
	D3D12Core::GraphicsDevice::device->CreateConstantBufferView(&cbvDesc, _emptyDLightSRV);

	_emptyDLightShadowMapSRV = DescriptorAllocator::CBVSRVUAV::Allocate();
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;
	D3D12Core::GraphicsDevice::device->CreateShaderResourceView(nullptr, &srvDesc, _emptyDLightShadowMapSRV);
}

void LightManager::BindPLightBuffers(std::shared_ptr<ShaderPass> shaderPass, std::shared_ptr<CommandContext> commandContext)
{
	if (auto slot = shaderPass->GetRootParameterIndex("pointLights"))
	{
		const UINT frameIndex = D3D12Core::Swapchain::swapchain ? D3D12Core::Swapchain::swapchain->GetCurrentBackBufferIndex() : 0;
		PointLightBuffer& buffer = _pLightBuffers[frameIndex];
		UpdatePointLightBuffer(buffer);
		commandContext->KeepAlive(buffer.resource);
		commandContext->GetCommandList()->SetGraphicsRootDescriptorTable(slot.value(), DescriptorAllocator::CBVSRVUAV::GetGPUHandle(buffer.srv));
	}
}

void LightManager::UpdatePointLightBuffer(PointLightBuffer& buffer)
{
	const uint32_t count = static_cast<uint32_t>(std::max(size_t{1}, _pLights.size()));
	if (count > buffer.capacity)
	{
		const CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
		const CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(uint64_t{count} * sizeof(PointLightDataGPU));
		MSWRL::ComPtr<ID3D12Resource> resource;
		ThrowIfFailed(D3D12Core::GraphicsDevice::device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
			&desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&resource)), "Point light buffer creation failed");
		ThrowIfFailed(resource->SetName(L"PointLightStructuredBuffer"));
		const CD3DX12_RANGE readRange(0, 0);
		ThrowIfFailed(resource->Map(0, &readRange, reinterpret_cast<void**>(&buffer.mapped)), "Point light buffer Map failed");
		buffer.resource = std::move(resource);
		buffer.capacity = count;
		if (buffer.srv.ptr == 0) buffer.srv = DescriptorAllocator::CBVSRVUAV::Allocate();
	}

	for (size_t i = 0; i < _pLights.size(); ++i)
	{
		const PointLight& light = *_pLights[i];
		buffer.mapped[i] = {light._position, light._intensity, light._color, 0.0f};
	}
	// A zero-intensity entry keeps the SRV valid when there are no lights.
	if (_pLights.empty()) buffer.mapped[0] = {};

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Buffer.NumElements = count;
	srvDesc.Buffer.StructureByteStride = sizeof(PointLightDataGPU);
	D3D12Core::GraphicsDevice::device->CreateShaderResourceView(buffer.resource.Get(), &srvDesc, buffer.srv);
}

void LightManager::UpdateDirectionalLightBuffer(DirectionalLightBuffer& buffer)
{
	const uint32_t count = static_cast<uint32_t>(std::max(size_t{ 1 }, _dLights.size()));
	if (count > buffer.capacity)
	{
		const CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
		const CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(uint64_t{ count } *sizeof(DirectionalLightDataGPU));
		MSWRL::ComPtr<ID3D12Resource> resource;
		ThrowIfFailed(D3D12Core::GraphicsDevice::device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE,
			&desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&resource)), "DirectionalLight buffer creation failed");
		ThrowIfFailed(resource->SetName(L"DirectionalLightStructuredBuffer"));
		const CD3DX12_RANGE readRange(0, 0);
		ThrowIfFailed(resource->Map(0, &readRange, reinterpret_cast<void**>(&buffer.mapped)), "DirectionalLight buffer Map failed");
		buffer.resource = std::move(resource);
		buffer.capacity = count;
		if (buffer.srv.ptr == 0) buffer.srv = DescriptorAllocator::CBVSRVUAV::Allocate();
	}

	for (size_t i = 0; i < _dLights.size(); ++i)
	{
		DirectionalLight& light = *_dLights[i];
		light.UpdateBuffer();
		buffer.mapped[i] = {	light._direction, light._shadowsEnabled, 
													light._intensity, light._color, 
													light._shadowMapIndex, XMFLOAT3(), 
													light._lightViewProjMatrix};
	}
	// A zero-intensity entry keeps the SRV valid when there are no lights.
	if (_dLights.empty()) buffer.mapped[0] = {};

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Buffer.NumElements = count;
	srvDesc.Buffer.StructureByteStride = sizeof(DirectionalLightDataGPU);
	D3D12Core::GraphicsDevice::device->CreateShaderResourceView(buffer.resource.Get(), &srvDesc, buffer.srv);
}

void LightManager::CreateDShadowMapResource(int32_t resolution)
{
	D3D12_VIEWPORT vp;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	vp.Width = static_cast<float>(resolution);
	vp.Height = static_cast<float>(resolution);
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;

	D3D12_RECT scissor;
	scissor.left = 0;
	scissor.top = 0;
	scissor.right = resolution;
	scissor.bottom = resolution;

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

	MSWRL::ComPtr<ID3D12Resource> resource;
	D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(D3D12Core::GraphicsDevice::device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&depthResourceDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&depthOptimizedClearValue,
		IID_PPV_ARGS(&resource)
	));

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	D3D12_CPU_DESCRIPTOR_HANDLE dsv = DescriptorAllocator::DSV::Allocate();
	D3D12Core::GraphicsDevice::device->CreateDepthStencilView(resource.Get(), &dsvDesc, dsv);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;

	D3D12_CPU_DESCRIPTOR_HANDLE srv = DescriptorAllocator::CBVSRVUAV::Allocate();
	D3D12Core::GraphicsDevice::device->CreateShaderResourceView(resource.Get(), &srvDesc, srv);

	_dShadowMaps.push_back({ resource, dsv, srv, vp, scissor });
}