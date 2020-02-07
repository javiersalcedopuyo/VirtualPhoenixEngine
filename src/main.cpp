#include "HelloTriangle.hpp"

int main()
{
  VulkanInstanceManager vkInstanceManager;
  DevicesManager        devicesManager(vkInstanceManager.getVkInstanceRef());
  SwapchainManager      swapchainManager;
  PipelineManager       pipelineManager;
  CommandBuffersManager commandBufManager;
  TrafficCop            trafficCop; // Manages the semaphores! (and the fences too)

  HelloTriangle app(vkInstanceManager,
                    devicesManager,
                    swapchainManager,
                    pipelineManager,
                    commandBufManager,
                    trafficCop);

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
