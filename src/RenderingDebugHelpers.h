#ifndef RENDERING_TOGGLES_H
#define RENDERING_TOGGLES_H

#include "ultra64/types.h"

namespace RenderingToggles {
	bool& GetMapMemory(); 
	bool& GetDrawTriangles();
	bool& GetUpdateStates();
	bool& GetRunRSP();
}

namespace RenderTiming {
	u64 GetLastRenderMs();
}

namespace RenderDebugHelpers {
#ifdef WITH_IMGUI
	void DrawToggleCheckboxes();
#endif
}

#endif // RENDERING_TOGGLES_H
