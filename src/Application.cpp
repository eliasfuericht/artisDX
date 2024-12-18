#include "Application.h"

Application::Application(const CHAR* name, INT w, INT h)
	: _window(name, w, h)
{
	_factory = nullptr;
	_adapter = nullptr;

#if defined(_DEBUG)																	 
	_debugController = nullptr;
	_debugDevice = nullptr;
#endif					

	_device = nullptr;
	_commandQueue = nullptr;
	_commandAllocator = nullptr;
	_commandList = nullptr;

	_rtvHeap = nullptr;
	_srvHeap = nullptr;
	for (size_t i = 0; i < _backBufferCount; ++i)
	{
		_renderTargets[i] = nullptr;
	}
	_swapchain = nullptr;

	// Sync																																					 
	_frameIndex = 0;
	_fenceEvent = nullptr;
	_fence = nullptr;
	_fenceValue = 0;

	_camera = Camera(
		glm::vec3(0.0f, 0.0f, -1.0f),
		glm::vec3(0.0f, 1.0f, 0.0f),
		90.0f,
		0.0f,
		0.5f,
		0.1f
	);

	InitDX12();
	InitSwapchain(w, h);
	InitResources();
	InitIMGUI();
	InitCommands();
}

void Application::InitIMGUI()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	_imguiIO = &ImGui::GetIO();
	(void)_imguiIO;
	_imguiIO->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	_imguiIO->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	_imguiIO->ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
	_imguiIO->ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows
	//io.ConfigViewportsNoAutoMerge = true;
	//io.ConfigViewportsNoTaskBarIcon = true;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();

	// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
	ImGuiStyle& style = ImGui::GetStyle();
	if (_imguiIO->ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	ImGui_ImplWin32_Init(_window.GetHWND());
	ImGui_ImplDX12_Init(_device.Get(), 3, DXGI_FORMAT_R8G8B8A8_UNORM, _srvHeap.Get(), _srvHeap->GetCPUDescriptorHandleForHeapStart(), _srvHeap->GetGPUDescriptorHandleForHeapStart());
}

void Application::InitDX12()
{
	// Create Factory
	UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
	MSWRL::ComPtr<ID3D12Debug> debugController;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
	ThrowIfFailed(debugController->QueryInterface(IID_PPV_ARGS(&_debugController)));
	_debugController->EnableDebugLayer();
	_debugController->SetEnableGPUBasedValidation(true);

	dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&_factory)));

	SIZE_T maxMemSize = 0;

	// iterate over all available adapters
	for (UINT adapterIndex = 0; ; ++adapterIndex)
	{
		MSWRL::ComPtr<IDXGIAdapter1> adapter;
		if (_factory->EnumAdapters1(adapterIndex, &adapter) == DXGI_ERROR_NOT_FOUND)
			break;

		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);

		// Skip software adapters
		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			continue;

		// Check if the adapter supports Direct3D 12
		if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), nullptr)))
		{
			// Update if this adapter has more dedicated video memory than previous best
			if (desc.DedicatedVideoMemory > maxMemSize)
			{
				maxMemSize = desc.DedicatedVideoMemory;
				_adapter = adapter; // Store the adapter with the largest video memory
			}
		}
		// Adapter will be released automatically at the end of scope
	}

	// Create Device
	// The device is the interface between the program(CPU) and the adapter(GPU)
	ThrowIfFailed(D3D12CreateDevice(_adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&_device)), "Device creation failed!");
	_device->SetName(L"artisDX_Device");

#if defined(_DEBUG)
	// Get debug device
	ThrowIfFailed(_device->QueryInterface(_debugDevice.GetAddressOf()));
#endif

	// Create Command Queue
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	// D3D12_COMMAND_LIST_TYPE_DIRECT = normal graphics pipeline
	// D3D12_COMMAND_LIST_TYPE_COMPUTE = compute pipeline
	// ...

	ThrowIfFailed(_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&_commandQueue)),"CommandQueue creation failed!");

	// Create Command Allocator
	ThrowIfFailed(_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_commandAllocator)));

	// Sync
	ThrowIfFailed(_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence)));
}

