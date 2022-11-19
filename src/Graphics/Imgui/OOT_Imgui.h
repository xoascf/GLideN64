#ifndef OOT_IMGUI_H
#define OOT_IMGUI_H

#ifdef WITH_IMGUI

#include "imgui.h"

#ifndef _WIN32
#include "backends/imgui_impl_sdl.h"
#endif

namespace OOT_Imgui
{
	void CreateImguiContext(void* window, void* ctx);
	void InitOpenGL();

	void SwapBuffers();

	void Shutdown();
}

#endif // WITH_IMGUI
#endif // OOT_IMGUI_H
