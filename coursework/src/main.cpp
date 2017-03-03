// 2017 Gordon Swan
// OpenGL Coursework Version 1

#include <glm\glm.hpp>
#include <graphics_framework.h>

using namespace std;
using namespace graphics_framework;
using namespace glm;

map<string, mesh> meshes;
map<string, mesh> pillars;
mesh groundPlane;
mesh stdPyramid;
mesh skybox;
mesh horizon;

effect sky_eff;
effect light_eff;
effect horiz_eff;

point_light light;

cubemap cube_map;
texture tex_01;
texture tex_02;
texture horizon_tex;

float t_time = 0.0f;
float r = 0.0f;
float s = 0.0f;
float theta = 0.0f;

free_camera cam;
double cursor_x;
double cursor_y;


// Creates cylinder geometry
geometry create_tube(const unsigned int stacks, const unsigned int slices,
	const glm::vec3 &dims) {
	// Type of geometry generated will be triangles
	geometry geom;
	geom.set_type(GL_TRIANGLES);
	// Declare required buffers - positions, normals, texture coordinates and
	// colour
	std::vector<glm::vec3> positions;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec2> tex_coords;
	std::vector<glm::vec4> colours;

	// Minimal and maximal points
	glm::vec3 minimal(0.0f, 0.0f, 0.0f);
	glm::vec3 maximal(0.0f, 0.0f, 0.0f);

	// Create top - similar to disk but now using triangles
	glm::vec3 centre(0.0f, 0.5f * dims.y, 0.0f);
	// Recalculate minimal and maximal
	minimal = glm::min(minimal, centre);
	maximal = glm::max(maximal, centre);
	auto prev_vert = glm::vec3(0.5f, 0.5f, 0.0f) * dims;
	// Recalculate minimal and maximal
	minimal = glm::min(minimal, prev_vert);
	maximal = glm::max(maximal, prev_vert);
	glm::vec3 curr_vert;
	glm::vec2 tex_coord(0.5f, 0.5f);
	// Angle per slice
	auto delta_angle = (2.0f * glm::pi<float>()) / static_cast<float>(slices);
	// Iterate through each slice
	for (unsigned int i = 1; i <= slices; ++i) {
		// Calculate unit length vertex
		curr_vert = glm::vec3(cos(i * delta_angle), 1.0f, -sin(i * delta_angle));
		// We want radius to be 1
		curr_vert /= 2.0f;
		// Multiply by dimensions
		curr_vert *= dims;
		// Recalculate minimal and maximal
		// Recalculate minimal and maximal
		minimal = glm::min(minimal, curr_vert);
		maximal = glm::max(maximal, curr_vert);
		// Push back vertices
		//positions.push_back(centre);
		//positions.push_back(prev_vert);
		//positions.push_back(curr_vert);
		// Push back normals and colours
		for (unsigned int j = 0; j < 3; ++j) {
			//normals.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
			//colours.push_back(glm::vec4(0.7f, 0.7f, 0.7f, 1.0f));
		}
		// Push back tex coordinates
		//tex_coords.push_back(tex_coord);
		//tex_coords.push_back(glm::vec2(tex_coord.x + prev_vert.x, tex_coord.y - prev_vert.z));
		//tex_coords.push_back(glm::vec2(tex_coord.x + curr_vert.x, tex_coord.y - curr_vert.z));
		// Set previous as current
		prev_vert = curr_vert;
	}

	// Create bottom - same process as top
	centre = glm::vec3(0.0f, -0.5f * dims.y, 0.0f);
	// Recalculate minimal and maximal
	minimal = glm::min(minimal, centre);
	maximal = glm::max(maximal, centre);

	prev_vert = glm::vec3(0.5f, -0.5f, 0.0f) * dims;
	// Recalculate minimal and maximal
	minimal = glm::min(minimal, prev_vert);
	maximal = glm::max(maximal, prev_vert);
	// Iterate for each slice
	for (unsigned int i = 1; i <= slices; ++i) {
		// Calculate unit length vertex
		curr_vert = glm::vec3(cos(i * delta_angle), -1.0f, sin(i * delta_angle));
		// We want radius to be 1
		curr_vert /= 2.0f;
		// Multiply by dimensions
		curr_vert *= dims;
		// Recalculate minimal and maximal
		minimal = glm::min(minimal, curr_vert);
		maximal = glm::max(maximal, curr_vert);
		// Push back vertices
		//positions.push_back(centre);
		//positions.push_back(prev_vert);
		//positions.push_back(curr_vert);
		// Push back normals and colours
		for (unsigned int j = 0; j < 3; ++j) {
			//normals.push_back(glm::vec3(0.0f, -1.0f, 0.0f));
			//colours.push_back(glm::vec4(0.7f, 0.7f, 0.7f, 1.0f));
		}
		// Push back texture coordinates
		//tex_coords.push_back(tex_coord);
		//tex_coords.push_back(glm::vec2(tex_coord.x - prev_vert.x, tex_coord.y - prev_vert.z));
		//tex_coords.push_back(glm::vec2(tex_coord.x - curr_vert.x, tex_coord.y - curr_vert.z));
		// Set previous as current
		prev_vert = curr_vert;
	}

	// Create stacks
	std::array<glm::vec3, 4> verts;
	std::array<glm::vec2, 4> coords;
	// Delta height - scaled during vertex creation
	auto delta_height = 2.0f / static_cast<float>(stacks);
	// Calculate circumference - could be ellipitical
	auto circ =
		glm::pi<float>() * ((3.0f * (dims.x + dims.z)) - (sqrtf((3.0f * dims.x + dims.z) * (dims.x + 3.0f * dims.z))));
	// Delta width is the circumference divided into slices
	auto delta_width = circ / static_cast<float>(slices);
	// Iterate through each stack
	for (unsigned int i = 0; i < stacks; ++i) {
		// Iterate through each slice
		for (unsigned int j = 0; j < slices; ++j) {
			// Caculate vertices
			verts[0] = glm::vec3(cos(j * delta_angle), 1.0f - (delta_height * i), sin(j * delta_angle));
			verts[1] = glm::vec3(cos((j + 1) * delta_angle), 1.0f - (delta_height * i), sin((j + 1) * delta_angle));
			verts[2] = glm::vec3(cos(j * delta_angle), 1.0f - (delta_height * (i + 1)), sin(j * delta_angle));
			verts[3] = glm::vec3(cos((j + 1) * delta_angle), 1.0f - (delta_height * (i + 1)), sin((j + 1) * delta_angle));
			// Scale by 0.5 * dims
			for (auto &v : verts)
				v *= dims * 0.5f;
			// Recalculate minimal and maximal
			for (auto &v : verts) {
				minimal = glm::min(minimal, v);
				maximal = glm::max(maximal, v);
			}

			// Calculate texture coordinates
			coords[0] = glm::vec2((-delta_width * j) / glm::pi<float>(), dims.y - ((delta_height * i * dims.y) / 2.0f));
			coords[1] = glm::vec2((-delta_width * (j + 1)) / glm::pi<float>(), dims.y - ((delta_height * i * dims.y) / 2.0f));
			coords[2] = glm::vec2((-delta_width * j) / glm::pi<float>(), dims.y - ((delta_height * (i + 1) * dims.y) / 2.0f));
			coords[3] =
				glm::vec2((-delta_width * (j + 1)) / glm::pi<float>(), dims.y - ((delta_height * (i + 1) * dims.y) / 2.0f));

			// Triangle 1
			positions.push_back(verts[0]);
			normals.push_back(glm::normalize(glm::vec3(verts[0].x, 0.0f, verts[0].z)));
			tex_coords.push_back(coords[0]);
			positions.push_back(verts[3]);
			normals.push_back(glm::normalize(glm::vec3(verts[3].x, 0.0f, verts[3].z)));
			tex_coords.push_back(coords[3]);
			positions.push_back(verts[2]);
			normals.push_back(glm::normalize(glm::vec3(verts[2].x, 0.0f, verts[2].z)));
			tex_coords.push_back(coords[2]);
			// Triangle 2
			positions.push_back(verts[0]);
			normals.push_back(glm::normalize(glm::vec3(verts[0].x, 0.0f, verts[0].z)));
			tex_coords.push_back(coords[0]);
			positions.push_back(verts[1]);
			normals.push_back(glm::normalize(glm::vec3(verts[1].x, 0.0f, verts[1].z)));
			tex_coords.push_back(coords[1]);
			positions.push_back(verts[3]);
			normals.push_back(glm::normalize(glm::vec3(verts[3].x, 0.0f, verts[3].z)));
			tex_coords.push_back(coords[3]);

			// Colours
			for (unsigned int k = 0; k < 6; ++k)
				colours.push_back(glm::vec4(0.7f, 0.7f, 0.7f, 1.0f));
		}
	}

	// Set minimal and maximal values
	geom.set_minimal_point(minimal);
	geom.set_maximal_point(maximal);

	// Add buffers to geometry
	geom.add_buffer(positions, BUFFER_INDEXES::POSITION_BUFFER);
	geom.add_buffer(normals, BUFFER_INDEXES::NORMAL_BUFFER);
	geom.add_buffer(colours, BUFFER_INDEXES::COLOUR_BUFFER);
	geom.add_buffer(tex_coords, BUFFER_INDEXES::TEXTURE_COORDS_0);

	// Generate tangent and binormal data
	geom.generate_tb(normals);

	return std::move(geom);
}