void Application::InitSwapchain(UINT w, UINT h)
{
	// Wait for the GPU to finish with the previous frame
	const UINT64 fence = _fenceValue;
	ThrowIfFailed(_commandQueue->Signal(_fence.Get(), fence));
	_fenceValue++;
	if (_fence->GetCompletedValue() < fence)
	{
		ThrowIfFailed(_fence->SetEventOnCompletion(fence, _fenceEvent));
		WaitForSingleObjectEx(_fenceEvent, INFINITE, false);
	}

	// Clean up old resources before resizing
	for (UINT n = 0; n < _backBufferCount; n++)
	{
		_renderTargets[n].Reset();  // Release old render targets
	}
	_rtvHeap.Reset();  // Release old descriptor heap

	_surfaceSize.left = 0;
	_surfaceSize.top = 0;
	_surfaceSize.right = static_cast<LONG>(w);
	_surfaceSize.bottom = static_cast<LONG>(h);

	_viewport.TopLeftX = 0.0f;
	_viewport.TopLeftY = 0.0f;
	_viewport.Width = static_cast<float>(w);
	_viewport.Height = static_cast<float>(h);
	_viewport.MinDepth = .1f;
	_viewport.MaxDepth = 1000.f;

	if (_swapchain != nullptr)
	{
		// Resize the swapchain buffers
		_swapchain->ResizeBuffers(_backBufferCount, w, h, DXGI_FORMAT_R8G8B8A8_UNORM, 0);
	}
	else
	{
		// Create the swapchain if it doesn't exist
		DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
		swapchainDesc.BufferCount = _backBufferCount;
		swapchainDesc.Width = w;
		swapchainDesc.Height = h;
		swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapchainDesc.SampleDesc.Count = 1;

		MSWRL::ComPtr<IDXGISwapChain1> swapchain;
		ThrowIfFailed(_factory->CreateSwapChainForHwnd(_commandQueue.Get(), _window.GetHWND(), &swapchainDesc, nullptr, nullptr, &swapchain), "Failed to create swapchain");

		MSWRL::ComPtr<IDXGISwapChain3> swapchain3;
		ThrowIfFailed(swapchain->QueryInterface(__uuidof(IDXGISwapChain3), (void**)&swapchain3), "QueryInterface for swapchain failed.");
		_swapchain = swapchain3;
	}

	_frameIndex = _swapchain->GetCurrentBackBufferIndex();

	// Recreate descriptor heaps and render targets
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = _backBufferCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&_rtvHeap)));

	_rtvDescriptorSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// Create frame resources
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle(_rtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT i = 0; i < _backBufferCount; i++)
	{
		ThrowIfFailed(_swapchain->GetBuffer(i, IID_PPV_ARGS(&_renderTargets[i])));
		_device->CreateRenderTargetView(_renderTargets[i].Get(), nullptr, rtvHandle);
		rtvHandle.ptr += _rtvDescriptorSize;
		_rtvDescriptor[i] = rtvHandle;

	}
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

		if (FAILED(_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData,sizeof(featureData))))
			featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;

		D3D12_DESCRIPTOR_RANGE1 ranges[1];
		ranges[0].BaseShaderRegister = 0;
		ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		ranges[0].NumDescriptors = 1;
		ranges[0].RegisterSpace = 0;
		ranges[0].OffsetInDescriptorsFromTableStart = 0;
		ranges[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

		D3D12_ROOT_PARAMETER1 rootParameters[1];
		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
		rootParameters[0].DescriptorTable.pDescriptorRanges = ranges;

		D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
		rootSignatureDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		rootSignatureDesc.Desc_1_1.NumParameters = 1;
		rootSignatureDesc.Desc_1_1.pParameters = rootParameters;
		rootSignatureDesc.Desc_1_1.NumStaticSamplers = 0;
		rootSignatureDesc.Desc_1_1.pStaticSamplers = nullptr;

		MSWRL::ComPtr<ID3DBlob> signature;
		MSWRL::ComPtr<ID3DBlob> error;

		ThrowIfFailed(D3D12SerializeVersionedRootSignature(&rootSignatureDesc, &signature, &error));
		ThrowIfFailed(_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&_rootSignature)));
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

		std::string vertCompiledPath = path + "shaders\\compiled\\triangle.vert.dxbc";
		std::string fragCompiledPath = path + "shaders\\compiled\\triangle.frag.dxbc";

