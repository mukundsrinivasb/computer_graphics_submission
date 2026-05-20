//
// Created by Göksu Güvendiren on 2019-05-14.
//

#include "Scene.hpp"


void Scene::buildBVH() {
    printf(" - Generating BVH...\n\n");
    this->bvh = new BVHAccel(objects, 1, BVHAccel::SplitMethod::NAIVE);
}

Intersection Scene::intersect(const Ray &ray) const
{
    return this->bvh->Intersect(ray);
}


void Scene::sampleLight(Intersection &pos, float &pdf) const
{
    float emit_area_sum = 0;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        if (objects[k]->hasEmit()){
            pos.happened=true;  // area light that has emission exists
            emit_area_sum += objects[k]->getArea();
        }
    }
    float p = get_random_float() * emit_area_sum;
    emit_area_sum = 0;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        if (objects[k]->hasEmit()){
            emit_area_sum += objects[k]->getArea();
            if (p <= emit_area_sum){
                objects[k]->Sample(pos, pdf);
                break;
            }
        }
    }
}

Vector3f Scene::getRandomDirection(Vector3f hitPoint, Vector3f N, float &pdf) const {
    Intersection i = Intersection();
    i.normal = -N;
    Sphere *interSphere = new Sphere(hitPoint, 1.0f, new Material(GLASS, Vector3f(1)));

    while (dotProduct(i.normal, N) < 0) {
        interSphere->Sample(i, pdf);
    }

    return i.coords;
}

