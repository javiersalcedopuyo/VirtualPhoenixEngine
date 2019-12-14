#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in  vec3 _fragColor;
layout(location = 0) out vec4 _outColor; // location=0: framebuffer index

void main()
{
  _outColor = vec4(_fragColor, 1.0);
}