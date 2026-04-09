#include "model.hpp"
#include "render.hpp"
#include <iostream>
#include <GLFW/glfw3.h>
#include <GL/glu.h>

using namespace std;

int main() {
    cout << "========================================" << endl;
    cout << " Task 3: Cubism Animal" << endl;
    cout << " Hierarchical Modelling" << endl;
    cout << "========================================" << endl << endl;

    auto animal = buildAnimalModel();

    cout << "--- Model Hierarchy ---" << endl;
    printHierarchy(animal.get());
    cout << endl;

    vector<Eigen::Vector3f> allVertices;
    vector<Triangle> allFaces;
    collectMeshes(animal.get(), Eigen::Matrix4f::Identity(), allVertices, allFaces);

    exportOBJ("../model/cubism_animal.obj", allVertices, allFaces);

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
