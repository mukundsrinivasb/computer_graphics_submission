#ifndef INPUT_HPP
#define INPUT_HPP
#include <GLFW/glfw3.h>

struct animal_state {
    int xPos = 0;
    int yPos = 0;
    int velocity = 0;
    int rotation = 0;
    float currentFrame = 0;
    const int maxVelocity = 100;
    const int acceleration = 2;
    const int rotationSpeed = 2;
    const float frameSpeed = 0.002f;
};

void handleInput(GLFWwindow *window, animal_state& state);

#endif