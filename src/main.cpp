#include "HelloTriangle.hpp"

int main()
{
  VulkanInstanceManager m_vkInstanceManager;
  HelloTriangle m_app(m_vkInstanceManager);

  std::cout << "Starting..." << std::endl;

  try
  {
    m_app.Run();
  }
  catch(const std::exception& e)
  {
    std::cerr << e.what() << '\n';
    return EXIT_FAILURE;
  }

  std::cout << "All went OK :D" << std::endl;
  return EXIT_SUCCESS;
}
