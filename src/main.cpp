#include "VPRenderer.hpp"

static double s_scrollY = 0;

// To avoid de compiler complaining
#define NOT_USED(x) ( (void)(x) )

int main()
{
  VPRenderer renderer;

  std::cout << "Starting..." << std::endl;

  glm::vec3 cameraUp      = glm::vec3(0.0f, 0.0f, 1.0f);
  glm::vec3 cameraPos     = glm::vec3(0.0f, -3.0f, 2.5f);
  glm::vec3 cameraForward = glm::normalize(-cameraPos);

  try
  {
    renderer.init();

    // Defaults: FoV: 45, Far: 10, Near: 0.1
    renderer.setCamera(cameraPos, cameraForward, cameraUp);

    glfwSetScrollCallback(renderer.getActiveWindow(),
                          [](GLFWwindow* w, double x, double y)
                          {
                            NOT_USED(w);
                            NOT_USED(x);
                            s_scrollY = y;
                          });

    renderer.m_pUserInputController->m_pScrollY = &s_scrollY;

    if (renderer.m_pUserInputController == nullptr)
      std::cout << "WARNING: No User Input Controller!" << std::endl;
    else
      renderer.m_pUserInputController->setCameraMovementCB( VPCallbacks::cameraMovementWASD );
      //renderer.m_pUserInputController->setCameraMovementCB( VPCallbacks::cameraMovementArrows );

    // TODO: renderer.createObj();

    renderer.mainLoop();
    renderer.cleanUp();
  }
  catch(const std::exception& e)
  {
    std::cerr << e.what() << '\n';
    return EXIT_FAILURE;
  }

  std::cout << "All went OK :D" << std::endl;
  return EXIT_SUCCESS;
}
