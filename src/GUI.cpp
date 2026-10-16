#include "GUI.h"
#include "CommandContext.h"
#include <cmath>

namespace GUI
{
	ImGuiIO* imguiIO = nullptr;
	std::vector<std::weak_ptr<IGUIComponent>> guiComponents;
	D3D12_GPU_DESCRIPTOR_HANDLE _viewportTexture;
	MSWRL::ComPtr<ID3D12Resource> _viewportTextureOwner;
	int32_t viewportWidth = 1920;
	int32_t viewportHeight = 1080;
	bool viewportResized = false;

	void InitializeGUI()
	{
		if (ImGui::GetCurrentContext()) ThrowException("GUI already initialized");
		// init imgui
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		GUI::imguiIO = &ImGui::GetIO();
		(void)imguiIO;
		GUI::imguiIO->ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		GUI::imguiIO->ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
		GUI::imguiIO->ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
		GUI::imguiIO->ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows
		//io.ConfigViewportsNoAutoMerge = true;
		//io.ConfigViewportsNoTaskBarIcon = true;

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();

		// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
		ImGuiStyle& style = ImGui::GetStyle();
		if (GUI::imguiIO->ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		if (!ImGui_ImplWin32_Init(Window::hWindow)) ThrowException("Failed to initialize ImGui Win32 backend");

		const auto handle = DescriptorAllocator::CBVSRVUAV::Allocate();

		if (!ImGui_ImplDX12_Init(	D3D12Core::GraphicsDevice::device.Get(), D3D12Core::Swapchain::backBufferCount,
															DXGI_FORMAT_R8G8B8A8_UNORM, DescriptorAllocator::CBVSRVUAV::GetHeap(), handle,
															DescriptorAllocator::CBVSRVUAV::GetGPUHandle(handle)))
			ThrowException("Failed to initialize ImGui DX12 backend");
		
		if (!ImGui_ImplDX12_CreateDeviceObjects()) ThrowException("Failed to create ImGui DX12 device objects");
	}

	void RegisterComponent(std::weak_ptr<IGUIComponent> component)
	{
		GUI::guiComponents.push_back(component);
	}

	int32_t GetViewportWidth()
	{
		return viewportWidth;
	}

	int32_t GetViewportHeight()
	{
		return viewportHeight;
	}

	void SetViewportTextureHandle(D3D12_GPU_DESCRIPTOR_HANDLE viewportTextureHandle, MSWRL::ComPtr<ID3D12Resource> owner)
	{
		_viewportTexture = viewportTextureHandle;
		_viewportTextureOwner = std::move(owner);
	}

	void SetViewportComponentData()
	{
		ImGui::Begin("Viewport");

		ImVec2 avail = ImGui::GetContentRegionAvail();
		ImTextureID texID = (ImTextureID)_viewportTexture.ptr;

		ImGui::Image(
			texID,
			avail,
			ImVec2(0, 0),
			ImVec2(1, 1),
			ImVec4(1, 1, 1, 1),
			ImVec4(0, 0, 0, 0)
		);
		
		bool hovered = ImGui::IsItemHovered();
		bool clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);

		if (hovered && clicked && !Window::captureMouse)
		{			
				RECT clientRect;
				GetClientRect(Window::hWindow, &clientRect);
				POINT center = { (clientRect.right - clientRect.left) / 2, (clientRect.bottom - clientRect.top) / 2 };
				ClientToScreen(Window::hWindow, &center);
				SetCursorPos(center.x, center.y);

				Window::captureMouse = true;
				SetCapture(Window::hWindow);
				ShowCursor(false);
		}

		SetViewportExtent(avail);

		ImGui::End();
	}

