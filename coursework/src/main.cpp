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
texture tex_sun;
texture tex_tree_l;
texture tex_tree_r;
texture tex_info;

geometry mask_quad;
frame_buffer framebuffer;

int enableColour = 1;

// SDL & FFTW Stuff
#define FILE_PATH "audio\\music.wav"
#define CHUNK_SIZE 1024
#define BIN_COUNT 70
#define DECAY_FACTOR 10
#define LOWEST_RANGE 20
#define HIGHEST_RANGE 5800

struct AudioData {
	Uint8* filePosition;
	Uint32 fileLength;
};

SDL_AudioSpec wavSpec;
Uint8* wavStart;
Uint32 wavLength;
SDL_AudioDeviceID aDevice;

Uint16 samples[CHUNK_SIZE];
Uint16 window[CHUNK_SIZE];
fftw_complex s_in[CHUNK_SIZE], s_out[CHUNK_SIZE];;
fftw_plan plan;
float fft_window[CHUNK_SIZE];

double freq_range[BIN_COUNT];
double freq_bin[BIN_COUNT];
double freq_plot[BIN_COUNT];

float t_time = 0.0f;
float r = 0.0f;
float s = 0.0f;
float theta = 0.0f;

free_camera cam;
double cursor_x;
double cursor_y;

void PlayAudioCallback(void* userData, Uint8* stream, int streamLength) {
	AudioData* audio = (AudioData*)userData;	

	if (audio->fileLength == 0) {
		return;
	}

	Uint32 length = (Uint32)streamLength;
	length = (length > audio->fileLength ? audio->fileLength : length);

	// Fill audio buffer with more samples
	SDL_memcpy(stream, audio->filePosition, length);
	// Fill FFT buffer with sampling chunk
	SDL_memcpy(samples, audio->filePosition, CHUNK_SIZE * sizeof(Uint16));

	double magnitude;

	// Fill Complex_FFT.REAL with sample
	for (int i = 0; i < CHUNK_SIZE; i++) {
		s_in[i][0] = samples[i];
		s_in[1][1] = 0;
	}

	fftw_execute(plan);

	// Calculate power spectrum
	for (int i = 1; i < CHUNK_SIZE / 2 - 1; i++) {
		double real = s_out[i][0];
		double imag = s_out[i][1];
		magnitude = sqrt((real*real) + (imag*imag));

		double freq = (i * 44100) / CHUNK_SIZE;

		for (int j = 0; j < BIN_COUNT; j++) {
			if ((freq > freq_range[j]) && (freq <= freq_range[j + 1])) {
				if (magnitude > freq_bin[j]) {
					freq_bin[j] = magnitude;
				}
			}
		}
	}

	audio->filePosition += length;
	audio->fileLength -= length;
}

// Windowing Functions:
// These window functions are my own interpretation of the default algorithms.
// I'm not 100% sure they are correct, with more time i could perfect the visualiser.
// With more time i could also visit the moon :/

// n is represented by a const 1.0, but to my understanding it should be relative,
// no idea where it comes from in this implementation tho. Bost functions work okayish so whatever...

