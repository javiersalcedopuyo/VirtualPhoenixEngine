#ifndef VP_CALLBACKS_HPP
#define VP_CALLBACKS_HPP

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif

#include <GLFW/glfw3.h>
#include <functional>

#include "VPCamera.hpp"


struct VPUserInputContext
{
  void*       pData;
  float       movementSpeed;
  float       deltaTime;
  VPCamera*   camera;
  GLFWwindow* window;
};

namespace VPCallbacks
{
  constexpr glm::vec3 UP    = glm::vec3( 0.0f,  0.0f,  1.0f);
  constexpr glm::vec3 DOWN  = glm::vec3( 0.0f,  0.0f, -1.0f);
  constexpr glm::vec3 LEFT  = glm::vec3(-1.0f,  0.0f,  0.0f);
  constexpr glm::vec3 RIGHT = glm::vec3( 1.0f,  0.0f,  0.0f);
  constexpr glm::vec3 FRONT = glm::vec3( 0.0f,  1.0f,  0.0f);
  constexpr glm::vec3 BACK  = glm::vec3( 0.0f, -1.0f,  0.0f);

  static inline void cameraMovementWASD(VPUserInputContext& _ctx)
  {
    if (_ctx.camera == nullptr) return;

    float     distance  = _ctx.movementSpeed * _ctx.deltaTime;
    glm::vec3 direction = glm::vec3();

    if (glfwGetKey(_ctx.window, GLFW_KEY_W) == GLFW_PRESS) direction += FRONT;
    if (glfwGetKey(_ctx.window, GLFW_KEY_S) == GLFW_PRESS) direction += BACK;
    if (glfwGetKey(_ctx.window, GLFW_KEY_A) == GLFW_PRESS) direction += LEFT;
    if (glfwGetKey(_ctx.window, GLFW_KEY_D) == GLFW_PRESS) direction += RIGHT;

    if (direction != glm::vec3(0)) direction = glm::normalize(direction);

    _ctx.camera->translate( direction * distance );
  }

  static inline void cameraMovementArrows(VPUserInputContext& _ctx)
  {
    if (_ctx.camera == nullptr) return;

    float     distance  = _ctx.movementSpeed * _ctx.deltaTime;
    glm::vec3 direction = glm::vec3();

    if (glfwGetKey(_ctx.window, GLFW_KEY_UP)    == GLFW_PRESS) direction += FRONT;
    if (glfwGetKey(_ctx.window, GLFW_KEY_DOWN)  == GLFW_PRESS) direction += BACK;
    if (glfwGetKey(_ctx.window, GLFW_KEY_LEFT)  == GLFW_PRESS) direction += LEFT;
    if (glfwGetKey(_ctx.window, GLFW_KEY_RIGHT) == GLFW_PRESS) direction += RIGHT;

    if (direction != glm::vec3(0)) direction = glm::normalize(direction);

    _ctx.camera->translate( direction * distance );
  }
}

#endif