#define COMPILESHADERS
#ifdef COMPILESHADERS
		std::wstring vertPath = wpath + L"shaders\\triangle.vert.hlsl";
		std::wstring fragPath = wpath + L"shaders\\triangle.frag.hlsl";
				
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
			std::cout << errStr;
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
		// Define the vertex input layout.
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
				{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
				 D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
				{"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
				 D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0} };

		// Create the UBO.
		{
			// Note: using upload heaps to transfer static data like vert
			// buffers is not recommended. Every time the GPU needs it, the
			// upload heap will be marshalled over. Please read up on Default
			// Heap usage. An upload heap is used here for code simplicity and
			// because there are very few verts to actually transfer.
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
			ThrowIfFailed(_device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&_uniformBufferHeap)));

			D3D12_DESCRIPTOR_HEAP_DESC desc = {};
			desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			desc.NumDescriptors = 1;
			desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			ThrowIfFailed(_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&_srvHeap)));

			D3D12_RESOURCE_DESC uboResourceDesc;
			uboResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			uboResourceDesc.Alignment = 0;
			uboResourceDesc.Width = (sizeof(_MVP) + 255) & ~255;
			uboResourceDesc.Height = 1;
			uboResourceDesc.DepthOrArraySize = 1;
			uboResourceDesc.MipLevels = 1;
			uboResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
			uboResourceDesc.SampleDesc.Count = 1;
			uboResourceDesc.SampleDesc.Quality = 0;
			uboResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			uboResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

			ThrowIfFailed(_device->CreateCommittedResource(
				&heapProps, D3D12_HEAP_FLAG_NONE, &uboResourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
				IID_PPV_ARGS(&_uniformBuffer)));
			_uniformBufferHeap->SetName(L"Constant Buffer Upload Resource Heap");

			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
			cbvDesc.BufferLocation = _uniformBuffer->GetGPUVirtualAddress();
			cbvDesc.SizeInBytes = (sizeof(_MVP) + 255) & ~255; // CB size is required to be 256-byte aligned.

			D3D12_CPU_DESCRIPTOR_HANDLE cbvHandle(_uniformBufferHeap->GetCPUDescriptorHandleForHeapStart());
			cbvHandle.ptr = cbvHandle.ptr + _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 0;

			_device->CreateConstantBufferView(&cbvDesc, cbvHandle);

			// We do not intend to read from this resource on the CPU. (End is
			// less than or equal to begin)
			D3D12_RANGE readRange;
			readRange.Begin = 0;
			readRange.End = 0;

			// setup matrices
			_MVP.projectionMatrix = glm::perspective(glm::radians(45.0f),
				static_cast<float>(_window.GetWidth()) / static_cast<float>(_window.GetHeight()),
				0.01f, 1024.0f);

			// Set the model matrix as an identity matrix
			_MVP.modelMatrix = glm::mat4(1.0f);

			ThrowIfFailed(_uniformBuffer->Map( 0, &readRange, reinterpret_cast<void**>(&_mappedUniformBuffer)));
										memcpy(_mappedUniformBuffer, &_MVP, sizeof(_MVP));
			_uniformBuffer->Unmap(0, &readRange);
		}

		// Describe and create the graphics pipeline state object (PSO).
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
		psoDesc.pRootSignature = _rootSignature.Get();

		D3D12_SHADER_BYTECODE vsBytecode;
		D3D12_SHADER_BYTECODE psBytecode;

#ifdef COMPILESHADERS
		vsBytecode.pShaderBytecode = vertexShader->GetBufferPointer();
		vsBytecode.BytecodeLength = vertexShader->GetBufferSize();

		psBytecode.pShaderBytecode = pixelShader->GetBufferPointer();
		psBytecode.BytecodeLength = pixelShader->GetBufferSize();
