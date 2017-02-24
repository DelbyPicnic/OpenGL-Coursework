// 2017 Gordon Swan
// OpenGL Coursework Version 1

#include <glm\glm.hpp>
#include <graphics_framework.h>

using namespace std;
using namespace graphics_framework;
using namespace glm;

mesh groundPlane;
mesh stdPyramid;
mesh skybox;
effect eff;
effect sky_eff;
effect light_eff;

cubemap cube_map;

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
	// ********************** LIGHTING LOAD **********************
	light_eff.add_shader("shaders/env_light.vert", GL_VERTEX_SHADER);
	light_eff.add_shader("shaders/env_light.frag", GL_FRAGMENT_SHADER);
	light_eff.build();

	// *********************** SKYBOX LOAD ***********************
	// Create box geometry for skybox
	skybox = mesh(geometry_builder::create_box(vec3(10.0f, 10.0f, 10.0f)));
	// Scale box by 100
	skybox.get_transform().scale = vec3(10.0f, 10.0f, 10.0f);
	// Load the cubemap
	array<string, 6> filenames = { "textures/sahara_ft.jpg", "textures/sahara_bk.jpg", "textures/sahara_up.jpg",
		"textures/sahara_dn.jpg", "textures/sahara_rt.jpg", "textures/sahara_lf.jpg" };
	// Create cube_map
	cube_map = cubemap(filenames);
	//Load skybox shaders
	sky_eff.add_shader("shaders/skybox.vert", GL_VERTEX_SHADER);
	sky_eff.add_shader("shaders/skybox.frag", GL_FRAGMENT_SHADER);
	// Build Effect
	sky_eff.build();

	// *********************** OBJECTS LOAD **********************
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

	// *********************** CAMERA CONFIG **********************
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
	// *********************** SKYBOX RENDER ***********************
	// Disable Depth Testing, Face Culling and Depth Masking
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	// Bind skybox effect
	renderer::bind(sky_eff);
	// Calculate MVP for Skybox
	mat4 M = skybox.get_transform().get_transform_matrix();
	auto V = cam.get_view();
	auto P = cam.get_projection();
	auto MVP = P * V * M;
	// Set MVP Matrix Uniform
	glUniformMatrix4fv(sky_eff.get_uniform_location("MVP"), 1, GL_FALSE, value_ptr(MVP));
	//Set the CubeMap Uniform
	renderer::bind(cube_map, 0);
	glUniform1i(sky_eff.get_uniform_location("cubemap"), 0);
	// Render Skybox
	renderer::render(skybox);
	// Enable Depth Testing, Face Culling and Depth Masking
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);

	// ********************** OBJECTS RENDER ***********************
	// Bind effect
	renderer::bind(eff);
	// Create MVP matrix
	M = mat4(1.0f);
	V = cam.get_view();
	P = cam.get_projection();
	MVP = P * V * M;
	// Set MVP matrix uniform
	glUniformMatrix4fv(eff.get_uniform_location("MVP"), 1, GL_FALSE, value_ptr(MVP));

	// Set the colour value for the shader
	glUniform4fv(eff.get_uniform_location("colour"), 1, value_ptr(vec4(0.765f, 0.082f, 0.196f, 1.0f)));

	// Render geometry
	renderer::render(groundPlane);
	glUniform4fv(eff.get_uniform_location("colour"), 1, value_ptr(vec4(0.5f, 0.3f, 1.0f, 1.0f)));
	renderer::render(stdPyramid);

	// ********************* LIGHTING CONFIG **********************
	
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