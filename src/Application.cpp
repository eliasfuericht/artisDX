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
		
	_commandAllocator = nullptr;
	_commandList = nullptr;																														 

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

	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	CommandQueue::InitializeCommandQueue(queueDesc);

	// Create Command Allocator - still stored in application, think about better place
	ThrowIfFailed(D3D12Core::GraphicsDevice::_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_commandAllocator)), "CommandAllocator creation failed!");

	D3D12Core::Swapchain::InitializeSwapchain(_width, _height, _window.GetHWND());

	DescriptorAllocator::Resource::InitializeDescriptorAllocator(NUM_MAX_RESOURCE_DESCRIPTORS);
	DescriptorAllocator::Sampler::InitializeDescriptorAllocator(NUM_MAX_SAMPLER_DESCRIPTORS);
}

void Application::InitResources()
{
	_mainPass;

	_mainPass.AddShader("../shaders/pbr_vert.fx", SHADERTYPE::VERTEX);
	_mainPass.AddShader("../shaders/pbr_frag.fx", SHADERTYPE::PIXEL);

	_mainPass.GenerateGraphicsRootSignature();
	_mainPass.GeneratePipeLineStateObject();
	
	ThrowIfFailed(D3D12Core::GraphicsDevice::_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _commandAllocator.Get(), _mainPass._pipelineState.Get(), IID_PPV_ARGS(&_commandList)), "CommandList creation failed!");
	_commandList->SetName(L"Render CommandList");

	_pLight = std::make_shared<PointLight>(1.0f, 1.0f, 1.0f);
	_pLight->RegisterWithGUI();

	// Constant Buffers and Samplers
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

		ThrowIfFailed(D3D12Core::GraphicsDevice::_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&_VPBufferHeap)));
		ThrowIfFailed(D3D12Core::GraphicsDevice::_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&_camPosBufferHeap)));

		D3D12_RESOURCE_DESC vpCBResourceDesc;
		vpCBResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		vpCBResourceDesc.Alignment = 0;
		vpCBResourceDesc.Width = (sizeof(XMFLOAT4X4) + 255) & ~255;
		vpCBResourceDesc.Height = 1;
		vpCBResourceDesc.DepthOrArraySize = 1;
		vpCBResourceDesc.MipLevels = 1;
		vpCBResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		vpCBResourceDesc.SampleDesc.Count = 1;
		vpCBResourceDesc.SampleDesc.Quality = 0;
		vpCBResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		vpCBResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		ThrowIfFailed(D3D12Core::GraphicsDevice::_device->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&vpCBResourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&_VPBufferResource)));

		_VPBufferHeap->SetName(L"VP Constant Buffer Upload Heap");

		D3D12_RESOURCE_DESC camPosCBResourceDesc;
		camPosCBResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		camPosCBResourceDesc.Alignment = 0;
		camPosCBResourceDesc.Width = (sizeof(XMFLOAT3) + 255) & ~255;
		camPosCBResourceDesc.Height = 1;
		camPosCBResourceDesc.DepthOrArraySize = 1;
		camPosCBResourceDesc.MipLevels = 1;
		camPosCBResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		camPosCBResourceDesc.SampleDesc.Count = 1;
		camPosCBResourceDesc.SampleDesc.Quality = 0;
		camPosCBResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		camPosCBResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		ThrowIfFailed(D3D12Core::GraphicsDevice::_device->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_NONE,
			&camPosCBResourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&_camPosBufferResource)));

		_camPosBufferHeap->SetName(L"Cam Pos Constant Buffer Upload Heap");

		D3D12_CPU_DESCRIPTOR_HANDLE vpCbvCpuHandle = DescriptorAllocator::Resource::Allocate();
		D3D12_CONSTANT_BUFFER_VIEW_DESC vpCbvDesc = {};
		vpCbvDesc.BufferLocation = _VPBufferResource->GetGPUVirtualAddress();
		vpCbvDesc.SizeInBytes = (sizeof(XMFLOAT4X4) + 255) & ~255; // CB size is required to be 256-byte aligned.
		D3D12Core::GraphicsDevice::_device->CreateConstantBufferView(&vpCbvDesc, vpCbvCpuHandle);
		_VPBufferDescriptor = vpCbvCpuHandle; // viewProjMatrix

		D3D12_CPU_DESCRIPTOR_HANDLE viewCbvCpuHandle = DescriptorAllocator::Resource::Allocate();
		D3D12_CONSTANT_BUFFER_VIEW_DESC viewCbvDesc = {};
		viewCbvDesc.BufferLocation = _camPosBufferResource->GetGPUVirtualAddress();
		viewCbvDesc.SizeInBytes = (sizeof(XMFLOAT3) + 255) & ~255; // CB size is required to be 256-byte aligned.
		D3D12Core::GraphicsDevice::_device->CreateConstantBufferView(&viewCbvDesc, viewCbvCpuHandle);
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

		_samplerCPUHandle = DescriptorAllocator::Sampler::Allocate();

		D3D12_SAMPLER_DESC samplerDesc{};
		samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		samplerDesc.MinLOD = 0;
		samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
		D3D12Core::GraphicsDevice::_device->CreateSampler(&samplerDesc, _samplerCPUHandle);
	}
	
	// MODELLOADING
	_modelManager = ModelManager(_commandList);

	//_modelManager.LoadModel("../assets/helmet.glb");
	//_modelManager.LoadModel("../assets/sponza.glb");
	//_modelManager.LoadModel("../assets/brick_wall.glb");
	_modelManager.LoadModel("../assets/DamagedHelmet.glb");
	//_modelManager.LoadModel("../assets/apollo.glb");
	//_modelManager.LoadModel("../assets/new_sponza_curtains.glb");

	// upload all textures from models
	ThrowIfFailed(_commandList->Close());

	// Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { _commandList.Get() };
	CommandQueue::_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	CommandQueue::WaitForFence();
}