Vector3f Scene::castRayBidirectional(const Ray &ray, int depth) const {
    Vector3f hitColor = Vector3f(0);
    auto inter = intersect(ray);  // find the cloest intersection of the ray and the objects
    if (!inter.happened)return backgroundColor;  // if no intersection, return background color

    Vector3f hitPoint = inter.coords;  // the intersection point
    Vector3f N = inter.normal; // normal
    Vector2f st = inter.tcoords; // texture coordinates (u, v)
    Vector3f dir = ray.direction;  // ray direction

    int numRaysFromLight = 1;
    while (get_random_float() <= RussianRoulette) {
        numRaysFromLight++;
    }

    int numRaysFromCamera = 1;
    while (get_random_float() <= RussianRoulette) {
        numRaysFromCamera++;
    }

    Intersection lightInter;
    float lightPdf;
    sampleLight(lightInter, lightPdf);

    Intersection lightInters[numRaysFromLight];
    Intersection cameraInters[numRaysFromCamera];
    Vector3f inverseDirections[numRaysFromCamera + numRaysFromLight];
    int totalRaysFromCamera = numRaysFromCamera;
    cameraInters[0] = inter;
    inverseDirections[0] = ray.direction_inv;

    Ray currentRay = ray;
    float pdf;

    // Still need to implement glass and mirrors
    for (int i = 1; i < numRaysFromCamera; i++) {
        Vector3f direction = getRandomDirection(hitPoint, N, pdf);
        inverseDirections[i] = direction;
        Intersection nextInter = intersect(Ray(hitPoint + N * EPSILON, direction));
        if (!nextInter.happened) {
            numRaysFromCamera = i - 1;
            break;
        }
        cameraInters[i] = nextInter;
        hitPoint = nextInter.coords;
        N = nextInter.normal;
    }

    hitPoint = lightInter.coords;
    N = lightInter.normal;


    for (int i = 0; i < numRaysFromLight; i++) {
        Vector3f direction = getRandomDirection(hitPoint, N, pdf);
        inverseDirections[totalRaysFromCamera + i] = direction;
        Intersection nextInter = intersect(Ray(hitPoint + N * EPSILON, direction));
        if (!nextInter.happened) {
            numRaysFromLight = i - 1;
            break;
        }
        lightInters[i] = nextInter;
        hitPoint = nextInter.coords;
        N = nextInter.normal;
    }

    Vector3f cameraColors[numRaysFromCamera];
    Vector3f lightColors[numRaysFromLight];

    lightColors[0] = lightInter.material->getEmission() / 255;
    // std::cout << "Initial Emission: " << lightColors[0] << "\n";

    // Vector3f nextColor = castRay(nextRay, depth)  * theta * 2 * std::max(pdf, EPSILON) / RussianRoulette;
    // Vector3f newColor = getIntersectionColor(ray, inter, hitPoint, N, st, dir) + nextColor / 255;

    Vector3f v;
    Intersection currentInter;
    float theta;

    // Precompute side from light since those won't change
    for (int i = 1; i < numRaysFromLight; i++) {
        v = inverseDirections[totalRaysFromCamera + i].normalized();
        currentInter = lightInters[i];
        theta = abs(dotProduct(-inverseDirections[totalRaysFromCamera + i - 1].normalized(), inverseDirections[totalRaysFromCamera + i].normalized()));
        // std::cout << totalRaysFromCamera + numRaysFromLight << ", " << totalRaysFromCamera << "\n";
        lightColors[i] = lightColors[i - 1] * theta * 2 * std::max(pdf, EPSILON) / RussianRoulette;
        // lightColors[i] = lightColors[i - 1] * (Vector3f(1, 1, 1) * currentInter.material->Ks * dotProduct(v, v - 2 * dotProduct(v, currentInter.normal.normalized()) * 
        //     currentInter.normal.normalized()) + currentInter.material->Kd * currentInter.material->m_color * 
        //     dotProduct(currentInter.normal.normalized(), (v * -1).normalized())) / 255;
            
        // std::cout << "Light: " << lightColors[i] << "\n";
    }

    // std::cout << "Next Emission: " << lightColors[1] << "\n";

    // We can also precompute the camera colors and will just have to multiply them by the previous light color
    v = inverseDirections[numRaysFromCamera - 1].normalized();
    currentInter = cameraInters[numRaysFromCamera - 1];
    cameraColors[numRaysFromCamera - 1] = Vector3f(1, 1, 1);
    // cameraColors[numRaysFromCamera - 1] = Vector3f(1, 1, 1) * currentInter.material->Ks * dotProduct(v, v - 2 * dotProduct(v, currentInter.normal.normalized()) * 
    //     currentInter.normal.normalized()) + currentInter.material->Kd * currentInter.material->m_color * 
    //     dotProduct(currentInter.normal.normalized(), (v * -1).normalized()) / 255;

    for (int i = numRaysFromCamera - 2; i >= 0; i--) {
        v = inverseDirections[i].normalized();
        currentInter = cameraInters[i];
        theta = abs(dotProduct(-inverseDirections[i + 1].normalized(), inverseDirections[i].normalized()));
        cameraColors[i] = cameraColors[i + 1] * theta * 2 * std::max(pdf, EPSILON) / RussianRoulette;
        // cameraColors[i] = cameraColors[i + 1] * (Vector3f(1, 1, 1) * currentInter.material->Ks * dotProduct(v, v - 2 * dotProduct(v, currentInter.normal.normalized()) * 
        //     currentInter.normal.normalized()) + currentInter.material->Kd * currentInter.material->m_color * 
        //     dotProduct(currentInter.normal.normalized(), (v * -1).normalized())) / 255;
        // std::cout << "Camera: " << cameraColors[i] << "\n";
    }


    for (int i = 0; i < numRaysFromCamera; i++) {
        for (int j = 0; j < numRaysFromLight; j++) {
            Intersection testOccluded = intersect(Ray(cameraInters[i].coords + cameraInters[i].normal.normalized() * EPSILON, lightInters[j].coords - (cameraInters[i].coords + cameraInters[i].normal.normalized() * EPSILON)));
            if (testOccluded.happened && testOccluded.obj == lightInters[j].obj) {
                theta = abs(dotProduct(-inverseDirections[i].normalized(), inverseDirections[totalRaysFromCamera + j].normalized()));
                Vector3f currentCameraColor = lightColors[j] * theta * 2 * std::max(pdf, EPSILON) / RussianRoulette * cameraColors[i]; // might be cameraColors[i-1]
                Vector3f albedo = cameraInters[i].obj->evalDiffuseColor(cameraInters[i].tcoords);
                Vector3f f = cameraInters[i].material->eval(-inverseDirections[i],cameraInters[i].normal,albedo);
                hitColor += f * cameraColors[i] * lightColors[j];
                // std::cout << cameraInters[i].tcoords.x << ", " << cameraInters[i].tcoords.y << "\n";
            }
        }
    }

    hitColor = hitColor / (numRaysFromCamera * numRaysFromLight);

    // std::cout << hitColor << "\n";

    if (hitColor.x < 0 || hitColor.y < 0 || hitColor.z < 0 || hitColor.x >= 0.8 || hitColor.y >= 0.8 || hitColor.z >= 0.8) {
        std::cout << hitColor << "\n";
    }
    

    return hitColor;
}

