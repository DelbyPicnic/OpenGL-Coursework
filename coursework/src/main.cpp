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
target_camera cam;

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
  // Update the camera
  cam.update(delta_time);
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
  application.set_load_content(load_content);
  application.set_update(update);
  application.set_render(render);
  // Run application
  application.run();
}