bool initialise() {
	// Capture cursor input
	glfwSetInputMode(renderer::get_window(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	// Capture initial cursor position
	glfwGetCursorPos(renderer::get_window(), &cursor_x, &cursor_y);
	return true;
}

bool load_content() {
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

	// *********************** HORIZON LOAD **********************
	horizon = mesh(create_tube(10, 10, vec3(1.0f, 1.0f, 1.0f)));
	horizon.get_transform().scale = vec3(200.0f, 50.0f, 200.0f);
	horizon_tex = texture("textures/wall_02.png");

	horiz_eff.add_shader("shaders/basic_textured.frag", GL_FRAGMENT_SHADER);
	horiz_eff.add_shader("shaders/basic_textured.vert", GL_VERTEX_SHADER);
	horiz_eff.build();

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
	// *********************** UPDATE SHAPES **********************
	t_time += delta_time;
	r = 1.0f + sinf(t_time);
	r = r / 8;

	s = 1.0f + cosf(t_time);
	s = s / 6;

	meshes["tetra01"].get_transform().rotate(vec3(0.0f, r, 0.0f));
	meshes["tetra02"].get_transform().rotate(vec3(0.0f, r*-0.5, 0.0f));
	meshes["tetra03"].get_transform().rotate(vec3(0.0f, r/2, 0.0f));
	
	r *= 5;
	s *= 5;

	meshes["tetra01"].get_transform().scale = vec3(s, 1.0f, s);
	meshes["tetra02"].get_transform().scale = vec3(r, 1.0f, s);
	meshes["tetra03"].get_transform().scale = vec3(s, 1.0f, r);
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

	// *********************** SKYBOX RENDER ***********************
	// Disable Depth Testing, Face Culling and Depth Masking
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);

	renderer::bind(horiz_eff);
	// Calculate Horizon MVP
	M = horizon.get_transform().get_transform_matrix();
	V = cam.get_view();
	P = cam.get_projection();
	MVP = P * V * M;
	// Set MVP Matrix Uniform 
	glUniformMatrix4fv(horiz_eff.get_uniform_location("MVP"), 1, GL_FALSE, value_ptr(MVP));
	// Bind texture
	renderer::bind(horizon_tex, 1);
	glUniform1i(horiz_eff.get_uniform_location("tex"), 1);
	renderer::render(horizon);

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

