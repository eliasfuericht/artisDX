#pragma once

#include "pch.h"

#include "D3D12Core.h"
#include "Window.h"
#include "CommandQueue.h"
#include "CommandContext.h"
#include "DescriptorAllocator.h"
#include "Shader.h"
#include "ShaderPass.h"
#include "ModelManager.h"
#include "Camera.h"
#include "DirectionalLight.h"
#include "PointLight.h"

class Application
{
public:
	Application(const CHAR* name, INT w, INT h, bool fullscreen);
	void Run();
	~Application();

private:
	void Init();
	void InitResources();
	void InitGUI();
	
	void UpdateConstantBuffers();
	void SetCommandList();
	void Present();

	void UpdateFPS();

	std::shared_ptr<PointLight> _pLight;
	std::shared_ptr<DirectionalLight> _dLight;

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
	CommandContext _mainLoopGraphicsContext;
	
	ShaderPass _mainPass;

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

	D3D12_CPU_DESCRIPTOR_HANDLE _samplerCPUHandle;

	ModelManager _modelManager;
};