#ifndef VP_TRANSFORM_HPP
#define VP_TRANSFORM_HPP

#ifndef GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_LEFT_HANDED
#endif

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "VPMath.hpp"

namespace vpe
{
constexpr glm::vec3 UP    = glm::vec3( 0.0f,  1.0f,  0.0f);
constexpr glm::vec3 DOWN  = glm::vec3( 0.0f, -1.0f,  0.0f);
constexpr glm::vec3 LEFT  = glm::vec3(-1.0f,  0.0f,  0.0f);
constexpr glm::vec3 RIGHT = glm::vec3( 1.0f,  0.0f,  0.0f);
constexpr glm::vec3 FRONT = glm::vec3( 0.0f,  0.0f,  1.0f);
constexpr glm::vec3 BACK  = glm::vec3( 0.0f,  0.0f, -1.0f);

enum class TransformOperation : uint8_t
{
  TRANSLATE,
  ROTATE_EULER,
  ROTATE_QUATERNION,
  SCALE
};

class Transform
{
public:
  Transform() : m_position(0), m_eulerAngles(0), m_scale(1), m_modelMatrix(1) {}
  ~Transform()
  {
    // TODO:
    //m_parent.reset();
    //for (auto child : children) child.reset();
  }

  inline void translate(glm::vec3 _displacement)
  {
    if (_displacement == glm::vec3(0)) return;

    m_position += _displacement;
    m_modelMatrix = glm::translate(m_modelMatrix, _displacement);
  }
  inline void scale(glm::vec3 _scaleFactors)
  {
    if (_scaleFactors == glm::vec3(0)) return;

    m_scale += _scaleFactors;
    m_modelMatrix = glm::scale(m_modelMatrix, _scaleFactors);
  }

  inline void rotate(glm::vec3 _eulerAngles)
  {
    if (_eulerAngles == glm::vec3(0)) return;

    m_eulerAngles += _eulerAngles;
    math::clampAngle(m_eulerAngles.x);
    math::clampAngle(m_eulerAngles.y);
    math::clampAngle(m_eulerAngles.z);
    // Z -> Y -> X
    // Avoid Gimball Lock while not rolling
    m_modelMatrix = glm::rotate(m_modelMatrix, _eulerAngles.z, glm::vec3(0,0,1));
    m_modelMatrix = glm::rotate(m_modelMatrix, _eulerAngles.y, glm::vec3(0,1,0));
    m_modelMatrix = glm::rotate(m_modelMatrix, _eulerAngles.x, glm::vec3(1,0,0));
  }

  inline const glm::vec3& getPosition()    const { return m_position; }
  inline const glm::vec3& getEulerAngles() const { return m_eulerAngles; }
  inline const glm::vec3& getScale()       const { return m_scale; }
  inline const glm::mat4& getModelMatrix() const { return m_modelMatrix; }
  // TODO: const vpe::math::quaternion& getRotation() const { return m_rotation; }

private:
  // TODO: std::shared_ptr<Transform> m_parent;
  // TODO: std::vector< shared_ptr<Transform> > m_children;
  glm::vec3 m_position;
  glm::vec3 m_eulerAngles;
  // TODO: vpe::math::quaternion m_rotation;
  glm::vec3 m_scale;
  glm::mat4 m_modelMatrix;
};
}

#endif