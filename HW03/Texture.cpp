//
// Created by LEI XU on 4/27/19.
//

#include "Texture.hpp"

Eigen::Vector3f Texture::getColorBilinear(float u, float v)
{
    // prevent from u, v bigger 1
    if (u > 1.f)
    {
        u = 1.f;
    }
    if (v > 1.f)
    {
        v = 1.f;
    }

    // bilinear interpolation need 4 pixels.
    auto u_img = u * width;
    auto v_img = (1 - v) * height;

    int u_left_col = floor(u_img);
    int u_right_col = ceil(u_img);
    int v_top_row = floor(v_img);
    int v_bottom_row = ceil(v_img);

    // boundary handle.
    if (v_bottom_row > height)
    {
        v_bottom_row = height;
    }
    else if (v_top_row < 1)
    {
        v_top_row = 1;
    }

    if (u_left_col < 1)
    {
        u_left_col = 1;
    }
    else if (u_right_col > width)
    {
        u_right_col = width;
    }

    // leftTop color
    auto _ltColor = getColor((float)u_left_col / width, 1.f - (float)v_top_row / height);
    auto _lbColor = getColor((float)u_left_col / width, 1.f - (float)v_bottom_row / height);
    auto _rtColor = getColor((float)u_right_col / width, 1.f - (float)v_top_row / height);
    auto _rbColor = getColor((float)u_right_col / width, 1.f - (float)v_bottom_row / height);

    Eigen::Vector3f ltColor(_ltColor[0], _ltColor[1], _ltColor[2]);
    Eigen::Vector3f lbColor(_lbColor[0], _lbColor[1], _lbColor[2]);
    Eigen::Vector3f rtColor(_ltColor[0], _ltColor[1], _ltColor[2]);
    Eigen::Vector3f rbColor(_rbColor[0], _rbColor[1], _rbColor[2]);
    
    // first interpolation
    Eigen::Vector3f color01 = rtColor * (u_img - u_left_col) + ltColor * (u_right_col - u_img);

    // second interpolation
    Eigen::Vector3f color02 = rbColor * (u_img - u_left_col) + lbColor * (u_right_col - u_img);

    // third interpolation
    Eigen::Vector3f color = color01 * (v_bottom_row - v_img) + color02 * (v_img - v_top_row);

    return color;
}