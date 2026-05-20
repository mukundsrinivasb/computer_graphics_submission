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

    } else if (inter.material->m_type == DIFF_MIRROR && TASK_N >= 1.3f) {  // Charlie: if the object is reflective but has its own colour
        // Charlie: this is adapted from this version of the glass implementation
        Vector3f phong_color = inter.material->getColor(); // TODO: please replace this with however you would calculate this intersection's colour if it was diffuse

        float kr = fresnel(ray.direction, N, inter.material->ior);
        if(kr<1){
            Vector3f reflectionDirection = reflect(ray.direction,N).normalized();
            Ray reflectionRay(hitPoint + N*EPSILON,reflectionDirection);
            Vector3f color_reflection = castRay(reflectionRay,depth+1) * inter.material->m_color;
            //Law of conservation of energy
            hitColor = phong_color*kr + color_reflection*(1-kr);
        }
        else{
            Vector3f reflectionDirection = reflect(ray.direction,N).normalized();
            Ray reflectionRay(hitPoint + N*EPSILON,reflectionDirection);
            hitColor = castRay(reflectionRay,depth+1) * inter.material->m_color;
        }
    }

    return hitColor;
    // return inter.material->m_color;
}
