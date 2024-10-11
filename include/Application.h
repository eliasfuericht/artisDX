#pragma once

#include "pch.h"
#include "Window.h"
#include "Renderer.h"

class Application
{
public:
	Application(const CHAR* name, INT w, INT h);
	CHECK Run();
	~Application();

private:
	CHECK Create();
	CHECK InitializeAPI();
	Window _window;
	Renderer* _renderer;

};