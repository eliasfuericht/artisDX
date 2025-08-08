#include "Renderer.h"

void Renderer::InitializeRenderer()
{
	CommandQueueManager::InitializeCommandQueueManager();

	_mainLoopGraphicsContext.InitializeCommandContext(QUEUETYPE::QUEUE_GRAPHICS);
	_mainLoopGraphicsContext.Finish(false);

	DescriptorAllocator::CBVSRVUAV::InitializeDescriptorAllocator(NUM_MAX_RESOURCE_DESCRIPTORS);
	DescriptorAllocator::RTV::InitializeDescriptorAllocator(NUM_MAX_RTV_DESCRIPTORS);
	DescriptorAllocator::DSV::InitializeDescriptorAllocator(NUM_MAX_RTV_DESCRIPTORS);
	DescriptorAllocator::Sampler::InitializeDescriptorAllocator(NUM_MAX_SAMPLER_DESCRIPTORS);
}

void Renderer::InitializeResources()
{
	CreateRenderTarget();
	CreateDepthBuffer();

	_depthPass = std::make_shared<ShaderPass>("DepthPass");
	_depthPass->RegisterWithGUI();
	_depthPass->AddShader("../shaders/dShadowMap_vert.hlsl", SHADERTYPE::SHADER_VERTEX);
	_depthPass->AddShader("../shaders/dShadowMap_frag.hlsl", SHADERTYPE::SHADER_PIXEL);
	_depthPass->GenerateGraphicsRootSignature();
	_depthPass->GeneratePipeLineStateObjectForwardPass(D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_BACK, false);

	_mainPass = std::make_shared<ShaderPass>("Main");
	_mainPass->RegisterWithGUI();
	_mainPass->AddShader("../shaders/pbr_vert.hlsl", SHADERTYPE::SHADER_VERTEX);
	_mainPass->AddShader("../shaders/pbr_frag.hlsl", SHADERTYPE::SHADER_PIXEL);
	_mainPass->GenerateGraphicsRootSignature();
	_mainPass->GeneratePipeLineStateObjectForwardPass(D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_BACK, true);

	_bbPass = std::make_shared<ShaderPass>("BoundingBox");
	_bbPass->_usePass = false;
	_bbPass->RegisterWithGUI();
	_bbPass->AddShader("../shaders/bb_vert.hlsl", SHADERTYPE::SHADER_VERTEX);
	_bbPass->AddShader("../shaders/bb_frag.hlsl", SHADERTYPE::SHADER_PIXEL);
	_bbPass->GenerateGraphicsRootSignature();
	_bbPass->GeneratePipeLineStateObjectForwardPass(D3D12_FILL_MODE_WIREFRAME, D3D12_CULL_MODE_NONE, false);

	CreateConstantBuffers();

	//_modelManager.LoadModel("../assets/helmet.glb");
	//_modelManager.LoadModel("../assets/helmets.glb");
	_modelManager.LoadModel("../assets/sponza.glb");
	//_modelManager.LoadModel("../assets/brick_wall.glb");
	//_modelManager.LoadModel("../assets/DamagedHelmet.glb");
	//_modelManager.LoadModel("../assets/apollo.glb");
	//_modelManager.LoadModel("../assets/bistro.glb");
}

void Renderer::CreateRenderTarget()
{
	_vp.TopLeftX = 0;
	_vp.TopLeftY = 0;
	_vp.Width = static_cast<float>(GUI::viewportWidth);
	_vp.Height = static_cast<float>(GUI::viewportHeight);
	_vp.MinDepth = 0.0f;
	_vp.MaxDepth = 1.0f;

	_scissor.left = 0;
	_scissor.top = 0;
	_scissor.right = GUI::viewportWidth;
	_scissor.bottom = GUI::viewportHeight;

	D3D12_RESOURCE_DESC texDesc = {};
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Width = GUI::viewportWidth;
	texDesc.Height = GUI::viewportHeight;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	clearValue.Color[0] = 0.2f;
	clearValue.Color[1] = 0.2f;
	clearValue.Color[2] = 0.2f;
	clearValue.Color[3] = 1.0f;

	CD3DX12_HEAP_PROPERTIES rtvheapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(D3D12Core::GraphicsDevice::device->CreateCommittedResource(
		&rtvheapProps,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		&clearValue,
		IID_PPV_ARGS(&_viewportTexture)
	));

	_viewportRTV = DescriptorAllocator::RTV::Allocate();
	D3D12Core::GraphicsDevice::device->CreateRenderTargetView(_viewportTexture.Get(), nullptr, _viewportRTV);

	_viewportSRV_CPU = DescriptorAllocator::CBVSRVUAV::Allocate();
	_viewportSRV_GPU = DescriptorAllocator::CBVSRVUAV::GetGPUHandle(_viewportSRV_CPU);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;

	D3D12Core::GraphicsDevice::device->CreateShaderResourceView(_viewportTexture.Get(), &srvDesc, _viewportSRV_CPU);
}

