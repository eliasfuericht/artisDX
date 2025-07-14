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

	UINT dxgiFactoryFlags = 0;
#if defined(_DEBUG)
	dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
	D3D12Core::GraphicsDevice::InitializeDebugController();
#endif

	D3D12Core::GraphicsDevice::InitializeFactory(dxgiFactoryFlags);
	D3D12Core::GraphicsDevice::InitializeAdapter();
	D3D12Core::GraphicsDevice::InitializeDevice();

#if defined(_DEBUG)
	D3D12Core::GraphicsDevice::IntializeDebugDevice();
#endif

	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	D3D12Core::CommandQueue::InitializeCommandQueue(queueDesc);

	// Create Command Allocator - still stored in application, think about better place
	ThrowIfFailed(D3D12Core::GraphicsDevice::GetDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_commandAllocator)), "CommandAllocator creation failed!");

	D3D12Core::Swapchain::InitializeSwapchain(_width, _height, _window.GetHWND());

	DescriptorAllocator::Instance().Initialize(NUM_MAX_DESCRIPTORS);
}

void Application::InitResources()
{
	// Create the root signature.
	{
		D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};

		// This is the highest version the sample supports. If
		// CheckFeatureSupport succeeds, the HighestVersion returned will not be
		// greater than this.
		featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

		if (FAILED(D3D12Core::GraphicsDevice::GetDevice()->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData,sizeof(featureData))))
			featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;

		D3D12_DESCRIPTOR_RANGE1 cbvRangeViewProjMatrix = {};
		cbvRangeViewProjMatrix.BaseShaderRegister = 0; // b0
		cbvRangeViewProjMatrix.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		cbvRangeViewProjMatrix.NumDescriptors = 1;
		cbvRangeViewProjMatrix.RegisterSpace = 0;
		cbvRangeViewProjMatrix.OffsetInDescriptorsFromTableStart = 0;
		cbvRangeViewProjMatrix.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

		D3D12_DESCRIPTOR_RANGE1 cbvRangeModelMatrix = {};
		cbvRangeModelMatrix.BaseShaderRegister = 1; // b1
		cbvRangeModelMatrix.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		cbvRangeModelMatrix.NumDescriptors = 1;
		cbvRangeModelMatrix.RegisterSpace = 0;
		cbvRangeModelMatrix.OffsetInDescriptorsFromTableStart = 0;
		cbvRangeModelMatrix.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

		D3D12_DESCRIPTOR_RANGE1 cbvRangeCameraPos = {};
		cbvRangeCameraPos.BaseShaderRegister = 2; // b2
		cbvRangeCameraPos.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		cbvRangeCameraPos.NumDescriptors = 1;
		cbvRangeCameraPos.RegisterSpace = 0;
		cbvRangeCameraPos.OffsetInDescriptorsFromTableStart = 0;
		cbvRangeCameraPos.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
		
		D3D12_DESCRIPTOR_RANGE1 cbvRangePointLight = {};
		cbvRangePointLight.BaseShaderRegister = 3; // b3
		cbvRangePointLight.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		cbvRangePointLight.NumDescriptors = 1;
		cbvRangePointLight.RegisterSpace = 0;
		cbvRangePointLight.OffsetInDescriptorsFromTableStart = 0;
		cbvRangePointLight.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

		D3D12_DESCRIPTOR_RANGE1 srvRanges[5] = {};

		for (int i = 0; i < 5; ++i) {
			srvRanges[i].BaseShaderRegister = i; // t0 to t4
			srvRanges[i].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			srvRanges[i].NumDescriptors = 1;
			srvRanges[i].RegisterSpace = 0;
			srvRanges[i].OffsetInDescriptorsFromTableStart = 0;
			srvRanges[i].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
		}

		D3D12_ROOT_PARAMETER1 rootParameters[9] = {};

		// View-Projection Matrix Buffer
		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
		rootParameters[0].DescriptorTable.pDescriptorRanges = &cbvRangeViewProjMatrix;

		// Model Matrix Buffer
		rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		rootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
		rootParameters[1].DescriptorTable.pDescriptorRanges = &cbvRangeModelMatrix;

		// View Matrix Buffer
		rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[2].DescriptorTable.NumDescriptorRanges = 1;
		rootParameters[2].DescriptorTable.pDescriptorRanges = &cbvRangeCameraPos;
		
		// PointLight Buffer
		rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[3].DescriptorTable.NumDescriptorRanges = 1;
		rootParameters[3].DescriptorTable.pDescriptorRanges = &cbvRangePointLight;

		// textures ugly asf need to rethink
		for (int i = 0; i < 5; ++i) {
			rootParameters[i + 4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			rootParameters[i + 4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
			rootParameters[i + 4].DescriptorTable.NumDescriptorRanges = 1;
			rootParameters[i + 4].DescriptorTable.pDescriptorRanges = &srvRanges[i];
		}

		CD3DX12_STATIC_SAMPLER_DESC staticSampler{ 0, D3D12_FILTER_MIN_MAG_MIP_LINEAR };

		D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
		rootSignatureDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		rootSignatureDesc.Desc_1_1.NumParameters = _countof(rootParameters);
		rootSignatureDesc.Desc_1_1.pParameters = rootParameters;
		rootSignatureDesc.Desc_1_1.NumStaticSamplers = 1;
		rootSignatureDesc.Desc_1_1.pStaticSamplers = &staticSampler;

		MSWRL::ComPtr<ID3DBlob> signature;
		MSWRL::ComPtr<ID3DBlob> error;

		ThrowIfFailed(D3D12SerializeVersionedRootSignature(&rootSignatureDesc, &signature, &error));
		ThrowIfFailed(D3D12Core::GraphicsDevice::GetDevice()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&_rootSignature)), "Root Signature creation failed!");
		_rootSignature->SetName(L"artisDX_rootSignature");
	}

	// Create the pipeline state, which includes compiling and loading shaders.
	{
		MSWRL::ComPtr<ID3DBlob> vertexShader;
		MSWRL::ComPtr<ID3DBlob> pixelShader;
		MSWRL::ComPtr<ID3DBlob> errors;

#if defined(_DEBUG)
		// Enable better shader debugging with the graphics debugging tools.
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlags = 0;
#endif

		std::string path = "";
		char pBuf[1024];

		_getcwd(pBuf, 1024);
		path = pBuf;
		path += "\\..\\";
		std::wstring wpath = std::wstring(path.begin(), path.end());

		std::filesystem::create_directories(path + "shaders\\compiled");

		std::string vertCompiledPath = path + "shaders\\compiled\\vert.dxbc";
		std::string fragCompiledPath = path + "shaders\\compiled\\frag.dxbc";

#define COMPILESHADERS
#ifdef COMPILESHADERS
		std::wstring vertPath = wpath + L"shaders\\pbr_vert.fx";
		std::wstring fragPath = wpath + L"shaders\\pbr_frag.fx";
				
		try
		{
			ThrowIfFailed(D3DCompileFromFile(vertPath.c_str(), nullptr, nullptr,
				"main", "vs_5_0", compileFlags, 0,
				&vertexShader, &errors));
			ThrowIfFailed(D3DCompileFromFile(fragPath.c_str(), nullptr, nullptr,
				"main", "ps_5_0", compileFlags, 0,
				&pixelShader, &errors));
		}
		catch (std::exception e)
		{
			const char* errStr = (const char*)errors->GetBufferPointer();
			OutputDebugStringA(errStr);
			errors->Release();
			errors = nullptr;
		}

		std::ofstream vsOut(vertCompiledPath, std::ios::out | std::ios::binary);
		std::ofstream	fsOut(fragCompiledPath, std::ios::out | std::ios::binary);

		vsOut.write((const char*)vertexShader->GetBufferPointer(), vertexShader->GetBufferSize());
		fsOut.write((const char*)pixelShader->GetBufferPointer(), pixelShader->GetBufferSize());

#else
		std::vector<char> vsBytecodeData = readFile(vertCompiledPath);
		std::vector<char> fsBytecodeData = readFile(fragCompiledPath);

#endif

		_pLight = std::make_shared<PointLight>(1.0f, 1.0f, 1.0f);
		_pLight->RegisterWithGUI();

		// Constant buffers
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

			ThrowIfFailed(D3D12Core::GraphicsDevice::GetDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&_VPBufferHeap)));
			ThrowIfFailed(D3D12Core::GraphicsDevice::GetDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&_camPosBufferHeap)));

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

			ThrowIfFailed(D3D12Core::GraphicsDevice::GetDevice()->CreateCommittedResource(
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

			ThrowIfFailed(D3D12Core::GraphicsDevice::GetDevice()->CreateCommittedResource(
				&heapProps,
				D3D12_HEAP_FLAG_NONE,
				&camPosCBResourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&_camPosBufferResource)));

			_camPosBufferHeap->SetName(L"Cam Pos Constant Buffer Upload Heap");

			D3D12_CPU_DESCRIPTOR_HANDLE vpCbvCpuHandle = DescriptorAllocator::Instance().Allocate();
			D3D12_CONSTANT_BUFFER_VIEW_DESC vpCbvDesc = {};
			vpCbvDesc.BufferLocation = _VPBufferResource->GetGPUVirtualAddress();
			vpCbvDesc.SizeInBytes = (sizeof(XMFLOAT4X4) + 255) & ~255; // CB size is required to be 256-byte aligned.
			D3D12Core::GraphicsDevice::GetDevice()->CreateConstantBufferView(&vpCbvDesc, vpCbvCpuHandle);
			_VPBufferDescriptor = vpCbvCpuHandle; // viewProjMatrix

			D3D12_CPU_DESCRIPTOR_HANDLE viewCbvCpuHandle = DescriptorAllocator::Instance().Allocate();
			D3D12_CONSTANT_BUFFER_VIEW_DESC viewCbvDesc = {};
			viewCbvDesc.BufferLocation = _camPosBufferResource->GetGPUVirtualAddress();
			viewCbvDesc.SizeInBytes = (sizeof(XMFLOAT3) + 255) & ~255; // CB size is required to be 256-byte aligned.
			D3D12Core::GraphicsDevice::GetDevice()->CreateConstantBufferView(&viewCbvDesc, viewCbvCpuHandle);
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
		}

		// Define the vertex input layout.
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 48, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		};

		// Describe and create the graphics pipeline state object (PSO).
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
		psoDesc.pRootSignature = _rootSignature.Get();

		D3D12_SHADER_BYTECODE vsBytecode;
		D3D12_SHADER_BYTECODE psBytecode;

		vsBytecode.pShaderBytecode = vertexShader->GetBufferPointer();
		vsBytecode.BytecodeLength = vertexShader->GetBufferSize();
		psoDesc.VS = vsBytecode;

		psBytecode.pShaderBytecode = pixelShader->GetBufferPointer();
		psBytecode.BytecodeLength = pixelShader->GetBufferSize();
		psoDesc.PS = psBytecode;

		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

		D3D12_RENDER_TARGET_BLEND_DESC transparencyBlendDesc = {};
		transparencyBlendDesc.BlendEnable = true;
		transparencyBlendDesc.LogicOpEnable = false;
		transparencyBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
		transparencyBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		transparencyBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
		transparencyBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
		transparencyBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
		transparencyBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
		transparencyBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
		transparencyBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		D3D12_BLEND_DESC blendDesc = {};
		blendDesc.AlphaToCoverageEnable = FALSE;
		blendDesc.IndependentBlendEnable = FALSE;
		blendDesc.RenderTarget[0] = transparencyBlendDesc;

		psoDesc.BlendState = blendDesc;
		psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;

		ThrowIfFailed(D3D12Core::GraphicsDevice::GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&_pipelineState)), "Graphics Pipeline creation failed!");
	}

	ThrowIfFailed(D3D12Core::GraphicsDevice::GetDevice()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _commandAllocator.Get(), _pipelineState.Get(), IID_PPV_ARGS(&_commandList)), "CommandList creation failed!");
	_commandList->SetName(L"Render CommandList");
	
	// MODELLOADING
	_modelManager = ModelManager(_commandList);

	//_modelManager.LoadModel("../assets/helmet.glb");
	//_modelManager.LoadModel("../assets/sponza.glb");
	_modelManager.LoadModel("../assets/brick_wall.glb");
	//_modelManager.LoadModel("../assets/DamagedHelmet.glb");
	//_modelManager.LoadModel("../assets/apollo.glb");

	// upload all textures from models
	ThrowIfFailed(_commandList->Close());

	// Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { _commandList.Get() };
	D3D12Core::CommandQueue::_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	D3D12Core::CommandQueue::WaitForFence();
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
	ThrowIfFailed(_commandList->Reset(_commandAllocator.Get(), _pipelineState.Get()));
	// Set necessary state.
	_commandList->SetGraphicsRootSignature(_rootSignature.Get());
	_commandList->RSSetViewports(1, &D3D12Core::Swapchain::_viewport);
	_commandList->RSSetScissorRects(1, &D3D12Core::Swapchain::_surfaceSize);

	ID3D12DescriptorHeap* heaps[] = { DescriptorAllocator::Instance().GetHeap() };
	_commandList->SetDescriptorHeaps(1, heaps);

	_commandList->SetGraphicsRootDescriptorTable(0, DescriptorAllocator::Instance().GetGPUHandle(_VPBufferDescriptor));
	_commandList->SetGraphicsRootDescriptorTable(2, DescriptorAllocator::Instance().GetGPUHandle(_camPosBufferDescriptor));
	_commandList->SetGraphicsRootDescriptorTable(3, DescriptorAllocator::Instance().GetGPUHandle(_pLight->_cbvpLightCPUHandle));

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

	_modelManager.DrawAll();

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

		D3D12Core::CommandQueue::WaitForFence();
		UpdateFPS();
		UpdateConstantBuffers();
		SetCommandList();
		ExecuteCommandList();
		GUI::Draw();
		Present();
	}

	D3D12Core::CommandQueue::WaitForFence();
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
	D3D12Core::CommandQueue::_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
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
	
	_rootSignature.Reset();
	_pipelineState.Reset();

	_VPBufferResource.Reset();
	_VPBufferHeap.Reset();
	_mappedVPBuffer = nullptr;

	// Cleanup GUI
#if defined(_DEBUG)
	D3D12Core::GraphicsDevice::GetDebugDevice()->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL);
	if (D3D12Core::GraphicsDevice::GetDebugDevice()) D3D12Core::GraphicsDevice::GetDebugDevice().Reset();
	if (D3D12Core::GraphicsDevice::GetDebugController()) D3D12Core::GraphicsDevice::GetDebugController().Reset();
#endif
	PRINT("SHUTDOWN");
}