#include <iostream>
#include <opencv2/opencv.hpp>
#include <math.h>

#include "global.hpp"
#include "rasterizer.hpp"
#include "Triangle.hpp"
#include "Shader.hpp"
#include "Texture.hpp"
#include "OBJ_Loader.h"
#include "Eigen/Eigen"

Eigen::Matrix4f get_view_matrix(Eigen::Vector3f eye_pos)
{
    Eigen::Matrix4f view = Eigen::Matrix4f::Identity();

    // 有很麻烦的问题，我得到的牛的坐标z值和作业pdf上的图的值是相反数，我目前将原因归结为摄像机一开始朝向z轴，view变换矩阵第三行第三个值改为-1使摄像机朝向-z轴
    Eigen::Matrix4f translate;
    translate << 1, 0, 0, -eye_pos[0],
        0, 1, 0, -eye_pos[1],
        0, 0, 1, -eye_pos[2],
        0, 0, 0, 1;

    view = translate * view;

    return view;
}

Eigen::Matrix4f get_model_matrix(float angle)
{
    Eigen::Matrix4f rotation;
    angle = angle * MY_PI / 180.f;
    rotation << cos(angle), 0, sin(angle), 0,
        0, 1, 0, 0,
        -sin(angle), 0, cos(angle), 0,
        0, 0, 0, 1;

    Eigen::Matrix4f scale;
    scale << 2.5, 0, 0, 0,
        0, 2.5, 0, 0,
        0, 0, 2.5, 0,
        0, 0, 0, 1;

    Eigen::Matrix4f translate;
    translate << 1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1;

    return translate * rotation * scale;
}

// important! 这里传进来的zNear和zFar都是正值，在右手系下，相机朝向是-z轴，应次zNear和zFar都是负值
Eigen::Matrix4f get_projection_matrix(float eye_fov, float aspect_ratio, float zNear, float zFar)
{
    zNear = -zNear;
    zFar = -zFar;
    // TODO: Use the same projection matrix from the previous assignments
    Eigen::Matrix4f projection = Eigen::Matrix4f::Identity();

    Eigen::Matrix4f Perspective2Ortho;
    Perspective2Ortho << zNear, 0, 0, 0,
        0, zNear, 0, 0,
        0, 0, zNear + zFar, -zNear * zFar,
        0, 0, 1, 0;

    float halfWidth, halfHeight, halfRadian;

    halfRadian = eye_fov * 0.5 / 180 * MY_PI;
    halfHeight = abs(zNear) * tan(halfRadian);
    halfWidth = halfHeight * aspect_ratio;

    // 这里定义了orhtogonality的左边l，右边r，上边t，下边b，近处n，远处f
    Eigen::Matrix4f Orthogonality;
    Eigen::Matrix4f OrthTranslate;
    Eigen::Matrix4f OrthScale;
    // 注意先平移再缩放，得到的Ortho矩阵平移值由于缩放矩阵是会改变的，这个bug找了挺久的
    OrthScale << 1.0 / halfWidth, 0, 0, 0,
        0, 1.0 / halfHeight, 0, 0,
        0, 0, 2.0 / (zNear - zFar), 0,
        0, 0, 0, 1.0;

    OrthTranslate << 1.f, 0, 0, 0,
        0, 1.f, 0, 0,
        0, 0, 1.f, -0.5f * (zNear + zFar),
        0, 0, 0, 1.f;

    Orthogonality = OrthScale * OrthTranslate;

    projection = Orthogonality * Perspective2Ortho * projection;

    return projection;
}

void test_projection()
{
    Eigen::Matrix4f projection = get_projection_matrix(90, 1, 10, 100);
    Eigen::Vector4f test_vector;
    test_vector << 100, 100, -100, 1;
    std::cout << projection * test_vector;
}

Eigen::Vector3f vertex_shader(const vertex_shader_payload &payload)
{
    return payload.position;
}

