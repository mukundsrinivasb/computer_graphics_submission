#include <iostream>
#include "input.hpp"

using namespace std;

// uses code from https://www.opengl-tutorial.org/beginners-tutorials/tutorial-6-keyboard-and-mouse/

void handleInput(GLFWwindow *window, animal_state& state) {
    // handle motion
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        state.velocity = min(state.maxVelocity, state.velocity + state.acceleration);
    }
    else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        state.velocity = max(-state.maxVelocity, state.velocity - state.acceleration);
    }
    else if (state.velocity < 0) {
        state.velocity += state.acceleration;
    }
    else if (state.velocity > 0) {
        state.velocity -= state.acceleration;
    }

    // handle rotation
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        state.rotation = (state.rotation - state.rotationSpeed) % 360;
    }
    else if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        state.rotation = (state.rotationSpeed + state.rotation) % 360;
    }
}