#pragma once

#include "pch.h"

#include "CommandContext.h"
#include "IGUIComponent.h"

#include "DirectionalLight.h"
#include "PointLight.h"
#include "ShaderPass.h"
#include <array>

class ModelManager;

class LightManager : public IGUIComponent
{
public:
	LightManager() = default;

	void AddPointLight(float x, float y, float z);
	void AddDirectionalLight(float x, float y, float z, bool shadows, int shadowMapResolution);

	void UpdateDirectionalLightBuffer(bool shadowsEnabled = true);
	
	void RenderDShadowMaps(std::shared_ptr<ShaderPass> shaderPass, std::shared_ptr<CommandContext> commandContext, std::shared_ptr<ModelManager> modelManager, const XMFLOAT4X4& viewMatrix);
	void BindDLightBuffers(std::shared_ptr<ShaderPass> shaderPass, std::shared_ptr<CommandContext> commandContext);
	
	void BindPLightBuffers(std::shared_ptr<ShaderPass> shaderPass, std::shared_ptr<CommandContext> commandContext);
	[[nodiscard]] bool HasShadowLights() const;

	virtual void DrawGUI();

private:
	struct PointLightDataGPU
	{
		XMFLOAT3 position;
		float intensity;
		XMFLOAT3 color;
		float padding;
	};

	struct PointLightBuffer
	{
		MSWRL::ComPtr<ID3D12Resource> resource;
		PointLightDataGPU* mapped = nullptr;
		D3D12_CPU_DESCRIPTOR_HANDLE srv = {};
		uint32_t capacity = 0;
	};

	struct DirectionalLightDataGPU
	{
		XMFLOAT3 direction;
		uint32_t shadowsEnabled;
		float intensity;
		XMFLOAT3 color;
		uint32_t shadowMapIndex;
		XMFLOAT3 padding;
		XMFLOAT4X4 lightViewProjection;
	};

	struct DirectionalLightBuffer
	{
		MSWRL::ComPtr<ID3D12Resource> resource;
		DirectionalLightDataGPU* mapped = nullptr;
		D3D12_CPU_DESCRIPTOR_HANDLE srv = {};
		uint32_t capacity = 0;
	};

	struct DShadowMap
	{
		MSWRL::ComPtr<ID3D12Resource> resource;
		D3D12_CPU_DESCRIPTOR_HANDLE dsv = {};
		D3D12_CPU_DESCRIPTOR_HANDLE srv = {};
		D3D12_VIEWPORT vp;
		D3D12_RECT scissor;
		uint32_t lightIndex;
		std::array<D3D12_CPU_DESCRIPTOR_HANDLE, D3D12Core::Swapchain::backBufferCount> lightSRVs = {};
	};

	std::vector<std::shared_ptr<PointLight>> _pLights;
	std::array<PointLightBuffer, D3D12Core::Swapchain::backBufferCount> _pLightBuffers;
	void UpdatePointLightBuffer(PointLightBuffer& buffer);

	std::vector<std::shared_ptr<DirectionalLight>> _dLights;
	std::array<DirectionalLightBuffer, D3D12Core::Swapchain::backBufferCount> _dLightBuffers;
	std::vector<DShadowMap> _dShadowMaps;

	void CreateDShadowMapResource(int32_t resolution);

};
