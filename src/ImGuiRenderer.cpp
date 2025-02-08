#include "ImGuiRenderer.h"

ImGuiIO* ImGuiRenderer::_imguiIO = nullptr;

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

void ImGuiRenderer::Render(MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList)
{
	ImGui::Render();
	
	if (_imguiIO->ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault(nullptr, (void*)commandList.Get());
	}
	// this throws errors
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList.Get());
}


void ImGuiRenderer::Shutdown()
{
	_imguiIO = nullptr;
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

// -----------------------------------------------------------------------------------------------
// 
// Some wrappers for the most common imgui ui features
// 
// -----------------------------------------------------------------------------------------------

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
