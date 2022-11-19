#ifndef OOT_IMGUI_H
#define OOT_IMGUI_H

#ifdef WITH_IMGUI

#include "imgui.h"

#include "backends/imgui_impl_sdl.h"

namespace OOT_Imgui
{
	void CreateImguiContext(void* window, void* ctx);
	void InitOpenGL();

	void SwapBuffers();

	void Shutdown();
}

#endif // WITH_IMGUI
#endif // OOT_IMGUI_H