#else
		vsBytecode.pShaderBytecode = vsBytecodeData.data();
		vsBytecode.BytecodeLength = vsBytecodeData.size();

		psBytecode.pShaderBytecode = fsBytecodeData.data();
		psBytecode.BytecodeLength = fsBytecodeData.size();
#endif

		psoDesc.VS = vsBytecode;
		psoDesc.PS = psBytecode;

		D3D12_RASTERIZER_DESC rasterDesc;
		rasterDesc.FillMode = D3D12_FILL_MODE_SOLID;
		rasterDesc.CullMode = D3D12_CULL_MODE_NONE;
		rasterDesc.FrontCounterClockwise = FALSE;
		rasterDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
		rasterDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		rasterDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		rasterDesc.DepthClipEnable = TRUE;
		rasterDesc.MultisampleEnable = FALSE;
		rasterDesc.AntialiasedLineEnable = FALSE;
		rasterDesc.ForcedSampleCount = 0;
		rasterDesc.ConservativeRaster =D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

		psoDesc.RasterizerState = rasterDesc;

		D3D12_BLEND_DESC blendDesc;
		blendDesc.AlphaToCoverageEnable = FALSE;
		blendDesc.IndependentBlendEnable = FALSE;
		const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc = {
				FALSE,
				FALSE,
				D3D12_BLEND_ONE,
				D3D12_BLEND_ZERO,
				D3D12_BLEND_OP_ADD,
				D3D12_BLEND_ONE,
				D3D12_BLEND_ZERO,
				D3D12_BLEND_OP_ADD,
				D3D12_LOGIC_OP_NOOP,
				D3D12_COLOR_WRITE_ENABLE_ALL,
		};
		for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
			blendDesc.RenderTarget[i] = defaultRenderTargetBlendDesc;

		psoDesc.BlendState = blendDesc;
		psoDesc.DepthStencilState.DepthEnable = FALSE;
		psoDesc.DepthStencilState.StencilEnable = FALSE;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;
		try
		{
			ThrowIfFailed(_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&_pipelineState)));
		}
		catch (std::exception e)
		{
			std::cout << "Failed to create Graphics Pipeline!";
		}
	}

	ThrowIfFailed(_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _commandAllocator.Get(), _pipelineState.Get(), IID_PPV_ARGS(&_commandList)));
	_commandList->SetName(L"artisDX CommandList");

	// Command lists are created in the recording state, but there is nothing
	// to record yet. The main loop expects it to be closed, so close it now.
	ThrowIfFailed(_commandList->Close());

	// Create the vertex buffer.
	{
		const UINT vertexBufferSize = sizeof(_vertexBufferData);

		// Note: using upload heaps to transfer static data like vert buffers is
		// not recommended. Every time the GPU needs it, the upload heap will be
		// marshalled over. Please read up on Default Heap usage. An upload heap
		// is used here for code simplicity and because there are very few verts
		// to actually transfer.
		D3D12_HEAP_PROPERTIES heapProps;
		heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
		heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapProps.CreationNodeMask = 1;
		heapProps.VisibleNodeMask = 1;

		D3D12_RESOURCE_DESC vertexBufferResourceDesc;
		vertexBufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		vertexBufferResourceDesc.Alignment = 0;
		vertexBufferResourceDesc.Width = vertexBufferSize;
		vertexBufferResourceDesc.Height = 1;
		vertexBufferResourceDesc.DepthOrArraySize = 1;
		vertexBufferResourceDesc.MipLevels = 1;
		vertexBufferResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		vertexBufferResourceDesc.SampleDesc.Count = 1;
		vertexBufferResourceDesc.SampleDesc.Quality = 0;
		vertexBufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		vertexBufferResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		ThrowIfFailed(_device->CreateCommittedResource(
			&heapProps, D3D12_HEAP_FLAG_NONE, &vertexBufferResourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
			IID_PPV_ARGS(&_vertexBuffer)));

		// Copy the triangle data to the vertex buffer.
		UINT8* pVertexDataBegin;

		// We do not intend to read from this resource on the CPU.
		D3D12_RANGE readRange;
		readRange.Begin = 0;
		readRange.End = 0;

		ThrowIfFailed(_vertexBuffer->Map(
			0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
		memcpy(pVertexDataBegin, _vertexBufferData, sizeof(_vertexBufferData));
		_vertexBuffer->Unmap(0, nullptr);

		// Initialize the vertex buffer view.
		_vertexBufferView.BufferLocation =
			_vertexBuffer->GetGPUVirtualAddress();
		_vertexBufferView.StrideInBytes = sizeof(Vertex);
		_vertexBufferView.SizeInBytes = vertexBufferSize;
	}

	// Create the index buffer.
	{
		const UINT indexBufferSize = sizeof(_indexBufferData);

		// Note: using upload heaps to transfer static data like vert buffers is
		// not recommended. Every time the GPU needs it, the upload heap will be
		// marshalled over. Please read up on Default Heap usage. An upload heap
		// is used here for code simplicity and because there are very few verts
		// to actually transfer.
		D3D12_HEAP_PROPERTIES heapProps;
		heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
		heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapProps.CreationNodeMask = 1;
		heapProps.VisibleNodeMask = 1;

		D3D12_RESOURCE_DESC vertexBufferResourceDesc;
		vertexBufferResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		vertexBufferResourceDesc.Alignment = 0;
		vertexBufferResourceDesc.Width = indexBufferSize;
		vertexBufferResourceDesc.Height = 1;
		vertexBufferResourceDesc.DepthOrArraySize = 1;
		vertexBufferResourceDesc.MipLevels = 1;
		vertexBufferResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		vertexBufferResourceDesc.SampleDesc.Count = 1;
		vertexBufferResourceDesc.SampleDesc.Quality = 0;
		vertexBufferResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		vertexBufferResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

		ThrowIfFailed(_device->CreateCommittedResource(
			&heapProps, D3D12_HEAP_FLAG_NONE, &vertexBufferResourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
			IID_PPV_ARGS(&_indexBuffer)));

		// Copy the triangle data to the vertex buffer.
		UINT8* pVertexDataBegin;

		// We do not intend to read from this resource on the CPU.
		D3D12_RANGE readRange;
		readRange.Begin = 0;
		readRange.End = 0;

		ThrowIfFailed(_indexBuffer->Map(
			0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
		memcpy(pVertexDataBegin, _indexBufferData, sizeof(_indexBufferData));
		_indexBuffer->Unmap(0, nullptr);

		// Initialize the vertex buffer view.
		_indexBufferView.BufferLocation = _indexBuffer->GetGPUVirtualAddress();
		_indexBufferView.Format = DXGI_FORMAT_R32_UINT;
		_indexBufferView.SizeInBytes = indexBufferSize;
	}

	// Create synchronization objects and wait until assets have been uploaded
	// to the GPU.
	{
		_fenceValue = 1;

		// Create an event handle to use for frame synchronization.
		_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (_fenceEvent == nullptr)
		{
			ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
		}

		// Wait for the command list to execute; we are reusing the same command
		// list in our main loop but for now, we just want to wait for setup to
		// complete before continuing.
		// Signal and increment the fence value.
		const UINT64 fence = _fenceValue;
		ThrowIfFailed(_commandQueue->Signal(_fence.Get(), fence));
		_fenceValue++;

		// Wait until the previous frame is finished.
		if (_fence->GetCompletedValue() < fence)
		{
			ThrowIfFailed(_fence->SetEventOnCompletion(fence, _fenceEvent));
			WaitForSingleObject(_fenceEvent, INFINITE);
		}

		_frameIndex = _swapchain->GetCurrentBackBufferIndex();
	}
}

void Application::InitCommands()
{
	// Ensure the command allocator has finished execution before resetting.
	ThrowIfFailed(_commandAllocator->Reset());

	// Reset the command list with the command allocator and pipeline state.
	ThrowIfFailed(_commandList->Reset(_commandAllocator.Get(), _pipelineState.Get()));

	// Set necessary state.
	_commandList->SetGraphicsRootSignature(_rootSignature.Get());
	_commandList->RSSetViewports(1, &_viewport);
	_commandList->RSSetScissorRects(1, &_surfaceSize);

	ID3D12DescriptorHeap* pDescriptorHeaps[] = { _uniformBufferHeap.Get() };
	_commandList->SetDescriptorHeaps(_countof(pDescriptorHeaps), pDescriptorHeaps);

	D3D12_GPU_DESCRIPTOR_HANDLE srvHandle(
		_uniformBufferHeap->GetGPUDescriptorHandleForHeapStart());
	_commandList->SetGraphicsRootDescriptorTable(0, srvHandle);

	// Transition the back buffer from present to render target state.
	D3D12_RESOURCE_BARRIER renderTargetBarrier = {};
	renderTargetBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	renderTargetBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	renderTargetBarrier.Transition.pResource = _renderTargets[_frameIndex].Get();
	renderTargetBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	renderTargetBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	renderTargetBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	_commandList->ResourceBarrier(1, &renderTargetBarrier);

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle(_rtvHeap->GetCPUDescriptorHandleForHeapStart());
	rtvHandle.ptr += (_frameIndex * _rtvDescriptorSize);
	_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	// Clear the render target.
	const float clearColor[] = { 0.2f, 0.2f, 0.2f, 1.0f };
	_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	_commandList->IASetVertexBuffers(0, 1, &_vertexBufferView);
	_commandList->IASetIndexBuffer(&_indexBufferView);

	_commandList->DrawIndexedInstanced(3, 1, 0, 0, 0);

	// START IMGUI commands
	if (_runImgui)
	{
		_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
		_commandList->SetDescriptorHeaps(1, _srvHeap.GetAddressOf());
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), _commandList.Get());
	}
	// END IMGUI

	// Transition back buffer to present state for the swap chain.
	D3D12_RESOURCE_BARRIER presentBarrier = {};
	presentBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	presentBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	presentBarrier.Transition.pResource = _renderTargets[_frameIndex].Get();
	presentBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	presentBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	presentBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	_commandList->ResourceBarrier(1, &presentBarrier);

	// Close the command list.
	ThrowIfFailed(_commandList->Close());
}


void Application::Run()
{
	_window.Show();

	MSG msg = { 0 };

	while (msg.message != WM_CLOSE)
	{
		_runImgui = true;
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		
		ImGui::Begin("Window");
		ImGui::DragFloat3("debug", &_camera._position.x);
		ImGui::End();

		ImGui::Render();

		Render();

		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void Application::Render()
{
	std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
	std::chrono::duration<float> dt = now - _tLastTime;
	FLOAT deltaTime = dt.count();
	_tLastTime = now;

	_camera.ConsumeMouse(_window.GetXChange(), _window.GetYChange());
	_camera.ConsumeKey(_window.GetKeys(), deltaTime);
	_MVP.viewMatrix = _camera.GetViewMatrix();

	D3D12_RANGE readRange;
	readRange.Begin = 0;
	readRange.End = 0;

	ThrowIfFailed(_uniformBuffer->Map(
		0, &readRange, reinterpret_cast<void**>(&_mappedUniformBuffer)), "Mapping failed.");
	memcpy(_mappedUniformBuffer, &_MVP, sizeof(_MVP));
	_uniformBuffer->Unmap(0, &readRange);
	// Record all the commands we need to render the scene into the command
	// list.
	InitCommands();

	// Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { _commandList.Get()};
	_commandQueue->ExecuteCommandLists(_countof(ppCommandLists),
		ppCommandLists);
	_swapchain->Present(1, 0);

	// WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.

	// Update and Render additional Platform Windows
	if (_imguiIO->ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault(nullptr, (void*)_commandList.Get());
	}

	// Signal and increment the fence value.
	const UINT64 fence = _fenceValue;
	ThrowIfFailed(_commandQueue->Signal(_fence.Get(), fence));
	_fenceValue++;

	// Wait until the previous frame is finished.
	if (_fence->GetCompletedValue() < fence)
	{
		ThrowIfFailed(_fence->SetEventOnCompletion(fence, _fenceEvent));
		WaitForSingleObject(_fenceEvent, INFINITE);
	}
	_frameIndex = _swapchain->GetCurrentBackBufferIndex();
}

Application::~Application()
{
	_window.CleanUp();
}