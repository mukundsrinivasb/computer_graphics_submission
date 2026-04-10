#include "model.hpp"
#include "render.hpp"
#include "input.hpp"
#include <Windows.h>
#include <iostream>
#include <sstream>
#include <GLFW/glfw3.h>
#include <GL/glu.h>

using namespace std;

float mod (float a, float b) {
  while (a > b) {
    a -= b;
  }

  return a;
}

unique_ptr<ModelNode> updateModel(animal_state& state) {
  float rotationInRadians = state.rotation / 180.0f * 3.1415926535f;
  state.xPos += state.velocity * cosf(rotationInRadians);
  state.yPos += state.velocity * sinf(rotationInRadians);

  // Charlie: here is where I update the frame based on the velocity. Feel free to change the frameSpeed in input.hpp, idk what the correct value would be yet.
  // The value stays in the range [0, 1) (basically 0% to 100% of the way through the animation). Just have your buildAnimalModelAtTime function use this instead
  // of the frame.
  state.currentFrame = mod(state.currentFrame + state.velocity * state.frameSpeed, 1.0f);
  unique_ptr<ModelNode> newModel = buildAnimalModelAtTime(state.currentFrame);

  // rotate model here

  return newModel;
}

int main(int argc, const char **argv) {
    cout << "========================================" << endl;
    cout << " Task 3: Cubism Animal" << endl;
    cout << " Hierarchical Modelling" << endl;
    cout << "========================================" << endl << endl;

    // use ./task3_cubism_animal [fps] to generate [fps*2] .obj files - each frame of the 2-"second" walk animation loop
    // currently I'm using "second" to just mean "unit of time" - it has nothing to do with actual seconds just yet, that 
    // will be handled when the model can actually animate on the screen rather than just generate a .obj file per frame
    float fps = 1;
    bool cycle = false;
    if (argc > 1){
      cycle = true;
      fps = max(1.f, stof(argv[1]));
    }
    auto animal = buildAnimalModelAtTime(0);
    animal_state state;
    

    cout << "--- Model Hierarchy ---" << endl;
    printHierarchy(animal.get());
    cout << endl;

    vector<Eigen::Vector3f> allVertices;
    vector<Triangle> allFaces;
    collectMeshes(animal.get(), Eigen::Matrix4f::Identity(), allVertices, allFaces);

    exportOBJ("../model/cubism_animal.obj", allVertices, allFaces);

    // this is just used for testing for now, but shows you how the model should be generated
    // each frame to show the motion
    if (cycle){
      for (int frame = 0; frame < fps*2; frame++){
        float time = frame/fps; // = frame * 1/fps = frame * (how long (in "seconds") one frame should be on screen for)
        animal = buildAnimalModelAtTime(time);

        // Here is probably where the transformation matrix corresponding to the dog's position according to 
        // Sam's input handling would be applied

        vector<Eigen::Vector3f> allVertices;
        vector<Triangle> allFaces;
        collectMeshes(animal.get(), Eigen::Matrix4f::Identity(), allVertices, allFaces);

        // remove/comment out this last bit when the animation can be visualised in OpenGL directly
        stringstream path;
        path << "../model/cubism_animal_" << frame << ".obj"; //names each .obj file according to its frame number
        exportOBJ(path.str(), allVertices, allFaces);
      }
    }
    

    if(!glfwInit())
      return -1;
      
    GLFWwindow* window = glfwCreateWindow(800,800,"Render Animal",NULL,NULL);
    if(!window){
      glfwTerminate();
      return -1;
    }

    glfwMakeContextCurrent(window);
    glEnable(GL_DEPTH_TEST);
    
    // Set up the Lens
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, 1.0f, 0.1f, 100.0f);

    while(!glfwWindowShouldClose(window)){
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      handleInput(window, state);
      animal = updateModel(state);
      
      // Set up the Camera
      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();
      gluLookAt(
          5.0f, 3.0f, 5.0f,
          0.0f, 0.0f, 0.0f,
          0.0f, 1.0f, 0.0f
      );
      
      // Render the animal
      renderNode(animal.get());
      
      glfwSwapBuffers(window);
      glfwPollEvents();
    }
    
    glfwTerminate();
    return 0;
}
