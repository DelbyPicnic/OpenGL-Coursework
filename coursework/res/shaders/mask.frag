#version 430 core

// Captured render
uniform sampler2D tex;
// Alpha map
uniform sampler2D alpha_map;
uniform vec4 clear_colour;

// Incoming texture coordinate
layout(location = 0) in vec2 tex_coord;

// Outgoing colour
layout(location = 0) out vec4 colour;

void main() {
  // *********************************
  // Sample textures
  vec4 r_tex = texture(tex, tex_coord);
  vec4 a_tex = texture(alpha_map, tex_coord);
  if(a_tex.a == 0.0f){
	colour = r_tex;
  }else{
	colour = a_tex;
  }
  // Ensure alpha is 1
  colour.a = 1.0;
  // *********************************
}