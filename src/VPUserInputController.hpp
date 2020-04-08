#ifndef VP_USER_INPUT_CONTROLLER
#define VP_USER_INPUT_CONTROLLER

#include "VPCallbacks.hpp"

namespace vpe
{
class UserInputController
{
public:
  UserInputController() {};
  ~UserInputController() {};

  double* m_pScrollY;

  inline void processInput(UserInputContext& _ctx)
  {
    updateCursorDelta(_ctx.window, _ctx.cursorDelta);
    if (cameraMovementCB) cameraMovementCB(_ctx);
  }

  inline void setCameraMovementCB(std::function<void(UserInputContext&)> _cb)
  {
    cameraMovementCB = _cb;
  }

private:

  double lastCursorPositionX;
  double lastCursorPositionY;

  std::function<void(UserInputContext&)> cameraMovementCB;

  void updateCursorDelta(GLFWwindow* _pWindow, std::array<double,2>& _cursorDelta)
  {
    double currentCursorX, currentCursorY;
    glfwGetCursorPos(_pWindow, &currentCursorX, &currentCursorY);

    _cursorDelta[0] = currentCursorX - lastCursorPositionX;
    _cursorDelta[1] = currentCursorY - lastCursorPositionY;

    lastCursorPositionX = currentCursorX;
    lastCursorPositionY = currentCursorY;
  }
};
}
#endif