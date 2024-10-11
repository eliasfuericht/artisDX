#pragma once
#include <stdio.h>

#include "Application.h"
#include "Window.h"
#include "Renderer.h"

int main()
{
	Application artisDX("artisDX", 1280, 720);
	artisDX.Run();
}