#include "Application.h"

Application::Application(const CHAR* name, INT w, INT h)
	: _window(name, w, h)
{

	_width = w;
	_height = h;
	_factory = nullptr;
	_adapter = nullptr;

#if defined(_DEBUG)																	 
	_debugController = nullptr;
	_debugDevice = nullptr;
#endif					

	_commandQueue = nullptr;
	_commandAllocator = nullptr;
	_commandList = nullptr;

	_rtvHeap = nullptr;
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
		XMVectorSet(0.0f, 0.0f, 5.0f, 0.0f),
		XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f),
		90.0f,
		0.0f,
		2.5f,
		0.1f
	);

	InitDX12();
	InitSwapchain(w, h);
	InitResources();
	InitGUI();
}

void Application::InitGUI()
{
	GUI::Init(_window, D3D12Core::GetDevice(), _commandQueue, _swapchain, _rtvHeap, _renderTargets, _rtvDescriptorSize);
}

void Application::InitDX12()
{
	CoInitializeEx(nullptr, COINIT_MULTITHREADED);
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
	MSWRL::ComPtr<ID3D12Device> device;
	ThrowIfFailed(D3D12CreateDevice(_adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&device)), "Device creation failed!");
	device->SetName(L"artisDX_Device");

	D3D12Core::Initialize(device);

#if defined(_DEBUG)
	// Get debug device
	ThrowIfFailed(D3D12Core::GetDevice()->QueryInterface(_debugDevice.GetAddressOf()));
