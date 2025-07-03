#pragma once

#include "pch.h"

#include "GraphicsDevice.h"
#include "Swapchain.h"
#include "CommandQueue.h"
#include "Window.h"
#include "IGUIComponent.h"

class GUI
{
public:
	static void Init(Window window);
	static void Draw();
	static void Render();
	static void Shutdown();

	static void RegisterComponent(std::weak_ptr<IGUIComponent> component);

	static void NewFrame();
	static void Begin(const char* title);
	static void PushID(INT id);
	static void End();
	static void PopID();

	static void DragInt(const char* label, int& value, float speed = 1.0f);
	static void SliderInt(const char* label, int& value, int min, int max);
	static void InputInt(const char* label, int& value);

	static void DragFloat(const char* label, float& value, float speed = 0.1f);
	static void DragFloat2(const char* label, XMFLOAT2& value, float speed = 0.1f);
	static void DragFloat3(const char* label, XMFLOAT3& value, float speed = 0.1f);
	static void DragFloat4(const char* label, XMFLOAT4& value, float speed = 0.1f);

	static void SliderFloat(const char* label, float& value, float min, float max);
	static void SliderFloat2(const char* label, XMFLOAT2& value, float min, float max);
	static void SliderFloat3(const char* label, XMFLOAT3& value, float min, float max);
	static void SliderFloat4(const char* label, XMFLOAT4& value, float min, float max);

	static void InputFloat(const char* label, float& value);
	static void InputFloat2(const char* label, XMFLOAT2& value);
	static void InputFloat3(const char* label, XMFLOAT3& value);
	static void InputFloat4(const char* label, XMFLOAT4& value);

	static void Checkbox(const char* label, bool& value);
	static void ColorEdit3(const char* label, XMFLOAT3& color);
	static void ColorEdit4(const char* label, XMFLOAT4& color);

	static void Text(const char* text);
	static bool Button(const char* label);

	static void SameLine();
	static void Separator();
	static void Spacing();

	static void BeginGroup();
	static void EndGroup();

	// singleton stuff - deleting copy and assignment operator
	GUI(const GUI&) = delete;
	GUI& operator=(const GUI&) = delete;

private:
	// singleton stuff - setting constructor and destructor private
	GUI() = default;
	~GUI() = default;

	// this isnt beautiful - dont like
	static ImGuiIO* _imguiIO;

	static MSWRL::ComPtr<ID3D12DescriptorHeap> _srvHeap;
	static MSWRL::ComPtr<ID3D12GraphicsCommandList> _commandList;
	static MSWRL::ComPtr<ID3D12CommandAllocator> _commandAllocator;
	static MSWRL::ComPtr<IDXGISwapChain3> _swapchain;
	static MSWRL::ComPtr<ID3D12DescriptorHeap> _rtvHeap;
	static UINT _rtvDescriptorSize;
	static MSWRL::ComPtr<ID3D12Resource> _renderTargets[2];

	static std::vector<std::weak_ptr<IGUIComponent>> _guiComponents;
};