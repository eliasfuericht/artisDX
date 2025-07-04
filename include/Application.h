#pragma once

#include "pch.h"

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
	
	void UpdateConstantBuffers();
	void SetCommandList();
	void ExecuteCommandList();
	void WaitForFence();
	void Present();

	void UpdateFPS();

	Window _window;
	std::shared_ptr<Camera> _camera;
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

	// ViewProjMatrix CBV
	MSWRL::ComPtr<ID3D12Resource> _VPBufferResource;
	MSWRL::ComPtr<ID3D12DescriptorHeap> _VPBufferHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE _VPBufferDescriptor;
	UINT8* _mappedVPBuffer;

	// ViewMatrix CBV
	MSWRL::ComPtr<ID3D12Resource> _camPosBufferResource;
	MSWRL::ComPtr<ID3D12DescriptorHeap> _camPosBufferHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE _camPosBufferDescriptor;
	UINT8* _mappedCamPosBuffer;

	XMFLOAT4X4 _projectionMatrix;
	XMFLOAT4X4 _viewMatrix;
	XMFLOAT4X4 _viewProjectionMatrix;

	ModelManager _modelManager;
};