	void SetViewportExtent(const ImVec2& available)
	{
		viewportResized = false;
		// Keep conversion defined, then validate the integer dimensions we consume.
		if (!std::isfinite(available.x) || !std::isfinite(available.y) || available.x <= 0 || available.y <= 0 ||
			available.x >= static_cast<float>(INT32_MAX) || available.y >= static_cast<float>(INT32_MAX))
			return;
		const int32_t width = static_cast<int32_t>(available.x);
		const int32_t height = static_cast<int32_t>(available.y);
		if (width <= 0 || height <= 0) return;
		if (width != viewportWidth || height != viewportHeight)
		{
			viewportWidth = width;
			viewportHeight = height;
			viewportResized = true;
		}
	}

	std::vector<std::shared_ptr<IGUIComponent>> SetGUIComponentData()
	{
		std::vector<std::shared_ptr<IGUIComponent>> components;
		components.reserve(GUI::guiComponents.size());
		for (auto const& weak : GUI::guiComponents)
		{
			if (auto shared = weak.lock())
				components.push_back(std::move(shared));
		}

		for (auto const& component : components)
		{
			component->DrawGUI();
		}
		return components;
	}

	void BeginDockspace()
	{
		ImGuiViewport* viewport = ImGui::GetMainViewport();

		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);
		ImGui::SetNextWindowViewport(viewport->ID);

		ImGuiWindowFlags windowFlags =
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoBringToFrontOnFocus |
			ImGuiWindowFlags_NoNavFocus |
			ImGuiWindowFlags_NoDocking; // important for root

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

		ImGui::Begin("DockspaceRoot", nullptr, windowFlags);
		ImGui::PopStyleVar(3);

		ImGuiID dockspaceID = ImGui::GetID("MainDockspace");
		ImGui::DockSpace(dockspaceID, ImVec2(0.0f, 0.0f));

		ImGui::End();
	}

	FrameData BuildFrame()
	{
		if (!ImGui::GetCurrentContext() || !ImGui::GetIO().BackendRendererUserData)
			ThrowException("GUI is not initialized");
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		BeginDockspace();
		auto components = SetGUIComponentData();
		SetViewportComponentData();
		ImGui::Render();
		return { ImGui::GetDrawData(), std::move(components), _viewportTextureOwner };
	}

	void RecordDrawData(std::shared_ptr<CommandContext> commandContext, ImDrawData* drawData, const RenderTarget& target)
	{
		if (!ImGui::GetCurrentContext() || !ImGui::GetIO().BackendRendererUserData ||
			!drawData || !target.resource || !target.rtv.ptr)
			ThrowException("GUI recording requires an initialized backend, draw data, and a render target");
		commandContext->DeclareResource(target.resource, D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_PRESENT, "GUI backbuffer");
		commandContext->KeepAlive(MSWRL::ComPtr<IUnknown>(DescriptorAllocator::CBVSRVUAV::GetHeap()));
		commandContext->UseResource(target.resource, D3D12_RESOURCE_STATE_RENDER_TARGET);
		const auto list = commandContext->GetCommandList();
		list->OMSetRenderTargets(1, &target.rtv, FALSE, nullptr);
		const float clearColor[] = { 0.2f, 0.2f, 0.2f, 1.0f };
		list->ClearRenderTargetView(target.rtv, clearColor, 0, nullptr);
		ID3D12DescriptorHeap* heaps[] = { DescriptorAllocator::CBVSRVUAV::GetHeap() };
		list->SetDescriptorHeaps(_countof(heaps), heaps);
		ImGui_ImplDX12_RenderDrawData(drawData, list.Get());
		commandContext->UseResource(target.resource, D3D12_RESOURCE_STATE_PRESENT);
	}

	void Shutdown()
	{
		if (!ImGui::GetCurrentContext()) return;
		if (ImGui::GetIO().BackendRendererUserData) ImGui_ImplDX12_Shutdown();
		GUI::imguiIO = nullptr;
		if (ImGui::GetIO().BackendPlatformUserData) ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
		_viewportTextureOwner.Reset();
		_viewportTexture = {};
	}
}