void Application::InitGUI()
{
	GUI::Init(_window);
}

void Application::SetCommandList()
{
	// Ensure the command allocator has finished execution before resetting.
	ThrowIfFailed(_commandAllocator->Reset());

	// Reset the command list with the command allocator and pipeline state.
	ThrowIfFailed(_commandList->Reset(_commandAllocator.Get(), _mainPass._pipelineState.Get()));
	// Set necessary state.
	_commandList->SetGraphicsRootSignature(_mainPass._rootSignature.Get());
	_commandList->RSSetViewports(1, &D3D12Core::Swapchain::_viewport);
	_commandList->RSSetScissorRects(1, &D3D12Core::Swapchain::_surfaceSize);

	ID3D12DescriptorHeap* heaps[] = { DescriptorAllocator::Resource::GetHeap(), DescriptorAllocator::Sampler::GetHeap() };
	_commandList->SetDescriptorHeaps(_countof(heaps), heaps);
	
	if (auto slot = _mainPass.GetRootParameterIndex("viewProjMatrixBuffer"))
		_commandList->SetGraphicsRootDescriptorTable(*slot, DescriptorAllocator::Resource::GetGPUHandle(_VPBufferDescriptor));

	if (auto slot = _mainPass.GetRootParameterIndex("cameraPosBuffer"))
		_commandList->SetGraphicsRootDescriptorTable(*slot, DescriptorAllocator::Resource::GetGPUHandle(_camPosBufferDescriptor));

	if (auto slot = _mainPass.GetRootParameterIndex("lightPosBuffer"))
		_commandList->SetGraphicsRootDescriptorTable(*slot, DescriptorAllocator::Resource::GetGPUHandle(_pLight->_cbvpLightCPUHandle));

	if (auto slot = _mainPass.GetRootParameterIndex("mySampler"))
		_commandList->SetGraphicsRootDescriptorTable(*slot, DescriptorAllocator::Sampler::GetGPUHandle(_samplerCPUHandle));

	// Transition the back buffer from present to render target state.
	D3D12_RESOURCE_BARRIER renderTargetBarrier = {};
	renderTargetBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	renderTargetBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	renderTargetBarrier.Transition.pResource = D3D12Core::Swapchain::_renderTargets[D3D12Core::Swapchain::_frameIndex].Get();
	renderTargetBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	renderTargetBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	renderTargetBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	_commandList->ResourceBarrier(1, &renderTargetBarrier);
	
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = D3D12Core::Swapchain::_rtvHeap->GetCPUDescriptorHandleForHeapStart();
	// increments pointer from buffer 0 to 1 if _frameindex is 1
	rtvHandle.ptr += (D3D12Core::Swapchain::_frameIndex * D3D12Core::Swapchain::_rtvDescriptorSize);
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = D3D12Core::Swapchain::_dsvHeap->GetCPUDescriptorHandleForHeapStart();
	_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
	_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0.0f, 0, nullptr);

	// Clear the render target.
	const float clearColor[] = { 0.2f, 0.2f, 0.2f, 1.0f };
	_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

	_modelManager.DrawAll(_mainPass);

	// Transition back buffer to present state for the swap chain (gets transitioned again in the GUI but I#ll leave it like this for now)
	D3D12_RESOURCE_BARRIER presentBarrier = {};
	presentBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	presentBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	presentBarrier.Transition.pResource = D3D12Core::Swapchain::_renderTargets[D3D12Core::Swapchain::_frameIndex].Get();
	presentBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	presentBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	presentBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	_commandList->ResourceBarrier(1, &presentBarrier);

	// Close the command list.
	ThrowIfFailed(_commandList->Close());
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

		CommandQueue::WaitForFence();
		UpdateFPS();
		UpdateConstantBuffers();
		SetCommandList();
		ExecuteCommandList();
		GUI::Draw();
		Present();
	}

	CommandQueue::WaitForFence();
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

	if (D3D12Core::Swapchain::_windowResized)
	{
		D3D12Core::Swapchain::_windowResized = false;

		_width = D3D12Core::Swapchain::_width;
		_height = D3D12Core::Swapchain::_height;

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

	XMStoreFloat4x4(&_viewProjectionMatrix, XMMatrixMultiply(XMLoadFloat4x4(&_viewMatrix), XMLoadFloat4x4(&_projectionMatrix)));

	memcpy(_mappedVPBuffer, &_viewProjectionMatrix, sizeof(_viewProjectionMatrix));
}

void Application::ExecuteCommandList()
{
	// Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { _commandList.Get() };
	CommandQueue::_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
}

void Application::Present()
{
	D3D12Core::Swapchain::_swapchain->Present(0, 0);

	D3D12Core::Swapchain::_frameIndex = D3D12Core::Swapchain::_swapchain->GetCurrentBackBufferIndex();
}

Application::~Application()
{
	CoUninitialize();
	_window.Shutdown();

	// Explicitly release all DX12 objects
	_commandList.Reset();
	_commandAllocator.Reset();
	
	_VPBufferResource.Reset();
	_VPBufferHeap.Reset();
	_mappedVPBuffer = nullptr;

	// Cleanup GUI
#if defined(_DEBUG)
	D3D12Core::GraphicsDevice::_debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL);
	if (D3D12Core::GraphicsDevice::_debugDevice) D3D12Core::GraphicsDevice::_debugDevice.Reset();
	if (D3D12Core::GraphicsDevice::_debugController) D3D12Core::GraphicsDevice::_debugController.Reset();
#endif
	PRINT("SHUTDOWN");
}