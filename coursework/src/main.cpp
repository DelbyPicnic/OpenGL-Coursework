// 2017 Gordon Swan
// OpenGL Coursework Version 1

#include <glm\glm.hpp>
#include <graphics_framework.h>
#include "fftw3.h"
#include "SDL.h"
#undef main

using namespace std;
using namespace graphics_framework;
using namespace glm;

map<string, mesh> meshes;
map<string, mesh> pillars;
mesh groundPlane;
mesh stdPyramid;
mesh skybox;

effect sky_eff;
effect mask_eff;
effect light_eff;

point_light light;

cubemap cube_map;
texture screen_mask;
texture tex_01;
texture tex_02;

geometry mask_quad;
frame_buffer framebuffer;

// SDL & FFTW Stuff
struct AudioData {
	Uint8* filePosition;
	Uint32 fileLength;
};

Uint8* sampData;
SDL_AudioSpec wavSpec;
Uint8* wavStart;
Uint32 wavLength;
SDL_AudioDeviceID aDevice;

#define FILE_PATH "C:\\Users\\40202556\\Desktop\\OpenGL-Coursework\\coursework\\res\\audio\\testFile.wav"

float t_time = 0.0f;
float r = 0.0f;
float s = 0.0f;
float theta = 0.0f;

free_camera cam;
double cursor_x;
double cursor_y;

void PlayAudioCallback(void* userData, Uint8* stream, int streamLength) {
	AudioData* audio = (AudioData*)userData;
	sampData = new Uint8;

	if (audio->fileLength == 0) {
		return;
	}

	Uint32 length = (Uint32)streamLength;
	length = (length > audio->fileLength ? audio->fileLength : length);

	SDL_memcpy(stream, audio->filePosition, length);
	sampData = stream;

	audio->filePosition += length;
	audio->fileLength -= length;
}

