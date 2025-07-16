#include "Application.h"

Application::Application(const CHAR* name, INT w, INT h, bool fullscreen)
	: _window(name, w, h, fullscreen)
{
	if (fullscreen)
	{
		_width = GetSystemMetrics(SM_CXSCREEN);
		_height = GetSystemMetrics(SM_CYSCREEN);
	}
	else
	{
		_width = w;
		_height = h;
	}																													 

	_camera = std::make_shared<Camera>(
		XMVectorSet(0.0f, 0.0f, 5.0f, 0.0f),
		XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f),
		90.0f,
		0.0f,
		2.5f,
		0.1f 
	);

	_camera->RegisterWithGUI();

	Init();
	InitResources();
	InitGUI();
}

void Application::Init()
{
	ThrowIfFailed(CoInitializeEx(nullptr, COINIT_MULTITHREADED));

#if defined(_DEBUG)
	D3D12Core::GraphicsDevice::InitializeDebugController();
#endif

	D3D12Core::GraphicsDevice::InitializeFactory();
	D3D12Core::GraphicsDevice::InitializeAdapter();
	D3D12Core::GraphicsDevice::InitializeDevice();

#if defined(_DEBUG)
	D3D12Core::GraphicsDevice::IntializeDebugDevice();
#endif

	CommandQueueManager::InitializeCommandQueueManager();

	_mainLoopGraphicsContext.InitializeCommandContext(QUEUETYPE::GRAPHICS);
	_mainLoopGraphicsContext.Finish();

	D3D12Core::Swapchain::InitializeSwapchain(_width, _height, _window.GetHWND());

	DescriptorAllocator::Resource::InitializeDescriptorAllocator(NUM_MAX_RESOURCE_DESCRIPTORS);
	DescriptorAllocator::Sampler::InitializeDescriptorAllocator(NUM_MAX_SAMPLER_DESCRIPTORS);
}

void Application::InitResources()
{
	_mainPass.AddShader("../shaders/pbr_vert.hlsl", SHADERTYPE::VERTEX);
	_mainPass.AddShader("../shaders/pbr_frag.hlsl", SHADERTYPE::PIXEL);

	_mainPass.GenerateGraphicsRootSignature();
	_mainPass.GeneratePipeLineStateObjectForwardPass(D3D12_FILL_MODE_SOLID, D3D12_CULL_MODE_BACK, true);
	_mainPass.GeneratePipeLineStateObjectForwardPass(D3D12_FILL_MODE_WIREFRAME, D3D12_CULL_MODE_NONE, false);

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

		// setup matrices
		XMStoreFloat4x4(&_projectionMatrix,
			XMMatrixPerspectiveFovLH(
				XMConvertToRadians(45.0f),
				static_cast<float>(_window.GetWidth()) / static_cast<float>(_window.GetHeight()),
				0.1f,
				10000.0f)
		);

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
		_pLight->RegisterWithGUI();

		_dLight = std::make_shared<DirectionalLight>(1.0f, 1.0f, 1.0f);
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
	_modelManager.LoadModel("../assets/sponza.glb");
	//_modelManager.LoadModel("../assets/brick_wall.glb");
	//_modelManager.LoadModel("../assets/DamagedHelmet.glb");
	//_modelManager.LoadModel("../assets/apollo.glb");
}

void Application::InitGUI()
{
	GUI::Init(_window);
}

