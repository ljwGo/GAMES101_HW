// clang-format off
//
// Created by goksu on 4/6/19.
//

#include <algorithm>
#include <vector>
#include "rasterizer.hpp"
#include <opencv2/opencv.hpp>
#include <math.h>


rst::pos_buf_id rst::rasterizer::load_positions(const std::vector<Eigen::Vector3f> &positions)
{
    auto id = get_next_id();
    pos_buf.emplace(id, positions);

    return {id};
}

rst::ind_buf_id rst::rasterizer::load_indices(const std::vector<Eigen::Vector3i> &indices)
{
    auto id = get_next_id();
    ind_buf.emplace(id, indices);

    return {id};
}

rst::col_buf_id rst::rasterizer::load_colors(const std::vector<Eigen::Vector3f> &cols)
{
    auto id = get_next_id();
    col_buf.emplace(id, cols);

    return {id};
}

auto to_vec4(const Eigen::Vector3f& v3, float w = 1.0f)
{
    return Vector4f(v3.x(), v3.y(), v3.z(), w);
}

enum class Side{
    Left = 0,
    Right = 1
};

inline Side operator|(Side a, Side b)
{
    return Side((int)a | (int)b);
}

inline Side operator&(Side a, Side b)
{
    return Side((int)a & (int)b);
}

static Side _whichSide(float centerX, float centerY, const Eigen::Vector3f &tailP, const Eigen::Vector3f &headP){
    float vector_x = headP.x() - tailP.x();
    float vector_y = headP.y() - tailP.y();
    float vector_p_x = centerX - tailP.x();
    float vector_p_y = centerY - tailP.y();

    // compare cross vector z value is positive or navigate.
    float vector_res_z = vector_p_x * vector_y - vector_x * vector_p_y;

    if (vector_res_z > 0){
        return Side::Right;
    }
    else if (vector_res_z < 0){
        return Side::Left;
    }
    else{
        return Side::Left;
    }
}


static bool insideTriangle(float x, float y, const Vector3f* _v)
{   
    // TODO : Implement this function to check if the point (x, y) is inside the triangle represented by _v[0], _v[1], _v[2]

    // return point p in the same side or not.
    // bug:: if return 1,0,0. return true.
    // 1==0return0, 0==0return1. can't judge if or the same whichSide.
    // (int)_whichSide(x, y, _v[0], _v[1]) == 
    // (int)_whichSide(x, y, _v[1], _v[2]) ==
    // (int)_whichSide(x, y, _v[2], _v[0]);
    int side01 = (int)_whichSide(x, y, _v[0], _v[1]);
    int side02 = (int)_whichSide(x, y, _v[1], _v[2]);
    int side03 = (int)_whichSide(x, y, _v[2], _v[0]);

    if (side01 != side02 || side01 != side03){
        return false;
    }else{
        return true;
    }
}

static std::tuple<float, float, float> computeBarycentric2D(float x, float y, const Vector3f* v)
{
    float c1 = (x*(v[1].y() - v[2].y()) + (v[2].x() - v[1].x())*y + v[1].x()*v[2].y() - v[2].x()*v[1].y()) / (v[0].x()*(v[1].y() - v[2].y()) + (v[2].x() - v[1].x())*v[0].y() + v[1].x()*v[2].y() - v[2].x()*v[1].y());
    float c2 = (x*(v[2].y() - v[0].y()) + (v[0].x() - v[2].x())*y + v[2].x()*v[0].y() - v[0].x()*v[2].y()) / (v[1].x()*(v[2].y() - v[0].y()) + (v[0].x() - v[2].x())*v[1].y() + v[2].x()*v[0].y() - v[0].x()*v[2].y());
    float c3 = (x*(v[0].y() - v[1].y()) + (v[1].x() - v[0].x())*y + v[0].x()*v[1].y() - v[1].x()*v[0].y()) / (v[2].x()*(v[0].y() - v[1].y()) + (v[1].x() - v[0].x())*v[2].y() + v[0].x()*v[1].y() - v[1].x()*v[0].y());
    return {c1,c2,c3};
}

