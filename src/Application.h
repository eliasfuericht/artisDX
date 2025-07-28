#pragma once

#include "pch.h"

#include "D3D12Core.h"
#include "Window.h"
#include "Renderer.h"
#include "CommandQueue.h"
#include "CommandContext.h"
#include "DescriptorAllocator.h"
#include "Shader.h"
#include "ShaderPass.h"
#include "ModelManager.h"
#include "Camera.h"
#include "DirectionalLight.h"
#include "PointLight.h"

class Application
{
public:
	Application(const char* name, int32_t w, int32_t h, bool fullscreen);
	void Run();
	~Application();

private:
	void InitializeApplication();
	void Update(float dt);

	Renderer _renderer;

	std::string _name;
	uint32_t _width;
	uint32_t _height;
	bool _fullscreen;

	std::chrono::steady_clock::time_point _tLastTime = std::chrono::steady_clock::now();
	std::chrono::high_resolution_clock::time_point _startTime;
	std::chrono::high_resolution_clock::time_point _lastTime;
	float _elapsedTime = 0.0;
	int32_t _frameCount = 0;
	float _fps = 0.0;
};