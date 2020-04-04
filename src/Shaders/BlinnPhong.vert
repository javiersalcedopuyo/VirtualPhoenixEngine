#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform modelViewProjectionNormalUBO
{
  mat4 modelView;
  mat4 view;
  mat4 proj;
  mat4 normal;
} u_mvpn;

layout(location = 0) in  vec3 _inPosition;
layout(location = 1) in  vec3 _inColor;
layout(location = 2) in  vec3 _inNormal;
layout(location = 3) in  vec2 _inTexCoord;

layout(location = 0) out vec3  _fragPosition;
layout(location = 1) out vec3  _fragNormal;
layout(location = 2) out vec2  _fragTexCoord;

void main()
{
  const vec4 cameraVertexPos = u_mvpn.modelView * vec4(_inPosition, 1.0);

  _fragPosition = cameraVertexPos.xyz;
  _fragNormal   = (u_mvpn.view * vec4(_inNormal, 0)).xyz;
  _fragTexCoord = _inTexCoord;

  gl_Position = u_mvpn.proj * cameraVertexPos;
 }