#ifndef VP_CAMERA_HPP
#define VP_CAMERA_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct VPCamera
{
  float     near;
  float     far;
  float     fieldOfView;
  glm::vec3 position;
  glm::vec3 forward;
  glm::vec3 up;

  inline glm::mat4 getProjMat(const float _aspectRatio)
  {
    glm::mat4 proj = glm::perspective(fieldOfView, _aspectRatio, near, far);

    // GLM was designed for OpenGL, that has the Y coordinate of the clip coordinates inverted
    proj[1][1] *= -1;

    return proj;
  }

  inline glm::mat4 getViewMat() const { return glm::lookAt(position, forward, up); }
};

#endif