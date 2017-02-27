// 2017 Gordon Swan
// OpenGL Coursework Version 1

#include <glm\glm.hpp>
#include <graphics_framework.h>

using namespace std;
using namespace graphics_framework;
using namespace glm;

map<string, mesh> meshes;
mesh groundPlane;
mesh stdPyramid;
mesh skybox;

effect sky_eff;
effect light_eff;

directional_light light;

cubemap cube_map;
texture tex;

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
	light.set_ambient_intensity(vec4(0.3f, 0.3f, 0.3f, 1.0f));
	light.set_light_colour(vec4(1.0f, 1.0f, 1.0f, 1.0f));
	light.set_direction(vec3(1.0f, 1.0f, -1.0f));

	light_eff.add_shader("shaders/gouraud.vert", GL_VERTEX_SHADER);
	light_eff.add_shader("shaders/gouraud.frag", GL_FRAGMENT_SHADER);
	light_eff.build();

	// *********************** SKYBOX LOAD ***********************
	// Create box geometry for skybox
	skybox = mesh(geometry_builder::create_box(vec3(10.0f, 10.0f, 10.0f)));
	// Scale box by 100
	skybox.get_transform().scale = vec3(100.0f, 100.0f, 100.0f);
	// Load the cubemap
	array<string, 6> filenames = { "textures/cwd_bk.jpg", "textures/cwd_bk.jpg", "textures/cwd_up.jpg",
		"textures/cwd_dn.jpg", "textures/cwd_rt.jpg", "textures/cwd_lf.jpg" };
	// Create cube_map
	cube_map = cubemap(filenames);
	//Load skybox shaders
	sky_eff.add_shader("shaders/skybox.vert", GL_VERTEX_SHADER);
	sky_eff.add_shader("shaders/skybox.frag", GL_FRAGMENT_SHADER);
	// Build Effect
	sky_eff.build();

	// *********************** OBJECTS LOAD **********************
	// Create Scene:
	meshes["plane"] = mesh(geometry_builder::create_plane(10.0f, 10.0f));
	meshes["pyramid"] = mesh(geometry_builder::create_pyramid(vec3(5.0f, 5.0f, 5.0f)));

	// Set Material Information
	meshes["plane"].get_material().set_emissive(vec4(0.0f, 0.0f, 0.0f, 1.0f));
	meshes["plane"].get_material().set_diffuse(vec4(1.0f, 1.0f, 1.0f, 1.0f));
	meshes["plane"].get_material().set_specular(vec4(1.0f, 1.0f, 1.0f, 1.0f));
	meshes["plane"].get_material().set_shininess(25.0f);

	meshes["pyramid"].get_material().set_emissive(vec4(0.0f, 0.0f, 0.0f, 1.0f));
	meshes["pyramid"].get_material().set_diffuse(vec4(1.0f, 1.0f, 1.0f, 1.0f));
	meshes["pyramid"].get_material().set_specular(vec4(1.0f, 1.0f, 1.0f, 1.0f));
	meshes["pyramid"].get_material().set_shininess(100.0f);


	tex = texture("textures/check_1.png");

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
	for (auto &index : meshes) {
		auto cur_mesh = index.second;
		// Bind Lighting Effect
		renderer::bind(light_eff);
		// Calculate MVP Matrix
		auto M = cur_mesh.get_transform().get_transform_matrix();
		auto V = cam.get_view();
		auto P = cam.get_projection();
		auto MVP = P * V * M;
		// Set MVP Matrix Uniform
		glUniformMatrix4fv(light_eff.get_uniform_location("MVP"), 1, GL_FALSE, value_ptr(MVP));
		// Set Normal and Model Matrices:
		glUniformMatrix4fv(light_eff.get_uniform_location("M"), 1, GL_FALSE, value_ptr(M));
		glUniformMatrix3fv(light_eff.get_uniform_location("N"), 1, GL_FALSE, value_ptr(cur_mesh.get_transform().get_normal_matrix()));
		// Bind Material
		renderer::bind(cur_mesh.get_material(), "mat");
		// Bind lighting model
		renderer::bind(light, "light");
		// Bind Texture
		renderer::bind(tex, 0);
		// Set texture uniform
		glUniform1i(light_eff.get_uniform_location("tex"), 0);
		// Set eye position uniform
		glUniform3fv(light_eff.get_uniform_location("eye_pos"), 1, value_ptr(cam.get_position()));
		// Render mesh
		renderer::render(cur_mesh);
	}

	
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