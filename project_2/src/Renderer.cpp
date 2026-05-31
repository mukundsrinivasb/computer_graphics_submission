//
// Created by goksu on 2/25/20.
//

#include <fstream>
#include <sstream>
#include "Ray.hpp"
#include "Scene.hpp"
#include "Renderer.hpp"
#include "Material.hpp"
#include "Vector.hpp"
#ifdef _OPENMP
#include <omp.h>
#endif


inline float deg2rad(const float& deg) { return deg * M_PI / 180.0; }

const float EPSILON = 1e-2;

// The main render function. This where we iterate over all pixels in the image,
// generate primary rays and cast these rays into the scene. The content of the
// framebuffer is saved to a file.
void Renderer::Render(const Scene& scene)
{
    std::vector<Vector3f> framebuffer(scene.width * scene.height);

    float scale = tan(deg2rad(scene.fov * 0.5));
    float imageAspectRatio = scene.width / (float)scene.height;

    //Anusha: get eye_pos from scene instead of hardcoding it
    //Anusha: this lets main.cpp control camera position via command line
    Vector3f eye_pos = scene.eye_pos;

    std::cout << "SPP: " << scene.spp << "\n";

    float progress = 0.0f;

#pragma omp parallel for num_threads(8) // use multi-threading for speedup if openmp is available
    for (uint32_t j = 0; j < scene.height; ++j) {
        for (uint32_t i = 0; i < scene.width; ++i) {

            int m = i + j * scene.width;  // pixel index
            if(scene.spp==1){
                // TODO: task 1.1 pixel projection
                // std::cout << (((float) i + 0.5f) / scene.height * 2 - 1) << "\n";
                Ray r = Ray(eye_pos, Vector3f((((float) i + 0.5f) / scene.height * 2 - 1) * scale, (((float) j + 0.5f) / scene.width * 2 - 1) * scale, 1));
                Vector3f pixel_color = scene.castRayBidirectional(r, 0, scene.spp);
                framebuffer[framebuffer.size() - 1 - m] = pixel_color;
            }else {
                // TODO: task 2 multi-sampling (anti-aliasing)
                Vector3f total = {0, 0, 0};
                for (int n = 0; n < scene.spp; n++) {
                    Vector3f direction = Vector3f((((float) i + (get_random_float())) / scene.height * 2 - 1) * scale, 
                        (((float) j + (get_random_float())) / scene.width * 2 - 1) * scale, 1);
                    // std::cout << direction << "\n";
                    Ray r = Ray(eye_pos, direction);
                    Vector3f pixel_color = scene.castRayBidirectional(r, 0, scene.spp);
                    total += pixel_color;
                }

                // if (dotProduct(total / scene.spp, Vector3f(1, 1, 1)) < 0.1) {
                //     std::cout << total / scene.spp << "\n";
                // }
                
                framebuffer[framebuffer.size() - 1 - m] = total;
            }
        }
        progress += 1.0f / (float)scene.height;
        UpdateProgress(progress);
    }
    UpdateProgress(1.f);

    // save framebuffer to file
    std::stringstream ss;
    ss << "binary_task" << TASK_N<<".ppm";
    std::string str = ss.str();
    const char* file_name = str.c_str();
    std::cout << "Storing to file" << file_name << "\n";
    FILE* fp = fopen(file_name, "wb");
    (void)fprintf(fp, "P6\n%d %d\n255\n", scene.width, scene.height);
    for (auto i = 0; i < scene.height * scene.width; ++i) {
        static unsigned char color[3];
        color[0] = (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].x), 0.6f));
        color[1] = (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].y), 0.6f));
        color[2] = (unsigned char)(255 * std::pow(clamp(0, 1, framebuffer[i].z), 0.6f));
        fwrite(color, 1, 3, fp);
    }
    fclose(fp);
}