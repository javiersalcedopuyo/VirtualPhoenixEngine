#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform UniformBufferObject
{
  mat4 model;
  mat4 view;
  mat4 proj;
} ubo;

layout(location = 0) in  vec3 _inPosition;
layout(location = 1) in  vec3 _inColor;
layout(location = 2) in  vec2 _inTexCoord;

layout(location = 0) out vec3 _fragColor;
layout(location = 1) out vec2 _fragTexCoord;

void main()
{
  gl_Position   = ubo.proj * ubo.view * ubo.model * vec4(_inPosition, 1.0);
  _fragColor    = _inColor;
  _fragTexCoord = _inTexCoord;
}