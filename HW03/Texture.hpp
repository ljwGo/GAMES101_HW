//
// Created by LEI XU on 4/27/19.
//

#ifndef RASTERIZER_TEXTURE_H
#define RASTERIZER_TEXTURE_H
#include "global.hpp"
#include <Eigen/Eigen>
#include <opencv2/opencv.hpp>
#include <iostream>

class Texture
{
private:
    cv::Mat image_data;

public:
    Texture(const std::string &name)
    {
        image_data = cv::imread(name);
        cv::cvtColor(image_data, image_data, cv::COLOR_RGB2BGR);
        width = image_data.cols;
        height = image_data.rows;

        std::cout << "Texture name: " << name << std::endl;
        std::cout << "Texture size: " << width << " x " << height << std::endl;
    }

    int width, height;

    Eigen::Vector3f getColor(float u, float v)
    {
        // 加的代码，防止u，v超过1
        if (u > 1.f)
        {
            u = 1.f;
        }
        if (v > 1.f)
        {
            v = 1.f;
        }

        auto u_img = u * width;
        auto v_img = (1 - v) * height;
        u_img = std::clamp(u_img, 0.f, (float)(width-1));
        v_img = std::clamp(v_img, 0.f, (float)(height - 1));
        auto color = image_data.at<cv::Vec3b>(v_img, u_img);
        return Eigen::Vector3f(color[0], color[1], color[2]);
    }

    Eigen::Vector3f getColorBilinear(float u, float v);
};
#endif // RASTERIZER_TEXTURE_H