Eigen::Vector3f normal_fragment_shader(const fragment_shader_payload &payload)
{
    Eigen::Vector3f return_color = (payload.normal.head<3>().normalized() + Eigen::Vector3f(1.0f, 1.0f, 1.0f)) / 2.f;
    Eigen::Vector3f result;
    result << return_color.x() * 255, return_color.y() * 255, return_color.z() * 255;
    return result;
}

// 求反射方向
static Eigen::Vector3f reflect(const Eigen::Vector3f &vec, const Eigen::Vector3f &axis)
{
    auto costheta = vec.dot(axis);
    return (2 * costheta * axis - vec).normalized();
}

struct light
{
    Eigen::Vector3f position;
    Eigen::Vector3f intensity;
};

Eigen::Vector3f texture_fragment_shader(const fragment_shader_payload &payload)
{
    Eigen::Vector3f return_color = {0, 0, 0};
    if (payload.texture)
    {
        // TODO: Get the texture value at the texture coordinates of the current fragment
        // 从材质中采样颜色是通过Eigen的接口实现的
        // return_color = payload.texture->getColor(payload.tex_coords.x(), payload.tex_coords.y());
        return_color = payload.texture->getColorBilinear(payload.tex_coords.x(), payload.tex_coords.y());
    }
    Eigen::Vector3f texture_color;
    texture_color << return_color.x(), return_color.y(), return_color.z();

    // ka是环境光遮蔽项
    Eigen::Vector3f ka = Eigen::Vector3f(0.005, 0.005, 0.005);
    Eigen::Vector3f kd = texture_color / 255.f;
    Eigen::Vector3f ks = Eigen::Vector3f(0.7937, 0.7937, 0.7937);

    auto l1 = light{{20, 20, 20}, {500, 500, 500}};
    auto l2 = light{{-20, 20, 0}, {500, 500, 500}};

    std::vector<light> lights = {l1, l2};
    // 假设间接光是个常量
    Eigen::Vector3f amb_light_intensity{10, 10, 10};
    Eigen::Vector3f eye_pos{0, 0, 10};

    float p = 150;

    Eigen::Vector3f color = texture_color;
    Eigen::Vector3f point = payload.view_pos;
    Eigen::Vector3f normal = payload.normal;

    Eigen::Vector3f diffuseLight;
    Eigen::Vector3f specularLight;
    Eigen::Vector3f ambientLight;
    Eigen::Vector3f shaderPoint2Light;
    Eigen::Vector3f halfWayVector;
    Eigen::Vector3f shaderPoint2Eye;

    float diffuseLightWeakness;
    float specularLightWeakness;

    Eigen::Vector3f result_color = {0, 0, 0};

    for (auto &light : lights)
    {
        // TODO: For each light source in the code, calculate what the *ambient*, *diffuse*, and *specular*
        // components are. Then, accumulate that result on the *result_color* object.

        shaderPoint2Light = light.position - point;
        diffuseLightWeakness = std::max(normal.dot(shaderPoint2Light.normalized()), 0.f) / pow(shaderPoint2Light.norm(), 2);

        // diffuseLight = kd * (intensity / pow(light2shader_point_distance, 2)) * dot(light2shader, normal);
        diffuseLight[0] = kd[0] * light.intensity[0] * diffuseLightWeakness;
        diffuseLight[1] = kd[1] * light.intensity[1] * diffuseLightWeakness;
        diffuseLight[2] = kd[2] * light.intensity[2] * diffuseLightWeakness;

        shaderPoint2Eye = eye_pos - point;
        halfWayVector = (shaderPoint2Eye.normalized() + shaderPoint2Light.normalized()).normalized();
        specularLightWeakness = pow(normal.dot(halfWayVector), p) / pow(shaderPoint2Light.norm(), 2);

        // specularLight = ks * (intensity / pow(light2shader_point_distance, 2)) * pow(cos(h, normal), 150));
        specularLight[0] = ks[0] * light.intensity[0] * specularLightWeakness;
        specularLight[1] = ks[1] * light.intensity[1] * specularLightWeakness;
        specularLight[2] = ks[2] * light.intensity[2] * specularLightWeakness;

        result_color += diffuseLight + specularLight;
    }

    // ambientLight = ka * amb_light_intensity;
    ambientLight[0] = ka.x() * amb_light_intensity.x();
    ambientLight[1] = ka.y() * amb_light_intensity.y();
    ambientLight[2] = ka.z() * amb_light_intensity.z();

    result_color += ambientLight;

    return result_color * 255.f;
}