bool initialise() {
	// Set screen size
	renderer::set_screen_dimensions(1280, 960);
	// Capture cursor input
	glfwSetInputMode(renderer::get_window(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	// Capture initial cursor position
	glfwGetCursorPos(renderer::get_window(), &cursor_x, &cursor_y);

	

	return true;
}

bool load_content() {
	// ********************** SCREEN MASK ************************
	framebuffer = frame_buffer(renderer::get_screen_width(), renderer::get_screen_height());
	vector<vec3> positions{vec3(-1.0f, 1.0f, 0.0f), vec3(-1.0f, -1.0f, 0.0f), vec3(1.0f, -1.0f, 0.0f), vec3(-1.0f, 1.0f, 0.0f), vec3(1.0f, -1.0f, 0.0f), vec3(1.0f, 1.0f, 0.0f)};
	vector<vec2> tex_coords{vec2(0.0f, 1.0f), vec2(0.0f, 0.0f), vec2(1.0f, 0.0f), vec2(0.0f, 1.0f), vec2(1.0f, 0.0f), vec2(1.0f, 1.0f)};
	mask_quad.add_buffer(positions, BUFFER_INDEXES::POSITION_BUFFER);
	mask_quad.add_buffer(tex_coords, BUFFER_INDEXES::TEXTURE_COORDS_0);
	screen_mask = texture("textures/ie_4.png");
	mask_eff.add_shader("shaders/simple_texture.vert", GL_VERTEX_SHADER);
	mask_eff.add_shader("shaders/mask.frag", GL_FRAGMENT_SHADER);
	mask_eff.build();

	// ********************** LIGHTING LOAD **********************
	light.set_position(vec3(0.0f, 10.0f, 0.0f));
	light.set_light_colour(vec4(1.0f, 1.0f, 1.0f, 1.0f));
	light.set_range(20.0f);

	light_eff.add_shader("shaders/point.vert", GL_VERTEX_SHADER);
	light_eff.add_shader("shaders/point.frag", GL_FRAGMENT_SHADER);
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
	meshes["plane"] = mesh(geometry_builder::create_plane(75.0f, 75.0f));
	meshes["pyramid"] = mesh(geometry_builder::create_pyramid(vec3(5.0f, 5.0f, 5.0f)));

	// Create VU collumns for visualizer
	meshes["cube01"] = mesh(geometry_builder::create_box(vec3(4.0f, 30.0f, 0.2f)));
	meshes["cube02"] = mesh(geometry_builder::create_box(vec3(4.0f, 30.0f, 0.2f)));
	meshes["cube03"] = mesh(geometry_builder::create_box(vec3(4.0f, 30.0f, 0.2f)));
	
	meshes["cube01"].get_transform().translate(vec3(-5.0f, 15.0f, -30.0f));
	meshes["cube02"].get_transform().translate(vec3(0.0f, 15.0f, -30.0f));
	meshes["cube03"].get_transform().translate(vec3(5.0f, 15.0f, -30.0f));
	
	meshes["cube01"].get_material().set_emissive(vec4(1.0f, 0.0f, 0.0f, 1.0f));
	meshes["cube01"].get_material().set_diffuse(vec4(1.0f, 1.0f, 1.0f, 1.0f));
	meshes["cube01"].get_material().set_specular(vec4(1.0f, 1.0f, 1.0f, 1.0f));
	meshes["cube01"].get_material().set_shininess(25.0f);

	meshes["cube02"].get_material().set_emissive(vec4(0.0f, 1.0f, 0.0f, 1.0f));
	meshes["cube02"].get_material().set_diffuse(vec4(1.0f, 1.0f, 1.0f, 1.0f));
	meshes["cube02"].get_material().set_specular(vec4(1.0f, 1.0f, 1.0f, 1.0f));
	meshes["cube02"].get_material().set_shininess(25.0f);

	meshes["cube03"].get_material().set_emissive(vec4(0.0f, 0.0f, 1.0f, 1.0f));
	meshes["cube03"].get_material().set_diffuse(vec4(1.0f, 1.0f, 1.0f, 1.0f));
	meshes["cube03"].get_material().set_specular(vec4(1.0f, 1.0f, 1.0f, 1.0f));
	meshes["cube03"].get_material().set_shininess(25.0f);

	// Set Pyramid & Tetrahedron Information
	meshes["tetra01"] = mesh(geometry_builder::create_tetrahedron(vec3(6.0f, 4.0f, 8.0f)));
	meshes["tetra02"] = mesh(geometry_builder::create_tetrahedron(vec3(2.0f, 7.0f, 4.0f)));
	meshes["tetra03"] = mesh(geometry_builder::create_tetrahedron(vec3(5.0f, 3.0f, 1.0f)));

	meshes["tetra01"].get_transform().translate(vec3(-5.0f, 2.0f, -22.0f));
	meshes["tetra02"].get_transform().translate(vec3(0.0f, 3.5f, -19.0f));
	meshes["tetra03"].get_transform().translate(vec3(5.0f, 1.5f, -26.0f));

	meshes["tetra01"].get_material().set_emissive(vec4(1.0f, 0.0f, 1.0f, 1.0f));
	meshes["tetra01"].get_material().set_diffuse(vec4(1.0f, 1.0f, 1.0f, 1.0f));
	meshes["tetra01"].get_material().set_specular(vec4(1.0f, 1.0f, 1.0f, 1.0f));
	meshes["tetra01"].get_material().set_shininess(25.0f);

	meshes["tetra02"].get_material().set_emissive(vec4(0.0f, 1.0f, 1.0f, 1.0f));
	meshes["tetra02"].get_material().set_diffuse(vec4(1.0f, 1.0f, 1.0f, 1.0f));
	meshes["tetra02"].get_material().set_specular(vec4(1.0f, 1.0f, 1.0f, 1.0f));
	meshes["tetra02"].get_material().set_shininess(25.0f);

	meshes["tetra03"].get_material().set_emissive(vec4(1.0f, 1.0f, 0.0f, 1.0f));
	meshes["tetra03"].get_material().set_diffuse(vec4(1.0f, 1.0f, 1.0f, 1.0f));
	meshes["tetra03"].get_material().set_specular(vec4(1.0f, 1.0f, 1.0f, 1.0f));
	meshes["tetra03"].get_material().set_shininess(25.0f);
	
	// Set Plane Information
	meshes["plane"].get_material().set_emissive(vec4(0.0f, 0.0f, 0.0f, 1.0f));
	meshes["plane"].get_material().set_diffuse(vec4(1.0f, 1.0f, 1.0f, 1.0f));
	meshes["plane"].get_material().set_specular(vec4(1.0f, 1.0f, 1.0f, 1.0f));
	meshes["plane"].get_material().set_shininess(25.0f);

	tex_01 = texture("textures/floor_tile_01.png");
	tex_02 = texture("textures/floor_tile_02.png");

	// *********************** CAMERA CONFIG **********************
	// Set camera properties
	cam.set_position(vec3(0.0f, 5.0f, 30.0f));
	cam.set_target(vec3(0.0f, 0.0f, 0.0f));
	cam.set_projection(quarter_pi<float>(), renderer::get_screen_aspect(), 0.1f, 1000.0f);
	return true;
}


bool update(float delta_time) {	
	// *********************** CAMERA CONTROL *********************
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

	// *********************** RENDER CONFIG ***********************
	renderer::set_render_target(framebuffer);
	renderer::clear();
	// This will render to the framebuffer until instructed otherwise.

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
		renderer::bind(light, "point");

		if (index.first.find("cube0") != std::string::npos) {
			renderer::bind(tex_02, 0);
		}
		else 
		{
			renderer::bind(tex_01, 0);
		}

		// Bind Texture
		
		// Set texture uniform
		glUniform1i(light_eff.get_uniform_location("tex"), 0);
		// Set eye position uniform
		glUniform3fv(light_eff.get_uniform_location("eye_pos"), 1, value_ptr(cam.get_position()));
		// Render mesh
		renderer::render(cur_mesh);
	}
	// *********************** RENDER CONFIG ***********************
	renderer::set_render_target();
	// This will direct the render target back to the window.

	// ********************* SCREEN MASK RENDER ******************** 
	renderer::bind(mask_eff);
	MVP = mat4(1);
	glUniformMatrix4fv(mask_eff.get_uniform_location("MVP"), 1, GL_FALSE, value_ptr(MVP));
	glUniform4fv(mask_eff.get_uniform_location("clear_colour"), 1, value_ptr(vec4(0.94f, 0.13f, 0.61f, 1.0f)));
	renderer::bind(framebuffer.get_frame(), 0);
	glUniform1i(mask_eff.get_uniform_location("tex"), 0);
	renderer::bind(screen_mask, 1);
	glUniform1i(mask_eff.get_uniform_location("alpha_map"), 1); 
	renderer::render(mask_quad);

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

	// ********************** FFTW INIT **************************
	int N;
	fftw_complex *in, *out;
	fftw_plan my_plan;	/*in = (fftw_complex*)fftwf_malloc(sizeof(fftw_complex)*N);
	out = (fftw_complex*)fftwf_malloc(sizeof(fftw_complex)*N);
	my_plan = fftw_plan_dft*/
	

	// *********************** SDL INIT **************************
	SDL_Init(SDL_INIT_AUDIO);
	cout << "SDL Audio Init Complete" << endl;
	if (SDL_LoadWAV(FILE_PATH, &wavSpec, &wavStart, &wavLength) == NULL) {
		cerr << "Couldnt load file: " << FILE_PATH << endl;
		getchar();		
	}
	cout << "Loaded " << FILE_PATH << endl;
	AudioData audio;
	audio.filePosition = wavStart;
	audio.fileLength = wavLength;

	wavSpec.callback = PlayAudioCallback;
	wavSpec.userdata = &audio;

	aDevice = SDL_OpenAudioDevice(NULL, 0, &wavSpec, NULL, SDL_AUDIO_ALLOW_ANY_CHANGE);
	cout << "Opening Audio Stream" << endl;
	if (aDevice == 0) {
		cerr << "Audio Device connection failed: " << SDL_GetError() << endl;
		getchar();		
	}
	SDL_PauseAudioDevice(aDevice, 0);

	// Run application
	application.run();
	
	

}

