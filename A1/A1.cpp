#include "A1.hpp"
#include "cs488-framework/GlErrorCheck.hpp"

#include <iostream>
#include <vector>

#include <imgui/imgui.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace glm;
using namespace std;

static const size_t DIM = 16;

const static int COORDS_PER_VERT = 3;
const static int VERTS_PER_FACE = 6;
const static int COORDS_PER_FACE = VERTS_PER_FACE * COORDS_PER_VERT;

const static int FACES_PER_BAR = 6;
const static int VERTS_PER_BAR = VERTS_PER_FACE * FACES_PER_BAR;
const static int COORDS_PER_BAR = COORDS_PER_FACE * FACES_PER_BAR;

const static int BARS_ON_GRID = DIM * DIM;
const static int COORDS_ON_GRID = BARS_ON_GRID * COORDS_PER_BAR;

namespace
{
	enum class Face
	{
		BOTTOM,
		TOP,
		LEFT,
		RIGHT,
		BACK,
		FRONT,
	};

	//----------------------------------------------------------------------------------------
	/*
	* Overwrites COORDS_PER_FACE consecutive values in arr starting at index offset
	* with the coordinates composing the specified face of a bar in the grid
	* Offset will be at the index after the last value written
	*/
	void getGridFaceCoordinates(Face face, int row, int col, int height, float* arr, int& offset)
	{
		const static float COORDS[] = {
			0.0f, 0.0f, 0.0f, // 0: left  bottom back
			0.0f, 0.0f, 1.0f, // 1: left  bottom front
			0.0f, 1.0f, 0.0f, // 2: left  top    back
			0.0f, 1.0f, 1.0f, // 3: left  top    front
			1.0f, 0.0f, 0.0f, // 4: right bottom back
			1.0f, 0.0f, 1.0f, // 5: right bottom front
			1.0f, 1.0f, 0.0f, // 6: right top    back
			1.0f, 1.0f, 1.0f  // 7: right top    front
		};
		const static int BOTTOM_INDICES[] = {
			0, 4, 1, 1, 5, 4
		};
		const static int TOP_INDICES[] = {
			2, 3, 6, 6, 3, 7
		};
		const static int LEFT_INDICES[] = {
			0, 1, 2, 2, 1, 3
		};
		const static int RIGHT_INDICES[] = {
			4, 6, 5, 5, 6, 7
		};
		const static int BACK_INDICES[] = {
			0, 2, 4, 4, 2, 6
		};
		const static int FRONT_INDICES[] = {
			1, 5, 3, 3, 5, 7
		};
		const static int* FACE_INDICES[] = {
			BOTTOM_INDICES,
			TOP_INDICES,
			LEFT_INDICES,
			RIGHT_INDICES,
			BACK_INDICES,
			FRONT_INDICES
		};

		const int* indices = FACE_INDICES[static_cast<unsigned int>(face)];
		for (int i = 0; i < VERTS_PER_FACE; ++i) {
			int coordsIndex = indices[i] * COORDS_PER_VERT;

			arr[offset + 0] = COORDS[coordsIndex + 0] + col;
			arr[offset + 1] = COORDS[coordsIndex + 1] * height;
			arr[offset + 2] = COORDS[coordsIndex + 2] + row;
			offset += 3;
		}
	}

	//----------------------------------------------------------------------------------------
	/*
	* Overwrites COORDS_PER_BAR consecutive values in arr starting at index offset
	* with the coordinates composing the specified bar in the grid
	* Offset will be at the index after the last value written
	*/
	void getGridBarCoordinates(int row, int col, int height, float* arr, int& offset)
	{
		if (height == 0) {
			std::fill(arr + offset, arr + offset + COORDS_PER_BAR, 0);
			offset += COORDS_PER_BAR;
		}
		else {
			for (int i = 0; i < FACES_PER_BAR; ++i) {
				Face face = static_cast<Face>(i);
				getGridFaceCoordinates(face, row, col, height, arr, offset);
			}
		}
	}
}

//----------------------------------------------------------------------------------------
// Constructor
A1::A1()
	: m_gridSelectedRow(0)
	, m_gridSelectedCol(0)
	, m_grid(DIM)
	, m_barCoords(new float[COORDS_ON_GRID])
	, m_currentColour(0)
	, m_colours(8)
	, m_copyMode(false)
{
	float colours[] = {
		0, 0, 0,
		1.0f, 1.0f, 1.0f,
		0.5f, 0.5f, 0.5f,
		1.0f, 0, 0,
		0, 1.0f, 0,
		0, 0, 1.0f,
		0.75f, 0.75f, 0,
		0, 0.75f, 0.75f
	};
	for (int i = 0; i < m_colours.size(); ++i) {
		auto& colour = m_colours[i];
		colour[0] = colours[i * 3 + 0];
		colour[1] = colours[i * 3 + 1];
		colour[2] = colours[i * 3 + 2];
	}
}

