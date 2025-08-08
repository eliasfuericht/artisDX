#include "GUI.h"

namespace GUI
{
	ImGuiIO* imguiIO = nullptr;
	CommandContext guiContext;
	std::vector<std::weak_ptr<IGUIComponent>> guiComponents;
	D3D12_GPU_DESCRIPTOR_HANDLE _viewportTexture;
	int32_t viewportWidth = 1920;
	int32_t viewportHeight = 1080;
	bool viewportResized = false;

	void InitializeGUI()
	{
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

		ImGui_ImplWin32_Init(Window::hWindow);
		auto handle = DescriptorAllocator::CBVSRVUAV::Allocate();
		ImGui_ImplDX12_Init(D3D12Core::GraphicsDevice::device.Get(), D3D12Core::Swapchain::backBufferCount, DXGI_FORMAT_R8G8B8A8_UNORM, DescriptorAllocator::CBVSRVUAV::GetHeap(), handle, DescriptorAllocator::CBVSRVUAV::GetGPUHandle(handle));
		
		GUI::guiContext.InitializeCommandContext(QUEUETYPE::QUEUE_GRAPHICS);
		GUI::guiContext.Finish(false);
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

	void SetViewportTextureHandle(D3D12_GPU_DESCRIPTOR_HANDLE viewportTextureHandle)
	{
		_viewportTexture = viewportTextureHandle;
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

		viewportResized = false;
		if (avail.x != viewportWidth || avail.y != viewportHeight)
		{
			viewportWidth = static_cast<int32_t>(avail.x);
			viewportHeight = static_cast<int32_t>(avail.y);
			viewportResized = true;
		}

		ImGui::End();
	}

	void SetGUIComponentData()
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

	void Draw()
	{
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		BeginDockspace();
		SetGUIComponentData();
		SetViewportComponentData();

		ImGui::Render();

		if (GUI::imguiIO->ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault(nullptr, (void*)GUI::guiContext.GetCommandList().Get());
		}

		GUI::guiContext.Reset();

		uint32_t frameIndex = D3D12Core::Swapchain::swapchain->GetCurrentBackBufferIndex();
		ID3D12Resource* backbuffer = D3D12Core::Swapchain::renderTargets[frameIndex].Get();

		auto toRT = CD3DX12_RESOURCE_BARRIER::Transition(
			backbuffer,
			D3D12_RESOURCE_STATE_PRESENT,
			D3D12_RESOURCE_STATE_RENDER_TARGET
		);
		GUI::guiContext.GetCommandList()->ResourceBarrier(1, &toRT);
		D3D12_CPU_DESCRIPTOR_HANDLE currentRTVHandle = D3D12Core::Swapchain::rtvCPUHandle[frameIndex];
		GUI::guiContext.GetCommandList()->OMSetRenderTargets(1, &currentRTVHandle, FALSE, nullptr);
		const float clearColor[] = { 0.2f, 0.2f, 0.2f, 1.0f };
		GUI::guiContext.GetCommandList()->ClearRenderTargetView(currentRTVHandle, clearColor, 0, nullptr);

		ID3D12DescriptorHeap* heaps[] = { DescriptorAllocator::CBVSRVUAV::GetHeap() };
		GUI::guiContext.GetCommandList()->SetDescriptorHeaps(_countof(heaps), heaps);

		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), GUI::guiContext.GetCommandList().Get());

		auto toPresent = CD3DX12_RESOURCE_BARRIER::Transition(
			backbuffer,
			D3D12_RESOURCE_STATE_RENDER_TARGET,
			D3D12_RESOURCE_STATE_PRESENT
		);
		GUI::guiContext.GetCommandList()->ResourceBarrier(1, &toPresent);

		ThrowIfFailed(GUI::guiContext.GetCommandList()->Close());
		ID3D12CommandList* lists[] = { GUI::guiContext.GetCommandList().Get() };
		CommandQueueManager::GetCommandQueue(QUEUETYPE::QUEUE_GRAPHICS)._commandQueue->ExecuteCommandLists(_countof(lists), lists);
	}

	void Shutdown()
	{
		GUI::imguiIO = nullptr;
		ImGui_ImplDX12_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}
}