#endif

	// Create Command Queue
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	// D3D12_COMMAND_LIST_TYPE_DIRECT = normal graphics pipeline
	// D3D12_COMMAND_LIST_TYPE_COMPUTE = compute pipeline
	// ...

	ThrowIfFailed(D3D12Core::GetDevice()->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&_commandQueue)),"CommandQueue creation failed!");

	// Create Command Allocator
	ThrowIfFailed(D3D12Core::GetDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_commandAllocator)));

	// Sync
	ThrowIfFailed(D3D12Core::GetDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence)));
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

	_viewport = CD3DX12_VIEWPORT{ 0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h) };
	_viewport.MinDepth = 0.0f;
	_viewport.MaxDepth = 1.0f;

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
	ThrowIfFailed(D3D12Core::GetDevice()->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&_rtvHeap)));

	_rtvDescriptorSize = D3D12Core::GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// Create frame resources
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle(_rtvHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT i = 0; i < _backBufferCount; i++)
	{
		ThrowIfFailed(_swapchain->GetBuffer(i, IID_PPV_ARGS(&_renderTargets[i])));
		D3D12Core::GetDevice()->CreateRenderTargetView(_renderTargets[i].Get(), nullptr, rtvHandle);
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

		if (FAILED(D3D12Core::GetDevice()->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData,sizeof(featureData))))
			featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;

		D3D12_DESCRIPTOR_RANGE1 cbvRangeViewProj = {};
		cbvRangeViewProj.BaseShaderRegister = 0; // b0
		cbvRangeViewProj.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		cbvRangeViewProj.NumDescriptors = 1;
		cbvRangeViewProj.RegisterSpace = 0;
		cbvRangeViewProj.OffsetInDescriptorsFromTableStart = 0;
		cbvRangeViewProj.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

		D3D12_DESCRIPTOR_RANGE1 cbvRangeModel = {};
		cbvRangeModel.BaseShaderRegister = 1; // b1
		cbvRangeModel.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		cbvRangeModel.NumDescriptors = 1;
		cbvRangeModel.RegisterSpace = 0;
		cbvRangeModel.OffsetInDescriptorsFromTableStart = 0;
		cbvRangeModel.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

		// Texture SRV (t0) Albedo + Normal
		D3D12_DESCRIPTOR_RANGE1 srvRange = {};
		srvRange.BaseShaderRegister = 0; // t0
		srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		srvRange.NumDescriptors = 1;
		srvRange.RegisterSpace = 0;
		srvRange.OffsetInDescriptorsFromTableStart = 0;
		srvRange.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;

		// constant buffers
		D3D12_ROOT_PARAMETER1 rootParameters[3] = {};

		// View Projection Buffer
		rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
		rootParameters[0].DescriptorTable.pDescriptorRanges = &cbvRangeViewProj;

		// Model Matrix Buffer
		rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		rootParameters[1].DescriptorTable.NumDescriptorRanges = 1;
		rootParameters[1].DescriptorTable.pDescriptorRanges = &cbvRangeModel;

		// texture
		rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[2].DescriptorTable.NumDescriptorRanges = 1;
		rootParameters[2].DescriptorTable.pDescriptorRanges = &srvRange;

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
		ThrowIfFailed(D3D12Core::GetDevice()->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&_rootSignature)));
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
		std::wstring vertPath = wpath + L"shaders\\vert.fx";
		std::wstring fragPath = wpath + L"shaders\\frag.fx";
				
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
		// Define the vertex input layout.
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
					D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
					D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"TANGENT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24,
					D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 40,
					D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
		};

		DescriptorAllocator::Instance().Initialize(D3D12Core::GetDevice().Get(), NUM_MAX_DESCRIPTORS);

		// Create the UBO.
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
			ThrowIfFailed(D3D12Core::GetDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&_uniformBufferHeap)));

			D3D12_RESOURCE_DESC uboResourceDesc;
			uboResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			uboResourceDesc.Alignment = 0;
			uboResourceDesc.Width = (sizeof(_viewProjectionMatrix) + 255) & ~255;
			uboResourceDesc.Height = 1;
			uboResourceDesc.DepthOrArraySize = 1;
			uboResourceDesc.MipLevels = 1;
			uboResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
			uboResourceDesc.SampleDesc.Count = 1;
			uboResourceDesc.SampleDesc.Quality = 0;
			uboResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			uboResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

			ThrowIfFailed(D3D12Core::GetDevice()->CreateCommittedResource(
				&heapProps, D3D12_HEAP_FLAG_NONE, &uboResourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
				IID_PPV_ARGS(&_uniformBuffer)));
			_uniformBufferHeap->SetName(L"Constant Buffer Upload Resource Heap");

			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
			cbvDesc.BufferLocation = _uniformBuffer->GetGPUVirtualAddress();
			cbvDesc.SizeInBytes = (sizeof(_viewProjectionMatrix) + 255) & ~255; // CB size is required to be 256-byte aligned.

			D3D12_CPU_DESCRIPTOR_HANDLE cbvCpuHandle = DescriptorAllocator::Instance().Allocate();
			D3D12Core::GetDevice()->CreateConstantBufferView(&cbvDesc, cbvCpuHandle);

			_uniformBufferDescriptor = cbvCpuHandle; // Save this for binding later

			// We do not intend to read from this resource on the CPU. (End is
			// less than or equal to begin)
			D3D12_RANGE readRange = { 0, 0 };

			// setup matrices
			XMStoreFloat4x4(&_projectionMatrix,
				XMMatrixPerspectiveFovLH(
					XMConvertToRadians(45.0f),
					static_cast<float>(_window.GetWidth()) / static_cast<float>(_window.GetHeight()),
					0.1f,
					1000.0f)
			);

			ThrowIfFailed(_uniformBuffer->Map(0, &readRange, reinterpret_cast<void**>(&_mappedUniformBuffer)));
			memcpy(_mappedUniformBuffer, &_viewProjectionMatrix, sizeof(_viewProjectionMatrix));
			_uniformBuffer->Unmap(0, nullptr);
		}

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
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;

		try
		{
			ThrowIfFailed(D3D12Core::GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&_pipelineState)));
		}
		catch (std::exception e)
		{
			std::cout << "Failed to create Graphics Pipeline!";
		}

		// Create the DSV Heap
		D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
		dsvHeapDesc.NumDescriptors = 1;
		dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
		dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

		ThrowIfFailed(D3D12Core::GetDevice()->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&_dsvHeap)));

		// Heap properties for creating the texture (GPU read/write)
		D3D12_HEAP_PROPERTIES heapProps = {};
		heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
		heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapProps.CreationNodeMask = 1;
		heapProps.VisibleNodeMask = 1;

		// Create Depth-Stencil Resource (Texture2D)
		D3D12_RESOURCE_DESC depthResourceDesc = {};
		depthResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		depthResourceDesc.Alignment = 0;
		depthResourceDesc.Width = _width;       
		depthResourceDesc.Height = _height;     
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

		ThrowIfFailed(D3D12Core::GetDevice()->CreateCommittedResource(
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

		// Create the DSV for the depth-stencil buffer
		D3D12Core::GetDevice()->CreateDepthStencilView(_depthStencilBuffer.Get(), &dsvDesc, _dsvHeap->GetCPUDescriptorHandleForHeapStart());
	}

	ThrowIfFailed(D3D12Core::GetDevice()->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _commandAllocator.Get(), _pipelineState.Get(), IID_PPV_ARGS(&_commandList)));
	_commandList->SetName(L"artisDX CommandList");

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

	// MODELLOADING
	_modelManager = ModelManager(D3D12Core::GetDevice(), _commandList);

	//_modelManager.LoadModel("../assets/cube.glb");
	//_modelManager.LoadModel("../assets/movedcube.glb");
	//_modelManager.LoadModel("../assets/elicube.glb");
	//_modelManager.LoadModel("../assets/cuberotated.glb");
	//_modelManager.LoadModel("../assets/uv_debug.glb");
	_modelManager.LoadModel("../assets/helmet.glb");

	// upload all textures from models
	ThrowIfFailed(_commandList->Close());
	ExecuteCommandList();
}

