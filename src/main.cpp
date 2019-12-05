#include "HelloTriangle.hpp"

int main()
{
  HelloTriangle app;

  std::cout << "Starting..." << std::endl;

  try
  {
    app.Run();
  }
  catch(const std::exception& e)
  {
    std::cerr << e.what() << '\n';
    return EXIT_FAILURE;
  }

  std::cout << "All went OK :D" << std::endl;
  return EXIT_SUCCESS;
}
