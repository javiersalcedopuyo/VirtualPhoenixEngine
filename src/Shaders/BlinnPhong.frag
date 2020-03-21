#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 1) uniform sampler2D _texSampler;

layout(location = 0) in  vec3 _fragColor;
layout(location = 1) in  vec3 _normal;
layout(location = 2) in  vec2 _texCoord;
layout(location = 3) in  vec3 _lightVector;
layout(location = 4) in  vec3 _viewVector;

layout(location = 0) out vec4 _outColor; // location=0: framebuffer index

void main()
{
  const vec3  texColor   = texture(_texSampler, _texCoord).rgb;
  const vec3  dirToLight = normalize(_lightVector);
  const vec3  halfVector = (dirToLight + normalize(_viewVector)) * 0.5;
  const float ligthDist  = length(_lightVector);

  // TODO: Get as uniforms /////////////////////////////////////////////////////////////////////////
  const float lightI   = 0.5;
  const float ambientI = 0.1;

  const float Ka       = 1.0;
  const float Kd       = 1.0;
  const float Ks       = 1.0;
  const float glossy   = 10.0;
  //////////////////////////////////////////////////////////////////////////////////////////////////

  const float attenuationFactor = 1.0 / ligthDist * ligthDist; // TODO: Use Attenuation coeficients

  float ambient  = ambientI * Ka;
  float diffuse  = lightI   * Kd * attenuationFactor * clamp(dot(dirToLight, _normal), 0, 1);
  float specular = lightI   * Ks * pow(clamp(dot(_normal, halfVector), 0, 1), glossy);

  _outColor.rgb  = texColor * (ambient + diffuse + specular);
  _outColor.a    = 1.0;
}