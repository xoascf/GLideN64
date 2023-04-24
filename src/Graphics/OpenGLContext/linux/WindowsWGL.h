#pragma once
#include <stdio.h>

//#if defined(OS_ANDROID)
//#include <GLES3/gl3.h>
//#include <GLES3/gl3ext.h>
//#else
//#include <GL/gl.h>
//#endif

class WindowsWGL
{
public:
	static bool start();
	static void stop();
	static void swapBuffers();

private:
};

