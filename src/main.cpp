#include "VPRenderer.hpp"

int main()
{
  VPRenderer renderer;

  std::cout << "Starting..." << std::endl;

  glm::vec3 cameraUp      = glm::vec3(0.0f, 0.0f, 1.0f);
  glm::vec3 cameraPos     = glm::vec3(2.0f, 2.0f, 3.0f);
  glm::vec3 cameraForward = glm::normalize(-cameraPos);

  try
  {
    renderer.init();

    // Defaults: FoV: 45, Far: 10, Near: 0.1
    renderer.setCamera(cameraPos, cameraForward, cameraUp);

    // renderer.createObj();

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
