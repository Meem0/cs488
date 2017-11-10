//#include <Windows.h>
//#include <stdio.h>
//int (WINAPIV * __vsnprintf)(char *, size_t, const char*, va_list) = _vsnprintf;

// Force GLFW to include gl3.h core functionality instead of gl.h deprecated code.
#define GLFW_INCLUDE_GLCOREARB
#include <GLFW/glfw3.h>

#include <gl3w/GL/gl3w.h>

#include <string>
#include <memory>
#include <sstream>
#include <iostream>
#include <cstdio>

using namespace std;

//----------------------------------------------------------------------------------------
/*
* Error callback to be registered with GLFW.
*/
void errorCallback(
	int error,
	const char * description
) {
	stringstream msg;
	msg << "GLFW Error Code: " << error << "\n" <<
		"GLFW Error Description: " << description << "\n";
	cout << msg.str();
}

void windowResizeCallBack(
	GLFWwindow * window,
	int width,
	int height
) {
}

void keyInputCallBack(
	GLFWwindow * window,
	int key,
	int scancode,
	int action,
	int mods
) {
}

void mouseScrollCallBack(
	GLFWwindow * window,
	double xOffSet,
	double yOffSet
) {
}

void mouseButtonCallBack(
	GLFWwindow * window,
	int button,
	int actions,
	int mods
) {
}

void mouseMoveCallBack(
	GLFWwindow * window,
	double xPos,
	double yPos
) {
}

void cursorEnterWindowCallBack(
	GLFWwindow * window,
	int entered
) {
}

//----------------------------------------------------------------------------------------
void centerWindow(GLFWwindow* window) {
	int windowWidth, windowHeight;
	glfwGetWindowSize(window, &windowWidth, &windowHeight);

	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	if (monitor == NULL) {
		return;
	}

	int x, y;
	const GLFWvidmode* videoMode = glfwGetVideoMode(monitor);
	x = videoMode->width;
	y = videoMode->height;

	x = (x - windowWidth) / 2;
	y = (y - windowHeight) / 2;

	glfwSetWindowPos(window, x, y);
}

//----------------------------------------------------------------------------------------
/*
* Register callbacks with GLFW, and associate events with the current GLFWwindow.
*/
void registerGlfwCallBacks(GLFWwindow* window) {
	glfwSetKeyCallback(window, keyInputCallBack);
	glfwSetWindowSizeCallback(window, windowResizeCallBack);
	glfwSetScrollCallback(window, mouseScrollCallBack);
	glfwSetMouseButtonCallback(window, mouseButtonCallBack);
	glfwSetCursorPosCallback(window, mouseMoveCallBack);
	glfwSetCursorEnterCallback(window, cursorEnterWindowCallBack);
}

//----------------------------------------------------------------------------------------
void init() {
	// Render only the front face of geometry.
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);

	// Setup depth testing
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LEQUAL);
	glDepthRange(0.0f, 1.0f);
	glEnable(GL_DEPTH_CLAMP);

	glClearDepth(1.0f);
	glClearColor(0.3, 0.5, 0.7, 1.0);

}

int main()
{
	string windowTitle = "TestWindow";
	size_t width = 1024;
	size_t height = 768;
	float fps = 60.0f;
	glfwSetErrorCallback(errorCallback);

	if (glfwInit() == GL_FALSE) {
		fprintf(stderr, "Call to glfwInit() failed.\n");
		std::abort();
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_VISIBLE, GL_TRUE);
	glfwWindowHint(GLFW_SAMPLES, 0);
	glfwWindowHint(GLFW_RED_BITS, 8);
	glfwWindowHint(GLFW_GREEN_BITS, 8);
	glfwWindowHint(GLFW_BLUE_BITS, 8);
	glfwWindowHint(GLFW_ALPHA_BITS, 8);

	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	if (monitor == NULL) {
		glfwTerminate();
		fprintf(stderr, "Error retrieving primary monitor.\n");
		std::abort();
	}

	GLFWwindow* window = glfwCreateWindow(width, height, windowTitle.c_str(), NULL, NULL);
	if (window == NULL) {
		glfwTerminate();
		fprintf(stderr, "Call to glfwCreateWindow failed.\n");
		std::abort();
	}

	int framebufferWidth, framebufferHeight;

	// Get default framebuffer dimensions in order to support high-definition
	// displays.
	glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);

	centerWindow(window);
	glfwMakeContextCurrent(window);
	gl3wInit();

	//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	registerGlfwCallBacks(window);

	// Clear error buffer.
	while (glGetError() != GL_NO_ERROR);

	try {
		// Wait until m_monitor refreshes before swapping front and back buffers.
		// To prevent tearing artifacts.
		glfwSwapInterval(1);

		// Call client-defined startup code.
		init();

		// steady_clock::time_point frameStartTime;

		// Main Program Loop:
		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();

			// Apply application-specific logic
			//appLogic();

			//guiLogic();

			// Ask the derived class to do the actual OpenGL drawing.
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			//draw();

			// In case of a window resize, get new framebuffer dimensions.
			glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);

			// Finally, blast everything to the screen.
			glfwSwapBuffers(window);
		}
	}
	catch (const  std::exception & e) {
		std::cerr << "Exception Thrown: ";
		std::cerr << e.what() << endl;
	}
	catch (...) {
		std::cerr << "Uncaught exception thrown!  Terminating Program." << endl;
	}

	//cleanup();
	glfwDestroyWindow(window);
}
