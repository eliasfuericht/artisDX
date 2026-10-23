#pragma once

#include "pch.h"

#include "CommandQueue.h"
#include "CommandContext.h"
#include "DescriptorAllocator.h"
#include "ShaderPass.h"
#include "ModelManager.h"
#include "LightManager.h"
#include "Camera.h"

class Renderer
{
public:
	Renderer() = default;
	void InitializeRenderer();
	void InitializeResources();
	void CreateRenderTarget();
	void CreateDepthBuffer();
	void CreateConstantBuffers();
	void PrepareFrame(float dt);
	void UpdateBuffers(float dt);
	void RecordScene(std::shared_ptr<CommandContext> commandContext);

	//viewport stuff
	MSWRL::ComPtr<ID3D12Resource> _viewportTexture;
	D3D12_CPU_DESCRIPTOR_HANDLE _viewportRTV;
	D3D12_CPU_DESCRIPTOR_HANDLE _viewportSRV_CPU;
	D3D12_GPU_DESCRIPTOR_HANDLE _viewportSRV_GPU;
	D3D12_VIEWPORT _vp;
	D3D12_RECT _scissor;

	MSWRL::ComPtr<ID3D12Resource> _depthStencilBuffer;
	CD3DX12_CPU_DESCRIPTOR_HANDLE _dsvCPUHandle;

	std::shared_ptr<ShaderPass> _depthPass;
	std::shared_ptr<ShaderPass> _mainPass;
	std::shared_ptr<ShaderPass> _bbPass;

	// ViewProjMatrix CBV
	MSWRL::ComPtr<ID3D12Resource> _VPBufferResource;
	D3D12_CPU_DESCRIPTOR_HANDLE _VPBufferDescriptor;
	uint8_t* _mappedVPBuffer;

	// ViewMatrix CBV
	MSWRL::ComPtr<ID3D12Resource> _camPosBufferResource;
	D3D12_CPU_DESCRIPTOR_HANDLE _camPosBufferDescriptor;
	uint8_t* _mappedCamPosBuffer;

	XMFLOAT4X4 _projectionMatrix;
	XMFLOAT4X4 _viewMatrix;
	XMFLOAT4X4 _viewProjectionMatrix;

	D3D12_CPU_DESCRIPTOR_HANDLE _samplerCPUHandle;

	std::shared_ptr<Camera> _camera;

	std::shared_ptr<ModelManager> _modelManager;
	std::shared_ptr<LightManager> _lightManager;
};