//----------------------------------------------------------------------------------------
// Destructor
A1::~A1()
{
	delete[] m_barCoords;
}

//----------------------------------------------------------------------------------------
/*
 * Called once, at program start.
 */
void A1::init()
{
	// Set the background colour.
	glClearColor( 0.3f, 0.5f, 0.7f, 1.0f );

	// Build the shader
	m_shader.generateProgramObject();
	m_shader.attachVertexShader(
		getAssetFilePath( "VertexShader.vs" ).c_str() );
	m_shader.attachFragmentShader(
		getAssetFilePath( "FragmentShader.fs" ).c_str() );
	m_shader.link();

	// Set up the uniforms
	P_uni = m_shader.getUniformLocation( "P" );
	V_uni = m_shader.getUniformLocation( "V" );
	M_uni = m_shader.getUniformLocation( "M" );
	col_uni = m_shader.getUniformLocation( "colour" );

	initGrid();
	initBars();
	initHighlight();

	reset();

	// Set up initial view and projection matrices (need to do this here,
	// since it depends on the GLFW window being set up correctly).
	view = glm::lookAt( 
		glm::vec3( 0.0f, float(DIM)*2.0*M_SQRT1_2, float(DIM)*2.0*M_SQRT1_2 ),
		glm::vec3( 0.0f, 0.0f, 0.0f ),
		glm::vec3( 0.0f, 1.0f, 0.0f ) );

	proj = glm::perspective( 
		glm::radians( 45.0f ),
		float( m_framebufferWidth ) / float( m_framebufferHeight ),
		1.0f, 1000.0f );
}

void A1::initGrid()
{
	const size_t sz = 3 * 2 * 2 * (DIM+3);

	float verts[sz];
	size_t ct = 0;
	for( int idx = 0; idx < DIM+3; ++idx ) {
		verts[ ct ] = -1.0f;
		verts[ ct+1 ] = 0;
		verts[ ct+2 ] = static_cast<float>(idx-1);
		verts[ ct+3 ] = static_cast<float>(DIM+1);
		verts[ ct+4 ] = 0;
		verts[ ct+5 ] = static_cast<float>(idx-1);
		ct += 6;

		verts[ ct ] = static_cast<float>(idx-1);
		verts[ ct+1 ] = 0;
		verts[ ct+2 ] = -1.0f;
		verts[ ct+3 ] = static_cast<float>(idx-1);
		verts[ ct+4 ] = 0;
		verts[ ct+5 ] = static_cast<float>(DIM+1);
		ct += 6;
	}

	// Create the vertex array to record buffer assignments.
	glGenVertexArrays( 1, &m_grid_vao );
	glBindVertexArray( m_grid_vao );

	// Create the bar vertex buffer
	glGenBuffers( 1, &m_grid_vbo );
	glBindBuffer( GL_ARRAY_BUFFER, m_grid_vbo );
	glBufferData( GL_ARRAY_BUFFER, sz*sizeof(float),
		verts, GL_STATIC_DRAW );

	// Specify the means of extracting the position values properly.
	GLint posAttrib = m_shader.getAttribLocation( "position" );
	glEnableVertexAttribArray( posAttrib );
	glVertexAttribPointer( posAttrib, 3, GL_FLOAT, GL_FALSE, 0, nullptr );

	// Reset state to prevent rogue code from messing with *my* 
	// stuff!
	glBindVertexArray( 0 );
	glBindBuffer( GL_ARRAY_BUFFER, 0 );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );

	CHECK_GL_ERRORS;
}


void A1::initBars()
{
	// Create the vertex array to record buffer assignments.
	glGenVertexArrays(1, &m_bar_vao);
	glBindVertexArray(m_bar_vao);

	// Create the bar vertex buffer
	glGenBuffers(1, &m_bar_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, m_bar_vbo);
	glBufferData(GL_ARRAY_BUFFER, COORDS_ON_GRID * sizeof(float), m_barCoords, GL_DYNAMIC_DRAW);

	// Specify the means of extracting the position values properly.
	GLint posAttrib = m_shader.getAttribLocation("position");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	// Reset state to prevent rogue code from messing with *my*
	// stuff!
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	CHECK_GL_ERRORS;
}

void A1::initHighlight()
{
	// Create the vertex array to record buffer assignments.
	glGenVertexArrays(1, &m_highlight_vao);
	glBindVertexArray(m_highlight_vao);

	// Create the highlight vertex buffer
	glGenBuffers(1, &m_highlight_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, m_highlight_vbo);

	// Specify the means of extracting the position values properly.
	GLint posAttrib = m_shader.getAttribLocation("position");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	setGridHighlightPosition(m_gridSelectedRow, m_gridSelectedCol);

	// Reset state to prevent rogue code from messing with *my*
	// stuff!
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	CHECK_GL_ERRORS;
}

