#include <stdlib.h>
#include <opencv2/opencv.hpp>
#include <Eigen/Eigen>
#include <SDL2/SDL_keyboard.h>

// Represents the current state of the animal
struct animal {
    int xPos = 0;
    int yPos = 0;
    int velocity = 0;
    int rotation = 0;
    float currentFrame = 0;
    int maxVelocity = 100;
};

int min(int a, int b) {
    if (a < b) {
        return a;
    }
    return b;
}

int max(int a, int b) {
    if (a > b) {
        return a;
    }
    return b;
}

int main() {
    animal animalState;

    while (true) {
        const Uint8 *currentKeys = SDL_GetKeyboardState(NULL); // get keyboard input

        // update velocity
        if (currentKeys[SDL_SCANCODE_W] == 1) {
            animalState.velocity = min(animalState.maxVelocity, animalState.velocity + 2);
        }
        else if (currentKeys[SDL_SCANCODE_S] == 1) {
            animalState.velocity = max(-animalState.maxVelocity, animalState.velocity - 2);
        }
        else if (animalState.velocity > 0) {
            animalState.velocity -= 1;
        }
        else if (animalState.velocity < 0) {
            animalState.velocity += 1;
        }

        // update rotation
        if (currentKeys[SDL_SCANCODE_A] == 1) {
            animalState.rotation -= 1;
        }
        else if (currentKeys[SDL_SCANCODE_D] == 1) {
            animalState.rotation += 1;
        }

        animalState.rotation %= 360;

        // update frame (currentFrame stays within the values [0, 1))
        animalState.currentFrame = animalState.currentFrame + 0.0002f * animalState.velocity;
        if (animalState.currentFrame >= 1) {
            animalState.currentFrame -= 1;
        }

        //get current model and orient it
        auto model = getModel(animalState.currentFrame);
        model.matrix *= getRotationMatrix(animalState.rotation);

        // update position
        moveAnimal(animalState);


        // display model
        displayModel(model, animalState);

        // Possibly add delay if model is updating too quickly?
    }
    return 0;
}