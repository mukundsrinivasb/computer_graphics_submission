#ifndef INPUT_HPP
#define INPUT_HPP

// #include <Windows.h>
#include <GLFW/glfw3.h>

struct animal_state {
    int xPos = 0;
    int yPos = 0;
    float velocity = 0;
    int rotation = 0;
    float currentFrame = 0;
    const float maxVelocity = 15;
    const float acceleration = 0.5f;
    const int rotationSpeed = 2;
    const float frameSpeed = 0.002f;
};

void handleInput(GLFWwindow *window, animal_state& state);

#endif
