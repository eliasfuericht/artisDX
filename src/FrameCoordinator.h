#pragma once

#include "CommandContext.h"

class Renderer;

class FrameCoordinator
{
public:
	void Initialize(uint32_t buffering);
	std::shared_ptr<CommandContext> BeginFrame(const UINT frameIndex);
	CommandCompletion SubmitFrame(const UINT frameIndex);
	void RenderFrame(Renderer& renderer, float dt);
	void Drain();
	void Shutdown();
private:
	std::vector<std::shared_ptr<CommandContext>> _graphics;
	bool _initialized = false;
	bool _active = false;
	std::vector<bool> _hasSubmitted;
	bool _failed = false;
	bool _shutdown = false;
	void RenderDetachedWindows(const CommandCompletion& completion);
};
