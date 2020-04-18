#ifndef VP_CAMERA_HPP
#define VP_CAMERA_HPP

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif

#include "VPTransform.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>

namespace vpe
{
class Camera
{
public:

  Camera() :
    near(0.1f),
    far(10.0f),
    fieldOfView(glm::radians(45.0f)),
    aspectRatio(1.0f),
    position(glm::vec3(0.0f)),
    forward(FRONT),
    up(UP)
  {
    init();
  };

  Camera(glm::vec3& _pos,  glm::vec3& _forward, glm::vec3& _up,
           float      _near, float      _far,     float      _fov, float _aspectRatio)
  :
    near(_near),
    far(_far),
    fieldOfView(glm::radians(_fov)),
    aspectRatio(_aspectRatio),
    position(_pos),
    forward(_forward),
    up(_up)
  {
    init();
  };

  ~Camera() {};

  inline void setNear(float _near, bool _shouldUpdateProj = true)
  {
    if (_near == near) return;

    near = _near;
    if (_shouldUpdateProj) updateProjection();
  }

  inline void setFar(float _far, bool _shouldUpdateProj = true)
  {
    if (_far == far) return;

    far = _far;
    if (_shouldUpdateProj) updateProjection();
  }

  inline void setFoV(float _fov, bool _shouldUpdateProj = true)
  {
    if (_fov == fieldOfView) return;

    fieldOfView = glm::radians(_fov);
    if (_shouldUpdateProj) updateProjection();
  }

  inline void setAspectRatio(float _aspectRatio, bool _shouldUpdateProj = true)
  {
    if (_aspectRatio == aspectRatio) return;

    aspectRatio = _aspectRatio;
    if (_shouldUpdateProj) updateProjection();
  }

  inline void setPosition(glm::vec3 _newPos, bool _shouldUpdateView = true)
  {
    if(_newPos == position) return;

    position = _newPos;
    if (_shouldUpdateView)
      view = glm::lookAt(position, position + forward, up);
  }

  inline void setForward(glm::vec3 _newForward, bool _shouldUpdateView = true)
  {
    if (_newForward == forward) return;

    forward = _newForward;
    if (_shouldUpdateView)
      view = glm::lookAt(position, position + forward, up);
  }

  inline void setUp(glm::vec3 _newUp, bool _shouldUpdateView = true)
  {
    if (_newUp == up) return;

    up = _newUp;
    if (_shouldUpdateView)
      view = glm::lookAt(position, position + forward, up);
  }

  inline void translate(const glm::vec3& _translation)
  {
    position += glm::normalize(glm::cross(up, forward)) * _translation.x +
                up * _translation.y +
                forward * _translation.z;

    view      = glm::lookAt(position, position + forward, up);
  }

  inline void rotate(const glm::vec3& _eulerAngles)
  { // TODO: Use quaternions
    // I'm using Z -> Y -> X to avoid gimball lock, since we are not rolling
    //Roll
    forward = glm::rotate(forward, glm::radians(_eulerAngles.z), forward);
    // Pan
    forward = glm::rotate(forward, -glm::radians(_eulerAngles.y), up);
    // Tilt
    forward = glm::rotate(forward, glm::radians(_eulerAngles.x), glm::cross(forward, up));

    forward = glm::normalize(forward); // Is it really necessary?

    view    = glm::lookAt(position, position + forward, up);
  }

  inline const glm::mat4& getProjMat() const { return projection; }
  inline const glm::mat4& getViewMat() const { return view; }

private:

  float     near;
  float     far;
  float     fieldOfView;
  float     aspectRatio;
  Transform transform;
  glm::vec3 position;
  glm::vec3 forward;
  glm::vec3 up;
  glm::mat4 view;
  glm::mat4 projection;

  inline void init()
  {
    view = glm::lookAt(position, position + forward, up);
    updateProjection();
  }

  inline void updateProjection()
  {
    projection = glm::perspective(fieldOfView, aspectRatio, near, far);

    // GLM was designed for OpenGL, that has the Y coordinate of the clip coordinates inverted
    projection[1][1] *= -1;
  }
};
}
#endif