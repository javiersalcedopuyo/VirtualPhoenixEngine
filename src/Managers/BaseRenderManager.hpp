#ifndef MANAGER_HPP
#define MANAGER_HPP

#ifndef GLFW_INCLUDE_VULKAN
  #define GLFW_INCLUDE_VULKAN
#endif
#include <GLFW/glfw3.h>

// Error management
#include <stdexcept>
#include <iostream>

#include <vector>

class BaseRenderManager
{
protected:
  const VkDevice* m_pLogicalDevice;

public:
  BaseRenderManager() : m_pLogicalDevice(nullptr) {};
  ~BaseRenderManager()
  {
    m_pLogicalDevice = nullptr; // I'm not the owner of the pointer, so I cannot delete it
  };

  inline void setLogicalDevice(const VkDevice* _d) { m_pLogicalDevice = _d; }

  virtual void cleanUp() = 0;
};

#endif