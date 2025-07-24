#include "GUI.h"

namespace GUI
{
	ImGuiIO* imguiIO = nullptr;
	MSWRL::ComPtr<ID3D12GraphicsCommandList> commandList;
	MSWRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
	MSWRL::ComPtr<ID3D12DescriptorHeap> srvHeap;
	std::vector<std::weak_ptr<IGUIComponent>> guiComponents;

	void InitializeGUI(HWND hwnd)
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

		D3D12_DESCRIPTOR_HEAP_DESC desc = {};
		desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		desc.NumDescriptors = 1;
		desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		ThrowIfFailed(D3D12Core::GraphicsDevice::device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&GUI::srvHeap)));

		// Create Command Allocator & Command List
		ThrowIfFailed(D3D12Core::GraphicsDevice::device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&GUI::commandAllocator)));
		ThrowIfFailed(D3D12Core::GraphicsDevice::device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, GUI::commandAllocator.Get(), nullptr, IID_PPV_ARGS(&GUI::commandList)));
		GUI::commandList->SetName(L"GUI CommandList");
		GUI::commandList->Close();

		ImGui_ImplWin32_Init(hwnd);
		ImGui_ImplDX12_Init(D3D12Core::GraphicsDevice::device.Get(), D3D12Core::Swapchain::backBufferCount, DXGI_FORMAT_R8G8B8A8_UNORM, GUI::srvHeap.Get(), GUI::srvHeap->GetCPUDescriptorHandleForHeapStart(), GUI::srvHeap->GetGPUDescriptorHandleForHeapStart());
	}

	void RegisterComponent(std::weak_ptr<IGUIComponent> component)
	{
		GUI::guiComponents.push_back(component);
	}

	void SetViewportTexture(D3D12_CPU_DESCRIPTOR_HANDLE viewportTextureHandle)
	{
		// TODO: set 
	}

	void Draw()
	{
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		GetGUIComponentData();
		Render();
	}

	void GetGUIComponentData()
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

	void Render()
	{
		ImGui::Render();

		if (GUI::imguiIO->ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault(nullptr, (void*)GUI::commandList.Get());
		}

		// Reset Command Allocator and List
		ThrowIfFailed(GUI::commandAllocator->Reset());
		ThrowIfFailed(GUI::commandList->Reset(GUI::commandAllocator.Get(), nullptr));

		// Set Descriptor Heap
		ID3D12DescriptorHeap* heaps[] = { GUI::srvHeap.Get() };
		GUI::commandList->SetDescriptorHeaps(1, heaps);

		uint32_t frameIndex = D3D12Core::Swapchain::frameIndex;

		D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = D3D12Core::Swapchain::rtvHeap->GetCPUDescriptorHandleForHeapStart();
		rtvHandle.ptr += (frameIndex * D3D12Core::Swapchain::rtvDescriptorSize);

		// Transition backbuffer from PRESENT -> RENDER_TARGET
		D3D12_RESOURCE_BARRIER renderTargetBarrier = {};
		renderTargetBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		renderTargetBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		renderTargetBarrier.Transition.pResource = D3D12Core::Swapchain::renderTargets[frameIndex].Get();
		renderTargetBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		renderTargetBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		renderTargetBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		GUI::commandList->ResourceBarrier(1, &renderTargetBarrier);

		// Set render target for ImGui
		GUI::commandList->OMSetRenderTargets(1, &rtvHandle, false, nullptr);

		// Set Descriptor Heap for ImGui
		ID3D12DescriptorHeap* descriptorHeaps[] = { GUI::srvHeap.Get() };
		GUI::commandList->SetDescriptorHeaps(1, descriptorHeaps);

		// Render ImGui onto the same backbuffer
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), GUI::commandList.Get());

		// Transition backbuffer back to PRESENT
		D3D12_RESOURCE_BARRIER presentBarrier = {};
		presentBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		presentBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		presentBarrier.Transition.pResource = D3D12Core::Swapchain::renderTargets[frameIndex].Get();
		presentBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		presentBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		presentBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		GUI::commandList->ResourceBarrier(1, &presentBarrier);

		// Close & Execute
		ThrowIfFailed(GUI::commandList->Close());
		ID3D12CommandList* lists[] = { GUI::commandList.Get() };
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