void Renderer::CreateDepthBuffer()
{
	D3D12_RESOURCE_DESC depthResourceDesc = {};
	depthResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthResourceDesc.Alignment = 0;
	depthResourceDesc.Width = GUI::viewportWidth;
	depthResourceDesc.Height = GUI::viewportHeight;
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

	D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

	ThrowIfFailed(D3D12Core::GraphicsDevice::device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&depthResourceDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthOptimizedClearValue,
		IID_PPV_ARGS(&_depthStencilBuffer)
	));

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

	_dsvCPUHandle = DescriptorAllocator::DSV::Allocate();

	// Create the DSV for the depth-stencil buffer
	D3D12Core::GraphicsDevice::device->CreateDepthStencilView(_depthStencilBuffer.Get(), &dsvDesc, _dsvCPUHandle);
}

void Renderer::CreateConstantBuffers()
{
	_VPBufferDescriptor = DescriptorAllocator::CBVSRVUAV::Allocate();
	_camPosBufferDescriptor = DescriptorAllocator::CBVSRVUAV::Allocate();

	D3D12_RESOURCE_DESC matrixBufferResourceDesc;
	matrixBufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	matrixBufferResourceDesc.Alignment = 0;
	matrixBufferResourceDesc.Width = (sizeof(XMFLOAT4X4) + 255) & ~255;
	matrixBufferResourceDesc.Height = 1;
	matrixBufferResourceDesc.DepthOrArraySize = 1;
	matrixBufferResourceDesc.MipLevels = 1;
	matrixBufferResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	matrixBufferResourceDesc.SampleDesc.Count = 1;
	matrixBufferResourceDesc.SampleDesc.Quality = 0;
	matrixBufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	matrixBufferResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	D3D12_HEAP_PROPERTIES heapProps;
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
	heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProps.CreationNodeMask = 1;
	heapProps.VisibleNodeMask = 1;

	ThrowIfFailed(D3D12Core::GraphicsDevice::device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&matrixBufferResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_VPBufferResource)));

	D3D12_RESOURCE_DESC float3BufferResourceDesc;
	float3BufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	float3BufferResourceDesc.Alignment = 0;
	float3BufferResourceDesc.Width = (sizeof(XMFLOAT3) + 255) & ~255;
	float3BufferResourceDesc.Height = 1;
	float3BufferResourceDesc.DepthOrArraySize = 1;
	float3BufferResourceDesc.MipLevels = 1;
	float3BufferResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	float3BufferResourceDesc.SampleDesc.Count = 1;
	float3BufferResourceDesc.SampleDesc.Quality = 0;
	float3BufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	float3BufferResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	ThrowIfFailed(D3D12Core::GraphicsDevice::device->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&float3BufferResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&_camPosBufferResource)));

	D3D12_CONSTANT_BUFFER_VIEW_DESC vpCbvDesc = {};
	vpCbvDesc.BufferLocation = _VPBufferResource->GetGPUVirtualAddress();
	vpCbvDesc.SizeInBytes = (sizeof(XMFLOAT4X4) + 255) & ~255; // CB size is required to be 256-byte aligned.
	D3D12Core::GraphicsDevice::device->CreateConstantBufferView(&vpCbvDesc, _VPBufferDescriptor);

	D3D12_CONSTANT_BUFFER_VIEW_DESC viewCbvDesc = {};
	viewCbvDesc.BufferLocation = _camPosBufferResource->GetGPUVirtualAddress();
	viewCbvDesc.SizeInBytes = (sizeof(XMFLOAT3) + 255) & ~255; // CB size is required to be 256-byte aligned.
	D3D12Core::GraphicsDevice::device->CreateConstantBufferView(&viewCbvDesc, _camPosBufferDescriptor);

	// setup matrices
	XMStoreFloat4x4(&_projectionMatrix,
		XMMatrixPerspectiveFovLH(
			XMConvertToRadians(45.0f),
			static_cast<float>(GUI::viewportWidth) / static_cast<float>(GUI::viewportHeight),
			0.1f,
			1000.0f)
	);

	_camera = std::make_shared<Camera>(
		XMVectorSet(0.0f, 0.0f, 5.0f, 0.0f),
		XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f),
		90.0f,
		0.0f,
		2.5f,
		0.1f
	);
	_camera->RegisterWithGUI();

	D3D12_RANGE readRange = { 0, 0 };
	ThrowIfFailed(_VPBufferResource->Map(0, &readRange, reinterpret_cast<void**>(&_mappedVPBuffer)));
	memcpy(_mappedVPBuffer, &_viewProjectionMatrix, sizeof(XMFLOAT4X4));
	_VPBufferResource->Unmap(0, nullptr);

	XMFLOAT3 camPos;
	XMStoreFloat3(&camPos, _camera->_position);
	ThrowIfFailed(_camPosBufferResource->Map(0, &readRange, reinterpret_cast<void**>(&_mappedCamPosBuffer)));
	memcpy(_mappedCamPosBuffer, &camPos, sizeof(XMFLOAT3));
	_camPosBufferResource->Unmap(0, nullptr);

	_pLight = std::make_shared<PointLight>(1.0f, 1.0f, 1.0f);
	//_pLight->RegisterWithGUI();

	_dLight = std::make_shared<DirectionalLight>(1.0f, 1.0f, 1.0f, true, 2048);
	_dLight->RegisterWithGUI();

	_samplerCPUHandle = DescriptorAllocator::Sampler::Allocate();

	D3D12_SAMPLER_DESC samplerDesc{};
	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.MinLOD = 0;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
	D3D12Core::GraphicsDevice::device->CreateSampler(&samplerDesc, _samplerCPUHandle);
}

