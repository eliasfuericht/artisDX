#include "Renderer.h"

void Renderer::InitializeRenderer()
{
	CommandQueueManager::InitializeCommandQueueManager();

	_mainLoopGraphicsContext.InitializeCommandContext(QUEUETYPE::QUEUE_GRAPHICS);
	_mainLoopGraphicsContext.Finish(false);

	DescriptorAllocator::Resource::InitializeDescriptorAllocator(NUM_MAX_RESOURCE_DESCRIPTORS);
	DescriptorAllocator::RenderTarget::InitializeDescriptorAllocator(NUM_MAX_RTV_DESCRIPTORS);
	DescriptorAllocator::Sampler::InitializeDescriptorAllocator(NUM_MAX_SAMPLER_DESCRIPTORS);
}

void Renderer::InitializeResources()
{
	_viewportWidth = GUI::GetViewportWidth();
	_viewportHeight = GUI::GetViewportHeight();

	D3D12_RESOURCE_DESC texDesc = {};
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Width = _viewportWidth;
	texDesc.Height = _viewportHeight;
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
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		&clearValue,
		IID_PPV_ARGS(&_viewportTexture)
	));

	_viewportRTV = DescriptorAllocator::RenderTarget::Allocate();
	D3D12Core::GraphicsDevice::device->CreateRenderTargetView(_viewportTexture.Get(), nullptr, _viewportRTV);

	_viewportSRV_CPU = DescriptorAllocator::Resource::Allocate();
	_viewportSRV_GPU = DescriptorAllocator::Resource::GetGPUHandle(_viewportSRV_CPU);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;

	D3D12Core::GraphicsDevice::device->CreateShaderResourceView(_viewportTexture.Get(), &srvDesc, _viewportSRV_CPU);

	_depthPass = std::make_shared<ShaderPass>("DepthPass");
	_bbPass->_usePass = false;
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

	// Constant Buffers and Samplers
	{
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.NumDescriptors = 1;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

		ThrowIfFailed(D3D12Core::GraphicsDevice::device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&_VPBufferHeap)));
		ThrowIfFailed(D3D12Core::GraphicsDevice::device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&_camPosBufferHeap)));

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

		_VPBufferHeap->SetName(L"VP Constant Buffer Upload Heap");

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

		_camPosBufferHeap->SetName(L"Cam Pos Constant Buffer Upload Heap");

		D3D12_CPU_DESCRIPTOR_HANDLE vpCbvCpuHandle = DescriptorAllocator::Resource::Allocate();
		D3D12_CONSTANT_BUFFER_VIEW_DESC vpCbvDesc = {};
		vpCbvDesc.BufferLocation = _VPBufferResource->GetGPUVirtualAddress();
		vpCbvDesc.SizeInBytes = (sizeof(XMFLOAT4X4) + 255) & ~255; // CB size is required to be 256-byte aligned.
		D3D12Core::GraphicsDevice::device->CreateConstantBufferView(&vpCbvDesc, vpCbvCpuHandle);
		_VPBufferDescriptor = vpCbvCpuHandle; // viewProjMatrix

		D3D12_CPU_DESCRIPTOR_HANDLE viewCbvCpuHandle = DescriptorAllocator::Resource::Allocate();
		D3D12_CONSTANT_BUFFER_VIEW_DESC viewCbvDesc = {};
		viewCbvDesc.BufferLocation = _camPosBufferResource->GetGPUVirtualAddress();
		viewCbvDesc.SizeInBytes = (sizeof(XMFLOAT3) + 255) & ~255; // CB size is required to be 256-byte aligned.
		D3D12Core::GraphicsDevice::device->CreateConstantBufferView(&viewCbvDesc, viewCbvCpuHandle);
		_camPosBufferDescriptor = viewCbvCpuHandle; // viewMatrix

		// Get viewport size from GUI::GetViewportSize()
		// setup matrices
		XMStoreFloat4x4(&_projectionMatrix,
			XMMatrixPerspectiveFovLH(
				XMConvertToRadians(45.0f),
				static_cast<float>(D3D12Core::Swapchain::width) / static_cast<float>(D3D12Core::Swapchain::height),
				0.1f,
				10000.0f)
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

	// MODELLOADING

	//_modelManager.LoadModel("../assets/helmet.glb");
	//_modelManager.LoadModel("../assets/helmets.glb");
	//_modelManager.LoadModel("../assets/sponza.glb");
	//_modelManager.LoadModel("../assets/brick_wall.glb");
	_modelManager.LoadModel("../assets/DamagedHelmet.glb");
	//_modelManager.LoadModel("../assets/apollo.glb");
	//_modelManager.LoadModel("../assets/bistro.glb");
}

void Renderer::Render(float dt)
{
	UpdateBuffers(dt);
	SetCommandlist();
	//GUI::SetViewportTextureHandle(_samplerCPUHandle);
}

