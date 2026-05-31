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
    float eye_x = 0, eye_y = 0.95, eye_z = -11.5; 

    // Charlie: new scene defaults (as they should be for the final render)
    int width = 1920, height = 1080; // Charlie: use something smaller like 480:270 for quick debugging
    int spp = 64; 
    float fov = 30;

    //Anusha: king position defaults
    float king_x = 0, king_y = 0, king_z = 7;

    //Anusha: king material colour defaults (gold)
    float king_r = 0.5, king_g = 0.4, king_b = 0;

    //Anusha: light_middle position defaults
    float lx = 0, ly = 0, lz = 8;

    //Anusha: background colour defaults (black)
    float bg_r = 0, bg_g = 0, bg_b = 0;

    //Anusha: light emission strength defaults
    float light_emission = 60;
    float dim_light_emission = 30;

    //Anusha: back wall colour defaults (dark grey, same as original)
    float wall_r = 0.07, wall_g = 0.07, wall_b = 0.07;

    // Charlie: Adding option to use less smooth models for quicker debugging
    bool smooth = true; // make this FALSE for quick debugging, TRUE for final render
    std::string pawn_path;
    std::string king_path; 

    // Sam: add option for turning shadows on and off
    bool shadows_on = true;

    //Anusha: read user input from command line if provided
    //Anusha: usage: ./RayTracing <task> <eye_x> <eye_y> <eye_z> <width> <height> <spp> <fov> <king_x> <king_y> <king_z> <king_r> <king_g> <king_b> <lx> <ly> <lz> <bg_r> <bg_g> <bg_b> <light_emission> <dim_light_emission> <wall_r> <wall_g> <wall_b> <smooth> <shadows_on>
    //Anusha: e.g:   ./RayTracing 2 0 1 -12.9 256 256 64 40 0 0 7 0.5 0.4 0 0 0 8 0 0 0 60 30 0.07 0.07 0.07 0 1
    //Anusha: smooth: 1 = smooth models (slow), 0 = low poly models (fast for debugging)
    //Sam: shadows_on: 1 for normal shadow generation, 0 for no shadows
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

    //Anusha: king position from command line
    if (argc >= 12) {
        king_x = atof(argv[9]);
        king_y = atof(argv[10]);
        king_z = atof(argv[11]);
    }

    //Anusha: king material colour from command line
    if (argc >= 15) {
        king_r = atof(argv[12]);
        king_g = atof(argv[13]);
        king_b = atof(argv[14]);
    }

    //Anusha: light position from command line
    if (argc >= 18) {
        lx = atof(argv[15]);
        ly = atof(argv[16]);
        lz = atof(argv[17]);
    }

    //Anusha: background colour from command line
    if (argc >= 21) {
        bg_r = atof(argv[18]);
        bg_g = atof(argv[19]);
        bg_b = atof(argv[20]);
    }

    //Anusha: light emission strength from command line
    if (argc >= 23) {
        light_emission = atof(argv[21]);
        dim_light_emission = atof(argv[22]);
    }

    //Anusha: back wall colour from command line
    if (argc >= 26) {
        wall_r = atof(argv[23]);
        wall_g = atof(argv[24]);
        wall_b = atof(argv[25]);
    }

    //Anusha: smooth model toggle from command line (1 = smooth, 0 = low poly)
    if (argc >= 27) {
        smooth = atoi(argv[26]) != 0;
    }

    //Sam: toggle shadows (1 = shadows on, 0 = shadows off)
    if (argc >= 28) {
        shadows_on = atoi(argv[27]) != 0;
    }

    //Anusha: print settings so its easy to verify before waiting for the render
    printf("Camera: (%.1f, %.1f, %.1f)  Resolution: %dx%d  SPP: %d  FOV: %.1f\n",
           eye_x, eye_y, eye_z, width, height, spp, fov);
    printf("King pos: (%.1f, %.1f, %.1f)  King colour: (%.2f, %.2f, %.2f)\n",
           king_x, king_y, king_z, king_r, king_g, king_b);
    printf("Light pos: (%.1f, %.1f, %.1f)\n", lx, ly, lz);
    printf("Background colour: (%.2f, %.2f, %.2f)\n", bg_r, bg_g, bg_b);
    printf("Light emission: %.1f  Dim light emission: %.1f\n", light_emission, dim_light_emission);
    printf("Back wall colour: (%.2f, %.2f, %.2f)\n", wall_r, wall_g, wall_b);
    printf("Smooth models: %s\n", smooth ? "yes" : "no");
    printf("Shadows on: %s\n", shadows_on ? "yes" : "no");

    Scene scene(width, height); 

    scene.RussianRoulette = 0.8;
    scene.spp = spp;
    scene.fov = fov;

    //Anusha: pass camera position into scene so Renderer.cpp can use it
    scene.eye_pos = Vector3f(eye_x, eye_y, eye_z);

    //Anusha: set background colour from user input (used when rays miss all objects)
    scene.backgroundColor = Vector3f(bg_r, bg_g, bg_b);

    // ---------------------- Charlie: chess scene -------------------------------------------

    // Charlie: creating new materials for our scene
    //Anusha: king colour now uses user input values
    Material* gold = new Material(DIFF_MIRROR, Vector3f(king_r, king_g, king_b));
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

    //Anusha: back wall colour now uses user input values
    Material* diffuse_grey = new Material(DIFFUSE, Vector3f(wall_r, wall_g, wall_b)); // working
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

    // determine which version of the models to use
    if (smooth) {
        king_path = "../models/chessScene/king_piece_smooth.obj";
        pawn_path = "../models/chessScene/pawn_piece_smooth.obj";
    } else {
        king_path = "../models/chessScene/king_piece.obj";
        pawn_path = "../models/chessScene/pawn_piece.obj";
    }

    //Anusha: king position now uses user input values
    MeshTriangle king(king_path, Vector3f(king_x, king_y, king_z), gold);

    // pawns on the left (light) - numbered for how close they are to the king (1 = closest)
    MeshTriangle light_pawn_1(pawn_path, Vector3f(2, 0, 5), light_pawn);
    MeshTriangle light_pawn_2(pawn_path, Vector3f(2, 0, 3), light_pawn);
    MeshTriangle light_pawn_3(pawn_path, Vector3f(2, 0, 1), light_pawn);
    MeshTriangle light_pawn_4(pawn_path, Vector3f(2, 0, -1), light_pawn);
    MeshTriangle light_pawn_5(pawn_path, Vector3f(2, 0, -3), light_pawn);
    MeshTriangle light_pawn_6(pawn_path, Vector3f(2, 0, -5), light_pawn);
    MeshTriangle light_pawn_7(pawn_path, Vector3f(2, 0, -7), light_pawn);

    // pawns on the right (dark) - numbered for how close they are to the king (1 = closest)
    MeshTriangle dark_pawn_1(pawn_path, Vector3f(-2, 0, 5), dark_pawn);
    MeshTriangle dark_pawn_2(pawn_path, Vector3f(-2, 0, 3), dark_pawn);
    MeshTriangle dark_pawn_3(pawn_path, Vector3f(-2, 0, 1), dark_pawn);
    MeshTriangle dark_pawn_4(pawn_path, Vector3f(-2, 0, -1), dark_pawn);
    MeshTriangle dark_pawn_5(pawn_path, Vector3f(-2, 0, -3), dark_pawn);
    MeshTriangle dark_pawn_6(pawn_path, Vector3f(-2, 0, -5), dark_pawn);
    MeshTriangle dark_pawn_7(pawn_path, Vector3f(-2, 0, -7), dark_pawn);

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
    //Anusha: light emission now uses user input
    light->m_emission = light_emission;

    Material* dim_light = new Material(EMIT, Vector3f(1));
    //Anusha: dim light emission now uses user input
    dim_light->m_emission = dim_light_emission;

    MeshTriangle light_behind("../models/chessScene/light.obj", Vector3f(0, 15, -20), dim_light); 
    //Anusha: light_middle position now uses user input values
    MeshTriangle light_middle("../models/chessScene/light.obj", Vector3f(lx, ly, lz), light); // working! 

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