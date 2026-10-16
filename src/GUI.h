#pragma once

#include "pch.h"

#include "D3D12Core.h"
#include "DescriptorAllocator.h"
#include "Window.h"
#include "IGUIComponent.h"

class CommandContext;

namespace GUI
{
	struct RenderTarget
	{
		ID3D12Resource* resource;
		D3D12_CPU_DESCRIPTOR_HANDLE rtv;
	};
	// Borrowed draw data is valid until the next ImGui frame. Owners cover image widgets.
	struct FrameData
	{
		ImDrawData* drawData;
		std::vector<std::shared_ptr<IGUIComponent>> components;
		MSWRL::ComPtr<ID3D12Resource> viewportTexture;
	};
	void InitializeGUI();
	FrameData BuildFrame();
	void RecordDrawData(std::shared_ptr<CommandContext> commandContext, ImDrawData* drawData, const RenderTarget& target);
	void Shutdown();

	void BeginDockspace();
	void RegisterComponent(std::weak_ptr<IGUIComponent> component);
	void SetViewportTextureHandle(D3D12_GPU_DESCRIPTOR_HANDLE viewportTextureHandle, MSWRL::ComPtr<ID3D12Resource> owner);
	std::vector<std::shared_ptr<IGUIComponent>> SetGUIComponentData();
	void SetViewportComponentData();
	void SetViewportExtent(const ImVec2& available);

	extern ImGuiIO* imguiIO;
	extern int32_t viewportWidth;
	extern int32_t viewportHeight;

	extern std::vector<std::weak_ptr<IGUIComponent>> guiComponents;

	extern bool viewportResized;
}
