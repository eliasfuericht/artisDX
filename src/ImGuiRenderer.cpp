#include "ImGuiRenderer.h"

ImGuiIO* ImGuiRenderer::_imguiIO = nullptr;

MSWRL::ComPtr<ID3D12GraphicsCommandList> ImGuiRenderer::_commandList;
MSWRL::ComPtr<ID3D12CommandAllocator> ImGuiRenderer::_commandAllocator;
MSWRL::ComPtr<ID3D12DescriptorHeap> ImGuiRenderer::_srvHeap;

void ImGuiRenderer::Init(Window window, MSWRL::ComPtr<ID3D12Device> device)
{
	// init imgui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiRenderer::_imguiIO = &ImGui::GetIO();
	(void)_imguiIO;
	_imguiIO->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	_imguiIO->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	_imguiIO->ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
	_imguiIO->ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows
	//io.ConfigViewportsNoAutoMerge = true;
	//io.ConfigViewportsNoTaskBarIcon = true;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
	ImGuiStyle& style = ImGui::GetStyle();
	if (_imguiIO->ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	desc.NumDescriptors = 1;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&_srvHeap)));

	// Create Command Allocator & Command List
	ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_commandAllocator)));
	ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _commandAllocator.Get(), nullptr, IID_PPV_ARGS(&_commandList)));
	_commandList->Close();

	ImGui_ImplWin32_Init(window.GetHWND());
	ImGui_ImplDX12_Init(device.Get(), 3, DXGI_FORMAT_R8G8B8A8_UNORM, _srvHeap.Get(), _srvHeap->GetCPUDescriptorHandleForHeapStart(), _srvHeap->GetGPUDescriptorHandleForHeapStart());
}

void ImGuiRenderer::NewFrame()
{
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void ImGuiRenderer::Begin(const char* title)
{
	ImGui::Begin(title);
}

void ImGuiRenderer::PushID(INT id)
{
	ImGui::PushID(id);
}

void ImGuiRenderer::End()
{
	ImGui::End();
}

void ImGuiRenderer::PopID()
{
	ImGui::PopID();
}

void ImGuiRenderer::Render(MSWRL::ComPtr<ID3D12CommandQueue> commandQueue, MSWRL::ComPtr<ID3D12Resource> currentBackBuffer, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle)
{
	ImGui::Render();
	
	if (_imguiIO->ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault(nullptr, (void*)_commandList.Get());
	}

	// Reset Command Allocator and List
	ThrowIfFailed(_commandAllocator->Reset());
	ThrowIfFailed(_commandList->Reset(_commandAllocator.Get(), nullptr));

	// Set Descriptor Heap
	ID3D12DescriptorHeap* heaps[] = { _srvHeap.Get() };
	_commandList->SetDescriptorHeaps(1, heaps);
	
	// Transition backbuffer from PRESENT -> RENDER_TARGET
	D3D12_RESOURCE_BARRIER renderTargetBarrier = {};
	renderTargetBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	renderTargetBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	renderTargetBarrier.Transition.pResource = currentBackBuffer.Get();
	renderTargetBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	renderTargetBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	renderTargetBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	_commandList->ResourceBarrier(1, &renderTargetBarrier);

	// Set render target for ImGui
	_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

	// Set Descriptor Heap for ImGui
	ID3D12DescriptorHeap* descriptorHeaps[] = { _srvHeap.Get() };
	_commandList->SetDescriptorHeaps(1, descriptorHeaps);

	// Render ImGui onto the same backbuffer
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), _commandList.Get());
	
	// Transition backbuffer back to PRESENT
	D3D12_RESOURCE_BARRIER presentBarrier = {};
	presentBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	presentBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	presentBarrier.Transition.pResource = currentBackBuffer.Get();
	presentBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	presentBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	presentBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	_commandList->ResourceBarrier(1, &presentBarrier);

	// Close & Execute
	ThrowIfFailed(_commandList->Close());
	ID3D12CommandList* lists[] = { _commandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(lists), lists);
}

void ImGuiRenderer::Shutdown()
{
	_imguiIO = nullptr;
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

// ----------------------------------------------------//
// 																										 //
// Some wrappers for the most common imgui ui features //
// 																										 //
// ----------------------------------------------------//

void ImGuiRenderer::DragFloat(const char* label, float& value, float speed)
{
	ImGui::DragFloat(label, &value, speed);
}

void ImGuiRenderer::DragFloat2(const char* label, XMFLOAT2& value, float speed)
{
	ImGui::DragFloat2(label, &value.x, speed);
}

void ImGuiRenderer::DragFloat3(const char* label, XMFLOAT3& value, float speed)
{
	ImGui::DragFloat3(label, &value.x, speed);
}

void ImGuiRenderer::DragFloat4(const char* label, XMFLOAT4& value, float speed)
{
	ImGui::DragFloat4(label, &value.x, speed);
}


void ImGuiRenderer::DragInt(const char* label, int& value, float speed)
{
	ImGui::DragInt(label, &value, static_cast<int>(speed));
}

void ImGuiRenderer::SliderFloat(const char* label, float& value, float min, float max)
{
	ImGui::SliderFloat(label, &value, min, max);
}

void ImGuiRenderer::SliderFloat2(const char* label, XMFLOAT2& value, float min, float max)
{
	ImGui::SliderFloat2(label, &value.x, min, max);
}

void ImGuiRenderer::SliderFloat3(const char* label, XMFLOAT3& value, float min, float max)
{
	ImGui::SliderFloat3(label, &value.x, min, max);
}

void ImGuiRenderer::SliderFloat4(const char* label, XMFLOAT4& value, float min, float max)
{
	ImGui::SliderFloat4(label, &value.x, min, max);
}

void ImGuiRenderer::SliderInt(const char* label, int& value, int min, int max)
{
	ImGui::SliderInt(label, &value, min, max);
}

void ImGuiRenderer::InputFloat(const char* label, float& value)
{
	ImGui::InputFloat(label, &value);
}

void ImGuiRenderer::InputInt(const char* label, int& value)
{
	ImGui::InputInt(label, &value);
}

void ImGuiRenderer::InputFloat2(const char* label, XMFLOAT2& value)
{
	ImGui::InputFloat2(label, &value.x);
}

void ImGuiRenderer::InputFloat3(const char* label, XMFLOAT3& value)
{
	ImGui::InputFloat3(label, &value.x);
}

void ImGuiRenderer::InputFloat4(const char* label, XMFLOAT4& value)
{
	ImGui::InputFloat4(label, &value.x);
}

void ImGuiRenderer::Checkbox(const char* label, bool& value)
{
	ImGui::Checkbox(label, &value);
}

void ImGuiRenderer::ColorEdit3(const char* label, XMFLOAT3& color)
{
	ImGui::ColorEdit3(label, &color.x);
}

void ImGuiRenderer::ColorEdit4(const char* label, XMFLOAT4& color)
{
	ImGui::ColorEdit4(label, &color.x);
}

void ImGuiRenderer::Text(const char* text)
{
	ImGui::Text("%s", text);
}

bool ImGuiRenderer::Button(const char* label)
{
	return ImGui::Button(label);
}

void ImGuiRenderer::SameLine()
{
	ImGui::SameLine();
}

void ImGuiRenderer::Separator()
{
	ImGui::Separator();
}

void ImGuiRenderer::Spacing()
{
	ImGui::Spacing();
}

void ImGuiRenderer::BeginGroup()
{
	ImGui::BeginGroup();
}

void ImGuiRenderer::EndGroup()
{
	ImGui::EndGroup();
}