// Hamming Window Function
void hamming(int windowLength, float *buffer) {
	for (int i = 0; i < windowLength; i++) {
		double a = 2 * M_PI * (i / ((windowLength - 1)));
		buffer[i] = 0.54 - (0.46 * cos(a * 1.0));
	}
}
// Blackman Harris Window Function
void blackman_harris(int windowLength, float *buffer) {
	for (int i = 0; i < windowLength; i++) {
		double a = 2 * M_PI * (i / ((windowLength - 1)));
		buffer[i] = 0.35875 - 0.48829*cos(a*1.0) + 0.14128*cos(2 * a*1.0) - 0.01168*cos(3 * a*1.0);
	}
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
	meshes["plane"] = mesh(geometry_builder::create_plane(300.0f, 300.0f));

	// Create Sun Element
	
	meshes["sun"] = mesh(geometry_builder::create_box(vec3(75.0f, 75.0f, 0.01f)));

	meshes["sun"].get_material().set_emissive(vec4(1.0f, 1.0f, 1.0f, 1.0f));
	meshes["sun"].get_material().set_diffuse(vec4(1.0f, 1.0f, 1.0f, 1.0f));
	meshes["sun"].get_material().set_specular(vec4(1.0f, 1.0f, 1.0f, 1.0f));
	meshes["sun"].get_material().set_shininess(25.0f);

	// Translate Sun into place
	meshes["sun"].get_transform().translate(vec3(0.0f, 19.0f, -110.0f));
	meshes["sun"].get_transform().rotate(vec3(0.0f, 0.0f, half_pi<float>() * 2));

	// Create Tree Element
	
	for (int i = 0; i < 10; i++) {
		string leftTree = "tree_left_" + to_string(i);
		string rightTree = "tree_right_" + to_string(i);
		meshes[leftTree] = mesh(geometry_builder::create_box(vec3(15.0f, 23.3f, 0.01f)));
		meshes[rightTree] = mesh(geometry_builder::create_box(vec3(15.0f, 23.0f, 0.01f)));

		meshes[leftTree].get_material().set_emissive(vec4(1.0f, 1.0f, 1.0f, 1.0f));
		meshes[leftTree].get_material().set_diffuse(vec4(1.0f, 1.0f, 1.0f, 1.0f)); 
		meshes[leftTree].get_material().set_specular(vec4(1.0f, 1.0f, 1.0f, 1.0f));
		meshes[leftTree].get_material().set_shininess(25.0f);

		meshes[rightTree].get_material().set_emissive(vec4(1.0f, 1.0f, 1.0f, 1.0f));
		meshes[rightTree].get_material().set_diffuse(vec4(1.0f, 1.0f, 1.0f, 1.0f));
		meshes[rightTree].get_material().set_specular(vec4(1.0f, 1.0f, 1.0f, 1.0f));
		meshes[rightTree].get_material().set_shininess(25.0f);

		meshes[leftTree].get_transform().translate(vec3(30.0+i*10, 7.5f, -100.0f + i*10));
		meshes[leftTree].get_transform().rotate(vec3(0.0f, M_PI/4, M_PI));

		meshes[rightTree].get_transform().translate(vec3(-30.0f-i*10, 7.5f, -100.0f + i*10));
		meshes[rightTree].get_transform().rotate(vec3(0.0f, -M_PI/4, M_PI));
	}

	// Create VU collumns for visualiser.
	float _barWidth = 1.5;
	float _barSpacing = 0.2;
	double _midPoint = (_barWidth*BIN_COUNT) + (_barSpacing*(BIN_COUNT - 1));

	for (int i = 0; i < BIN_COUNT; i++) {
		string meshName = "bar_" + to_string(i);
		float xPos = (i * _barWidth) + (i * _barSpacing) - _midPoint /2;
		// Create Mesh:
		meshes[meshName] = mesh(geometry_builder::create_box(vec3(_barWidth, 5.0f, 0.2f)));
		// Translate Mesh:
		meshes[meshName].get_transform().translate(vec3(xPos, 20.0f, -100.0f));
		// Set Mesh Material Properties
		meshes[meshName].get_material().set_emissive(vec4(1.0f, 0.2f, 1.0f, 1.0f));
		meshes[meshName].get_material().set_diffuse(vec4(1.0f, 1.0f, 1.0f, 1.0f));
		meshes[meshName].get_material().set_specular(vec4(1.0f, 1.0f, 1.0f, 1.0f));
		meshes[meshName].get_material().set_shininess(25.0f);
	}
	geometry infoWindow;
	vector<vec3> info_positions{ vec3(-1.0f, 1.0f, 0.0f), vec3(-1.0f, -1.0f, 0.0f), vec3(1.0f, -1.0f, 0.0f), vec3(-1.0f, 1.0f, 0.0f), vec3(1.0f, -1.0f, 0.0f), vec3(1.0f, 1.0f, 0.0f) };
	vector<vec2> info_tex_coords{ vec2(0.0f, 1.0f), vec2(0.0f, 0.0f), vec2(1.0f, 0.0f), vec2(0.0f, 1.0f), vec2(1.0f, 0.0f), vec2(1.0f, 1.0f) };
	infoWindow.add_buffer(positions, BUFFER_INDEXES::POSITION_BUFFER);
	infoWindow.add_buffer(tex_coords, BUFFER_INDEXES::TEXTURE_COORDS_0);

	meshes["info"] = mesh(infoWindow);
	meshes["info"].get_transform().translate(vec3(0.0f, 2.0f, 5.0f));
	meshes["info"].get_transform().scale = vec3(7.0f, 6.0f, 1.0f);
	meshes["info"].get_transform().rotate(vec3(-M_PI/2.5, 0.0f, 0.0f));

	meshes["info"].get_material().set_emissive(vec4(0.5f, 0.5f, 0.5f, 1.0f));
	meshes["info"].get_material().set_diffuse(vec4(1.0f, 1.0f, 1.0f, 1.0f));
	meshes["info"].get_material().set_specular(vec4(1.0f, 1.0f, 1.0f, 1.0f));
	meshes["info"].get_material().set_shininess(25.0f);
	
	// Set Plane Information
	meshes["plane"].get_material().set_emissive(vec4(0.5f, 0.5f, 0.5f, 1.0f));
	meshes["plane"].get_material().set_diffuse(vec4(1.0f, 1.0f, 1.0f, 1.0f));
	meshes["plane"].get_material().set_specular(vec4(1.0f, 1.0f, 1.0f, 1.0f));
	meshes["plane"].get_material().set_shininess(25.0f);

	tex_01 = texture("textures/floor_tile_01.png");
	tex_02 = texture("textures/floor_tile_02.png");
	tex_sun = texture("textures/sun.png");
	tex_tree_r = texture("textures/tree_right.png");
	tex_tree_l = texture("textures/tree_left.png");
	tex_info = texture("textures/win98se.png");

	SDL_PauseAudioDevice(aDevice, 0);

	// *********************** CAMERA CONFIG **********************
	// Set camera properties
	cam.set_position(vec3(0.0f, 8.0f, 30.0f));
	cam.set_target(vec3(0.0f, 0.0f, 0.0f));
	cam.set_projection(quarter_pi<float>(), renderer::get_screen_aspect(), 0.1f, 1000.0f);
	return true;
}