void Renderer::Render(float dt)
{
	UpdateBuffers(dt);
	SetCommandlist();
	GUI::SetViewportTextureHandle(_viewportSRV_GPU);
}

void Renderer::UpdateBuffers(float dt)
{
	if (GUI::viewportResized)
	{
		XMStoreFloat4x4(&_projectionMatrix,
			XMMatrixPerspectiveFovLH(
				XMConvertToRadians(45.0f),
				static_cast<float>(GUI::viewportWidth) / static_cast<float>(GUI::viewportHeight),
				0.1f,
				100.0f)
		);
	}
	
	_camera->ConsumeKey(Window::keys, dt);
	_camera->ConsumeMouse(Window::GetXChange(), Window::GetYChange());
	_camera->Update();
	_viewMatrix = _camera->GetViewMatrix();

	XMFLOAT3 camPos;
	XMStoreFloat3(&camPos, _camera->_position);

	memcpy(_mappedCamPosBuffer, &camPos, sizeof(XMFLOAT3));

	_pLight->UpdateBuffer();
	_dLight->UpdateBuffer();

	XMStoreFloat4x4(&_viewProjectionMatrix, XMMatrixMultiply(XMLoadFloat4x4(&_viewMatrix), XMLoadFloat4x4(&_projectionMatrix)));

	memcpy(_mappedVPBuffer, &_viewProjectionMatrix, sizeof(_viewProjectionMatrix));
}


