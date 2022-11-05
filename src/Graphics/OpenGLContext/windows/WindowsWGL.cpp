#define WGL_WGLEXT_PROTOTYPES
#include "WindowsWGL.h"
#include <Config.h>
#include <GLideN64.h>
#include <Graphics/OpenGLContext/GLFunctions.h>

#ifdef WITH_IMGUI
#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_opengl3.h"
#endif
#include <functional>
#include <vector>

HGLRC WindowsWGL::hRC = NULL;
HDC WindowsWGL::hDC = NULL;

bool WindowsWGL::start()
{
	int pixelFormat;

	PIXELFORMATDESCRIPTOR pfd = {
		sizeof(PIXELFORMATDESCRIPTOR),    // size of this pfd
		1,                                // version number
		PFD_DRAW_TO_WINDOW |              // support window
		PFD_SUPPORT_OPENGL |              // support OpenGL
		PFD_DOUBLEBUFFER,                 // double buffered
		PFD_TYPE_RGBA,                    // RGBA type
		32,								  // color depth
		0, 0, 0, 0, 0, 0,                 // color bits ignored
		0,                                // no alpha buffer
		0,                                // shift bit ignored
		0,                                // no accumulation buffer
		0, 0, 0, 0,                       // accum bits ignored
		32,								  // z-buffer
		0,                                // no stencil buffer
		0,                                // no auxiliary buffer
		PFD_MAIN_PLANE,                   // main layer
		0,                                // reserved
		0, 0, 0                           // layer masks ignored
	};

	if (hWnd == NULL)
		hWnd = GetActiveWindow();

#ifdef WITH_IMGUI
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(hWnd);
#endif

	if ((hDC = GetDC(hWnd)) == NULL) {
		MessageBoxW(hWnd, L"Error while getting a device context!", pluginNameW, MB_ICONERROR | MB_OK);
		return false;
	}

	if ((pixelFormat = ChoosePixelFormat(hDC, &pfd)) == 0) {
		MessageBoxW(hWnd, L"Unable to find a suitable pixel format!", pluginNameW, MB_ICONERROR | MB_OK);
		stop();
		return false;
	}

	if ((SetPixelFormat(hDC, pixelFormat, &pfd)) == FALSE) {
        auto err = GetLastError();
        auto currentFormat = GetPixelFormat(hDC);
		//MessageBoxW(hWnd, L"Error while setting pixel format!", pluginNameW, MB_ICONERROR | MB_OK);
	}

	if ((hRC = wglCreateContext(hDC)) == NULL) {
		MessageBoxW(hWnd, L"Error while creating OpenGL context!", pluginNameW, MB_ICONERROR | MB_OK);
		stop();
		return false;
	}

	if ((wglMakeCurrent(hDC, hRC)) == FALSE) {
		MessageBoxW(hWnd, L"Error while making OpenGL context current!", pluginNameW, MB_ICONERROR | MB_OK);
		stop();
		return false;
	}

	initGLFunctions();

#ifdef WITH_IMGUI
	const char* glsl_version = "#version 130";
	ImGui_ImplOpenGL3_Init(glsl_version);
#endif

	PFNWGLGETEXTENSIONSSTRINGARBPROC wglGetExtensionsStringARB =
		(PFNWGLGETEXTENSIONSSTRINGARBPROC)wglGetProcAddress("wglGetExtensionsStringARB");

	if (wglGetExtensionsStringARB != NULL) {
		const char * wglextensions = wglGetExtensionsStringARB(hDC);

		if (strstr(wglextensions, "WGL_ARB_create_context_profile") != nullptr) {
			PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB =
				(PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");

			GLint majorVersion = 0;
			ptrGetIntegerv(GL_MAJOR_VERSION, &majorVersion);
			GLint minorVersion = 0;
			ptrGetIntegerv(GL_MINOR_VERSION, &minorVersion);

			const int attribList[] =
			{
				WGL_CONTEXT_MAJOR_VERSION_ARB, majorVersion,
				WGL_CONTEXT_MINOR_VERSION_ARB, minorVersion,
#ifdef FORCE_UNBUFFERED_DRAWER
				WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
#else
				WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
#endif
				0        //End
			};

			HGLRC coreHrc = wglCreateContextAttribsARB(hDC, 0, attribList);
			if (coreHrc != NULL) {
				wglDeleteContext(hRC);
				wglMakeCurrent(hDC, coreHrc);
				hRC = coreHrc;
			}
		}

		if (strstr(wglextensions, "WGL_EXT_swap_control") != nullptr) {
			PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");

			// use adaptive vsync when supported and
			// when vsync is enabled
			if (strstr(wglextensions, "WGL_EXT_swap_control_tear") != nullptr &&
				config.video.verticalSync > 0) {
				wglSwapIntervalEXT(-1);
			} else {
				wglSwapIntervalEXT(config.video.verticalSync);
			}
		}
	}

	return true;
}

void WindowsWGL::stop()
{
#ifdef WITH_IMGUI
	ImGui_ImplWin32_Shutdown();
	ImGui_ImplOpenGL3_Shutdown();
	ImGui::DestroyContext();
#endif

	wglMakeCurrent(NULL, NULL);

	if (hRC != NULL) {
		wglDeleteContext(hRC);
		hRC = NULL;
	}

	if (hDC != NULL) {
		ReleaseDC(hWnd, hDC);
		hDC = NULL;
	}
}

#ifdef WITH_IMGUI
using ImguiCommandList = std::vector< std::function<void(void)>>;
ImguiCommandList gImguiCommandList;

void EnqueueImguiCommand(std::function<void(void)> newCommand)
{
	gImguiCommandList.push_back(newCommand);
}
#endif

void WindowsWGL::swapBuffers()
{
#ifdef WITH_IMGUI
	ImGui_ImplWin32_NewFrame();
	ImGui_ImplOpenGL3_NewFrame();
	ImGui::NewFrame();

	for (auto& command : gImguiCommandList)
	{
		std::invoke(command);
	}
	gImguiCommandList.clear();

	// Rendering
	ImGui::Render();
	const ImGuiIO& io = ImGui::GetIO();
	glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);

	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#endif

	if (hDC == NULL)
		SwapBuffers(wglGetCurrentDC());
	else
		SwapBuffers(hDC);

}