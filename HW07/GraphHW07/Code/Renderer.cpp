//
// Created by goksu on 2/25/20.
//
#include <fstream>
#include "Scene.hpp"
#include "Renderer.hpp"
#include <thread>


inline float deg2rad(const float& deg) { return deg * M_PI / 180.0; }

const float EPSILON = 0.00001;

void Renderer::operator() (Scene const& scene, float const scale, float const imageAspectRatio, int const rowIx) {
    // less fine-gradient, one thread handle one row
    float x, y;
    size_t baseIx = rowIx * scene.width;

    y = (1 - 2 * (rowIx + 0.5) * scene.invHeight) * scale;
    for (int i = 0; i < scene.width; ++i) {
        // generate primary ray
        x = (2 * (i + 0.5) * scene.invWidth - 1) * imageAspectRatio * scale;

        Ray primaryRay(scene.eyePos, normalize(Vector3f(-x, y, 1)));

        // cache-line friendly
        for (int k = 0; k < spp; ++k) {
            framebuffer[baseIx + i] += scene.castRay(primaryRay, 0, false) * invSpp;
        }
    }
}

// The main render function. This where we iterate over all pixels in the image,
// generate primary rays and cast these rays into the scene. The content of the
// framebuffer is saved to a file.
void Renderer::RenderMultipleThread(const Scene& scene)
{
    float scale = tan(deg2rad(scene.fov * 0.5));
    float imageAspectRatio = scene.width / (float)scene.height;
    framebuffer = std::vector<Vector3f>(scene.width * scene.height);

    std::vector<std::thread> threads(threadCount);

    std::cout << "SPP: " << spp << "\n";

    int curRowIx = 0;
    while (curRowIx < scene.height) {
        for (int k = 0; k < threadCount; ++k) {
            // 这句话编译报错, 尝试引用删除了的函数-->scene改为std::ref(scene), *this也要改为std::ref(*this)
            threads[k] = std::thread(std::ref(*this), std::ref(scene), scale, imageAspectRatio, curRowIx++);

            if (curRowIx >= scene.height) {
                break;
            }
        }

        for (int k = 0; k < threads.size(); ++k) {
            if (threads[k].joinable()) {
                threads[k].join();
                UpdateProgress((float)curRowIx / scene.height);
            }
        }
    }
    UpdateProgress(1.f);

    // save framebuffer to file
    FILE* fp;
    errno_t error = fopen_s(&fp, "binary.ppm", "wb");
    std::cout << "error_t: " << error << std::endl;
    if (!error) {
        (void)fprintf(fp, "P6\n%d %d\n255\n", scene.width, scene.height);
        for (auto i = 0; i < scene.height * scene.width; ++i) {
            static unsigned char color[3];
            color[0] = (unsigned char)(255 * std::pow(clamp(0, 1, this->framebuffer[i].x), 0.6f));
            color[1] = (unsigned char)(255 * std::pow(clamp(0, 1, this->framebuffer[i].y), 0.6f));
            color[2] = (unsigned char)(255 * std::pow(clamp(0, 1, this->framebuffer[i].z), 0.6f));
            fwrite(color, 1, 3, fp);
        }
        fclose(fp);
    }
}

void Renderer::Render(const Scene& scene)
{
    framebuffer = std::vector<Vector3f>(scene.width * scene.height);

    float scale = tan(deg2rad(scene.fov * 0.5));
    float imageAspectRatio = scene.width / (float)scene.height;

    int m = 0;
    std::cout << "SPP: " << spp << "\n";
    for (uint32_t j = 0; j < scene.height; ++j) {
        for (uint32_t i = 0; i < scene.width; ++i) {
            float x = (2 * (i + 0.5) * scene.invWidth - 1) *
                imageAspectRatio * scale;
            float y = (1 - 2 * (j + 0.5) * scene.invHeight) * scale;

            // 妈的,这里是-x, z是1
            Vector3f dir = Vector3f(-x, y, 1);
            Ray primary(scene.eyePos, normalize(dir));

            for (int k = 0; k < spp; k++) {
                framebuffer[m] += scene.castRay(primary, 0) * invSpp;
            }
            m++;
        }
        float process = j / (float)scene.height;
        UpdateProgress(process);
    }
    UpdateProgress(1.f);

    // save framebuffer to file
    FILE* fp = nullptr;
    errno_t e = fopen_s(&fp, "binary.ppm", "wb");
    if (!e) {
        (void)fprintf(fp, "P6\n%d %d\n255\n", scene.width, scene.height);
        for (auto i = 0; i < scene.height * scene.width; ++i) {
            static unsigned char color[3];
            color[0] = (unsigned char)(255 * clamp(0, 1, framebuffer[i].x));
            color[1] = (unsigned char)(255 * clamp(0, 1, framebuffer[i].y));
            color[2] = (unsigned char)(255 * clamp(0, 1, framebuffer[i].z));
            fwrite(color, 1, 3, fp);
        }
        fclose(fp);
    }
}
