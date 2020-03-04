#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in  vec2 _inPosition;
layout(location = 1) in  vec3 _inColor;

layout(location = 0) out vec3 _fragColor;

void main()
{
  gl_Position = vec4(_inPosition, 0.0, 1.0);
  _fragColor  = _inColor;
}