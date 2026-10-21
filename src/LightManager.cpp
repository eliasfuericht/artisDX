#include "LightManager.h"

void LightManager::AddPointLight(float x, float y, float z)
{
	std::shared_ptr<PointLight> pLight = std::make_shared<PointLight>(x, y, z);
	pLight->RegisterWithGUI();
	_pLights.push_back(pLight);
}

void LightManager::AddDirectionalLight(float x, float y, float z, bool shadows, int shadowMapResolution)
{
	std::shared_ptr<DirectionalLight> dLight = std::make_shared<DirectionalLight>(x, y, z, shadows, shadowMapResolution);
	dLight->RegisterWithGUI();
	_dLights.push_back(dLight);
}

void LightManager::UpdateLightBuffers()
{
	for (auto pl : _pLights)
	{
		pl->UpdateBuffer();
	}

	for (auto dl : _dLights)
	{
		dl->UpdateBuffer();
	}
}

void LightManager::TransitionStartDLightShadowMap(std::shared_ptr<CommandContext> commandContext)
{
	for (auto dL : _dLights)
	{
		if (dL->_shadowsEnabled)
		{
			commandContext->DeclareResource(dL->_directionalShadowMapBuffer.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, "shadow map");
			commandContext->KeepAlive(dL);
		}
	}
}

void LightManager::TransitionEndDLightShadowMap(std::shared_ptr<CommandContext> commandContext)
{
	for (auto dL : _dLights)
	{
		if (dL->_shadowsEnabled)
		{
			commandContext->UseResource(dL->_directionalShadowMapBuffer.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		}
	}
}

void LightManager::RenderDLightShadowMap(std::shared_ptr<CommandContext> commandContext)
{
	for (auto dL : _dLights)
	{
		if (dL->_shadowsEnabled)
		{
			commandContext->GetCommandList()->RSSetViewports(1, &dL->_vp);
			commandContext->GetCommandList()->RSSetScissorRects(1, &dL->_scissor);
			commandContext->UseResource(dL->_directionalShadowMapBuffer.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE);
			commandContext->GetCommandList()->OMSetRenderTargets(0, nullptr, false, &dL->_directionalShadowMapDSVCPUHandle);
			commandContext->GetCommandList()->ClearDepthStencilView(dL->_directionalShadowMapDSVCPUHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, '\0', 0, nullptr);
		}
	}
}

void LightManager::BindDLightVPBuffer(std::shared_ptr<ShaderPass> shaderPass, std::shared_ptr<CommandContext> commandContext)
{
	for (auto dL : _dLights)
	{
		if (auto slot = shaderPass->GetRootParameterIndex("lightViewProjMatrixBuffer"))
			commandContext->GetCommandList()->SetGraphicsRootDescriptorTable(slot.value(), DescriptorAllocator::CBVSRVUAV::GetGPUHandle(dL->_dLightLVPCPUHandle));
	}
}

void LightManager::BindDLightBuffers(std::shared_ptr<ShaderPass> shaderPass, std::shared_ptr<CommandContext> commandContext)
{
	for (auto dL : _dLights)
	{
		if (auto slot = shaderPass->GetRootParameterIndex("dlightBuffer"))
			commandContext->GetCommandList()->SetGraphicsRootDescriptorTable(slot.value(), DescriptorAllocator::CBVSRVUAV::GetGPUHandle(dL->_dLightDirectionCPUHandle));

		if (auto slot = shaderPass->GetRootParameterIndex("dShadowMap"))
			commandContext->GetCommandList()->SetGraphicsRootDescriptorTable(slot.value(), DescriptorAllocator::CBVSRVUAV::GetGPUHandle(dL->_directionalShadowMapSRVCPUHandle));
	}
}

void LightManager::BindPLightBuffers(std::shared_ptr<ShaderPass> shaderPass, std::shared_ptr<CommandContext> commandContext)
{
	for (auto pL : _pLights)
	{
		if (auto slot = shaderPass->GetRootParameterIndex("plightBuffer"))
			commandContext->GetCommandList()->SetGraphicsRootDescriptorTable(slot.value(), DescriptorAllocator::CBVSRVUAV::GetGPUHandle(pL->_cbvpLightCPUHandle));
	}
}
