#pragma once

#ifdef __APPLE__
	#include <OpenGL/gl3.h>
#elif defined __linux__
    #include <gl3w/GL/gl3w.h>
#elif defined WIN32
#if RENDER_DEBUG
	#define GLEW_STATIC
	#include <GL/glew.h>
#else
    #include <gl3w/GL/gl3w.h>
#endif
#endif
