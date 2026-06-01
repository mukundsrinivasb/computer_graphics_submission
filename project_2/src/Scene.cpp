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

Vector3f Scene::castRayBidirectional(const Ray &ray, int depth, bool shadows_on) const {
    Vector3f hitColor = Vector3f(0);
    auto inter = intersect(ray);  // find the cloest intersection of the ray and the objects
    while (depth <= maxDepth && inter.happened && inter.material->m_type == DIFF_MIRROR && fresnel(ray.direction.normalized(), inter.normal.normalized(), inter.material->ior) < get_random_float()) {
        inter = intersect(Ray(inter.coords + inter.normal.normalized() * EPSILON, reflect(ray.direction.normalized(), inter.normal.normalized())));
        depth++;
    }
    if (!inter.happened)return backgroundColor;  // if no intersection, return background color
    if (inter.material->hasEmission()) return inter.material->getEmission();

    int numRaysFromLight = 1;
    while (get_random_float() <= RussianRoulette) {
        numRaysFromLight++;
    }

    int numRaysFromCamera = 1;
    while (get_random_float() <= RussianRoulette) {
        numRaysFromCamera++;
    }

    // Determine start of light's path
    Intersection lightInter;
    float lightPdf;
    sampleLight(lightInter, lightPdf);

    Vert ints[numRaysFromCamera + numRaysFromLight]; // Intersections/beta values of both the path from the camera and the path from the light

    Vector3f w_s[numRaysFromCamera + numRaysFromLight]; // Incoming w_ value for each intersection
    int totalRaysFromCamera = numRaysFromCamera;
    ints[0].inter = inter;
    w_s[0] = ray.direction;
    ints[totalRaysFromCamera].inter = lightInter;

    Vector3f cameraColors[numRaysFromCamera];
    Vector3f lightColors[numRaysFromLight];
    Vector2f st; // texture coordinates (u, v)
    Vector3f N; // normal

    // Start path from camera
    Vert start;
    start.inter = inter;
    start.beta = Vector3f(1);
    ints[0] = start;

    // Start path from light (light path starts with light at index numRaysFromCamera and increases from there)
    Vert end;
    end.inter = lightInter;
    ints[numRaysFromCamera] = end;

    // Create path from camera
    for (int i = 0; i < numRaysFromCamera; i++) {
        N = inter.normal.normalized();
        st = inter.tcoords;

        // outgoing w_
        Vector3f w_ = inter.material->sample(w_s[i], N).normalized();
        float pdf = inter.material->pdf(w_s[i], w_, N);

        // calculate color
        Vector3f albedo = inter.obj->evalDiffuseColor(st);
        Vector3f f = inter.material->eval(w_,N,albedo);
        float cosTheta_ = std::max(0.0f,dotProduct(N,w_));
        cameraColors[i] = f * cosTheta_ / pdf / RussianRoulette;

        ints[i].beta = ints[std::max(0, i - 1)].beta * cameraColors[i];
        // ints[i].inter = inter;

        if (i < numRaysFromCamera - 1) {
            // Calculate next intersection
            w_s[i + 1] = w_;
            ints[i + 1].inter = inter;
            inter = intersect(Ray(inter.coords + N*EPSILON, w_));
            while (depth <= maxDepth && inter.happened && inter.material->m_type == DIFF_MIRROR && fresnel(w_, inter.normal.normalized(), inter.material->ior) < get_random_float()) {
                inter = intersect(Ray(inter.coords + inter.normal.normalized() * EPSILON, reflect(w_, inter.normal.normalized())));
                depth++;
            }
            if (!inter.happened) {
                numRaysFromCamera = i + 1;
            }
        }
    }


    // Calculate color of light
    N = lightInter.normal.normalized();
    w_s[totalRaysFromCamera] = lightInter.material->sample(-N, N).normalized();
    float pdf = lightInter.material->pdf(-N, w_s[totalRaysFromCamera], N);
    Vector3f albedo = lightInter.obj->evalDiffuseColor(lightInter.tcoords);
    Vector3f f = lightInter.material->eval(w_s[totalRaysFromCamera],N,albedo);
    float cosTheta_ = std::max(0.0f,dotProduct(N,w_s[totalRaysFromCamera]));
    float r = std::pow(55, 2);
    lightColors[0] = lightInter.material->getEmission() / (pdf * r / std::abs(dotProduct(lightInter.normal, w_s[totalRaysFromCamera])));
    ints[totalRaysFromCamera].beta = lightColors[0];

    // Create first path from light
    if (numRaysFromLight > 1) {
        ints[totalRaysFromCamera + 1].inter = intersect(Ray(lightInter.coords + N*EPSILON, w_s[totalRaysFromCamera]));
        while (depth <= maxDepth && ints[totalRaysFromCamera + 1].inter.happened && ints[totalRaysFromCamera + 1].inter.material->m_type == DIFF_MIRROR && fresnel(w_s[totalRaysFromCamera], ints[totalRaysFromCamera + 1].inter.normal, ints[totalRaysFromCamera + 1].inter.material->ior) < get_random_float()) {
            ints[totalRaysFromCamera + 1].inter = intersect(Ray(ints[totalRaysFromCamera + 1].inter.coords + ints[totalRaysFromCamera + 1].inter.normal.normalized() * EPSILON, reflect(w_s[totalRaysFromCamera], ints[totalRaysFromCamera + 1].inter.normal.normalized())));
            depth++;
        }
        if (!ints[totalRaysFromCamera + 1].inter.happened) {
            numRaysFromLight = 1;
        }
        inter = ints[totalRaysFromCamera + 1].inter;
    }

    // Calculate rest of path from light
    for (int i = 1; i < numRaysFromLight; i++) {
        N = inter.normal.normalized();
        st = inter.tcoords;

        // outgoing w_
        Vector3f w_ = inter.material->sample(w_s[totalRaysFromCamera + i - 1], N).normalized();
        
        pdf = inter.material->pdf(w_s[totalRaysFromCamera + i - 1], w_, N);

        // Fix for weird error where intersection had no object associated with it
        if (inter.obj != nullptr){
            albedo = inter.obj->evalDiffuseColor(st);
            f = inter.material->eval(w_,N,albedo);
        }
        else {
            f = Vector3f(0.156,0.25,0.268);
        }

        // calculate color
        cosTheta_ = std::max(0.0f,dotProduct(N,w_));
        lightColors[i] = f * cosTheta_ / pdf / RussianRoulette;
        ints[totalRaysFromCamera + i].beta = ints[totalRaysFromCamera + std::max(0, i - 1)].beta * lightColors[i];

        w_s[totalRaysFromCamera + i] = w_;
        ints[totalRaysFromCamera + i].inter = inter;

        // calculate next intersection
        inter = intersect(Ray(inter.coords + N*EPSILON, w_));
        while (depth <= maxDepth && inter.happened && inter.material->m_type == DIFF_MIRROR && fresnel(w_, inter.normal.normalized(), inter.material->ior) < get_random_float()) {
            inter = intersect(Ray(inter.coords + inter.normal.normalized() * EPSILON, reflect(w_, inter.normal.normalized())));
            depth++;
        }
        if (!inter.happened) {
            numRaysFromLight = i + 1;
        }
    }

    // Calculate probabilities for MIS
    Intersection dummy;
    float camera_ps[numRaysFromCamera];
    float light_ps[numRaysFromLight];
    ints[0].inter.obj->Sample(dummy, pdf);
    camera_ps[0] = pdf;

    for (int i = 1; i < numRaysFromCamera; i++) {
        camera_ps[i] = camera_ps[i-1] * dotProduct(w_s[i], w_s[i-1]) / std::powf(getLen(w_s[i], w_s[i-1]), 2) * ints[i].inter.material->pdf(w_s[i-1], w_s[i], ints[i].inter.normal);
    }

    light_ps[0] = lightPdf;
    for (int i = 1; i < numRaysFromLight; i++) {
        light_ps[i] = light_ps[i - 1] * dotProduct(w_s[totalRaysFromCamera + i], w_s[totalRaysFromCamera + i - 1]) / 
            std::powf(getLen(w_s[totalRaysFromCamera + i], w_s[totalRaysFromCamera + i - 1]), 2) * ints[totalRaysFromCamera + i].inter.material->pdf(w_s[totalRaysFromCamera + i-1], w_s[totalRaysFromCamera + i], ints[totalRaysFromCamera + i].inter.normal);
    }

    // Calculate sum of squares
    float pSum = 0;

    for (int i = 0; i < numRaysFromCamera; i++) {
        for (int j = 0; j < numRaysFromLight; j++) {
            pSum += std::powf(camera_ps[i] * light_ps[j], 2);
        }
    }


    // Calculate final color
    for (int i = 0; i < numRaysFromCamera; i++) {
        for (int j = 0; j < numRaysFromLight; j++) {
            Vector3f start = ints[i].inter.coords + ints[i].inter.normal.normalized() * EPSILON;
            Vector3f current_w_ = (ints[totalRaysFromCamera + j].inter.coords - start).normalized();
            Intersection testOccluded = intersect(Ray(start, current_w_));
            Intersection oppositeTest = intersect(Ray(ints[totalRaysFromCamera + j].inter.coords + ints[totalRaysFromCamera + j].inter.normal * EPSILON, -current_w_));
            if (!shadows_on || testOccluded.happened && oppositeTest.happened && std::abs(testOccluded.tnear - oppositeTest.tnear) < 0.1) {
                // calculate beta value of connection from camera to light
                cosTheta_ = std::max(0.0f,dotProduct(ints[i].inter.normal,current_w_));
                if (i == 0) {
                    pdf = ints[i].inter.material->pdf(ray.direction, current_w_, ints[i].inter.normal);
                }
                else {
                    pdf = ints[i].inter.material->pdf(w_s[i-1], current_w_, ints[i].inter.normal);
                }

                albedo = ints[i].inter.obj->evalDiffuseColor(ints[i].inter.tcoords);
                f = ints[i].inter.material->eval(current_w_,ints[i].inter.normal,albedo);
                if (pdf < EPSILON) {
                    continue;
                }

                Vector3f cameraBeta = f * cosTheta_ / std::max(pdf, EPSILON) / RussianRoulette;

                // Calculate geometry term
                Vector3f d = (ints[totalRaysFromCamera + j].inter.coords - ints[i].inter.coords) / 100;
                float distSquared = dotProduct(d, d);
                float dist = sqrt(distSquared);
                d = d / dist;
                float G = std::abs(dotProduct(ints[i].inter.normal, d)) * abs(dotProduct(ints[totalRaysFromCamera + j].inter.normal, -d)) / distSquared;

                if (distSquared < EPSILON) {
                    continue; // colors can blow up if two points are two close together
                }

                // Calculate color excluding MIS
                Vector3f color = ints[i].beta * ints[totalRaysFromCamera + j].beta * cameraBeta * G;

                // include MIS in final color
                hitColor += color / pSum * std::powf(camera_ps[i] * light_ps[j], 2);
            }
        }
    }
    

    return hitColor;
}

