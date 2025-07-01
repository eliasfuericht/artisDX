#pragma once

#include "precompiled/pch.h"

#include "GraphicsDevice.h"
#include "Swapchain.h"
#include "CommandQueue.h"
#include "Window.h"
#include "Camera.h"
#include "ModelManager.h"
#include "DescriptorAllocator.h"

class Application
{
public:
	Application(const CHAR* name, INT w, INT h);
	void Run();
	~Application();

private:
	void Init();
	void InitResources();
	void InitGUI();
	
	void UpdateConstantBuffer();
	void SetCommandList();
	void ExecuteCommandList();
	void WaitForFence();
	void Present();

	void UpdateFPS();

	Window _window;
	Camera _camera;
	INT _width, _height;

	std::chrono::steady_clock::time_point _tLastTime = std::chrono::steady_clock::now();

	std::chrono::high_resolution_clock::time_point _startTime;
	std::chrono::high_resolution_clock::time_point _lastTime;
	double _elapsedTime = 0.0;
	int _frameCount = 0;
	double _fps = 0.0;

	// DX12 Specific
	MSWRL::ComPtr<ID3D12CommandAllocator> _commandAllocator;
	MSWRL::ComPtr<ID3D12GraphicsCommandList> _commandList;

	MSWRL::ComPtr<ID3D12RootSignature> _rootSignature;
	MSWRL::ComPtr<ID3D12PipelineState> _pipelineState;

	// DepthBuffer
	MSWRL::ComPtr<ID3D12Resource> _depthStencilBuffer;
	MSWRL::ComPtr<ID3D12DescriptorHeap> _dsvHeap;

	// ViewProj
	MSWRL::ComPtr<ID3D12Resource> _uniformBuffer;
	MSWRL::ComPtr<ID3D12DescriptorHeap> _uniformBufferHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE _uniformBufferDescriptor;
	UINT8* _mappedUniformBuffer;

	XMFLOAT4X4 _projectionMatrix;
	XMFLOAT4X4 _viewMatrix;
	XMFLOAT4X4 _viewProjectionMatrix;

	ModelManager _modelManager;
};