#include "Renderer.hpp"
#include "Scene.hpp"
#include "Triangle.hpp"
#include "Sphere.hpp"
#include "Vector.hpp"
#include "global.hpp"
#include <chrono>

float TASK_N=2; // 1.1, 1.2, 1.3, 2

// In the main function of the program, we create the scene (create objects and
// lights) as well as set the options for the render (image width and height,
// maximum recursion depth, field-of-view, etc.). We then call the render
// function().
int main(int argc, char** argv)
{
    if (argc>=2)
        TASK_N=(float)atof(argv[1]);

    // Charlie: new camera defaults
    float eye_x = 0, eye_y = 1, eye_z = -12.9; 

    // Charlie: new scene defaults
    int width = 512, height = 512; // Charlie: eventually needs to be 1920:1080 (or something of this ratio - I've been using 480:270)
    int spp = 16; // change to >64 for final render
    float fov = 40;

    //Anusha: read user input from command line if provided
    //Anusha: usage: ./RayTracing <task> <eye_x> <eye_y> <eye_z> <width> <height> <spp> <fov>
    //Anusha: e.g:   ./RayTracing 2 278 273 -800 256 256 64 40
    if (argc >= 5) {
        eye_x = atof(argv[2]);
        eye_y = atof(argv[3]);
        eye_z = atof(argv[4]);
    }
    if (argc >= 7) {
        width  = atoi(argv[5]);
        height = atoi(argv[6]);
    }
    if (argc >= 8) spp = atoi(argv[7]);
    if (argc >= 9) fov = atof(argv[8]);

    //Anusha: print settings so its easy to verify before waiting for the render
    printf("Camera: (%.1f, %.1f, %.1f)  Resolution: %dx%d  SPP: %d  FOV: %.1f\n",
           eye_x, eye_y, eye_z, width, height, spp, fov);

    Scene scene(width, height); 

    scene.RussianRoulette = 0.8;
    scene.spp = spp;
    scene.fov = fov;

    //Anusha: pass camera position into scene so Renderer.cpp can use it
    scene.eye_pos = Vector3f(eye_x, eye_y, eye_z);

    // ---------------------- Charlie: chess scene -------------------------------------------

    // Charlie: creating new materials for our scene
    Material* gold = new Material(DIFF_MIRROR, Vector3f(0.5, 0.4, 0)); //working
    gold->Kd=0.8;
    gold->Ks=0.2;
    gold->specularExponent=32;
    gold->ior=5; // higher number = more reflection, not refraction (this is just a note to self)

    Material* mirror_tile = new Material(DIFF_MIRROR, Vector3f(0.03, 0.03, 0.03)); //working
    mirror_tile->Kd=1;
    mirror_tile->Ks=0;
    mirror_tile->ior=6; 

    Material* diffuse_tile = new Material(DIFF_MIRROR, Vector3f(0.07, 0.07, 0.07)); // working 
    diffuse_tile->Kd=1;
    diffuse_tile->Ks=0;
    diffuse_tile->ior=1.5;

    Material* diffuse_grey = new Material(DIFFUSE, Vector3f(0.07, 0.07, 0.07)); // working
    diffuse_tile->Kd=0.7;
    diffuse_tile->Ks=0.3;

    Material* dark_pawn = new Material(DIFF_MIRROR, Vector3f(0.00, 0.00, 0.00)); // working
    dark_pawn->Kd=0.9;
    dark_pawn->Ks=0.1;
    dark_pawn->specularExponent=1;
    dark_pawn->ior=1.1; // 1.1

    Material* light_pawn = new Material(DIFF_MIRROR, Vector3f(0.8, 0.8, 0.8)); // working
    light_pawn->Kd=0.5; 
    light_pawn->Ks=0.5;
    light_pawn->specularExponent=100;
    light_pawn->ior=8; // 8

    MeshTriangle light_floor("../models/chessScene/light_floor.obj", 0, diffuse_tile); 
    MeshTriangle dark_floor("../models/chessScene/dark_floor.obj", 0, mirror_tile); 
    MeshTriangle back_wall("../models/chessScene/back_wall.obj", Vector3f(0, 0, 0), diffuse_grey);

    MeshTriangle king("../models/chessScene/king_piece.obj", Vector3f(0, 0, 7), gold);

    // pawns on the left (light) - numbered for how close they are to the king (1 = closest)
    MeshTriangle light_pawn_1("../models/chessScene/pawn_piece.obj", Vector3f(2, 0, 5), light_pawn);
    MeshTriangle light_pawn_2("../models/chessScene/pawn_piece.obj", Vector3f(2, 0, 3), light_pawn);
    MeshTriangle light_pawn_3("../models/chessScene/pawn_piece.obj", Vector3f(2, 0, 1), light_pawn);
    MeshTriangle light_pawn_4("../models/chessScene/pawn_piece.obj", Vector3f(2, 0, -1), light_pawn);
    MeshTriangle light_pawn_5("../models/chessScene/pawn_piece.obj", Vector3f(2, 0, -3), light_pawn);
    MeshTriangle light_pawn_6("../models/chessScene/pawn_piece.obj", Vector3f(2, 0, -5), light_pawn);
    MeshTriangle light_pawn_7("../models/chessScene/pawn_piece.obj", Vector3f(2, 0, -7), light_pawn);

    // pawns on the right (dark) - numbered for how close they are to the king (1 = closest)
    MeshTriangle dark_pawn_1("../models/chessScene/pawn_piece.obj", Vector3f(-2, 0, 5), dark_pawn);
    MeshTriangle dark_pawn_2("../models/chessScene/pawn_piece.obj", Vector3f(-2, 0, 3), dark_pawn);
    MeshTriangle dark_pawn_3("../models/chessScene/pawn_piece.obj", Vector3f(-2, 0, 1), dark_pawn);
    MeshTriangle dark_pawn_4("../models/chessScene/pawn_piece.obj", Vector3f(-2, 0, -1), dark_pawn);
    MeshTriangle dark_pawn_5("../models/chessScene/pawn_piece.obj", Vector3f(-2, 0, -3), dark_pawn);
    MeshTriangle dark_pawn_6("../models/chessScene/pawn_piece.obj", Vector3f(-2, 0, -5), dark_pawn);
    MeshTriangle dark_pawn_7("../models/chessScene/pawn_piece.obj", Vector3f(-2, 0, -7), dark_pawn);

    scene.Add(&light_floor);
    scene.Add(&dark_floor);
    scene.Add(&back_wall);
    scene.Add(&king);
    scene.Add(&light_pawn_1);
    scene.Add(&light_pawn_2);
    scene.Add(&light_pawn_3);
    scene.Add(&light_pawn_4);
    scene.Add(&light_pawn_5);
    scene.Add(&light_pawn_6);
    scene.Add(&light_pawn_7);
    scene.Add(&dark_pawn_1);
    scene.Add(&dark_pawn_2);
    scene.Add(&dark_pawn_3);
    scene.Add(&dark_pawn_4);
    scene.Add(&dark_pawn_5);
    scene.Add(&dark_pawn_6);
    scene.Add(&dark_pawn_7);
   
    Material* light = new Material(EMIT, Vector3f(1));
    light->m_emission=60; 

    Material* dim_light = new Material(EMIT, Vector3f(1));
    dim_light->m_emission=30;

    MeshTriangle light_behind("../models/chessScene/light.obj", Vector3f(0, 15, -20), dim_light); 
    MeshTriangle light_middle("../models/chessScene/light.obj", Vector3f(0, 0, 8), light); // working! 


    scene.Add(&light_middle);
    scene.Add(&light_behind);
    

    // -----------------------------------------------------------------------

    scene.buildBVH();

    Renderer r;

    auto start = std::chrono::system_clock::now();
    r.Render(scene);
    auto stop = std::chrono::system_clock::now();

    std::cout << "Render complete: \n";
    std::cout << "Time taken: " << std::chrono::duration_cast<std::chrono::hours>(stop - start).count() << " hours\n";
    std::cout << "          : " << std::chrono::duration_cast<std::chrono::minutes>(stop - start).count() << " minutes\n";
    std::cout << "          : " << std::chrono::duration_cast<std::chrono::seconds>(stop - start).count() << " seconds\n";

    return 0;
}
