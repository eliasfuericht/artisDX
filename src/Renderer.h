#pragma once

#include "pch.h"

#include "CommandQueue.h"
#include "CommandContext.h"
#include "DescriptorAllocator.h"
#include "Shader.h"
#include "ShaderPass.h"
#include "ModelManager.h"
#include "Camera.h"
#include "DirectionalLight.h"
#include "PointLight.h"

class Renderer
{
public:
	Renderer() {};
	void InitializeRenderer(Window* window);
	void Shutdown();

	// returns handle to final image
	void Render(float dt);
	void UpdateBuffers(float dt);
	void SetCommandlist();

	Window* _window;

	D3D12_CPU_DESCRIPTOR_HANDLE _viewportRTV;

	CommandContext _mainLoopGraphicsContext;
	std::shared_ptr<ShaderPass> _depthPass;
	std::shared_ptr<ShaderPass> _mainPass;
	std::shared_ptr<ShaderPass> _bbPass;

	// ViewProjMatrix CBV
	MSWRL::ComPtr<ID3D12Resource> _VPBufferResource;
	MSWRL::ComPtr<ID3D12DescriptorHeap> _VPBufferHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE _VPBufferDescriptor;
	uint8_t* _mappedVPBuffer;

	// ViewMatrix CBV
	MSWRL::ComPtr<ID3D12Resource> _camPosBufferResource;
	MSWRL::ComPtr<ID3D12DescriptorHeap> _camPosBufferHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE _camPosBufferDescriptor;
	uint8_t* _mappedCamPosBuffer;

	XMFLOAT4X4 _projectionMatrix;
	XMFLOAT4X4 _viewMatrix;
	XMFLOAT4X4 _viewProjectionMatrix;

	D3D12_CPU_DESCRIPTOR_HANDLE _samplerCPUHandle;

	std::shared_ptr<PointLight> _pLight;
	std::shared_ptr<DirectionalLight> _dLight;
	std::shared_ptr<Camera> _camera;

	ModelManager _modelManager;

private:
	void InitializeResources();
};