// Implementation of Path Tracing
Vector3f Scene::castRay(const Ray &ray, int depth, bool shadows_on) const
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

    if (inter.material->m_type == EMIT) {  // if the object is a light source
        return inter.material->m_emission;  // return light color
    } else if (inter.material->m_type == DIFFUSE || depth > maxDepth) {
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
        return le + ((f*castRay(recurseRay,depth+1, shadows_on) * cosTheta_) / pdf / RussianRoulette);

    } else if (inter.material->m_type == GLASS) {  // if the object is glass
        // if the depth exceeds the maximum depth, return the hitColor to avoid infinite recursion
        //When the ray hits the surface at a shallow angle , we can skip the refraction part entirrly
        float kr=fresnel(ray.direction,N,inter.material->ior);
        if(kr<1){
            Vector3f refractionDirection = refract(ray.direction,N,inter.material->ior).normalized();
            // While the ray enters the object , nudge the hitpoint inside the object and when the ray exits the object , nudge the hitpoint outside the object
            Vector3f refractionRayOrigin = (dotProduct(refractionDirection,N)<0) ? hitPoint - N*EPSILON : hitPoint + N*EPSILON;
            Ray refractionRay(refractionRayOrigin,refractionDirection);
            Vector3f color_refraction = castRay(refractionRay,depth+1, shadows_on);
            Vector3f reflectionDirection = reflect(ray.direction,N).normalized();
            Ray reflectionRay(hitPoint + N*EPSILON,reflectionDirection);
            Vector3f color_reflection = castRay(reflectionRay,depth+1, shadows_on) * inter.material->m_color;
            //Law of conservation of energy
            hitColor = color_refraction*kr + color_reflection*(1-kr);
        }
        else{
            Vector3f reflectionDirection = reflect(ray.direction,N).normalized();
            Ray reflectionRay(hitPoint + N*EPSILON,reflectionDirection);
            hitColor = castRay(reflectionRay,depth+1, shadows_on) * inter.material->m_color;

        }
    } else if (inter.material->m_type == MIRROR) {  // if the object is mirror
        Vector3f reflectionDirection = reflect(ray.direction,N).normalized();
        Ray reflectionRay(hitPoint + N*EPSILON,reflectionDirection);
        hitColor = castRay(reflectionRay,depth+1, shadows_on) * inter.material->m_color;
    } else if (inter.material->m_type == DIFF_MIRROR) {  // Charlie: if the object is reflective but has its own colour
        Ray reflectionRay = Ray(hitPoint + N.normalized() * EPSILON, reflect(ray.direction, N.normalized()));
        Vector3f reflection_color = castRay(reflectionRay, depth + 1, shadows_on);

        //Vector3f phong_color = inter.material->getColor(); // this is a decent backup if the phong rendering still looks wrong 
        Vector3f phong_color = castRay(ray, maxDepth + 1, shadows_on); // get the normal phong shading for this intersection

        float kr = fresnel(ray.direction, N, inter.material->ior);

        hitColor =  kr * reflection_color + (1 - kr) * phong_color;
    
    }

    return hitColor;
    // return inter.material->m_color;
}