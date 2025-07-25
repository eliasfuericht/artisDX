#pragma once

#include "pch.h"

#include "D3D12Core.h"
#include "DescriptorAllocator.h"
#include "CommandQueue.h"
#include "CommandContext.h"
#include "Window.h"
#include "IGUIComponent.h"

namespace GUI
{
	void InitializeGUI();
	void Draw();
	void Render();
	void Shutdown();

	void BeginDockspace();
	void RegisterComponent(std::weak_ptr<IGUIComponent> component);
	void SetViewportTextureHandle(D3D12_GPU_DESCRIPTOR_HANDLE viewportTextureHandle);
	void SetGUIComponentData();
	void SetViewportComponentData();

	void End();

	extern ImGuiIO* imguiIO;
	
	extern CommandContext guiContext;

	extern D3D12_GPU_DESCRIPTOR_HANDLE viewportTexture;
	extern int32_t viewportWidth;
	extern int32_t viewportHeight;

	extern std::vector<std::weak_ptr<IGUIComponent>> guiComponents;

	extern bool viewportResized;
}