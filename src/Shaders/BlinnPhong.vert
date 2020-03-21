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
layout(location = 2) in  vec3 _inNormal;
layout(location = 3) in  vec2 _inTexCoord;

layout(location = 0) out vec3 _fragColor;
layout(location = 1) out vec3 _fragNormal;
layout(location = 2) out vec2 _fragTexCoord;
layout(location = 3) out vec3 _lightVector;
layout(location = 4) out vec3 _viewVector;

void main()
{
  const mat4 normalMatrix = transpose(inverse(ubo.model));

  _fragColor    = _inColor;
  _fragTexCoord = _inTexCoord;

  const vec4 modelNormal = normalMatrix * vec4(_inNormal, 0.0);
  _fragNormal            = normalize(ubo.view * modelNormal).xyz;

  const vec4 modelVertexPos  = ubo.model * vec4(_inPosition, 1.0);
  const vec4 cameraVertexPos = ubo.view * modelVertexPos;
  const vec4 cameraLightPos  = ubo.view * vec4(0.0, -3.0, 2.5, 1.0); // TODO: Get as uniform

  _lightVector = (cameraLightPos - cameraVertexPos).xyz;
  _viewVector  = -cameraVertexPos.xyz;

  gl_Position = ubo.proj * cameraVertexPos;
}