void Application::SetCommandList()
{
	_mainLoopGraphicsContext.Reset();
	_mainLoopGraphicsContext.SetPipelineState(_mainPass._pipelineStateFill);

	// Set necessary state.
	_mainLoopGraphicsContext.GetCommandList()->SetGraphicsRootSignature(_mainPass._rootSignature.Get());
	_mainLoopGraphicsContext.GetCommandList()->RSSetViewports(1, &D3D12Core::Swapchain::viewport);
	_mainLoopGraphicsContext.GetCommandList()->RSSetScissorRects(1, &D3D12Core::Swapchain::surfaceSize);

	ID3D12DescriptorHeap* heaps[] = { DescriptorAllocator::Resource::GetHeap(), DescriptorAllocator::Sampler::GetHeap() };
	_mainLoopGraphicsContext.GetCommandList()->SetDescriptorHeaps(_countof(heaps), heaps);
	
	if (auto slot = _mainPass.GetRootParameterIndex("viewProjMatrixBuffer"))
		_mainLoopGraphicsContext.GetCommandList()->SetGraphicsRootDescriptorTable(*slot, DescriptorAllocator::Resource::GetGPUHandle(_VPBufferDescriptor));

	if (auto slot = _mainPass.GetRootParameterIndex("cameraBuffer"))
		_mainLoopGraphicsContext.GetCommandList()->SetGraphicsRootDescriptorTable(*slot, DescriptorAllocator::Resource::GetGPUHandle(_camPosBufferDescriptor));

	if (auto slot = _mainPass.GetRootParameterIndex("plightBuffer"))
		_mainLoopGraphicsContext.GetCommandList()->SetGraphicsRootDescriptorTable(*slot, DescriptorAllocator::Resource::GetGPUHandle(_pLight->_cbvpLightCPUHandle));

	if (auto slot = _mainPass.GetRootParameterIndex("dlightBuffer"))
		_mainLoopGraphicsContext.GetCommandList()->SetGraphicsRootDescriptorTable(*slot, DescriptorAllocator::Resource::GetGPUHandle(_dLight->_cbvdLightCPUHandle));

	if (auto slot = _mainPass.GetRootParameterIndex("mySampler"))
		_mainLoopGraphicsContext.GetCommandList()->SetGraphicsRootDescriptorTable(*slot, DescriptorAllocator::Sampler::GetGPUHandle(_samplerCPUHandle));

	D3D12_RESOURCE_BARRIER renderTargetBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
		D3D12Core::Swapchain::renderTargets[D3D12Core::Swapchain::frameIndex].Get(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);
	
	_mainLoopGraphicsContext.GetCommandList()->ResourceBarrier(1, &renderTargetBarrier);
	
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = D3D12Core::Swapchain::rtvHeap->GetCPUDescriptorHandleForHeapStart();
	rtvHandle.ptr += (D3D12Core::Swapchain::frameIndex * D3D12Core::Swapchain::rtvDescriptorSize);
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = D3D12Core::Swapchain::dsvHeap->GetCPUDescriptorHandleForHeapStart();
	_mainLoopGraphicsContext.GetCommandList()->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
	_mainLoopGraphicsContext.GetCommandList()->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0.0f, 0, nullptr);

	// Clear the render target.
	const float clearColor[] = { 0.2f, 0.2f, 0.2f, 1.0f };
	_mainLoopGraphicsContext.GetCommandList()->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

	_modelManager.DrawAll(_mainPass, _mainLoopGraphicsContext);

	_mainLoopGraphicsContext.SetPipelineState(_mainPass._pipelineStateWireframe);
	_mainLoopGraphicsContext.GetCommandList()->SetGraphicsRootSignature(_mainPass._rootSignature.Get());
	
	_modelManager.DrawAllBoundingBoxes(_mainPass, _mainLoopGraphicsContext);

	D3D12_RESOURCE_BARRIER presentBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
		D3D12Core::Swapchain::renderTargets[D3D12Core::Swapchain::frameIndex].Get(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES);

	_mainLoopGraphicsContext.GetCommandList()->ResourceBarrier(1, &presentBarrier);

	_mainLoopGraphicsContext.Finish();
}

void Application::Run()
{
	_lastTime = std::chrono::high_resolution_clock::now(); // Initialize timing
	_window.Show();
	MSG msg = { 0 };

	bool running = true;

	while (running)
	{
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
			{
				running = false;
				break;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (!running)
			break;

		CommandQueueManager::GetCommandQueue(QUEUETYPE::GRAPHICS).WaitForFence();
		UpdateFPS();
		UpdateConstantBuffers();
		SetCommandList();
		GUI::Draw();
		Present();
	}

	CommandQueueManager::GetCommandQueue(QUEUETYPE::GRAPHICS).WaitForFence();
	GUI::Shutdown();
}

void Application::UpdateFPS()
{
	auto now = std::chrono::high_resolution_clock::now();
	double deltaTime = std::chrono::duration<double>(now - _lastTime).count();
	_lastTime = now;

	_elapsedTime += deltaTime;
	_frameCount++;

	if (_elapsedTime >= 1.0)
	{
		_fps = _frameCount / _elapsedTime; 
		_frameCount = 0;
		_elapsedTime = 0.0;
		char title[256];
		sprintf_s(title, "artisDX - FPS: %.2f", _fps);
		SetWindowTextA(_window.GetHWND(), title);
	}
}

void Application::UpdateConstantBuffers()
{
	std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
	std::chrono::duration<float> dt = now - _tLastTime;
	FLOAT deltaTime = dt.count();
	_tLastTime = now;

	if (D3D12Core::Swapchain::windowResized)
	{
		D3D12Core::Swapchain::windowResized = false;

		_width = D3D12Core::Swapchain::width;
		_height = D3D12Core::Swapchain::height;

		XMStoreFloat4x4(&_projectionMatrix,
			XMMatrixPerspectiveFovLH(
				XMConvertToRadians(45.0f),
				static_cast<float>(_window.GetWidth()) / static_cast<float>(_window.GetHeight()),
				0.1f,
				10000.0f)
		);
	}
	_camera->ConsumeKey(_window.GetKeys(), deltaTime);
	_camera->ConsumeMouse(_window.GetXChange(), _window.GetYChange());
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

void Application::Present()
{
	D3D12Core::Swapchain::swapchain->Present(0, 0);

	D3D12Core::Swapchain::frameIndex = D3D12Core::Swapchain::swapchain->GetCurrentBackBufferIndex();
}

Application::~Application()
{
	CoUninitialize();
	_window.Shutdown();
	
	_VPBufferResource.Reset();
	_VPBufferHeap.Reset();
	_mappedVPBuffer = nullptr;

	// Cleanup GUI
#if defined(_DEBUG)
	D3D12Core::GraphicsDevice::debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL);
	if (D3D12Core::GraphicsDevice::debugDevice) D3D12Core::GraphicsDevice::debugDevice.Reset();
	if (D3D12Core::GraphicsDevice::debugController) D3D12Core::GraphicsDevice::debugController.Reset();
#endif
	PRINT("SHUTDOWN");
}