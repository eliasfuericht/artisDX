#include "Application.h"

Application::Application(const CHAR* name, INT w, INT h)
{
	_window = Window(name, w, h);
	Create();
	InitializeAPI();
	_renderer = new Renderer(_window);
}

CHECK Application::Create()
{
	if (_window.Create() != OK)
		return NOTOK;
}

CHECK Application::InitializeAPI()
{
	return OK;
}

CHECK Application::Run()
{
	if (_window.Show() != OK)
		return NOTOK;

	MSG msg = { 0 };

	while (msg.message != WM_QUIT)
	{
		_renderer->Render();
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
}

Application::~Application()
{
	delete _renderer;
}