Eigen::Vector3f phong_fragment_shader(const fragment_shader_payload &payload)
{
    Eigen::Vector3f ka = Eigen::Vector3f(0.005, 0.005, 0.005);
    Eigen::Vector3f kd = payload.color;
    //  Eigen::Vector3f kd = Eigen::Vector3f(1.0, 1.0, 1.0);
    Eigen::Vector3f ks = Eigen::Vector3f(0.7937, 0.7937, 0.7937);

    // light[postion, intensity]
    auto l1 = light{{20, 20, 20}, {500, 500, 500}};
    auto l2 = light{{-20, 20, 0}, {500, 500, 500}};

    std::vector<light> lights = {l1, l2};
    // enviroment light.
    Eigen::Vector3f amb_light_intensity{10, 10, 10};
    Eigen::Vector3f eye_pos{0, 0, 10};

    float p = 150;

    Eigen::Vector3f color = payload.color;
    Eigen::Vector3f point = payload.view_pos;
    Eigen::Vector3f normal = payload.normal;

    Eigen::Vector3f diffuseLight;
    Eigen::Vector3f specularLight;
    Eigen::Vector3f ambientLight;
    Eigen::Vector3f shaderPoint2Light;
    Eigen::Vector3f halfWayVector;
    Eigen::Vector3f shaderPoint2Eye;

    float diffuseLightWeakness;
    float specularLightWeakness;

    Eigen::Vector3f result_color = {0, 0, 0};
    for (auto &light : lights)
    {
        // TODO: For each light source in the code, calculate what the *ambient*, *diffuse*, and *specular*
        // components are. Then, accumulate that result on the *result_color* object.

        shaderPoint2Light = light.position - point;
        diffuseLightWeakness = std::max(normal.dot(shaderPoint2Light.normalized()), 0.f) / pow(shaderPoint2Light.norm(), 2);

        // diffuseLight = kd * (intensity / pow(light2shader_point_distance, 2)) * dot(light2shader, normal);
        diffuseLight[0] = kd[0] * light.intensity[0] * diffuseLightWeakness;
        diffuseLight[1] = kd[1] * light.intensity[1] * diffuseLightWeakness;
        diffuseLight[2] = kd[2] * light.intensity[2] * diffuseLightWeakness;

        shaderPoint2Eye = eye_pos - point;
        halfWayVector = (shaderPoint2Eye.normalized() + shaderPoint2Light.normalized()).normalized();
        specularLightWeakness = pow(normal.dot(halfWayVector), p) / pow(shaderPoint2Light.norm(), 2);

        // specularLight = ks * (intensity / pow(light2shader_point_distance, 2)) * pow(cos(h, normal), 150));
        specularLight[0] = ks[0] * light.intensity[0] * specularLightWeakness;
        specularLight[1] = ks[1] * light.intensity[1] * specularLightWeakness;
        specularLight[2] = ks[2] * light.intensity[2] * specularLightWeakness;

        result_color += diffuseLight + specularLight;
    }

    // ambientLight = ka * amb_light_intensity;
    ambientLight[0] = ka.x() * amb_light_intensity.x();
    ambientLight[1] = ka.y() * amb_light_intensity.y();
    ambientLight[2] = ka.z() * amb_light_intensity.z();

    result_color += ambientLight;

    return result_color * 255.f;
}

