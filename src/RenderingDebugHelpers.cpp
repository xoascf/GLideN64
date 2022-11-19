#include "RenderingDebugHelpers.h"
#include <imgui_commands.h>

#ifdef WITH_IMGUI
void RenderDebugHelpers::DrawToggleCheckboxes()
{
	OOT_Imgui_Commands::EnqueueCheckbox("Map memory", &RenderingToggles::GetMapMemory());
	OOT_Imgui_Commands::EnqueueCheckbox("Draw Triangles", &RenderingToggles::GetDrawTriangles());
	OOT_Imgui_Commands::EnqueueCheckbox("Update States", &RenderingToggles::GetUpdateStates());
	OOT_Imgui_Commands::EnqueueCheckbox("Run RSP", &RenderingToggles::GetRunRSP());
}
#endif // WITH_IMGUI