void rst::rasterizer::draw(pos_buf_id pos_buffer, ind_buf_id ind_buffer, col_buf_id col_buffer, Primitive type)
{
    auto& buf = pos_buf[pos_buffer.pos_id];
    auto& ind = ind_buf[ind_buffer.ind_id];
    auto& col = col_buf[col_buffer.col_id];

    float f1 = (50 - 0.1) / 2.0;
    float f2 = (50 + 0.1) / 2.0;

    Eigen::Matrix4f mvp = projection * view * model;
    for (auto& i : ind)
    {
        Triangle t;
        Eigen::Vector4f v[] = {
                mvp * to_vec4(buf[i[0]], 1.0f),
                mvp * to_vec4(buf[i[1]], 1.0f),
                mvp * to_vec4(buf[i[2]], 1.0f)
        };
        //Homogeneous division
        for (auto& vec : v) {
            vec /= vec.w();
        }
        //Viewport transformation
        for (auto & vert : v)
        {
            vert.x() = 0.5*width*(vert.x()+1.0);
            vert.y() = 0.5*height*(vert.y()+1.0);
            vert.z() = vert.z() * -f1 + f2;
        }

        for (int i = 0; i < 3; ++i)
        {
            t.setVertex(i, v[i].head<3>());
            //t.setVertex(i, v[i].head<3>());
            //t.setVertex(i, v[i].head<3>());
        }

        auto col_x = col[i[0]];
        auto col_y = col[i[1]];
        auto col_z = col[i[2]];

        t.setColor(0, col_x[0], col_x[1], col_x[2]);
        t.setColor(1, col_y[0], col_y[1], col_y[2]);
        t.setColor(2, col_z[0], col_z[1], col_z[2]);

        rasterize_triangle(t);
    }
}

//Screen space rasterization
void rst::rasterizer::rasterize_triangle(const Triangle& t) {
    auto v = t.toVector4();
    
    // TODO : Find out the bounding box of current triangle.
    // iterate through the pixel and find if the current pixel is inside the triangle
    float minXf, maxXf, minYf, maxYf;
    int minXi, maxXi, minYi, maxYi;

    // init use first point value
    minXf = maxXf = t.v[0].x();
    minYf = maxYf = t.v[0].y();

    // find min value.
    for (auto &vertex : t.v){
         if (vertex.x() < minXf){
            minXf = vertex.x();
         }else if(vertex.x() > maxXf){
            maxXf = vertex.x();
         }

         if (vertex.y() < minYf){
            minYf = vertex.y();
         }else if(vertex.y() > maxYf){
            maxYf = vertex.y();
         }
    }

    // 确定边界
    minXi = floor(minXf);
    maxXi = ceil(maxXf);
    minYi = floor(minYf);
    maxYi = ceil(maxYf);

    // 采样边界
    for (int i=minXi; i<=maxXi; i++){
        for (int j=minYi; j<=maxYi; j++){
            // // 像素中心
            // float centerX = i + 0.5, centerY = j + 0.5;

            // // 如果在三角形内
            // if (insideTriangle(centerX, centerY, t.v)){
            //     // 计算该像素在三角形中的深度
            //     float x=i+0.5f, y=j+0.5f;

            //     auto[alpha, beta, gamma] = computeBarycentric2D(x, y, t.v);
            //     reciprocal意思是相互的相反的，可能是为了消除向量齐次坐标下w不为1时，计算会出现系数上偏差
            //     float w_reciprocal = 1.0/(alpha / v[0].w() + beta / v[1].w() + gamma / v[2].w());
            //     float z_interpolated = alpha * v[0].z() / v[0].w() + beta * v[1].z() / v[1].w() + gamma * v[2].z() / v[2].w();
            //     z_interpolated *= w_reciprocal;

            //     // 不考虑相等的情况
            //     int index = get_index(i, j);
            //     // 注意depth_buf默认值为0,而z_interpolated为正数
            //     if (z_interpolated < depth_buf[index] || depth_buf[index] == 0){
            //         depth_buf[index] = z_interpolated;
            //         // 更近，渲染新的颜色
            //         Eigen::Vector3f tempP(i, j, 1.0);
            //         set_pixel(tempP, t.getColor());
            //     }
            // }

            // bonus
            _MSAA2_2(i, j, t, v);
        }
    }

    //Depth interpolation
    // If so, use the following code to get the interpolated z value.
    //auto[alpha, beta, gamma] = computeBarycentric2D(x, y, t.v);
    //float w_reciprocal = 1.0/(alpha / v[0].w() + beta / v[1].w() + gamma / v[2].w());
    //float z_interpolated = alpha * v[0].z() / v[0].w() + beta * v[1].z() / v[1].w() + gamma * v[2].z() / v[2].w();
    //z_interpolated *= w_reciprocal;

    // TODO : set the current pixel (use the set_pixel function) to the color of the triangle (use getColor function) if it should be painted.
    // implement above;
}

