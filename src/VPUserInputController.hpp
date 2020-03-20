#ifndef VP_USER_INPUT_CONTROLLER
#define VP_USER_INPUT_CONTROLLER

#include "VPCallbacks.hpp"

class VPUserInputController
{
public:
  VPUserInputController() {};
  ~VPUserInputController() {};

  double* m_pScrollY;

  inline void processInput(VPUserInputContext& _ctx)
  {
    updateCursorDelta(_ctx.window, _ctx.cursorDelta);
    if (cameraMovementCB) cameraMovementCB(_ctx);
  }

  inline void setCameraMovementCB(std::function<void(VPUserInputContext&)> _cb)
  {
    cameraMovementCB = _cb;
  }

private:

  double lastCursorPositionX;
  double lastCursorPositionY;

  std::function<void(VPUserInputContext&)> cameraMovementCB;

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

#endif