#include "FrameCoordinator.h"
#include "Renderer.h"
#include "GUI.h"
#include "Window.h"

void FrameCoordinator::Initialize(uint32_t buffering)
{
	if (_initialized) ThrowException("Frame coordinator already initialized");

	_graphics.reserve(buffering);
	for (uint32_t i = 0; i < buffering; ++i)
	{
		auto gContext = std::make_shared<CommandContext>();
		gContext->InitializeCommandContext(QUEUE_GRAPHICS);
		_graphics.push_back(std::move(gContext));
		_hasSubmitted.push_back(false);
	}
	_initialized = true;
}

std::shared_ptr<CommandContext> FrameCoordinator::BeginFrame(const UINT frameIndex)
{
	if (!_initialized || _shutdown) ThrowException("Frame coordinator is not running");
	if (_failed) ThrowException("Cannot reuse a frame coordinator after a frame failure");
	if (_active) ThrowException("Cannot begin an overlapping frame");

	try
	{
		if (_hasSubmitted[frameIndex]) _graphics[frameIndex]->Reset();
		_active = true;
		return _graphics[frameIndex];
	}
	catch (...) { _failed = true; throw; }
}

CommandCompletion FrameCoordinator::SubmitFrame(const UINT frameIndex)
{
	if (!_initialized || _shutdown || !_active) ThrowException("Submission requires an active frame");
	if (_failed) ThrowException("Cannot submit after a frame failure");
	try
	{
		const auto completion = _graphics[frameIndex]->Finish(false);
		_hasSubmitted[frameIndex] = true;
		_active = false;
		return completion;
	}
	catch (...) { _failed = true; throw; }
}

void FrameCoordinator::RenderDetachedWindows(const CommandCompletion& completion)
{
	if (!(ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)) return;
	ImGui::UpdatePlatformWindows();
	const auto& viewports = ImGui::GetPlatformIO().Viewports;
	bool drawable = false;
	for (int i = 1; i < viewports.Size; ++i)
		if (!(viewports[i]->Flags & ImGuiViewportFlags_IsMinimized)) drawable = true;
	if (!drawable) return;
	CommandQueueManager::GetCommandQueue(QUEUE_GRAPHICS).WaitCPU(completion);
	ImGui::RenderPlatformWindowsDefault();
}

void FrameCoordinator::RenderFrame(Renderer& renderer, float dt)
{
	if (const auto resize = Window::ConsumePendingResize())
	{
		if (D3D12Core::Swapchain::swapchain &&
			(static_cast<uint32_t>(resize->width) != D3D12Core::Swapchain::width ||
			 static_cast<uint32_t>(resize->height) != D3D12Core::Swapchain::height))
		{
			Drain();
			D3D12Core::Swapchain::Resize(resize->width, resize->height);
		}
	}

	const UINT framebufferIndex = D3D12Core::Swapchain::swapchain->GetCurrentBackBufferIndex();
	
	std::shared_ptr<CommandContext> graphics = BeginFrame(framebufferIndex);

	GUI::SetViewportTextureHandle(renderer._viewportSRV_GPU, renderer._viewportTexture);
	const GUI::FrameData guiFrame = GUI::BuildFrame();

	renderer.PrepareFrame(dt);
	renderer.RecordScene(graphics);

	for (const auto& owner : guiFrame.components)
		graphics->KeepAlive(owner);

	graphics->KeepAlive(guiFrame.viewportTexture);

	GUI::RecordDrawData(graphics, guiFrame.drawData, { D3D12Core::Swapchain::renderTargets[framebufferIndex].Get(), D3D12Core::Swapchain::rtvCPUHandle[framebufferIndex] });

	const CommandCompletion completion = SubmitFrame(framebufferIndex);

	RenderDetachedWindows(completion);

	ThrowIfFailed(D3D12Core::Swapchain::swapchain->Present(0, 0), "Presenting main window failed");
}

void FrameCoordinator::Drain()
{
	if (!_initialized || _shutdown) return;
	CommandQueueManager::GetCommandQueue(QUEUE_GRAPHICS).Flush();
}

void FrameCoordinator::Shutdown()
{
	if (!_initialized || _shutdown) return;
	Drain();
	_shutdown = true;
}