void Application::SetCommandList()
{
	// Ensure the command allocator has finished execution before resetting.
	ThrowIfFailed(_commandAllocator->Reset());

	// Reset the command list with the command allocator and pipeline state.
	ThrowIfFailed(_commandList->Reset(_commandAllocator.Get(), _pipelineState.Get()));
	// Set necessary state.
	_commandList->SetGraphicsRootSignature(_rootSignature.Get());
	_commandList->RSSetViewports(1, &_viewport);
	_commandList->RSSetScissorRects(1, &_surfaceSize);

	ID3D12DescriptorHeap* heaps[] = { DescriptorAllocator::Instance().GetHeap() };
	_commandList->SetDescriptorHeaps(1, heaps);

	_commandList->SetGraphicsRootDescriptorTable(0, DescriptorAllocator::Instance().GetGPUHandle(_uniformBufferDescriptor));

	// Transition the back buffer from present to render target state.
	D3D12_RESOURCE_BARRIER renderTargetBarrier = {};
	renderTargetBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	renderTargetBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	renderTargetBarrier.Transition.pResource = _renderTargets[_frameIndex].Get();
	renderTargetBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	renderTargetBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	renderTargetBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	_commandList->ResourceBarrier(1, &renderTargetBarrier);

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = _rtvHeap->GetCPUDescriptorHandleForHeapStart();
	rtvHandle.ptr += (_frameIndex * _rtvDescriptorSize);
	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = _dsvHeap->GetCPUDescriptorHandleForHeapStart();
	_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
	_commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0.0f, 0, nullptr);

	// Clear the render target.
	const float clearColor[] = { 0.2f, 0.2f, 0.2f, 1.0f };
	_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);

	_modelManager.DrawAll();

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
	_lastTime = std::chrono::high_resolution_clock::now(); // Initialize timing
	_window.Show();
	MSG msg = { 0 };

	while (msg.message != WM_CLOSE)
	{
		UpdateFPS(); // Calculate FPS

		UpdateConstantBuffer();
		SetCommandList();
		ExecuteCommandList();
		GUI::Draw();
		Present();

		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	GUI::Shutdown();
}


void Application::UpdateFPS()
{
	using namespace std::chrono;

	auto now = high_resolution_clock::now();
	double deltaTime = duration<double>(now - _lastTime).count();
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

void Application::UpdateConstantBuffer()
{
	std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
	std::chrono::duration<float> dt = now - _tLastTime;
	FLOAT deltaTime = dt.count();
	_tLastTime = now;

	_camera.ConsumeKey(_window.GetKeys(), deltaTime);
	_camera.ConsumeMouse(_window.GetXChange(), _window.GetYChange());
	_viewMatrix = _camera.GetViewMatrix();

	XMStoreFloat4x4(&_viewProjectionMatrix, XMMatrixMultiply(XMLoadFloat4x4(&_viewMatrix), XMLoadFloat4x4(&_projectionMatrix)));

	memcpy(_mappedUniformBuffer, &_viewProjectionMatrix, sizeof(_viewProjectionMatrix));
}

void Application::ExecuteCommandList()
{
	// Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { _commandList.Get() };
	_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	WaitForFence();
}

void Application::Present()
{
	_swapchain->Present(1, 0);

	WaitForFence();

	_frameIndex = _swapchain->GetCurrentBackBufferIndex();
}

void Application::WaitForFence()
{
	// Signal and increment the fence value.
	const UINT64 fence = _fenceValue;
	ThrowIfFailed(_commandQueue->Signal(_fence.Get(), fence));
	_fenceValue++;

	if (_fence->GetCompletedValue() < fence)
	{
		ThrowIfFailed(_fence->SetEventOnCompletion(fence, _fenceEvent));
		WaitForSingleObject(_fenceEvent, INFINITE);
	}
}

Application::~Application()
{
	CoUninitialize();
	_window.Shutdown();
	// Ensure GPU has finished executing commands before releasing resources
	_fenceValue++;
	_commandQueue->Signal(_fence.Get(), _fenceValue);

	if (_fence->GetCompletedValue() < _fenceValue)
	{
		_fence->SetEventOnCompletion(_fenceValue, _fenceEvent);
		WaitForSingleObject(_fenceEvent, INFINITE);
	}

	// Close event handle
	if (_fenceEvent)
	{
		CloseHandle(_fenceEvent);
		_fenceEvent = nullptr;
	}

	// Explicitly release all DX12 objects
	_commandList.Reset();
	_commandAllocator.Reset();
	_commandQueue.Reset();
	_swapchain.Reset();
	_factory.Reset();
	_adapter.Reset();

	_rtvHeap.Reset();
	_rootSignature.Reset();
	_pipelineState.Reset();
	_dsvHeap.Reset();

	for (UINT i = 0; i < _backBufferCount; i++)
	{
		_renderTargets[i].Reset();
	}
	_depthStencilBuffer.Reset();

	_uniformBuffer.Reset();
	_uniformBufferHeap.Reset();

	_fence.Reset();

	// Clean up other objects
	_mappedUniformBuffer = nullptr;

	// Cleanup GUI
#if defined(_DEBUG)
	_debugDevice->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL);
	if (_debugDevice) _debugDevice.Reset();
	if (_debugController) _debugController.Reset();
#endif
	PRINT("SHUTDOWN");
}