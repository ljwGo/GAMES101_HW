//
// Created by Göksu Güvendiren on 2019-05-14.
//

#include "Scene.hpp"
#include <cassert>


void Scene::buildBVH() {
    printf(" - Generating scene BVH...\n\n");
    this->bvh = new BVHAccel(objects, 1, BVHAccel::SplitMethod::NAIVE);
}

Intersection Scene::intersect(const Ray& ray) const
{
    return this->bvh->Intersect(ray);
}

void Scene::sampleLight(Intersection& pos, float& pdf) const
{
    // 发光面积总合, 这里不是light而是object
    float emit_area_sum = 0;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        if (objects[k]->hasEmit()) {
            emit_area_sum += objects[k]->getArea();
        }
    }
    float p = get_random_float() * emit_area_sum;
    emit_area_sum = 0;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        if (objects[k]->hasEmit()) {
            emit_area_sum += objects[k]->getArea();
            if (p <= emit_area_sum) {
                // model(triangle set) random a triangle though bvh sample.
                objects[k]->Sample(pos, pdf);
                break;
            }
        }
    }
}

bool Scene::trace(
    const Ray& ray,
    const std::vector<Object*>& objects,
    float& tNear, uint32_t& index, Object** hitObject)
{
    *hitObject = nullptr;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        float tNearK = kInfinity;
        uint32_t indexK;
        Vector2f uvK;
        if (objects[k]->intersect(ray, tNearK, indexK) && tNearK < tNear) {
            *hitObject = objects[k];
            tNear = tNearK;
            index = indexK;
        }
    }

    return (*hitObject != nullptr);
}

// Implementation of Path Tracing
Vector3f Scene::castRay(const Ray& ray, int depth, bool onlyDirect) const
{
    float pdfL;
    Vector3f directL(0, 0, 0);
    Vector3f indirectL(0, 0, 0);
    Intersection interP, sampleL;

    interP = bvh->Intersect(ray);

    // not hit bvh
    if (!interP.happened) {
        return backgroundColor;
    }

    // primary hit light
    if (interP.obj->hasEmit()) {
        return interP.m->getEmission();
    }

    // sample in the lights
    sampleLight(sampleL, pdfL);

    // check light path is occlusion or not
    Vector3f woL = normalize(sampleL.coords - interP.coords);
    Ray rayP2L(interP.coords, woL);

    // take attention to ray hit origin inter point
    Intersection interP2L = bvh->Intersect(rayP2L);
    Vector3f vectorP2L = sampleL.coords - interP.coords;
    float distance = sqrt(dotProduct(vectorP2L, vectorP2L));

    // not occlusion and light isn't in object back face
    // if (dotProduct(woL, interP.normal) >= 0 && fabs(distance - interP2L.distance) < EPSILON*20) {
    // 直接光被遮挡
    // 就不用上面的语句了, 因为有折射的材质是允许光来自"另一个面", 折射使用BTDF
    if (fabs(distance - interP2L.distance) < EPSILON * 20) {
        float cosphi2 = dotProduct(rayP2L.direction, sampleL.normal);

        if (cosphi2 < 0) {
            cosphi2 = -cosphi2;
        }

        // 光贡献的radiance: f(p, wi->wo)L(wi)cos(phi)cos(phi2)dA / (L_hit - p) ^ 2 / pdf
        directL = interP.m->eval(ray.direction, rayP2L.direction, interP.normal)
            * sampleL.m->m_emission
            * dotProduct(rayP2L.direction, interP.normal)
            * cosphi2
            /// std::max(EPSILON, dotProduct(vectorP2L, vectorP2L)) 这种方式仍然可能导致结果过大
            / dotProduct(vectorP2L, vectorP2L)
            / pdfL;

        //if (directL.norm() > sampleL.m->m_emission.norm()) {
        //    std::cout << " directL " << directL.norm() << " more than light radiance\n";
        //    directL = sampleL.m->m_emission;
        //}

        //if (directL.x < 0 || directL.y < 0 || directL.z < 0) {
        //    std::cout << "directL is negative\n";
        //}
    }

    // 俄罗斯轮盘发射反射光
    // generate p in [0: 1]. if p < possible then reflect.
    float p = get_random_float();

    // 使用onlyDirect方便忽略间接光, 通过直接光来初步判断结果是否正确
    if (!onlyDirect && p < RussianRoulette) {
        // 给出的wo可能指向表面的背面, wo dot N < 0
        Vector3f wo = interP.m->sample(ray.direction, interP.normal);

        if (dotProduct(wo, interP.normal) > 0) {
            Ray rayP2Wo(interP.coords, wo);
            // 注意作业中如果击中了光源, 没重新选择方向 if not hit light
            Intersection interP2Wo = bvh->Intersect(rayP2Wo);

            if (interP2Wo.happened && !interP2Wo.m->hasEmission()) {
                // f(p, wi->wo)L(wi)cos(phi)dw / possible / pdf
                indirectL = interP.m->eval(ray.direction, rayP2Wo.direction, interP.normal)
                    * castRay(rayP2Wo, ++depth, onlyDirect)
                    * -dotProduct(-rayP2Wo.direction, interP.normal)
                    / RussianRoulette
                    / interP.m->pdf(ray.direction, wo, interP.normal);

                //if (indirectL.x < 0 || indirectL.y < 0 || indirectL.z < 0) {
                //    std::cout << "indirectL is negative\n";
                //}
            }
        }
    }
    return onlyDirect ? directL : directL + indirectL;
}