void rst::rasterizer::set_model(const Eigen::Matrix4f& m)
{
    model = m;
}

void rst::rasterizer::set_view(const Eigen::Matrix4f& v)
{
    view = v;
}

void rst::rasterizer::_MSAA2_2(int x, int y, const Triangle& t, std::array<Eigen::Vector4f, 3> v){

    int index = get_index(x, y);

    for (int i=1; i<=2; i++){
        for (int j=1; j<=2; j++){
            float xAxis = x + i*0.333333f;
            float yAxis = y + j*0.333333f;
            // super sample index in array
            int super_sample_index = (i-1)*2 + (j-1);
            // sample super point
            if (insideTriangle(xAxis, yAxis, t.v)){
                // Central coordinate
                auto[alpha, beta, gamma] = computeBarycentric2D(xAxis, yAxis, t.v);
                // depth lerp
                float w_reciprocal = 1.0/(alpha / v[0].w() + beta / v[1].w() + gamma / v[2].w());
                float z_interpolated = alpha * v[0].z() / v[0].w() + beta * v[1].z() / v[1].w() + gamma * v[2].z() / v[2].w();
                z_interpolated *= w_reciprocal;

                // gain super sample depth value.
                float cur_depth = super_sample_depth_buf[index][super_sample_index];
                // depth compare
                if (z_interpolated < cur_depth || cur_depth == 0){
                    super_sample_depth_buf[index][super_sample_index] = z_interpolated;
                    
                    super_sample_frame_buf[index][super_sample_index] = t.getColor() * 0.25f;
                }
            }
        }
    }
    
    // set color
    Eigen::Vector3f newColor{0, 0, 0};
    for (auto color : super_sample_frame_buf[index]){
        newColor += color;
    }

    Eigen::Vector3f tempPos(x, y, 1.0);
    set_pixel(tempPos, newColor);
}

void rst::rasterizer::set_projection(const Eigen::Matrix4f& p)
{
    projection = p;
}

void rst::rasterizer::clear(rst::Buffers buff)
{
    if ((buff & rst::Buffers::Color) == rst::Buffers::Color)
    {
        std::fill(frame_buf.begin(), frame_buf.end(), Eigen::Vector3f{0, 0, 0});
    }
    if ((buff & rst::Buffers::Depth) == rst::Buffers::Depth)
    {
        std::fill(depth_buf.begin(), depth_buf.end(), std::numeric_limits<float>::infinity());
    }
    if ((buff & rst::Buffers::SuperSampleDepth) == rst::Buffers::SuperSampleDepth)
    {
        for (auto array : super_sample_depth_buf){
            std::fill(array.begin(), array.end(), std::numeric_limits<float>::infinity());
        }
    }
    if ((buff & rst::Buffers::SuperSampleColor) == rst::Buffers::SuperSampleColor)
    {
        for (auto array : super_sample_frame_buf){
            std::fill(array.begin(), array.end(), Eigen::Vector3f{0, 0, 0});
        }
    }
}

rst::rasterizer::rasterizer(int w, int h) : width(w), height(h)
{
    frame_buf.resize(w * h);
    depth_buf.resize(w * h);
    super_sample_depth_buf.resize(w * h);
    super_sample_frame_buf.resize(w * h);
}

int rst::rasterizer::get_index(int x, int y)
{
    return (height-1-y)*width + x;
}

void rst::rasterizer::set_pixel(const Eigen::Vector3f& point, const Eigen::Vector3f& color)
{
    //old index: auto ind = point.y() + point.x() * width;
    auto ind = (height-1-point.y())*width + point.x();
    frame_buf[ind] = color;

}

// clang-format on