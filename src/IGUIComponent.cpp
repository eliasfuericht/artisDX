#include "IGUIComponent.h"
#include "GUI.h"

void IGUIComponent::RegisterWithGUI()
{
	GUI::RegisterComponent(weak_from_this());
}