// Implementation of Path Tracing
Vector3f Scene::castRay(const Ray &ray, int depth) const
{   
    Vector3f hitColor = Vector3f(0);
    auto inter = intersect(ray);  // find the cloest intersection of the ray and the objects
    if (!inter.happened)return backgroundColor;  // if no intersection, return background color

    Vector3f hitPoint = inter.coords;  // the intersection point
    Vector3f N = inter.normal; // normal
    Vector2f st = inter.tcoords; // texture coordinates (u, v)
    Vector3f dir = ray.direction;  // ray direction

    if (inter.material->m_type == EMIT) {  // if the object is a light source
        return inter.material->m_emission;  // return light color
    } else if (inter.material->m_type == DIFFUSE ||
           ((inter.material->m_type == GLASS || inter.material->m_type == MIRROR) && TASK_N < 1.3f) || depth > maxDepth) {

        if (TASK_N == 2) {
            // TODO: task 2 Monte Carlo Path Tracing with Russian Roulette termination
            if (get_random_float() > RussianRoulette) {
                return Vector3f(0);
            }

            
            Intersection i = Intersection();
            i.normal = -N;
            Sphere *interSphere = new Sphere(hitPoint, 1.0f, new Material(GLASS, Vector3f(1)));
            float pdf;

            while (dotProduct(i.normal, N) < 0) {
                interSphere->Sample(i, pdf);
            }
            Ray nextRay = Ray(hitPoint + N.normalized() * EPSILON, i.normal.normalized());
            auto nextInter = intersect(nextRay);
            auto theta = abs(dotProduct(i.normal.normalized(), -dir));

            if (!nextInter.happened || nextInter.material->m_type != EMIT) {
                Vector3f nextColor = castRay(nextRay, depth)  * theta * 2 * std::max(pdf, EPSILON) / RussianRoulette;
                Vector3f newColor = getIntersectionColor(ray, inter, hitPoint, N, st, dir) + nextColor / 255;
                
                return newColor;
            }

            return (nextInter.material->m_emission * inter.material->Kd * theta * (std::max(pdf, EPSILON) * 2) / RussianRoulette) / 255 + getIntersectionColor(ray, inter, hitPoint, N, st, dir);
        }

        return getIntersectionColor(ray, inter, hitPoint, N, st, dir);
    
    } else if (inter.material->m_type == GLASS && TASK_N >= 1.3f) {  // if the object is glass
        // TODO: task 1.3 Glass Material
        // if the depth exceeds the maximum depth, return the hitColor to avoid infinite recursion
        Ray reflectionRay = Ray(hitPoint + N.normalized() * EPSILON, reflect(ray.direction, N.normalized()));
        Vector3f reflection_color = castRay(reflectionRay, depth + 1);

        Ray refractionRay = Ray(hitPoint - N.normalized() * EPSILON, refract(ray.direction, N.normalized(), inter.material->ior));
        Vector3f refraction_color = castRay(refractionRay, depth + 1);
        
        float kr = fresnel(ray.direction, N, inter.material->ior);

        hitColor =  kr * reflection_color + (1 - kr) * refraction_color;
    
    } else if (inter.material->m_type == MIRROR && TASK_N >= 1.3f) {  // if the object is mirror
        // TODO: task 1.3 Mirror Refection

        Ray reflectionRay = Ray(hitPoint + N.normalized() * EPSILON, reflect(ray.direction, N));
        hitColor = castRay(reflectionRay, depth + 1);
    }

    return hitColor;
}


Vector3f Scene::getIntersectionColor(const Ray &ray, const Intersection &inter, Vector3f hitPoint, Vector3f N, Vector2f st, Vector3f dir) const {
    Vector3f diffuseColor = 0, specularColor = 0;
    int light_sample=4;  // the number of samples on the area light
            
    for (int i = 0; i < light_sample; ++i) {
        // TODO: task 1.1 Basic ray tracing (Whitted style) with area light sampling
        // sample area light, basic shading, compute Phong illumination model
        Intersection interLight = Intersection();
        float pdf;
        sampleLight(interLight, pdf);
        Vector3f origin = hitPoint + N.normalized() * EPSILON;
        Vector3f target = interLight.coords;
                        
        Ray r = Ray(origin, target - origin);
        auto rayInter = intersect(r);

        // Check if a ray from the object to the light ends at the light
        if (!rayInter.happened || !interLight.happened || rayInter.obj->getArea() != interLight.obj->getArea() / 2) {
            // std::cout << rayInter.obj->getArea() << ", " << interLight.obj->getArea() << "\n\n\n";
            continue;
        }

        auto v = ray.direction_inv.normalized();
        auto l = r.direction.normalized();

        // Otherwise, can add color
        Vector3f material_color;
        if (inter.material->textured) {
            material_color = inter.material->getColorAt(st.x, st.y);
        }
        else {
            material_color = inter.material->m_color;
        }
        specularColor += interLight.material->getEmission() * inter.material->Ks * dotProduct(v, v - 2 * dotProduct(v, inter.normal.normalized()) * inter.normal.normalized());
        diffuseColor += interLight.material->m_emission * inter.material->Kd * material_color * dotProduct(inter.normal.normalized(), l);
    }

    Vector3f hitColor = (diffuseColor + specularColor) / 255;
    return hitColor;
}