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

float Scene::getLen(Vector3f a, Vector3f b) const {
    return std::sqrt(std::abs(a.x * b.x + a.y * b.y + a.z * b.z));
}

Vector3f Scene::castRayBidirectional(const Ray &ray, int depth, int spp) const {
    Vector3f hitColor = Vector3f(0);
    auto inter = intersect(ray);  // find the cloest intersection of the ray and the objects
    if (!inter.happened)return backgroundColor;  // if no intersection, return background color
    if (inter.material->hasEmission()) return inter.material->getEmission();

    Vector3f hitPoint = inter.coords;  // the intersection point
    Vector3f N = inter.normal.normalized(); // normal
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

    Vert ints[numRaysFromCamera + numRaysFromLight];

    float vcs[numRaysFromCamera + numRaysFromLight + 1];
    float vcms[numRaysFromCamera + numRaysFromLight + 1];

    vcs[0] = 0;


    Intersection lightInters[numRaysFromLight];
    Intersection cameraInters[numRaysFromCamera];
    Vector3f w_s[numRaysFromCamera + numRaysFromLight];
    int totalRaysFromCamera = numRaysFromCamera;
    int totalRaysFromLight = numRaysFromLight;
    cameraInters[0] = inter;
    w_s[0] = ray.direction;
    lightInters[0] = lightInter;

    Vector3f cameraColors[numRaysFromCamera];
    Vector3f lightColors[numRaysFromLight];


    // Create path from camera
    // Still need to implement glass and mirrors
    Vert start;
    start.inter = inter;
    start.beta = Vector3f(1);
    ints[0] = start;
    Vert end;
    end.inter = lightInter;
    end.beta = Vector3f(1);
    ints[numRaysFromCamera] = end;

    // Create path from camera
    // Still need to implement glass and mirrors
    for (int i = 0; i < numRaysFromCamera; i++) {
        N = inter.normal.normalized();
        st = inter.tcoords;
        Vector3f w_ = inter.material->sample(w_s[i], N).normalized();
        float pdf = inter.material->pdf(w_s[i], w_, N);

        Vector3f albedo = inter.obj->evalDiffuseColor(st);
        Vector3f f = inter.material->eval(w_,N,albedo);

        if(pdf<EPSILON)
            numRaysFromCamera = i;

        float cosTheta_ = std::max(0.0f,dotProduct(N,w_));

        cameraColors[i] = f * cosTheta_ / pdf / RussianRoulette;
        ints[i].beta = ints[std::max(0, i - 1)].beta * cameraColors[i];
        ints[i].inter = inter;

        if (i < numRaysFromCamera - 1) {
            w_s[i + 1] = w_;
            cameraInters[i + 1] = inter;
            inter = intersect(Ray(inter.coords + N*EPSILON, w_));
            //todo: update to work with semi-transparent
            // while (depth <= maxDepth && inter.happened && (inter.material->m_type == MIRROR || inter.material->m_type == GLASS || inter.material->m_type == DIFF_MIRROR)) {
            //     float kr = fresnel(w_, inter.normal, inter.material->ior);
            //     // std::cout << kr << "\n";
            //     if (inter.material->m_type == MIRROR || get_random_float() > kr || kr > 1) {
            //         Intersection prevInter = inter;
            //         w_ = reflect(w_, inter.normal).normalized();
            //         // std::cout << inter.coords << "\n";
            //         inter = intersect(Ray(inter.coords + inter.normal * EPSILON, w_));
            //         // std::cout << inter.coords << "\n\n";
            //         w_s[i + 1] = w_;
            //         if (!inter.happened) {
            //             inter = prevInter;
            //             numRaysFromCamera = i + 1;
            //             w_s[i + 1] = w_;
            //             break;
            //         }
            //     }
            //     else if (inter.material->m_type == DIFF_MIRROR) {
            //         break;
            //     }
            //     else {
            //         // refract
            //         w_ = refract(w_,inter.normal,inter.material->ior).normalized();
            //         Vector3f refractionRayOrigin = (dotProduct(w_,N)<0) ? inter.coords - inter.normal*EPSILON : inter.coords + inter.normal*EPSILON;
            //         inter = intersect(Ray(refractionRayOrigin, w_));
            //         w_s[i + 1] = w_;
            //     }
            //     depth++;
            // }
            if (!inter.happened) {
                numRaysFromCamera = i + 1;
                // std::cout << i + 1 << "\n";
            }
        }
    }


    // Create path from light
    N = lightInter.normal.normalized();
    w_s[totalRaysFromCamera] = lightInter.material->sample(-N, N).normalized();
    float pdf = lightInter.material->pdf(-N, w_s[totalRaysFromCamera], N);
    Vector3f albedo = lightInter.obj->evalDiffuseColor(lightInter.tcoords);
    Vector3f f = lightInter.material->eval(w_s[totalRaysFromCamera],N,albedo);
    float cosTheta_ = std::max(0.0f,dotProduct(N,w_s[totalRaysFromCamera]));
    float r = std::pow(55, 2);
    lightColors[0] = lightInter.material->getEmission() / (pdf * r / std::abs(dotProduct(lightInter.normal, w_s[totalRaysFromCamera])));

    // std::cout << lightColors[0] << "; " << f << "\n";
    ints[totalRaysFromCamera].beta = lightColors[0];
    if (numRaysFromLight > 1) {
        lightInters[1] = intersect(Ray(lightInter.coords + N*EPSILON, w_s[totalRaysFromCamera]));
        if (!lightInters[1].happened) {
            numRaysFromLight = 1;
        }
        inter = lightInters[1];
    }

    
    

    for (int i = 1; i < numRaysFromLight; i++) {
        // inter = lightInters[i];
        N = inter.normal.normalized();
        st = inter.tcoords;

        Vector3f w_ = inter.material->sample(w_s[totalRaysFromCamera + i - 1], N).normalized();

        // if (i == 1) {
        //     std::cout << w_ << "\n";
        // }
        
        pdf = inter.material->pdf(w_s[totalRaysFromCamera + i - 1], w_, N);

        if (inter.obj != nullptr){
            albedo = inter.obj->evalDiffuseColor(st);
            f = inter.material->eval(w_,N,albedo);
        }
        else {
            f = Vector3f(0.156,0.25,0.268);
        }
        
        

        if(pdf<EPSILON)
            numRaysFromLight = i;

        cosTheta_ = std::max(0.0f,dotProduct(N,w_));

        lightColors[i] = f * cosTheta_ / pdf / RussianRoulette;

        ints[totalRaysFromCamera + i].beta = ints[totalRaysFromCamera + std::max(0, i - 1)].beta * lightColors[i];
        ints[totalRaysFromCamera + i].inter = inter;

        w_s[totalRaysFromCamera + i] = w_;

        
        
        lightInters[i] = inter;
        inter = intersect(Ray(inter.coords + N*EPSILON, w_));
        // while (depth <= maxDepth && inter.happened && (inter.material->m_type == MIRROR || inter.material->m_type == GLASS || inter.material->m_type == DIFF_MIRROR)) {
        //     float kr = fresnel(w_, inter.normal, inter.material->ior);
        //     if (inter.material->m_type == MIRROR || get_random_float() > kr || kr > 1) {
        //         Intersection prevInter = inter;
        //         w_ = reflect(w_, inter.normal).normalized();
        //         inter = intersect(Ray(inter.coords + inter.normal * EPSILON, w_));
        //         if (!inter.happened) {
        //             inter = prevInter;
        //             numRaysFromLight = i + 1;
        //             break;
        //         }
        //     }
        //     else if (inter.material->m_type == DIFF_MIRROR) {
        //         break;
        //     }
        //     else {
        //         // refract
        //         w_ = refract(w_,inter.normal,inter.material->ior).normalized();
        //         Vector3f refractionRayOrigin = (dotProduct(w_,N)<0) ? inter.coords - inter.normal*EPSILON : inter.coords + inter.normal*EPSILON;
        //         inter = intersect(Ray(refractionRayOrigin, w_));
        //     }
        //     depth++;
        // }
        if (!inter.happened) {
            numRaysFromLight = i + 1;
            // std::cout << i << "\n";
        }
    }

    Intersection dummy;
    float camera_ps[numRaysFromCamera];
    float light_ps[numRaysFromLight];
    cameraInters[0].obj->Sample(dummy, pdf);
    camera_ps[0] = pdf;

    for (int i = 1; i < numRaysFromCamera; i++) {
        camera_ps[i] = camera_ps[i-1] * dotProduct(w_s[i], w_s[i-1]) / std::powf(getLen(w_s[i], w_s[i-1]), 2) * cameraInters[i].material->pdf(w_s[i-1], w_s[i], cameraInters[i].normal);
        // std::cout << camera_ps[i] << "\n";
    }

    light_ps[0] = lightPdf;
    for (int i = 1; i < numRaysFromLight; i++) {
        light_ps[i] = light_ps[i - 1] * dotProduct(w_s[totalRaysFromCamera + i], w_s[totalRaysFromCamera + i - 1]) / 
            std::powf(getLen(w_s[totalRaysFromCamera + i], w_s[totalRaysFromCamera + i - 1]), 2) * lightInters[i].material->pdf(w_s[totalRaysFromCamera + i-1], w_s[totalRaysFromCamera + i], lightInters[i].normal);
        // std::cout << light_ps[i] << "\n";
    }

    

    float pSum = 0;

    for (int i = 0; i < numRaysFromCamera; i++) {
        for (int j = 0; j < numRaysFromLight; j++) {
            pSum += std::powf(camera_ps[i] * light_ps[j], 2);
        }
    }


    // Calculate final color
    for (int i = 0; i < numRaysFromCamera; i++) {
        for (int j = 0; j < numRaysFromLight; j++) {
            Vector3f start = cameraInters[i].coords + cameraInters[i].normal.normalized() * EPSILON;
            Vector3f current_w_ = (lightInters[j].coords - start).normalized();
            Intersection testOccluded = intersect(Ray(start, current_w_));
            Intersection oppositeTest = intersect(Ray(lightInters[j].coords + lightInters[j].normal * EPSILON, -current_w_));
            // if (testOccluded.happened && testOccluded.obj == lightInters[j].obj) {
            //     std::cout << std::abs(testOccluded.tnear - oppositeTest.tnear) << "\n";
            // }
            if (testOccluded.happened && oppositeTest.happened && std::abs(testOccluded.tnear - oppositeTest.tnear) < 0.1) {
                // Vector3f color = lightColors[0] + lightInters[0].material->getEmission();
                // Vector3f color = lightColors[0];
                // // std::cout << color << "\n";
                // for (int k = 1; k < j; k++) {
                //     Vector3f le = lightInters[k].material->hasEmission() ? lightInters[k].material->getEmission() : Vector3f(0);
                //     color = le + lightColors[k] * color;
                // }

                // if (i == numRaysFromCamera - 1 && numRaysFromCamera != totalRaysFromCamera) {
                //     color = backgroundColor * color;
                // }

                // for (int k = i; k >= 0; k--) {
                //     Vector3f le = cameraInters[k].material->hasEmission() ? cameraInters[k].material->getEmission() : Vector3f(0);
                //     color = le + cameraColors[k] * color;
                // }

                // go from camera to light
                cosTheta_ = std::max(0.0f,dotProduct(cameraInters[i].normal,current_w_));
                if (i == 0) {
                    pdf = cameraInters[i].material->pdf(ray.direction, current_w_, cameraInters[i].normal);
                }
                else {
                    pdf = cameraInters[i].material->pdf(w_s[i-1], current_w_, cameraInters[i].normal);
                }

                albedo = cameraInters[i].obj->evalDiffuseColor(cameraInters[i].tcoords);
                f = cameraInters[i].material->eval(current_w_,cameraInters[i].normal,albedo);
                if (pdf < EPSILON) {
                    continue;
                }

                Vector3f cameraBeta = f * cosTheta_ / std::max(pdf, EPSILON) / RussianRoulette;
                pdf *= RussianRoulette;

                // go from light to camera
                cosTheta_ = std::max(0.0f,dotProduct(lightInters[j].normal,-current_w_));
                pdf = lightInters[j].material->pdf(w_s[j], -current_w_, lightInters[j].normal);
                
                if (pdf < EPSILON) {
                    continue;
                }
                
                albedo = lightInters[j].obj->evalDiffuseColor(lightInters[j].tcoords);
                f = lightInters[j].material->eval(-current_w_,lightInters[j].normal,albedo);

                pdf *= RussianRoulette;

                Vector3f lightBeta = f * cosTheta_ / std::max(pdf, EPSILON) / RussianRoulette;

                // Vector3f w_ = inter.material->sample(w_s[totalRaysFromCamera + i - 1], N).normalized();
                // pdf = inter.material->pdf(w_s[totalRaysFromCamera + i - 1], w_, N);

                // if (inter.obj != nullptr){
                //     albedo = inter.obj->evalDiffuseColor(st);
                //     f = inter.material->eval(w_,N,albedo);

                Vector3f d = (ints[totalRaysFromCamera + j].inter.coords - ints[i].inter.coords) / 100; // change to 800 for box

                

                

                float dist2 = dotProduct(d, d);

                float dist = sqrt(dist2);

                d = d / dist;

                // std::cout << dist << "\n";

                float G = std::abs(dotProduct(ints[i].inter.normal, d)) * abs(dotProduct(ints[totalRaysFromCamera + j].inter.normal, -d)) / dist2;

                if (dist2 < EPSILON) {
                    continue;
                }

                
                Vector3f color = ints[i].beta * ints[totalRaysFromCamera + j].beta * cameraBeta * G;

                

                // std::cout << pSum << "\n";

                hitColor += color / pSum * std::powf(camera_ps[i] * light_ps[j], 2);

                // std::cout << hitColor << "\n";
            }
        }
    }

    // if (hitColor.x <= 0 || hitColor.y <= 0 || hitColor.z <= 0 || hitColor.x >= 1 || hitColor.y >= 1 || hitColor.z >= 1) {
    //     std::cout << hitColor << "\n";
    // }

    // if (hitColor.x == 0 && hitColor.y == 0 && hitColor.z == 0) {
    //     std::cout << numRaysFromCamera << ", " << numRaysFromLight << "\n";
    // }

    // std::cout << hitColor << "\n";

    return hitColor;
}

