#ifndef VP_CALLBACKS_HPP
#define VP_CALLBACKS_HPP

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#endif

#include <GLFW/glfw3.h>
#include <functional>
#include <array>

#include "VPCamera.hpp"

namespace vpe
{
struct UserInputContext
{
  void* pData;
  float cameraMoveSpeed;
  float cameraRotateSpeed;
  float deltaTime;
  float scrollY;

  std::array<double,2> cursorDelta;

  Camera*   camera;
  GLFWwindow* window;
};

namespace callbacks
{
  static inline void cameraMovementWASD(UserInputContext& _ctx)
  {
    if (_ctx.camera == nullptr) return;

    float     angle     = _ctx.cameraRotateSpeed * _ctx.deltaTime;
    float     distance  = _ctx.cameraMoveSpeed * _ctx.deltaTime;
    glm::vec3 direction = glm::vec3();

    if (glfwGetMouseButton(_ctx.window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
    { // Rotate the camera by dragging
      float panAngle  = 0;
      float tiltAngle = 0;

      if (!_ctx.cursorDelta.empty())
      {
        panAngle  = -angle * _ctx.cursorDelta[0] * 50;
        tiltAngle = -angle * _ctx.cursorDelta[1] * 50;
      }

      _ctx.camera->rotate(RIGHT * tiltAngle);
      _ctx.camera->rotate(UP    * panAngle);
    }

    if (glfwGetMouseButton(_ctx.window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS)
    { // Move camera by dragging
      if (!_ctx.cursorDelta.empty())
      {
        direction += LEFT * static_cast<float>(_ctx.cursorDelta[0]) +
                     UP   * static_cast<float>(_ctx.cursorDelta[1]);

        distance  *= direction.length();
      }

      if (direction != glm::vec3(0)) direction = glm::normalize(direction);
      _ctx.camera->translate( direction * distance );

      direction = glm::vec3(0);
      distance  = _ctx.cameraMoveSpeed * _ctx.deltaTime;
    }

    if (_ctx.scrollY != 0.0f)
    {
      direction += FRONT * _ctx.scrollY;
      distance  *= 50.0f;
    }
    else
    {
      // DOUBT: Should I use Ctrl + key instead?
      if (glfwGetKey(_ctx.window, GLFW_KEY_W) == GLFW_PRESS) direction += FRONT;
      if (glfwGetKey(_ctx.window, GLFW_KEY_S) == GLFW_PRESS) direction += BACK;
      if (glfwGetKey(_ctx.window, GLFW_KEY_A) == GLFW_PRESS) direction += LEFT;
      if (glfwGetKey(_ctx.window, GLFW_KEY_D) == GLFW_PRESS) direction += RIGHT;
    }

    if (direction != glm::vec3(0)) direction = glm::normalize(direction);

    _ctx.camera->translate( direction * distance );
  }

  static inline void cameraMovementArrows(UserInputContext& _ctx)
  {
    if (_ctx.camera == nullptr) return;

    float     distance  = _ctx.cameraMoveSpeed * _ctx.deltaTime;
    glm::vec3 direction = glm::vec3();

    if (glfwGetKey(_ctx.window, GLFW_KEY_UP)    == GLFW_PRESS) direction += FRONT;
    if (glfwGetKey(_ctx.window, GLFW_KEY_DOWN)  == GLFW_PRESS) direction += BACK;
    if (glfwGetKey(_ctx.window, GLFW_KEY_LEFT)  == GLFW_PRESS) direction += LEFT;
    if (glfwGetKey(_ctx.window, GLFW_KEY_RIGHT) == GLFW_PRESS) direction += RIGHT;

    if (direction != glm::vec3(0)) direction = glm::normalize(direction);

    _ctx.camera->translate( direction * distance );
  }
}
}
#endif