Eigen::Vector3f displacement_fragment_shader(const fragment_shader_payload &payload)
{

    Eigen::Vector3f ka = Eigen::Vector3f(0.005, 0.005, 0.005);
    Eigen::Vector3f kd = payload.color;
    Eigen::Vector3f ks = Eigen::Vector3f(0.7937, 0.7937, 0.7937);

    auto l1 = light{{20, 20, 20}, {500, 500, 500}};
    auto l2 = light{{-20, 20, 0}, {500, 500, 500}};

    std::vector<light> lights = {l1, l2};
    Eigen::Vector3f amb_light_intensity{10, 10, 10};
    Eigen::Vector3f eye_pos{0, 0, 10};

    float p = 150;

    Eigen::Vector3f color = payload.color;
    Eigen::Vector3f point = payload.view_pos;
    Eigen::Vector3f normal = payload.normal;

    Eigen::Vector3f diffuseLight;
    Eigen::Vector3f specularLight;
    Eigen::Vector3f ambientLight;
    Eigen::Vector3f shaderPoint2Light;
    Eigen::Vector3f halfWayVector;
    Eigen::Vector3f shaderPoint2Eye;

    float diffuseLightWeakness;
    float specularLightWeakness;

    float kh = 0.2, kn = 0.1;

    // TODO: Implement displacement mapping here
    // Let n = normal = (x, y, z)
    // Vector t = (x*y/sqrt(x*x+z*z),sqrt(x*x+z*z),z*y/sqrt(x*x+z*z))
    // Vector b = n cross product t
    // Matrix TBN = [t b n]
    // dU = kh * kn * (h(u+1/w,v)-h(u,v))
    // dV = kh * kn * (h(u,v+1/h)-h(u,v))
    // Vector ln = (-dU, -dV, 1)
    // Position p = p + kn * n * h(u,v)
    // Normal n = normalize(TBN * ln)

    Eigen::Vector3f result_color = {0, 0, 0};

    Eigen::Vector3f n = normal;
    Eigen::Vector3f t;
    t << n.x() * n.y() / sqrt(n.x() * n.x() + n.z() * n.z()),
        sqrt(n.x() * n.x() + n.z() * n.z()),
        n.z() * n.y() / sqrt(n.x() * n.x() + n.z() * n.z());
    Eigen::Vector3f b = t.cross(n);

    Eigen::Matrix3f TBN;
    TBN << t, b, n;

    float du = kn * kh * (payload.texture->getColor(payload.tex_coords[0] + 1.f / payload.texture->width, payload.tex_coords[1]).norm() - payload.texture->getColor(payload.tex_coords[0], payload.tex_coords[1]).norm());
    float dv = kn * kh * (payload.texture->getColor(payload.tex_coords[0], payload.tex_coords[1] + 1.f / payload.texture->height).norm() - payload.texture->getColor(payload.tex_coords[0], payload.tex_coords[1]).norm());

    Eigen::Vector3f ln;
    ln << -du, -dv, 1;

    point += kn * n * payload.texture->getColor(payload.tex_coords[0], payload.tex_coords[1]).norm();

    normal = TBN * ln;
    normal.normalize();

    for (auto &light : lights)
    {
        shaderPoint2Light = light.position - point;
        diffuseLightWeakness = std::max(normal.dot(shaderPoint2Light.normalized()), 0.f) / pow(shaderPoint2Light.norm(), 2);

        // diffuseLight = kd * (intensity / pow(light2shader_point_distance, 2)) * dot(light2shader, normal);
        diffuseLight[0] = kd[0] * light.intensity[0] * diffuseLightWeakness;
        diffuseLight[1] = kd[1] * light.intensity[1] * diffuseLightWeakness;
        diffuseLight[2] = kd[2] * light.intensity[2] * diffuseLightWeakness;

        shaderPoint2Eye = eye_pos - point;
        halfWayVector = (shaderPoint2Eye.normalized() + shaderPoint2Light.normalized()).normalized();
        specularLightWeakness = pow(normal.dot(halfWayVector), p) / pow(shaderPoint2Light.norm(), 2);

        // specularLight = ks * (intensity / pow(light2shader_point_distance, 2)) * pow(cos(h, normal), 150));
        specularLight[0] = ks[0] * light.intensity[0] * specularLightWeakness;
        specularLight[1] = ks[1] * light.intensity[1] * specularLightWeakness;
        specularLight[2] = ks[2] * light.intensity[2] * specularLightWeakness;

        result_color += diffuseLight + specularLight;
    }

    ambientLight[0] = ka.x() * amb_light_intensity.x();
    ambientLight[1] = ka.y() * amb_light_intensity.y();
    ambientLight[2] = ka.z() * amb_light_intensity.z();

    result_color += ambientLight;

    return result_color * 255.f;
}

