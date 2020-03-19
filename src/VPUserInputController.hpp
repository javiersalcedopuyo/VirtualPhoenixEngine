#ifndef VP_USER_INPUT_CONTROLLER
#define VP_USER_INPUT_CONTROLLER

#include "VPCallbacks.hpp"

class VPUserInputController
{
public:
  VPUserInputController() {};
  ~VPUserInputController() {};

  inline void processInput(VPUserInputContext& _ctx)
  {
    if (cameraMovementCB) cameraMovementCB(_ctx);
  }

  inline void setCameraMovementCB(std::function<void(VPUserInputContext&)> _cb)
  {
    cameraMovementCB = _cb;
  }

private:

  std::function<void(VPUserInputContext&)> cameraMovementCB;
};

#endif