void Renderer::SetCommandlist()
{
	_mainLoopGraphicsContext.Reset();

	ID3D12DescriptorHeap* heaps[] = { DescriptorAllocator::CBVSRVUAV::GetHeap(), DescriptorAllocator::Sampler::GetHeap() };
	_mainLoopGraphicsContext.GetCommandList()->SetDescriptorHeaps(_countof(heaps), heaps);

	if (_depthPass->_usePass)
	{
		_mainLoopGraphicsContext.SetPipelineState(_depthPass->_pipelineState);
		_mainLoopGraphicsContext.GetCommandList()->SetGraphicsRootSignature(_depthPass->_rootSignature.Get());
		_mainLoopGraphicsContext.GetCommandList()->RSSetViewports(1, &_dLight->_vp);
		_mainLoopGraphicsContext.GetCommandList()->RSSetScissorRects(1, &_dLight->_scissor);

		D3D12_RESOURCE_BARRIER depthmapbarrier = CD3DX12_RESOURCE_BARRIER::Transition(
			_dLight->_directionalShadowMapBuffer.Get(),
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);

		_mainLoopGraphicsContext.GetCommandList()->ResourceBarrier(1, &depthmapbarrier);

		_mainLoopGraphicsContext.GetCommandList()->OMSetRenderTargets(0, nullptr, false, &_dLight->_directionalShadowMapDSVCPUHandle);
		_mainLoopGraphicsContext.GetCommandList()->ClearDepthStencilView(_dLight->_directionalShadowMapDSVCPUHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, '\0', 0, nullptr);

		if (auto slot = _depthPass->GetRootParameterIndex("lightViewProjMatrixBuffer"))
			_mainLoopGraphicsContext.GetCommandList()->SetGraphicsRootDescriptorTable(slot.value(), DescriptorAllocator::CBVSRVUAV::GetGPUHandle(_dLight->_dLightLVPCPUHandle));

		_modelManager.DrawAll(*_depthPass, _mainLoopGraphicsContext);

		D3D12_RESOURCE_BARRIER depthmapbarrierpresent = CD3DX12_RESOURCE_BARRIER::Transition(
			_dLight->_directionalShadowMapBuffer.Get(),
			D3D12_RESOURCE_STATE_DEPTH_WRITE,
			D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);

		_mainLoopGraphicsContext.GetCommandList()->ResourceBarrier(1, &depthmapbarrierpresent);
	}

	D3D12_RESOURCE_BARRIER renderTargetBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
		_viewportTexture.Get(),
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);

	_mainLoopGraphicsContext.GetCommandList()->ResourceBarrier(1, &renderTargetBarrier);
	_mainLoopGraphicsContext.GetCommandList()->OMSetRenderTargets(1, &_viewportRTV, false, &_dsvCPUHandle);

	// Clear the render target.
	_mainLoopGraphicsContext.GetCommandList()->ClearDepthStencilView(_dsvCPUHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, '\0', 0, nullptr);
	const float clearColor[] = { 0.2f, 0.2f, 0.2f, 1.0f };
	_mainLoopGraphicsContext.GetCommandList()->ClearRenderTargetView(_viewportRTV, clearColor, 0, nullptr);

	if (_mainPass->_usePass)
	{
		_mainLoopGraphicsContext.SetPipelineState(_mainPass->_pipelineState);
		_mainLoopGraphicsContext.GetCommandList()->SetGraphicsRootSignature(_mainPass->_rootSignature.Get());
		_mainLoopGraphicsContext.GetCommandList()->RSSetViewports(1, &_vp);
		_mainLoopGraphicsContext.GetCommandList()->RSSetScissorRects(1, &_scissor);

		if (auto slot = _mainPass->GetRootParameterIndex("viewProjMatrixBuffer"))
			_mainLoopGraphicsContext.GetCommandList()->SetGraphicsRootDescriptorTable(slot.value(), DescriptorAllocator::CBVSRVUAV::GetGPUHandle(_VPBufferDescriptor));

		if (auto slot = _mainPass->GetRootParameterIndex("cameraBuffer"))
			_mainLoopGraphicsContext.GetCommandList()->SetGraphicsRootDescriptorTable(slot.value(), DescriptorAllocator::CBVSRVUAV::GetGPUHandle(_camPosBufferDescriptor));

		if (auto slot = _mainPass->GetRootParameterIndex("plightBuffer"))
			_mainLoopGraphicsContext.GetCommandList()->SetGraphicsRootDescriptorTable(slot.value(), DescriptorAllocator::CBVSRVUAV::GetGPUHandle(_pLight->_cbvpLightCPUHandle));

		if (auto slot = _mainPass->GetRootParameterIndex("dlightBuffer"))
			_mainLoopGraphicsContext.GetCommandList()->SetGraphicsRootDescriptorTable(slot.value(), DescriptorAllocator::CBVSRVUAV::GetGPUHandle(_dLight->_dLightDirectionCPUHandle));

		if (auto slot = _mainPass->GetRootParameterIndex("mySampler"))
			_mainLoopGraphicsContext.GetCommandList()->SetGraphicsRootDescriptorTable(slot.value(), DescriptorAllocator::Sampler::GetGPUHandle(_samplerCPUHandle));

		_modelManager.DrawAll(*_mainPass, _mainLoopGraphicsContext);
	}

	if (_bbPass->_usePass)
	{
		_mainLoopGraphicsContext.SetPipelineState(_bbPass->_pipelineState);
		_mainLoopGraphicsContext.GetCommandList()->SetGraphicsRootSignature(_bbPass->_rootSignature.Get());
		_mainLoopGraphicsContext.GetCommandList()->RSSetViewports(1, &_vp);
		_mainLoopGraphicsContext.GetCommandList()->RSSetScissorRects(1, &_scissor);

		_mainLoopGraphicsContext.SetPipelineState(_bbPass->_pipelineState);
		_mainLoopGraphicsContext.GetCommandList()->SetGraphicsRootSignature(_bbPass->_rootSignature.Get());

		if (auto slot = _bbPass->GetRootParameterIndex("viewProjMatrixBuffer"))
			_mainLoopGraphicsContext.GetCommandList()->SetGraphicsRootDescriptorTable(slot.value(), DescriptorAllocator::CBVSRVUAV::GetGPUHandle(_VPBufferDescriptor));

		_modelManager.DrawAllBoundingBoxes(*_bbPass, _mainLoopGraphicsContext);
	}

	D3D12_RESOURCE_BARRIER srvBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
		_viewportTexture.Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);

	_mainLoopGraphicsContext.GetCommandList()->ResourceBarrier(1, &srvBarrier);

	_mainLoopGraphicsContext.Finish(true);
}

void Renderer::Shutdown()
{
	CommandQueueManager::GetCommandQueue(QUEUETYPE::QUEUE_GRAPHICS).WaitForFence();
}
