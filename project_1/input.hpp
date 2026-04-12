#ifndef INPUT_HPP
#define INPUT_HPP

// #include <Windows.h>
#include <GLFW/glfw3.h>
#include <GL/glu.h>

struct animal_state {
    int xPos = 0;
    int yPos = 0;
    int velocity = 0;
    int rotation = 0;
    float currentFrame = 0;
    int maxVelocity = 100;
    int acceleration = 2;
    int rotationAcceleration = 2;
    float frameSpeed = 0.002f;
};

void handleInput(GLFWwindow *window, animal_state& state);

#endif
