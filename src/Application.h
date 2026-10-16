#pragma once

#include "pch.h"

#include "D3D12Core.h"
#include "Window.h"
#include "Renderer.h"
#include "FrameCoordinator.h"
#include "CommandQueue.h"
#include "CommandContext.h"
#include "DescriptorAllocator.h"
#include "Shader.h"
#include "ShaderPass.h"
#include "ModelManager.h"
#include "Camera.h"
#include "DirectionalLight.h"
#include "PointLight.h"
#include "TestLauncher.h"

class Application
{
public:
	static int Start(int argc, char** argv, const char* name, uint32_t w, uint32_t h, uint32_t buffering, bool fullscreen);
	Application(const char* name, int32_t w, int32_t h, uint32_t buffering, bool fullscreen);
	void Run();
	~Application();

private:
	void InitializeApplication();
	void UpdateWindowFPS(float dt);
	void ShutdownGraphics() noexcept;

	Renderer _renderer;
	FrameCoordinator _frameCoordinator;
	bool _comInitialized = false;

	std::string _name;
	uint32_t _width;
	uint32_t _height;
	uint32_t _buffering;
	bool _fullscreen;

	std::chrono::steady_clock::time_point _tLastTime = std::chrono::steady_clock::now();
	std::chrono::high_resolution_clock::time_point _startTime;
	std::chrono::high_resolution_clock::time_point _lastTime;
	float _elapsedTime = 0.0;
	int32_t _frameCount = 0;
	float _fps = 0.0;
};
