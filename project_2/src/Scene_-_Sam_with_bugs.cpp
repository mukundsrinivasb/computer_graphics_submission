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

    Intersection lightInters[numRaysFromLight];
    Intersection cameraInters[numRaysFromCamera];
    Vector3f w_s[numRaysFromCamera + numRaysFromLight];
    int totalRaysFromCamera = numRaysFromCamera;
    cameraInters[0] = inter;
    w_s[0] = ray.direction;
    lightInters[0] = lightInter;

    Vector3f cameraColors[numRaysFromCamera];
    Vector3f lightColors[numRaysFromLight];


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

        if (i < numRaysFromCamera - 1) {
            w_s[i + 1] = w_;
            cameraInters[i + 1] = inter;
            inter = intersect(Ray(inter.coords + N*EPSILON, w_));
            while (depth >= 0 && inter.happened && (inter.material->m_type == MIRROR || inter.material->m_type == GLASS)) {
                float kr = fresnel(w_, inter.normal, inter.material->ior);
                if (inter.material->m_type == MIRROR || get_random_float() > kr || kr > 1) {
                    w_ = reflect(w_, inter.normal);
                    inter = intersect(Ray(inter.coords + inter.normal * EPSILON, w_));
                }
                else {
                    // refract
                    w_ = refract(w_,inter.normal,inter.material->ior).normalized();
                    Vector3f refractionRayOrigin = (dotProduct(w_,N)<0) ? inter.coords - inter.normal*EPSILON : inter.coords + inter.normal*EPSILON;
                    inter = intersect(Ray(refractionRayOrigin, w_));
                }
                depth--;
            }
            if (!inter.happened) {
                numRaysFromCamera = i;
            }
        }
    }


    // Create path from light
    N = lightInter.normal.normalized();
    w_s[totalRaysFromCamera] = lightInter.material->sample(-N, N);
    float pdf = lightInter.material->pdf(-N, w_s[totalRaysFromCamera], N);
    Vector3f albedo = lightInter.obj->evalDiffuseColor(lightInter.tcoords);
    Vector3f f = lightInter.material->eval(w_s[totalRaysFromCamera],N,albedo);
    float cosTheta_ = std::max(0.0f,dotProduct(N,w_s[totalRaysFromCamera]));
    lightColors[0] = f * cosTheta_ / pdf / RussianRoulette;
    if (numRaysFromLight > 1) {
        lightInters[1] = intersect(Ray(lightInter.coords + N*EPSILON, w_s[totalRaysFromCamera]));
        if (!lightInters[1].happened) {
            numRaysFromLight = 1;
        }
    }

    for (int i = 1; i < numRaysFromLight; i++) {
        inter = lightInters[i];
        N = inter.normal.normalized();
        st = inter.tcoords;

        Vector3f w_ = inter.material->sample(w_s[totalRaysFromCamera + i - 1], N).normalized();
        pdf = inter.material->pdf(w_s[totalRaysFromCamera + i - 1], w_, N);

        if (inter.obj != nullptr){
            albedo = inter.obj->evalDiffuseColor(st);
        }
        else {
            albedo = Vector3f(0.5, 0.5, 0.5);
        }
        
        f = inter.material->eval(w_,N,albedo);

        if(pdf<EPSILON)
            numRaysFromLight = i;

        cosTheta_ = std::max(0.0f,dotProduct(N,w_));

        lightColors[i] = f * cosTheta_ / pdf / RussianRoulette;

        w_s[totalRaysFromCamera + i] = w_;
        lightInters[i] = inter;
        inter = intersect(Ray(inter.coords + N*EPSILON, w_));
        while (depth >= 0 && inter.happened && (inter.material->m_type == MIRROR || inter.material->m_type == GLASS)) {
            float kr = fresnel(w_, inter.normal, inter.material->ior);
            if (inter.material->m_type == MIRROR || get_random_float() > kr || kr > 1) {
                w_ = reflect(w_, inter.normal);
                inter = intersect(Ray(inter.coords + inter.normal * EPSILON, w_));
            }
            else {
                // refract
                w_ = refract(w_,inter.normal,inter.material->ior).normalized();
                Vector3f refractionRayOrigin = (dotProduct(w_,N)<0) ? inter.coords - inter.normal*EPSILON : inter.coords + inter.normal*EPSILON;
                inter = intersect(Ray(refractionRayOrigin, w_));
            }
            depth--;
        }
        if (!inter.happened) {
            numRaysFromLight = i;
        }
    }


    // Calculate final color
    for (int i = 0; i < numRaysFromCamera; i++) {
        for (int j = 0; j < numRaysFromLight; j++) {
            Vector3f start = cameraInters[i].coords + cameraInters[i].normal.normalized() * EPSILON;
            Intersection testOccluded = intersect(Ray(start, lightInters[j].coords - start));
            if (testOccluded.happened && testOccluded.obj == lightInters[j].obj) {
                // Vector3f color = lightColors[0] + lightInters[0].material->getEmission();
                Vector3f color = lightColors[0];
                // std::cout << color << "\n";
                for (int k = 1; k < j; k++) {
                    Vector3f le = lightInters[k].material->hasEmission() ? lightInters[k].material->getEmission() : Vector3f(0);
                    color = le + lightColors[k] * color;
                }

                for (int k = i; k >= 0; k--) {
                    Vector3f le = cameraInters[k].material->hasEmission() ? cameraInters[k].material->getEmission() : Vector3f(0);
                    color = le + cameraColors[k] * color;
                }

                hitColor += color;
            }
        }
    }

    // if (hitColor.x < 0 || hitColor.y < 0 || hitColor.z < 0 || hitColor.x >= 1 || hitColor.y >= 1 || hitColor.z >= 1) {
    //     std::cout << hitColor << "\n";
    // }

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