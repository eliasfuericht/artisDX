#include "DirectionalLight.h"

DirectionalLight::DirectionalLight(float x, float y, float z, float enableShadowMap)
{
	_direction = XMFLOAT3(x, y, z);

	BuildLightProjMatrix();

	_shadowsEnabled = enableShadowMap;
}

void DirectionalLight::BuildLightProjMatrix() 
{
	XMVECTOR sceneCenter = XMVectorSet(0, 0, 0, 0);
	XMVECTOR lightPos = XMLoadFloat3(&_direction);

	XMVECTOR up = XMVectorSet(0, 1, 0, 0);
	XMMATRIX lightView = XMMatrixLookAtRH(lightPos, sceneCenter, up);

	XMMATRIX lightProj = XMMatrixOrthographicRH(_orthoWidth, _orthoHeight, _nearPlane, _farPlane);

	XMMATRIX lightViewProj = lightView * lightProj;

	XMStoreFloat4x4(&_lightViewProjMatrix, lightViewProj);
}

void DirectionalLight::UpdateBuffer()
{
	BuildLightProjMatrix();
}

void DirectionalLight::DrawGUI()
{
	std::string windowName = std::to_string(_id) + "_DirectionalLight###DirectionalLight_" + std::to_string(_id);
	ImGui::Begin(windowName.c_str());
	ImGui::DragFloat3("Direction", &_direction.x, 0.01f);
	ImGui::DragFloat("width", &_orthoWidth, 0.01f);
	ImGui::DragFloat("height", &_orthoHeight, 0.01f);
	ImGui::DragFloat("near", &_nearPlane, 0.01f);
	ImGui::DragFloat("far", &_farPlane, 0.01f);
	ImGui::Checkbox("Casts shadows", &_shadowsEnabled);
	/*
	{
		windowName = std::to_string(_id) + "_DirectionalLightShadowMap###DirectionalLightShadowMap_" + std::to_string(_id);
		ImGui::Begin(windowName.c_str());
		auto gpuHandle = DescriptorAllocator::CBVSRVUAV::GetGPUHandle(_directionalShadowMapSRVCPUHandle);
		ImTextureID texID = (ImTextureID)gpuHandle.ptr;

		ImGui::Image(
			texID,
			ImVec2(300, 300),
			ImVec2(0, 0),
			ImVec2(1, 1),
			ImVec4(1, 1, 1, 1),
			ImVec4(0, 0, 0, 0)
		);
		ImGui::End();
	}
*/
	ImGui::End();
}