void Renderer::UpdateBuffers(float dt)
{
	if (D3D12Core::Swapchain::windowResized)
	{
		D3D12Core::Swapchain::windowResized = false;

		// TODO: Get viewport size from GUI::GetViewportSize()
		XMStoreFloat4x4(&_projectionMatrix,
			XMMatrixPerspectiveFovLH(
				XMConvertToRadians(45.0f),
				static_cast<float>(Window::width) / static_cast<float>(Window::height),
				0.1f,
				10000.0f)
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
	_mainLoopGraphicsContext.SetPipelineState(_mainPass->_pipelineState);

	// Set necessary state.
	_mainLoopGraphicsContext.GetCommandList()->SetGraphicsRootSignature(_mainPass->_rootSignature.Get());
	_mainLoopGraphicsContext.GetCommandList()->RSSetViewports(1, &D3D12Core::Swapchain::viewport);
	_mainLoopGraphicsContext.GetCommandList()->RSSetScissorRects(1, &D3D12Core::Swapchain::surfaceSize);

	ID3D12DescriptorHeap* heaps[] = { DescriptorAllocator::Resource::GetHeap(), DescriptorAllocator::Sampler::GetHeap() };
	_mainLoopGraphicsContext.GetCommandList()->SetDescriptorHeaps(_countof(heaps), heaps);

	if (_depthPass->_usePass)
	{
		D3D12_CPU_DESCRIPTOR_HANDLE shadowMapHandle = _dLight->_directionalShadowMapHeap->GetCPUDescriptorHandleForHeapStart();
		_mainLoopGraphicsContext.GetCommandList()->OMSetRenderTargets(0, nullptr, false, &shadowMapHandle);
		_mainLoopGraphicsContext.GetCommandList()->ClearDepthStencilView(shadowMapHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0.0f, 0, nullptr);

		if (auto slot = _depthPass->GetRootParameterIndex("lightViewProjMatrixBuffer"))
			_mainLoopGraphicsContext.GetCommandList()->SetGraphicsRootDescriptorTable(slot.value(), DescriptorAllocator::Resource::GetGPUHandle(_dLight->_dLightLVPCPUHandle));

		_modelManager.DrawAll(*_depthPass, _mainLoopGraphicsContext);
	}

	D3D12_RESOURCE_BARRIER renderTargetBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
		D3D12Core::Swapchain::renderTargets[D3D12Core::Swapchain::frameIndex].Get(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);

	_mainLoopGraphicsContext.GetCommandList()->ResourceBarrier(1, &renderTargetBarrier);

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = D3D12Core::Swapchain::rtvHeap->GetCPUDescriptorHandleForHeapStart();
	rtvHandle.ptr += (D3D12Core::Swapchain::frameIndex * D3D12Core::Swapchain::rtvDescriptorSize);
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = D3D12Core::Swapchain::dsvHeap->GetCPUDescriptorHandleForHeapStart();
	_mainLoopGraphicsContext.GetCommandList()->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);
	_mainLoopGraphicsContext.GetCommandList()->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0.0f, 0, nullptr);

	// Clear the render target.
	const float clearColor[] = { 0.2f, 0.2f, 0.2f, 1.0f };
	_mainLoopGraphicsContext.GetCommandList()->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

	if (_mainPass->_usePass)
	{
		if (auto slot = _mainPass->GetRootParameterIndex("viewProjMatrixBuffer"))
			_mainLoopGraphicsContext.GetCommandList()->SetGraphicsRootDescriptorTable(slot.value(), DescriptorAllocator::Resource::GetGPUHandle(_VPBufferDescriptor));

		if (auto slot = _mainPass->GetRootParameterIndex("cameraBuffer"))
			_mainLoopGraphicsContext.GetCommandList()->SetGraphicsRootDescriptorTable(slot.value(), DescriptorAllocator::Resource::GetGPUHandle(_camPosBufferDescriptor));

		if (auto slot = _mainPass->GetRootParameterIndex("plightBuffer"))
			_mainLoopGraphicsContext.GetCommandList()->SetGraphicsRootDescriptorTable(slot.value(), DescriptorAllocator::Resource::GetGPUHandle(_pLight->_cbvpLightCPUHandle));

		if (auto slot = _mainPass->GetRootParameterIndex("dlightBuffer"))
			_mainLoopGraphicsContext.GetCommandList()->SetGraphicsRootDescriptorTable(slot.value(), DescriptorAllocator::Resource::GetGPUHandle(_dLight->_dLightDirectionCPUHandle));

		if (auto slot = _mainPass->GetRootParameterIndex("mySampler"))
			_mainLoopGraphicsContext.GetCommandList()->SetGraphicsRootDescriptorTable(slot.value(), DescriptorAllocator::Sampler::GetGPUHandle(_samplerCPUHandle));

		_modelManager.DrawAll(*_mainPass, _mainLoopGraphicsContext);
	}

	if (_bbPass->_usePass)
	{
		_mainLoopGraphicsContext.SetPipelineState(_bbPass->_pipelineState);
		_mainLoopGraphicsContext.GetCommandList()->SetGraphicsRootSignature(_bbPass->_rootSignature.Get());

		if (auto slot = _bbPass->GetRootParameterIndex("viewProjMatrixBuffer"))
			_mainLoopGraphicsContext.GetCommandList()->SetGraphicsRootDescriptorTable(slot.value(), DescriptorAllocator::Resource::GetGPUHandle(_VPBufferDescriptor));

		_modelManager.DrawAllBoundingBoxes(*_bbPass, _mainLoopGraphicsContext);
	}

	D3D12_RESOURCE_BARRIER presentBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
		D3D12Core::Swapchain::renderTargets[D3D12Core::Swapchain::frameIndex].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);

	_mainLoopGraphicsContext.GetCommandList()->ResourceBarrier(1, &presentBarrier);

	_mainLoopGraphicsContext.Finish(true);
}

void Renderer::Shutdown()
{
	CommandQueueManager::GetCommandQueue(QUEUETYPE::QUEUE_GRAPHICS).WaitForFence();
}