Eigen::Vector3f bump_fragment_shader(const fragment_shader_payload &payload)
{

    Eigen::Vector3f ka = Eigen::Vector3f(0.005, 0.005, 0.005);
    Eigen::Vector3f kd = payload.color;
    Eigen::Vector3f ks = Eigen::Vector3f(0.7937, 0.7937, 0.7937);

    auto l1 = light{{20, 20, 20}, {500, 500, 500}};
    auto l2 = light{{-20, 20, 0}, {500, 500, 500}};

    std::vector<light> lights = {l1, l2};
    Eigen::Vector3f amb_light_intensity{10, 10, 10};
    Eigen::Vector3f eye_pos{0, 0, 10};

    float p = 150;

    Eigen::Vector3f color = payload.color;
    Eigen::Vector3f point = payload.view_pos;
    Eigen::Vector3f normal = payload.normal;

    // kh,kn decide coefficient texutre to normal.
    float kh = 0.2, kn = 0.1;

    Eigen::Vector3f n = normal;
    Eigen::Vector3f t;
    t << n.x() * n.y() / sqrt(n.x() * n.x() + n.z() * n.z()),
        sqrt(n.x() * n.x() + n.z() * n.z()),
        n.z() * n.y() / sqrt(n.x() * n.x() + n.z() * n.z());
    Eigen::Vector3f b = t.cross(n);

    Eigen::Matrix3f TBN;
    TBN << t, b, n;

    // use vector length to represent heigh.
    // bug: 1 / width equal 0
    float du = kn * kh * (payload.texture->getColor(payload.tex_coords[0] + 1.f / payload.texture->width, payload.tex_coords[1]).norm() - payload.texture->getColor(payload.tex_coords[0], payload.tex_coords[1]).norm());
    // bug: v is 1, 1.f / height > 1, find texture error.
    float dv = kn * kh * (payload.texture->getColor(payload.tex_coords[0], payload.tex_coords[1] + 1.f / payload.texture->height).norm() - payload.texture->getColor(payload.tex_coords[0], payload.tex_coords[1]).norm());

    Eigen::Vector3f ln;
    ln << -du, -dv, 1;

    // transform local coordinate to world coordinate.
    normal = TBN * ln;
    normal.normalize();

    Eigen::Vector3f result_color = {0, 0, 0};
    result_color = normal;

    return result_color * 255.f;
}

