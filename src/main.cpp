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

    glm::mat4 modelMat1 = glm::mat4(1);
    glm::mat4 modelMat2 = glm::mat4(1);

    modelMat2 = glm::translate(modelMat1, glm::vec3( 1,0,0));
    modelMat1 = glm::translate(modelMat1, glm::vec3(-1,0,0));

    modelMat2 = glm::rotate(modelMat2,
                            glm::radians(90.0f),          // Rotation angle
                            glm::vec3(0.0f, 0.0f, 1.0f)); // Up axis

    uint32_t dragonIdx1 = renderer.createObject("../Models/StanfordDragonWithUvs.obj", modelMat1);
    uint32_t dragonIdx2 = renderer.createObject("../Models/StanfordDragonWithUvs.obj", modelMat2);

    uint32_t newMaterialIdx = renderer.createMaterial(DEFAULT_VERT, DEFAULT_FRAG, "../Textures/ColorTestTex.png");

    renderer.setObjMaterial(dragonIdx1, newMaterialIdx);
    renderer.setObjMaterial(dragonIdx2, DEFAULT_MATERIAL_IDX);
    //renderer.loadTextureToMaterial("../Textures/ColorTestTex.png", DEFAULT_MATERIAL_IDX);

    auto rotateCB = [](const float _deltaTime, glm::mat4& _model)
    {
      _model = glm::rotate(_model,
                           _deltaTime * glm::radians(90.0f), // Rotation angle (90° per second)
                           glm::vec3(0.0f, 0.0f, 1.0f));     // Up axis
    };

    auto jumpingCB = [](const float _deltaTime, glm::mat4& _model)
    {
      auto currentTime = glfwGetTime();

      _model = glm::translate(_model,
                              _deltaTime * static_cast<float>(sin(currentTime)) * glm::vec3(0,0,1));
    };

    renderer.setObjUpdateCB(dragonIdx1, rotateCB);
    renderer.setObjUpdateCB(dragonIdx2, jumpingCB);

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