bool update(float delta_time) {	
	// ********************* VISUALISER UPDATE ********************
	// Scale each bar along the y-axis by the respective frequency bin value.

	for (int i = 0; i < BIN_COUNT; i++) {		
		freq_bin[i] -= 100000;
		if (freq_bin[i] < 0) {
			freq_bin[i] = 0;
		}
	}

	for (int i = 0; i < BIN_COUNT; i++) {
		string meshName = "bar_" + to_string(i);
		float scaleY = freq_bin[i] / 1000000;
		meshes[meshName].get_transform().scale = vec3(1.0f, scaleY, 1.0f);
	}

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

	// Other keyboard controls
	if (glfwGetKey(renderer::get_window(), GLFW_KEY_C)) {
		enableColour = 0;
	}
	if (glfwGetKey(renderer::get_window(), GLFW_KEY_V)) {
		enableColour = 1;
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
		// Set Colour Definition Uniform
		glUniform1i(light_eff.get_uniform_location("isColour"), enableColour); 
		// Set Normal and Model Matrices:
		glUniformMatrix4fv(light_eff.get_uniform_location("M"), 1, GL_FALSE, value_ptr(M));
		glUniformMatrix3fv(light_eff.get_uniform_location("N"), 1, GL_FALSE, value_ptr(cur_mesh.get_transform().get_normal_matrix()));
		// Bind Material
		renderer::bind(cur_mesh.get_material(), "mat"); 
		// Bind lighting model
		renderer::bind(light, "point");

		if (index.first.find("sun") != std::string::npos) {
			renderer::bind(tex_sun, 0); 
		}
		else if(index.first.find("tree") != std::string::npos)
		{
			if (index.first.find("left") != std::string::npos) {
				renderer::bind(tex_tree_l, 0);
			}
			else {
				renderer::bind(tex_tree_r, 0);
			}
		} else if(index.first.find("info") != std::string::npos) {
			renderer::bind(tex_info, 0);
		} else {
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
	// ********************* OPENGL INIT *************************
	app application("a e s t h e t i c");
	// Set load content, update and render methods
	application.set_initialise(initialise);
	application.set_load_content(load_content);
	application.set_update(update);
	application.set_render(render);

	// Initialise the frequency bins with their ranges
	float audioBarRange = (HIGHEST_RANGE - LOWEST_RANGE)/BIN_COUNT;
	for (int i = 0; i < BIN_COUNT; i++) {
		freq_range[i] = audioBarRange*i;
	}

	// ********************** FFTW INIT **************************
	plan = fftw_plan_dft_1d(CHUNK_SIZE, s_in, s_out, FFTW_FORWARD, FFTW_ESTIMATE);
	hamming(CHUNK_SIZE, fft_window);
	for (int i = 0; i < BIN_COUNT; i++) {
		freq_bin[i] = 0.0;
		freq_plot[i] = 0.0;
	}
	

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
	
	// ********************** OGL START **************************

	application.run();
	
}

