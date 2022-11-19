#ifdef WITH_IMGUI

#include "OOT_Imgui.h"
#include "backends/imgui_impl_opengl3.h"
#include "imgui_commands.h"
#include <vector>
#include <functional>
#include <Graphics/OpenGLContext/GLFunctions.h>
#include <RenderingDebugHelpers.h>

#ifdef _WIN32
#include "backends/imgui_impl_win32.h"
#endif

namespace OOT_Imgui
{
	static bool bImguiInitialized = false;

	using ImguiCommandList = std::vector< std::function<void(void)>>;
	static ImguiCommandList gImguiCommandList;
}

void OOT_Imgui::CreateImguiContext(void* window, void* ctx)
{
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGui::StyleColorsDark();

#ifdef _WIN32
	ImGui_ImplWin32_Init(window);
#else
	ImGui_ImplSDL2_InitForOpenGL((SDL_Window*)window, ctx);
#endif
}

void OOT_Imgui::InitOpenGL()
{
#ifdef _WIN32
	const char* glsl_version = "#version 130";
#else
	const char* glsl_version = "#version 100";
#endif
	ImGui_ImplOpenGL3_Init(glsl_version);

	OOT_Imgui::bImguiInitialized = true;
}

void OOT_Imgui::SwapBuffers()
{
	if (OOT_Imgui::bImguiInitialized) {
#ifdef _WIN32
		ImGui_ImplWin32_NewFrame();
#else
		ImGui_ImplSDL2_NewFrame();
#endif

		ImGui_ImplOpenGL3_NewFrame();
		ImGui::NewFrame();

		for (auto& command : OOT_Imgui::gImguiCommandList)
		{
			std::invoke(command);
		}
		OOT_Imgui::gImguiCommandList.clear();

		RenderDebugHelpers::DrawToggleCheckboxes();

		// Rendering
		ImGui::Render();
		const ImGuiIO& io = ImGui::GetIO();
		glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);

		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}
}

void OOT_Imgui_Commands::EnqueueImguiCommand(std::function<void(void)> newCommand)
{
	OOT_Imgui::gImguiCommandList.push_back(newCommand);
}

void OOT_Imgui::Shutdown()
{
	if (OOT_Imgui::bImguiInitialized) {
#ifdef _WIN32
		ImGui_ImplWin32_Shutdown();
#else
		ImGui_ImplSDL2_Shutdown();
#endif
		ImGui_ImplOpenGL3_Shutdown();
		ImGui::DestroyContext();
	}
}

#endif // WITH_IMGUI