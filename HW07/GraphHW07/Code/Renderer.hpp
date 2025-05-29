//
// Created by goksu on 2/25/20.
//
#include "Scene.hpp"

#pragma once
struct hit_payload
{
    float tNear;
    uint32_t index;
    Vector2f uv;
    Object* hit_obj;
};

class Renderer
{
public:
    void Render(const Scene& scene);
    Renderer() {
        invSpp = 1. / spp;
    }
    void operator() (Scene const& scene, float const scale, float const imageAspectRatio, int const rowIx);
    void RenderMultipleThread(const Scene& scene);

    //std::vector<Vector3f> framebuffer;
    // change the spp value to change sample ammount
    float invSpp;
    int spp = 32;
    int threadCount = 8;
    std::vector<Vector3f> framebuffer;
private:
};
