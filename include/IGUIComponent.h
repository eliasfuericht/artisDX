#pragma once

class IGUIComponent : public std::enable_shared_from_this<IGUIComponent>
{
public:
	virtual void RegisterWithGUI() = 0;
	virtual void DrawGUI() = 0;
};