#include "ImGuiRenderer.h"

ImGuiIO* ImGuiRenderer::_imguiIO = nullptr;

void ImGuiRenderer::Init(Window window, MSWRL::ComPtr<ID3D12Device> device, MSWRL::ComPtr<ID3D12DescriptorHeap> srvHeap)
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

	ImGui_ImplWin32_Init(window.GetHWND());
	ImGui_ImplDX12_Init(device.Get(), 3, DXGI_FORMAT_R8G8B8A8_UNORM, srvHeap.Get(), srvHeap->GetCPUDescriptorHandleForHeapStart(), srvHeap->GetGPUDescriptorHandleForHeapStart());


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

void ImGuiRenderer::Render(MSWRL::ComPtr<ID3D12CommandList> commandList)
{
	ImGui::Render();
	
	if (_imguiIO->ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault(nullptr, (void*)commandList.Get());
	}


}

void ImGuiRenderer::DragFloat3(const char* label, DirectX::XMFLOAT3& value)
{
	ImGui::DragFloat3(label, &value.x, 0.1f);
}

void ImGuiRenderer::Shutdown()
{
	_imguiIO = nullptr;
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}