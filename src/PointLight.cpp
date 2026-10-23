#include "PointLight.h"

PointLight::PointLight(float x, float y, float z)
{
	_position = XMFLOAT3(x, y, z);
}

void PointLight::DrawGUI() 
{
	const std::string windowName = std::to_string(_id) + "_PointLight###PointLight_" + std::to_string(_id);
	ImGui::Begin(windowName.c_str());
	
	ImGui::DragFloat3("position", &_position.x, 0.01f);
	ImGui::DragFloat("intensity", &_intensity, 0.01f, 0.0f, FLT_MAX);
	ImGui::ColorEdit3("color", &_color.x);

	ImGui::End();
}
