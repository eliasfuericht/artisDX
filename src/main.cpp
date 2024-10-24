#pragma once
#include <stdio.h>

#include "Application.h"

int main()
{
	Application artisDX("artisDX", 1280, 720);
	artisDX.Run();
	PRINT("SHUTDOWN");
}