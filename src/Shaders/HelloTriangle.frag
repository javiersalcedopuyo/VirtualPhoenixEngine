#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 1) uniform sampler2D _texSampler;

layout(location = 0) in  vec3 _fragColor;
layout(location = 1) in  vec2 _texCoord;

layout(location = 0) out vec4 _outColor; // location=0: framebuffer index

void main()
{
  _outColor = texture(_texSampler, _texCoord);
}