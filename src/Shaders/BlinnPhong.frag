#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_nonuniform_qualifier    : enable

layout(set = 0, binding = 0) uniform modelViewProjectionNormalUBO
{
  mat4 modelView;
  mat4 view;
  mat4 proj;
  mat4 Normal;
} u_mvpn;

layout(set = 0, binding = 1) uniform lightsUBO
{
  float intensity;
  float range;
  float angle;
  vec3  position;
  vec3  color;
  vec3  direction;
} u_lights[];

layout(set = 0, binding = 2) uniform sampler2D _texSampler;

layout(push_constant) uniform PushConstant
{
  int count;
} _numLights;

layout(location = 0) in  vec3 _position;
layout(location = 1) in  vec3 _normal;
layout(location = 2) in  vec2 _texCoord;

layout(location = 0) out vec4 _outColor; // location=0: framebuffer index

void main()
{
  const vec3  texColor       = texture(_texSampler, _texCoord).rgb;
  const vec3  viewDirection  = normalize(-_position);
  const vec3  normal         = normalize(_normal);
  const float glossy         = 10.0; // TODO: Get as uniform

  vec3 ambient  = 0.1 * vec3(1);
  vec3 diffuse  = vec3(0);
  vec3 specular = vec3(0);

  for (int i = 0; i < _numLights.count; ++i)
  {
    vec3  lightVector = (u_mvpn.view * vec4(u_lights[i].position, 1.0) - vec4(_position, 1.0)).xyz;
    vec3  halfVector  = (normalize(lightVector) + viewDirection) * 0.5;
    float distToLight = length(lightVector);
    float attenuation = 1.0 / (distToLight * distToLight);

    diffuse  += attenuation * u_lights[i].color * u_lights[i].intensity * clamp(dot(normalize(lightVector), _normal), 0, 1);
    specular += attenuation * u_lights[i].color * pow( dot(_normal, halfVector), glossy );
  }

  _outColor.rgb = texColor * (ambient + diffuse + specular);
  _outColor.a   = 1.0;
}