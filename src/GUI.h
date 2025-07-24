#pragma once

#include "pch.h"

#include "D3D12Core.h"
#include "CommandQueue.h"
#include "Window.h"
#include "IGUIComponent.h"

namespace GUI
{
	void InitializeGUI(HWND hwnd);
	void Draw();
	void Render();
	void Shutdown();

	void RegisterComponent(std::weak_ptr<IGUIComponent> component);
	void SetViewportTexture(D3D12_CPU_DESCRIPTOR_HANDLE viewportTextureHandle);
	void GetGUIComponentData();

	void End();

	extern ImGuiIO* imguiIO;
	
	extern D3D12_CPU_DESCRIPTOR_HANDLE _viewportTexture;
	extern MSWRL::ComPtr<ID3D12DescriptorHeap> srvHeap;
	extern MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList;
	extern MSWRL::ComPtr<ID3D12CommandAllocator> commandAllocator;

	extern std::vector<std::weak_ptr<IGUIComponent>> guiComponents;
}