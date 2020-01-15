#include "HelloTriangle.hpp"

int main()
{
  VulkanInstanceManager vkInstanceManager;
  DevicesManager        devicesManager(vkInstanceManager.getVkInstanceRef());

  HelloTriangle app(vkInstanceManager, devicesManager);

  std::cout << "Starting..." << std::endl;

  try
  {
    app.run();
  }
  catch(const std::exception& e)
  {
    std::cerr << e.what() << '\n';
    return EXIT_FAILURE;
  }

  std::cout << "All went OK :D" << std::endl;
  return EXIT_SUCCESS;
}