// Implementation of Path Tracing
Vector3f Scene::castRay(const Ray &ray, int depth) const
{   
    if(depth>this->maxDepth){
        Vector3f hitColor = Vector3f(0,0,0);
        return hitColor;
    }
    Vector3f hitColor = Vector3f(0);
    auto inter = intersect(ray);  // find the cloest intersection of the ray and the objects
    if (!inter.happened)return backgroundColor;  // if no intersection, return background color
    if(!inter.obj || !inter.material){
        return Vector3f(0.156,0.25,0.268);
    }

    Vector3f hitPoint = inter.coords;  // the intersection point
    Vector3f N = inter.normal; // normal
    Vector2f st = inter.tcoords; // texture coordinates (u, v)
    Vector3f dir = ray.direction;  // ray direction
    //[Debug to see if the floor normals are pointing in the right direction]
    // return Vector3f(std::abs(N.x),std::abs(N.y),std::abs(N.z));
    // return N;
    if (inter.material->m_type == EMIT) {  // if the object is a light source
        return inter.material->m_emission;  // return light color
    } else if (inter.material->m_type == DIFFUSE ||
           ((inter.material->m_type == GLASS || inter.material->m_type == MIRROR) && TASK_N < 1.3f)) {

        if (TASK_N == 2) {
            // TODO: task 2 Monte Carlo Path Tracing with Russian Roulette termination
            // Emission term Le(x,w)
            Vector3f le = Vector3f(0);
            if(inter.material->hasEmission())
                le = inter.material->getEmission();
            
           //Prevent recursion in higher depths 
            if(get_random_float()>RussianRoulette)
                return le;
            
            //Sample w' from brdf
            Vector3f w_ = inter.material->sample(dir,N).normalized();
            float pdf = inter.material->pdf(dir,w_,N);
            //Evaluate f(w,w')
            Vector3f albedo = inter.obj->evalDiffuseColor(st);
            Vector3f f = inter.material->eval(w_,N,albedo);

            //Prevent division by zero
            if(pdf<EPSILON)
                return le;
            //cos(thetai)
            float cosTheta_ = std::max(0.0f,dotProduct(N,w_));
            //recurse
            Ray recurseRay(hitPoint+N*EPSILON,w_);
            return le + ((f*castRay(recurseRay,depth+1) * cosTheta_) / pdf / RussianRoulette);
        }

        Vector3f diffuseColor = 0, specularColor = 0;
        int light_sample=4;  // the number of samples on the area light
        // int light_sample=8;
        // int light_sample=16;
        // int light_sample=32;
        // int light_sample=64;
        // int light_sample=128;
        // int light_sample=256;
        for (int i = 0; i < light_sample; ++i) {
            // TODO: task 1.1 Basic ray tracing (Whitted style) with area light sampling
            // sample area light, basic shading, compute Phong illumination model
            // Get a random light emmitting surface from the scene , 
            // calculate its interaction with the objects on the scene
            Intersection light_pos;
            float light_pdf;
            sampleLight(light_pos,light_pdf);
            //Direction of light from the source to hitpoint
            if(!light_pos.material){
                continue;
            }
            Vector3f L = light_pos.coords - hitPoint;
            float light_distance = L.norm();
            L = normalize(L);

            Vector3f V = normalize(-dir);

            Ray shadow_ray(hitPoint + N*EPSILON , L);
            // Ray shadow_ray(hitPoint , L);
            Intersection shadow_inter = intersect(shadow_ray);
            //Check if there the ray hits the object without an obstruction
            //i.e if there is a shadow , there is obstruction or
            //the shadow caused by the object is somewhere far away 
            if(!shadow_inter.happened || (shadow_inter.material && shadow_inter.material->m_type == EMIT)){
                Vector3f Kd = inter.obj->evalDiffuseColor(st);
                float diff = std::max(0.0f,dotProduct(N,L));
                Vector3f diffuse = Kd * light_pos.material->m_emission * diff;
                Vector3f R = reflect(-L,N);
                float spec_angle = std::max(0.0f,dotProduct(V,R));
                // Phong shading
                Vector3f specular = Vector3f(0.5f) * light_pos.material->m_emission*std::pow(spec_angle,32.0f);
                diffuseColor+= diffuse;
                specularColor+= specular;
            }

        }
        // Vector3f Kd = inter.obj->evalDiffuseColor(st);
        // Vector3f ambient_light_intensity= Vector3f(0.05f);
        // Vector3f ambientColor = Kd*ambient_light_intensity;
        // hitColor = (ambientColor + diffuseColor + specularColor) / (float)light_sample;
        hitColor = (diffuseColor + specularColor) / (float)light_sample;
    
    } else if (inter.material->m_type == GLASS && TASK_N >= 1.3f) {  // if the object is glass
        // TODO: task 1.3 Glass Material
        // if the depth exceeds the maximum depth, return the hitColor to avoid infinite recursion
        //When the ray hits the surface at a shallow angle , we can skip the refraction part entirrly
        float kr=fresnel(ray.direction,N,inter.material->ior);
        if(kr<1){
            Vector3f refractionDirection = refract(ray.direction,N,inter.material->ior).normalized();
            // While the ray enters the object , nudge the hitpoint inside the object and when the ray exits the object , nudge the hitpoint outside the object
            Vector3f refractionRayOrigin = (dotProduct(refractionDirection,N)<0) ? hitPoint - N*EPSILON : hitPoint + N*EPSILON;
            Ray refractionRay(refractionRayOrigin,refractionDirection);
            Vector3f color_refraction = castRay(refractionRay,depth+1);
            Vector3f reflectionDirection = reflect(ray.direction,N).normalized();
            Ray reflectionRay(hitPoint + N*EPSILON,reflectionDirection);
            Vector3f color_reflection = castRay(reflectionRay,depth+1) * inter.material->m_color;
            //Law of conservation of energy
            hitColor = color_refraction*kr + color_reflection*(1-kr);
        }
        else{
            Vector3f reflectionDirection = reflect(ray.direction,N).normalized();
            Ray reflectionRay(hitPoint + N*EPSILON,reflectionDirection);
            hitColor = castRay(reflectionRay,depth+1) * inter.material->m_color;

        }
    } else if (inter.material->m_type == MIRROR && TASK_N >= 1.3f) {  // if the object is mirror
        // TODO: task 1.3 Mirror Refection
        
        Vector3f reflectionDirection = reflect(ray.direction,N).normalized();
        Ray reflectionRay(hitPoint + N*EPSILON,reflectionDirection);
        hitColor = castRay(reflectionRay,depth+1) * inter.material->m_color;
    }

    return hitColor;
    // return inter.material->m_color;
}