void A1::reset()
{
	m_grid.reset();

	setSelectedPosition(0, 0);

	std::fill(m_barCoords, m_barCoords + COORDS_ON_GRID, 0);

	glBindBuffer(GL_ARRAY_BUFFER, m_bar_vbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, COORDS_ON_GRID * sizeof(float), m_barCoords);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void A1::moveSelectedPosition(int deltaRow, int deltaCol)
{
	int row = m_gridSelectedRow + deltaRow;
	int col = m_gridSelectedCol + deltaCol;

	if (row < 0 || row >= DIM || col < 0 || col >= DIM) {
		return;
	}

	int previousHeight = m_grid.getHeight(m_gridSelectedRow, m_gridSelectedCol);

	setSelectedPosition(row, col);

	if (m_copyMode) {
		setHeight(row, col, previousHeight);
	}
}

void A1::setSelectedPosition(int row, int col)
{
	if (row < 0 || row >= DIM || col < 0 || col >= DIM) {
		return;
	}

	m_gridSelectedRow = row;
	m_gridSelectedCol = col;

	setGridHighlightPosition(row, col);
}

void A1::setHeight(int row, int col, int height)
{
	if (height < 0) {
		return;
	}

	m_grid.setHeight(row, col, height);
	m_gridSelectedRow = row;
	m_gridSelectedCol = col;

	int startOffset = COORDS_PER_BAR * (DIM * row + col);
	int offset = startOffset;
	getGridBarCoordinates(row, col, height, m_barCoords, offset);
	glBindBuffer(GL_ARRAY_BUFFER, m_bar_vbo);
	glBufferSubData(GL_ARRAY_BUFFER, startOffset * sizeof(float), COORDS_PER_BAR * sizeof(float), m_barCoords + startOffset);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void A1::setGridHighlightPosition(int row, int col)
{
	const size_t NUM_COORDS = 4 * COORDS_PER_FACE;

	float verts[NUM_COORDS];
	int offset = 0;
	getGridFaceCoordinates(Face::BOTTOM, row, -1,  0, verts, offset);
	getGridFaceCoordinates(Face::BOTTOM, row, DIM, 0, verts, offset);
	getGridFaceCoordinates(Face::BOTTOM, -1,  col, 0, verts, offset);
	getGridFaceCoordinates(Face::BOTTOM, DIM, col, 0, verts, offset);

	glBindBuffer(GL_ARRAY_BUFFER, m_highlight_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, before guiLogic().
 */
void A1::appLogic()
{
	// Place per frame, application logic here ...
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, after appLogic(), but before the draw() method.
 */
void A1::guiLogic()
{
	// We already know there's only going to be one window, so for 
	// simplicity we'll store button states in static local variables.
	// If there was ever a possibility of having multiple instances of
	// A1 running simultaneously, this would break; you'd want to make
	// this into instance fields of A1.
	static bool showTestWindow(false);
	static bool showDebugWindow(true);

	ImGuiWindowFlags windowFlags(ImGuiWindowFlags_AlwaysAutoResize);
	float opacity(0.5f);

	ImGui::Begin("Debug Window", &showDebugWindow, ImVec2(100,100), opacity, windowFlags);
		if( ImGui::Button( "Quit Application" ) ) {
			glfwSetWindowShouldClose(m_window, GL_TRUE);
		}

		// Eventually you'll create multiple colour widgets with
		// radio buttons.  If you use PushID/PopID to give them all
		// unique IDs, then ImGui will be able to keep them separate.
		// This is unnecessary with a single colour selector and
		// radio button, but I'm leaving it in as an example.

		// Prefixing a widget name with "##" keeps it from being
		// displayed.

		for (int i = 0; i < m_colours.size(); ++i) {
			ImGui::PushID(i);
			ImGui::ColorEdit3("##Colour", m_colours[i].data());
			ImGui::SameLine();
			if (ImGui::RadioButton("##Col", &m_currentColour, i)) {
				// Select this colour.
			}
			ImGui::PopID();
		}

/*
		// For convenience, you can uncomment this to show ImGui's massive
		// demonstration window right in your application.  Very handy for
		// browsing around to get the widget you want.  Then look in 
		// shared/imgui/imgui_demo.cpp to see how it's done.
		if( ImGui::Button( "Test Window" ) ) {
			showTestWindow = !showTestWindow;
		}
*/

		ImGui::Text( "Framerate: %.1f FPS", ImGui::GetIO().Framerate );

	ImGui::End();

	if( showTestWindow ) {
		ImGui::ShowTestWindow( &showTestWindow );
	}
}

//----------------------------------------------------------------------------------------
/*
 * Called once per frame, after guiLogic().
 */
void A1::draw()
{
	// Create a global transformation for the model (centre it).
	mat4 W;
	W = glm::translate( W, vec3( -float(DIM)/2.0f, 0, -float(DIM)/2.0f ) );

	m_shader.enable();
		glEnable( GL_DEPTH_TEST );

		glUniformMatrix4fv( P_uni, 1, GL_FALSE, value_ptr( proj ) );
		glUniformMatrix4fv( V_uni, 1, GL_FALSE, value_ptr( view ) );
		glUniformMatrix4fv( M_uni, 1, GL_FALSE, value_ptr( W ) );

		// Just draw the grid for now.
		glBindVertexArray(m_grid_vao);
		glUniform3f(col_uni, 1, 1, 1);
		glDrawArrays(GL_LINES, 0, (3 + DIM) * 4);

		// Draw the bars
		glBindVertexArray(m_bar_vao);
		glUniform3f(col_uni, 1, 1, 1);
		glDrawArrays(GL_TRIANGLES, 0, VERTS_PER_BAR * BARS_ON_GRID);

		// Highlight the active square.
		glBindVertexArray(m_highlight_vao);
		glUniform3f(col_uni, 1, 1, 1);
		glDrawArrays(GL_TRIANGLES, 0, 4 * 2 * 3);
	m_shader.disable();

	// Restore defaults
	glBindVertexArray( 0 );

	CHECK_GL_ERRORS;
}

//----------------------------------------------------------------------------------------
/*
 * Called once, after program is signaled to terminate.
 */
void A1::cleanup()
{}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles cursor entering the window area events.
 */
bool A1::cursorEnterWindowEvent (
		int entered
) {
	bool eventHandled(false);

	// Fill in with event handling code...

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles mouse cursor movement events.
 */
bool A1::mouseMoveEvent(double xPos, double yPos) 
{
	bool eventHandled(false);

	if (!ImGui::IsMouseHoveringAnyWindow()) {
		// Put some code here to handle rotations.  Probably need to
		// check whether we're *dragging*, not just moving the mouse.
		// Probably need some instance variables to track the current
		// rotation amount, and maybe the previous X position (so 
		// that you can rotate relative to the *change* in X.
	}

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles mouse button events.
 */
bool A1::mouseButtonInputEvent(int button, int actions, int mods) {
	bool eventHandled(false);

	if (!ImGui::IsMouseHoveringAnyWindow()) {
		// The user clicked in the window.  If it's the left
		// mouse button, initiate a rotation.
	}

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles mouse scroll wheel events.
 */
bool A1::mouseScrollEvent(double xOffSet, double yOffSet) {
	bool eventHandled(false);

	// Zoom in or out.

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles window resize events.
 */
bool A1::windowResizeEvent(int width, int height) {
	bool eventHandled(false);

	// Fill in with event handling code...

	return eventHandled;
}

//----------------------------------------------------------------------------------------
/*
 * Event handler.  Handles key input events.
 */
bool A1::keyInputEvent(int key, int action, int mods) {
	bool eventHandled(false);

	// Fill in with event handling code...
	if( action == GLFW_PRESS ) {
		if (key == GLFW_KEY_UP) {
			moveSelectedPosition(-1, 0);
			eventHandled = true;
		}
		if (key == GLFW_KEY_DOWN) {
			moveSelectedPosition(1, 0);
			eventHandled = true;
		}
		if (key == GLFW_KEY_LEFT) {
			moveSelectedPosition(0, -1);
			eventHandled = true;
		}
		if (key == GLFW_KEY_RIGHT) {
			moveSelectedPosition(0, 1);
			eventHandled = true;
		}
		if (key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT) {
			m_copyMode = true;
			eventHandled = true;
		}
		if (key == GLFW_KEY_SPACE) {
			int height = m_grid.getHeight(m_gridSelectedRow, m_gridSelectedCol);
			setHeight(m_gridSelectedRow, m_gridSelectedCol, height + 1);
			eventHandled = true;
		}
		if (key == GLFW_KEY_BACKSPACE) {
			int height = m_grid.getHeight(m_gridSelectedRow, m_gridSelectedCol);
			setHeight(m_gridSelectedRow, m_gridSelectedCol, height - 1);
			eventHandled = true;
		}
		if (key == GLFW_KEY_R) {
			reset();
			eventHandled = true;
		}
		if (key == GLFW_KEY_1) {
			int offset = 0;
			getGridBarCoordinates(2, 2, 0, m_barCoords, offset);
			glBindBuffer(GL_ARRAY_BUFFER, m_bar_vbo);
			glBufferSubData(GL_ARRAY_BUFFER, 0, offset * sizeof(float), m_barCoords);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			eventHandled = true;
		}
	}
	else if (action == GLFW_RELEASE) {
		if (key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT) {
			m_copyMode = false;
			eventHandled = true;
		}
	}

	return eventHandled;
}
