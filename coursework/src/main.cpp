// 2017 Gordon Swan
// OpenGL Coursework Version 1

#include <glm\glm.hpp>
#include <graphics_framework.h>

using namespace std;
using namespace graphics_framework;
using namespace glm;

mesh groundPlane;
mesh stdPyramid;
effect eff;

free_camera cam;
double cursor_x;
double cursor_y;

bool initialise() {
	// Capture cursor input
	glfwSetInputMode(renderer::get_window(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	// Capture initial cursor position
	glfwGetCursorPos(renderer::get_window(), &cursor_x, &cursor_y);
	return true;
}

bool load_content() {
	// Surface plane data
	geometry sPlane;
	vector<vec3> sPlanePos{
		vec3(-10.0f, 0.0f, -10.0f), vec3(-10.0f, 0.0f, 10.0f), vec3(10.0f, 0.0f, 10.0f),
		vec3(-10.0f, 0.0f, -10.0f), vec3(10.0f, 0.0f, 10.0f), vec3(10.0f, 0.0f, -10.0f)
	};
  
	// Add surface plane to the geometry
	sPlane.add_buffer(sPlanePos, BUFFER_INDEXES::POSITION_BUFFER);

	// Add surface plane geometry to mesh
	groundPlane = mesh(sPlane);

	// Pyramid data
	geometry sPyramid;
	vector<vec3> sPyramidPos{
		// Top (pointy bit)
		vec3(0.0f, 2.0f, 0.0f), vec3(-1.0f, 0.0f, 1.0f), vec3(1.0f, 0.0f, 1.0f),
		vec3(0.0f, 2.0f, 0.0f), vec3(1.0f, 0.0f, 1.0f), vec3(1.0f, 0.0f, -1.0f),
		vec3(0.0f, 2.0f, 0.0f), vec3(1.0f, 0.0f, -1.0f), vec3(-1.0f, 0.0f, -1.0f),
		vec3(0.0f, 2.0f, 0.0f), vec3(-1.0f, 0.0f, -1.0f), vec3(-1.0f, 0.0f, 1.0f)
	};
	
	// Add pyramid to geometry
	sPyramid.add_buffer(sPyramidPos, BUFFER_INDEXES::POSITION_BUFFER);

	// Add pyramid geometry
	stdPyramid = mesh(sPyramid);

	// Load in shaders
	eff.add_shader("shaders/basic_colour.vert", GL_VERTEX_SHADER);
	eff.add_shader("shaders/basic_colour.frag", GL_FRAGMENT_SHADER);

	// Build effect
	eff.build();

	// Set camera properties
	cam.set_position(vec3(0.0f, 5.0f, 10.0f));
	cam.set_target(vec3(0.0f, 0.0f, 0.0f));
	cam.set_projection(quarter_pi<float>(), renderer::get_screen_aspect(), 0.1f, 1000.0f);
	return true;
}


bool update(float delta_time) {
	// The ratio of pixels to rotation - remember the FOV
	static double ratio_width = quarter_pi<float>() / static_cast<float>(renderer::get_screen_width());
	static double ratio_height =
		(quarter_pi<float>() *
		(static_cast<float>(renderer::get_screen_height()) / static_cast<float>(renderer::get_screen_width()))) /
		static_cast<float>(renderer::get_screen_height());

	double current_x;
	double current_y;

	// Get the current cursor position
	glfwGetCursorPos(renderer::get_window(), &current_x, &current_y);
	// Calculate delta of cursor positions from last frame
	double delta_x = current_x - cursor_x;
	double delta_y = cursor_y - current_y;

	delta_x = delta_x * ratio_width;
	delta_y = delta_y * ratio_height;

	cam.rotate(delta_x, delta_y);
	// Use keyboard to move the camera - WSAD
	if (glfwGetKey(renderer::get_window(), GLFW_KEY_W)) {
		cam.move(vec3(0.0f, 0.0f, 0.5f));
	}
	if (glfwGetKey(renderer::get_window(), GLFW_KEY_A)) {
		cam.move(vec3(-0.5f, 0.0f, 0.0f));
	}
	if (glfwGetKey(renderer::get_window(), GLFW_KEY_S)) {
		cam.move(vec3(0.0f, 0.0f, -0.5));
	}
	if (glfwGetKey(renderer::get_window(), GLFW_KEY_D)) {
		cam.move(vec3(0.5f, 0.0f, 0.0f));
	}
	// Update the camera
	cam.update(delta_time);
	// Update cursor pos
	cursor_x = current_x;
	cursor_y = current_y;

	// Update cursor pos
	cursor_x = current_x;
	cursor_y = current_y;

	return true;
}

bool render() {
  // Bind effect
  renderer::bind(eff);
  // Create MVP matrix
  mat4 M(1.0f);
  auto V = cam.get_view();
  auto P = cam.get_projection();
  auto MVP = P * V * M;
  // Set MVP matrix uniform
  glUniformMatrix4fv(eff.get_uniform_location("MVP"), 1, GL_FALSE, value_ptr(MVP));

  // Set the colour value for the shader
  glUniform4fv(eff.get_uniform_location("colour"), 1, value_ptr(vec4(0.765f, 0.082f, 0.196f, 1.0f)));

  // Render geometry
  renderer::render(groundPlane);
  glUniform4fv(eff.get_uniform_location("colour"), 1, value_ptr(vec4(0.5f, 0.3f, 1.0f, 1.0f)));
  renderer::render(stdPyramid);
  return true;
}

void main() {
  // Create application
  app application("Graphics Coursework");
  // Set load content, update and render methods
  application.set_initialise(initialise);
  application.set_load_content(load_content);
  application.set_update(update);
  application.set_render(render);
  // Run application
  application.run();
}