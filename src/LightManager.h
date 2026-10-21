#pragma once

#include "pch.h"

#include "CommandContext.h"

#include "DirectionalLight.h"
#include "PointLight.h"
#include "ShaderPass.h"

class LightManager
{
public:
	LightManager() = default;

	void AddPointLight(float x, float y, float z);
	void AddDirectionalLight(float x, float y, float z, bool shadows, int shadowMapResolution);
	
	void UpdateLightBuffers();
	
	void TransitionStartDLightShadowMap(std::shared_ptr<CommandContext> commandContext);
	void TransitionEndDLightShadowMap(std::shared_ptr<CommandContext> commandContext);
	void RenderDLightShadowMap(std::shared_ptr<CommandContext> commandContext);
	void BindDLightVPBuffer(std::shared_ptr<ShaderPass> shaderPass, std::shared_ptr<CommandContext> commandContext);
	void BindDLightBuffers(std::shared_ptr<ShaderPass> shaderPass, std::shared_ptr<CommandContext> commandContext);
	
	void BindPLightBuffers(std::shared_ptr<ShaderPass> shaderPass, std::shared_ptr<CommandContext> commandContext);

private:
	std::vector<std::shared_ptr<PointLight>> _pLights;
	std::vector<std::shared_ptr<DirectionalLight>> _dLights;
};