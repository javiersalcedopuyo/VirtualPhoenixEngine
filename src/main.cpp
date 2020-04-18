#include "VPRenderer.hpp"

static double s_scrollY = 0;

// To avoid de compiler complaining
#define NOT_USED(x) ( (void)(x) )
#define PTR_NOT_USED(x) ( (void*)(x) )

int main()
{
  vpe::Renderer renderer;

  std::cout << "Starting..." << std::endl;

  glm::vec3 cameraUp      = vpe::UP;
  glm::vec3 cameraPos     = glm::vec3(0, 1, -4);
  glm::vec3 cameraForward = vpe::FRONT;

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

    if (renderer.m_pUserInputController == nullptr)
      std::cout << "WARNING: No User Input Controller!" << std::endl;
    else
    {
      renderer.m_pUserInputController->m_pScrollY = &s_scrollY;
      renderer.m_pUserInputController->setCameraMovementCB( vpe::callbacks::cameraMovementWASD );
      //renderer.m_pUserInputController->setCameraMovementCB( VPCallbacks::cameraMovementArrows );
    }

    vpe::Light light1;
    light1.ubo.color     = glm::vec3(1, 0.5, 0.5);
    light1.ubo.position  = glm::vec3(-1, 2, 0);
    light1.ubo.intensity = 1.5;
    renderer.addLight(light1);

    light1.ubo.color     = glm::vec3(0.5, 0.5, 1);
    light1.ubo.position  = glm::vec3(1, 2, -1);
    light1.ubo.intensity = 1.5;

    uint32_t dragonIdx1 = renderer.createObject("../Models/teapot.obj");
    renderer.transformObject(dragonIdx1, glm::vec3(-1,0,0), vpe::TransformOperation::TRANSLATE);
    //renderer.transformObject(dragonIdx1,
    //                         glm::radians(90.0f) * vpe::UP,
    //                         vpe::TransformOperation::ROTATE_EULER);

    uint32_t newMaterialIdx = renderer.createMaterial(vpe::DEFAULT_VERT,
                                                      vpe::DEFAULT_FRAG,
                                                      "../Textures/ColorTestTex.png");

    renderer.setObjMaterial(dragonIdx1, newMaterialIdx);

    uint32_t dragonIdx2 = renderer.createObject("../Models/dragon.obj");
    renderer.transformObject(dragonIdx2, glm::vec3(1,0,0), vpe::TransformOperation::TRANSLATE);
    //renderer.transformObject(dragonIdx2,
    //                         glm::radians(90.0f) * vpe::UP,
    //                         vpe::TransformOperation::ROTATE_EULER);

    renderer.setObjMaterial(dragonIdx2, vpe::DEFAULT_MATERIAL_IDX);
    renderer.loadTextureToMaterial("../Textures/UVTestTex.png", vpe::DEFAULT_MATERIAL_IDX);

    renderer.addLight(light1);

    auto rotateCB = [](const float _deltaTime, vpe::Transform& _transform)
    {
      _transform.rotate( _deltaTime * glm::radians(90.0f) * vpe::UP );
    };

    auto jumpRotCB = [](const float _deltaTime, vpe::Transform& _transform)
    {
      double currentTime = glfwGetTime();

      _transform.translate( 0.5f * vpe::UP * _deltaTime * static_cast<float>(sin(currentTime)) );
      _transform.rotate( _deltaTime * glm::radians(90.0f) * vpe::UP );
    };

    renderer.setObjUpdateCB(dragonIdx1, rotateCB);
    renderer.setObjUpdateCB(dragonIdx2, jumpRotCB);

    renderer.renderLoop();
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
