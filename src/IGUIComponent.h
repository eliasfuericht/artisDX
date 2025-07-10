#pragma once

class GUI;

class IGUIComponent : public std::enable_shared_from_this<IGUIComponent>
{
public:
	virtual void RegisterWithGUI();

	virtual void DrawGUI() = 0;
};