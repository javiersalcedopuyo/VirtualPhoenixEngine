#include "VPRenderer.hpp"

static double s_scrollY = 0;

// To avoid de compiler complaining
#define NOT_USED(x) ( (void)(x) )

int main()
{
  vpe::Renderer renderer;

  std::cout << "Starting..." << std::endl;

  glm::vec3 cameraUp      = UP;
  glm::vec3 cameraPos     = glm::vec3(0, 1, -4);
  glm::vec3 cameraForward = FRONT;

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
    renderer.addLight(light1);

    glm::mat4 modelMat1 = glm::mat4(1);
    glm::mat4 modelMat2 = glm::mat4(1);

    modelMat1 = glm::translate(modelMat1, glm::vec3(-1, 0, 0));
    modelMat2 = glm::translate(modelMat2, glm::vec3( 1, 0, 0));

    //modelMat1 = glm::scale(modelMat1, glm::vec3(0.5));
    //modelMat2 = glm::scale(modelMat2, glm::vec3(0.5));

    modelMat1 = glm::rotate(modelMat1,
                            glm::radians(90.0f),          // Rotation angle
                            UP); // Up axis
    modelMat2 = glm::rotate(modelMat2,
                            glm::radians(90.0f),          // Rotation angle
                            UP); // Up axis

    uint32_t dragonIdx1 = renderer.createObject("../Models/teapot.obj", modelMat1);
    uint32_t dragonIdx2 = renderer.createObject("../Models/dragon.obj", modelMat2);

    uint32_t newMaterialIdx = renderer.createMaterial(vpe::DEFAULT_VERT,
                                                      vpe::DEFAULT_FRAG,
                                                      "../Textures/ColorTestTex.png");

    renderer.setObjMaterial(dragonIdx1, newMaterialIdx);
    renderer.setObjMaterial(dragonIdx2, vpe::DEFAULT_MATERIAL_IDX);
    //renderer.loadTextureToMaterial("../Textures/ColorTestTex.png", DEFAULT_MATERIAL_IDX);

    auto rotateCB = [](const float _deltaTime, glm::mat4& _model)
    {
      _model = glm::rotate(_model, _deltaTime * glm::radians(90.0f), UP);
    };

    //auto jumpingCB = [](const float _deltaTime, glm::mat4& _model)
    //{
    //  auto currentTime = glfwGetTime();
    // _model = glm::translate(_model, 0.5f * UP * _deltaTime * static_cast<float>(sin(currentTime)));
    //};

    auto jumpRotCB = [](const float _deltaTime, glm::mat4& _model)
    {
      auto currentTime = glfwGetTime();

      _model = glm::translate(_model, 0.5f * UP * _deltaTime * static_cast<float>(sin(currentTime)));
      _model = glm::rotate(_model, _deltaTime * glm::radians(90.0f), UP);
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
