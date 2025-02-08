#pragma once

#include "pch.h"

#include "Window.h"

class ImGuiRenderer
{
public:
	static void Init(Window window, MSWRL::ComPtr<ID3D12Device> device, MSWRL::ComPtr<ID3D12DescriptorHeap> srvHeap);
	static void NewFrame();
	static void Begin(const char* title);
	static void PushID(INT id);
	static void End();
	static void PopID();
	static void Render(MSWRL::ComPtr<ID3D12CommandList> commandList);
	static void Shutdown();

	static void DragFloat3(const char* label, DirectX::XMFLOAT3& value);


	// singleton stuff - deleting copy and assignment operator
	ImGuiRenderer(const ImGuiRenderer&) = delete;
	ImGuiRenderer& operator=(const ImGuiRenderer&) = delete;

private:
	// singleton stuff - setting constructor and destructor private
	ImGuiRenderer() = default;
	~ImGuiRenderer() = default;

	// this isnt beautiful - dont like
	static ImGuiIO* _imguiIO;
};