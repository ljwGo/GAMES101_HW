#include "Triangle.hpp"
#include "rasterizer.hpp"
#include "Eigen/Eigen"
#include <iostream>
#include <opencv2/opencv.hpp>
#include <cmath>

constexpr double MY_PI = 3.1415926;

Eigen::Matrix4f get_view_matrix(Eigen::Vector3f eye_pos)
{
    Eigen::Matrix4f view = Eigen::Matrix4f::Identity();

    Eigen::Matrix4f translate;
    translate << 1, 0, 0, -eye_pos[0], 0, 1, 0, -eye_pos[1], 0, 0, 1,
        -eye_pos[2], 0, 0, 0, 1;

    view = translate * view;

    return view;
}

Eigen::Matrix4f get_model_matrix(float rotation_angle)
{
    Eigen::Matrix4f model = Eigen::Matrix4f::Identity();

    // TODO: Implement this function
    // Create the model matrix for rotating the triangle around the Z axis.
    // Then return it.

    float radian = rotation_angle / 180.0 * MY_PI;

    Eigen::Matrix4f rotate;
    rotate << 
        cos(radian), -sin(radian), 0, 0,
        sin(radian), cos(radian), 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1;

    model = rotate * model;
    return model;
}

Eigen::Matrix4f get_projection_matrix(float eye_fov, float aspect_ratio,
                                      float zNear, float zFar)
{
    // Students will implement this function

    Eigen::Matrix4f projection = Eigen::Matrix4f::Identity();

    // TODO: Implement this function
    // Create the projection matrix for the given parameters.
    // Then return it.

    Eigen::Matrix4f Perspective2Ortho;
    Perspective2Ortho << 
        zNear, 0, 0, 0,
        0, zNear, 0, 0,
        0, 0, zNear + zFar, -zNear * zFar,
        0, 0, 1, 0;

    float halfWidth, halfHeight, halfRadian;

    halfRadian = eye_fov * 0.5 / 180 * MY_PI;
    halfHeight = zNear * tan(halfRadian);
    halfWidth = halfHeight * aspect_ratio;

    Eigen::Matrix4f Orthogonality;
    Orthogonality << 
        1.0 / halfWidth, 0, 0, 0,
        0, 1.0 / halfHeight, 0, 0,
        0, 0, 2.0 / (zNear - zFar), 0.5 * (zNear + zFar),
        0, 0, 0, 1;

    projection = Orthogonality * Perspective2Ortho * projection;

    return projection;
}

Eigen::Matrix4f get_rotation(Eigen::Vector4f axis, float angle){
    Eigen::Matrix4f axisRotate2Z;

    Eigen::Matrix4f axisRotateByX;
    float angleByX = atan(axis.y() / axis.z());
// std::cout << "angleByX: " << angleByX << std::endl;

    // x value will not change.
    axisRotateByX << 1, 0, 0, 0,
                     0, cos(angleByX), -sin(angleByX), 0,
                     0, sin(angleByX), cos(angleByX), 0,
                     0, 0, 0, 1;

// std::cout << "axisRotateByX * axis: " << (axisRotateByX * axis).transpose() << std::endl;

    Eigen::Matrix4f axisRotateByY;
    float angleByY = acos(axis.x() / axis.norm());
    angleByY = -(0.5 * MY_PI - angleByY);

// std::cout << "angleByY: " << angleByY << std::endl;

    // y value will not change. 
    axisRotateByY << cos(angleByY), 0, sin(angleByY), 0,
                     0, 1, 0, 0,
                     -sin(angleByY), 0, cos(angleByY), 0,
                     0, 0, 0, 1;

    // change axis to Z axis.
    axisRotate2Z = axisRotateByY * axisRotateByX;

    return axisRotate2Z.transpose() * get_model_matrix(angle) * axisRotate2Z;
}


int main(int argc, const char** argv)
{
    float angle = 0;
    bool command_line = false;
    std::string filename = "output.png";

    if (argc >= 3) {
        command_line = true;
        angle = std::stof(argv[2]); // -r by default
        if (argc == 4) {
            filename = std::string(argv[3]);
        }
        else
            return 0;
    }

    rst::rasterizer r(700, 700);

    Eigen::Vector3f eye_pos = {0, 0, 5};

    std::vector<Eigen::Vector3f> pos{{2, 0, -2}, {0, 2, -2}, {-2, 0, -2}};

    std::vector<Eigen::Vector3i> ind{{0, 1, 2}};

    auto pos_id = r.load_positions(pos);
    auto ind_id = r.load_indices(ind);

    int key = 0;
    int frame_count = 0;

    if (command_line) {
        r.clear(rst::Buffers::Color | rst::Buffers::Depth);

        r.set_model(get_model_matrix(angle));
        r.set_view(get_view_matrix(eye_pos));
        r.set_projection(get_projection_matrix(45, 1, 0.1, 50));

        r.draw(pos_id, ind_id, rst::Primitive::Triangle);
        cv::Mat image(700, 700, CV_32FC3, r.frame_buffer().data());
        image.convertTo(image, CV_8UC3, 1.0f);

        cv::imwrite(filename, image);

        return 0;
    }

    while (key != 27) {
        r.clear(rst::Buffers::Color | rst::Buffers::Depth);

        // r.set_model(get_model_matrix(angle));
        r.set_model(get_rotation(Eigen::Vector4f(1,1,1,0), angle));
        r.set_view(get_view_matrix(eye_pos));
        r.set_projection(get_projection_matrix(45, 1, 0.1, 50));

        r.draw(pos_id, ind_id, rst::Primitive::Triangle);

        cv::Mat image(700, 700, CV_32FC3, r.frame_buffer().data());
        image.convertTo(image, CV_8UC3, 1.0f);
        cv::imshow("image", image);
        key = cv::waitKey(10);

        std::cout << "frame count: " << frame_count++ << '\n';

        if (key == 'a') {
            angle += 10;
        }
        else if (key == 'd') {
            angle -= 10;
        }
    }

    return 0;
}