int main(int argc, const char **argv)
{
    // test_projection();
    std::vector<Triangle *> TriangleList;

    float angle = 180.0;
    bool command_line = false;

    std::string filename = "output.png";
    objl::Loader Loader;
    // texture and models in dir.
    std::string obj_path = "../models/spot/";

    // Load .obj File
    bool loadout = Loader.LoadFile("../models/spot/spot_triangulated_good.obj");
    
    if (!loadout) return -1;

    for (auto mesh : Loader.LoadedMeshes)
    {
        for (int i = 0; i < mesh.Vertices.size(); i += 3)
        {
            Triangle *t = new Triangle();
            for (int j = 0; j < 3; j++)
            {
                t->setVertex(j, Vector4f(mesh.Vertices[i + j].Position.X, mesh.Vertices[i + j].Position.Y, mesh.Vertices[i + j].Position.Z, 1.0));
                t->setNormal(j, Vector3f(mesh.Vertices[i + j].Normal.X, mesh.Vertices[i + j].Normal.Y, mesh.Vertices[i + j].Normal.Z));
                t->setTexCoord(j, Vector2f(mesh.Vertices[i + j].TextureCoordinate.X, mesh.Vertices[i + j].TextureCoordinate.Y));
            }
            TriangleList.push_back(t);
        }
    }

    rst::rasterizer r(800, 800);

    // height texture name
    auto texture_path = "spot_texture.png";
    r.set_texture(Texture(obj_path + texture_path));

    // C# delegate, phong shader
    std::function<Eigen::Vector3f(fragment_shader_payload)> active_shader = texture_fragment_shader;

    if (argc >= 2)
    {
        command_line = true;
        filename = std::string(argv[1]);

        if (argc == 3 && std::string(argv[2]) == "texture")
        {
            std::cout << "Rasterizing using the texture shader\n";
            active_shader = texture_fragment_shader;
            texture_path = "spot_texture.png";
            r.set_texture(Texture(obj_path + texture_path));
             filename = "texture.png";
        }
        else if (argc == 3 && std::string(argv[2]) == "normal")
        {
            std::cout << "Rasterizing using the normal shader\n";
            active_shader = normal_fragment_shader;
             filename = "normal.png";
        }
        else if (argc == 3 && std::string(argv[2]) == "phong")
        {
            std::cout << "Rasterizing using the phong shader\n";
            active_shader = phong_fragment_shader;
             filename = "phong.png";
        }
        else if (argc == 3 && std::string(argv[2]) == "bump")
        {
            std::cout << "Rasterizing using the bump shader\n";
            active_shader = bump_fragment_shader;
             filename = "bump.png";
        }
        else if (argc == 3 && std::string(argv[2]) == "displacement")
        {
            std::cout << "Rasterizing using the bump shader\n";
            active_shader = displacement_fragment_shader;
             filename = "displacement.png";
        }
    }

    Eigen::Vector3f eye_pos = {0, 0, 10};

    r.set_vertex_shader(vertex_shader);
    r.set_fragment_shader(active_shader);

    int key = 0;
    int frame_count = 0;

    if (command_line)
    {
        r.clear(rst::Buffers::Color | rst::Buffers::Depth);
        r.set_model(get_model_matrix(angle));
        r.set_view(get_view_matrix(eye_pos));
        r.set_projection(get_projection_matrix(90.0, 1, 0.1, 50));

        r.draw(TriangleList);
        cv::Mat image(800, 800, CV_32FC3, r.frame_buffer().data());
        image.convertTo(image, CV_8UC3, 1.0f);
        cv::cvtColor(image, image, cv::COLOR_RGB2BGR);

        cv::imwrite(filename, image);

        return 0;
    }

    int counter = 0;

    while (key != 27)
    {
        r.clear(rst::Buffers::Color | rst::Buffers::Depth);

        r.set_model(get_model_matrix(angle));
        r.set_view(get_view_matrix(eye_pos));
        r.set_projection(get_projection_matrix(90, 1, 0.1, 50));

        // r.draw(pos_id, ind_id, col_id, rst::Primitive::Triangle);
        r.draw(TriangleList);
        cv::Mat image(800, 800, CV_32FC3, r.frame_buffer().data());
        image.convertTo(image, CV_8UC3, 1.0f);
        cv::cvtColor(image, image, cv::COLOR_RGB2BGR);

        cv::imshow("image", image);
        cv::imwrite(filename, image);

        std::cout << "frame_counter: " << counter++ << "angle: " << angle << std::endl;
        key = cv::waitKey(10);

        if (key == 'a')
        {
            angle -= 10;
        }
        else if (key == 'd')
        {
            angle += 10;
        }
    }
    return 0;
}
