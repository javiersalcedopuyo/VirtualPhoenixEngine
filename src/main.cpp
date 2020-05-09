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
    light1.ubo.color     = glm::vec3(1);
    light1.ubo.position  = glm::vec3(0, 2, -1);
    light1.ubo.intensity = 2.0f;
    renderer.addLight(light1);

    renderer.setMaterialTexture(vpe::DEFAULT_MATERIAL_IDX, "../Textures/ColorTestTex.png");

    const uint32_t cube1Idx = renderer.createObject("../Models/sphere.obj");
    renderer.transformObject(cube1Idx, glm::vec3(-1,0,0), vpe::TransformOperation::TRANSLATE);
    //renderer.transformObject(cube1Idx, 0.5f *glm::vec3(1), vpe::TransformOperation::SCALE);

    const uint32_t normalMapMatIdx = renderer.createMaterial(vpe::DEFAULT_VERT, vpe::DEFAULT_FRAG);
    renderer.setMaterialTexture(normalMapMatIdx, "../Textures/ColorTestTex.png");
    renderer.setMaterialNormalMap(normalMapMatIdx, "../Textures/BricksNormalMap.jpg");
    renderer.setObjMaterial(cube1Idx, normalMapMatIdx);

    const uint32_t cube2Idx = renderer.createObject("../Models/sphere.obj");
    renderer.transformObject(cube2Idx, glm::vec3(1,0,0), vpe::TransformOperation::TRANSLATE);
    //renderer.transformObject(cube2Idx, 0.5f *glm::vec3(1), vpe::TransformOperation::SCALE);

    auto rotateCB = [](const float _deltaTime, vpe::Transform& _transform)
    {
      _transform.rotate( _deltaTime * glm::radians(90.0f) * vpe::UP );
    };

    auto jumpRotCB = [](const float _deltaTime, vpe::Transform& _transform)
    {
      //double currentTime = glfwGetTime();

      //_transform.translate( 0.5f * vpe::UP * _deltaTime * static_cast<float>(sin(currentTime)) );
      _transform.rotate( _deltaTime * glm::radians(90.0f) * vpe::UP );
    };

    renderer.setObjUpdateCB(cube1Idx, rotateCB);
    renderer.setObjUpdateCB(cube2Idx, jumpRotCB);

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
