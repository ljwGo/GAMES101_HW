#include <chrono>
#include <iostream>
#include <opencv2/opencv.hpp>

std::vector<cv::Point2f> control_points;

void __blurBezier(cv::Point2f& point, cv::Mat& window, cv::Vec3b color);

void mouse_handler(int event, int x, int y, int flags, void *userdata)
{
    if (event == cv::EVENT_LBUTTONDOWN && control_points.size() < 4)
    {
        std::cout << "Left button of the mouse is clicked - position (" << x << ", "
                  << y << ")" << '\n';
        control_points.emplace_back(x, y);
    }
}

void naive_bezier(const std::vector<cv::Point2f> &points, cv::Mat &window)
{
    auto &p_0 = points[0];
    auto &p_1 = points[1];
    auto &p_2 = points[2];
    auto &p_3 = points[3];

    for (double t = 0.0; t <= 1.0; t += 0.001)
    {
        auto point = std::pow(1 - t, 3) * p_0 + 3 * t * std::pow(1 - t, 2) * p_1 +
                     3 * std::pow(t, 2) * (1 - t) * p_2 + std::pow(t, 3) * p_3;

        window.at<cv::Vec3b>(point.y, point.x)[2] = 255;
    }
}

cv::Point2f recursive_bezier(const std::vector<cv::Point2f> &control_points, float t)
{
    // TODO: Implement de Casteljau's algorithm
    if (control_points.size() == 1)
    {
        return control_points[0];
    }

    std::vector<cv::Point2f> next_control_points;
    cv::Point2f new_control_point;

    for (int i = 0; i < control_points.size() - 1; i++)
    {
        new_control_point = (1.f - t) * control_points[i] + t * control_points[i + 1];

        next_control_points.emplace_back(new_control_point);
    }

    return recursive_bezier(next_control_points, t);
}

void bezier(const std::vector<cv::Point2f> &control_points, cv::Mat &window)
{
    // TODO: Iterate through all t = 0 to t = 1 with small steps, and call de Casteljau's
    // recursive Bezier algorithm.
    cv::Point2f point;

    for (float t = 0; t <= 1; t += 0.001)
    {

        point = recursive_bezier(control_points, t);

        // at方法参数为row和col,对应y和x
        // cv::Vec3b &color = window.at<cv::Vec3b>(point.y, point.x);

        // 2是红色,1是绿色,0是蓝色
        // color[1] = 255;

        __blurBezier(point, window, cv::Vec3b(0, 150, 150));
    }
}

void __blurBezier(cv::Point2f& point, cv::Mat& window, cv::Vec3b color)
{
    int l, r, t, b;

    l = floor(point.x);
    r = ceil(point.x);
    t = floor(point.y);
    b = ceil(point.y);

    // 边界处理
    if (l < 1)
    {
        l = 1;
    }
    else if (r > window.cols)
    {
        r = window.cols;
    }

    if (t < 1)
    {
        t = 1;
    }
    else if (b > window.rows)
    {
        b = window.rows;
    }

    // 点周围四个像素中心
    cv::Point2f lt(l, t), lb(l, b), rt(r, t), rb(r, b);

    // 点到周围四个像素中心的距离
    float lt_distance = sqrt(pow((point - lt).x, 2) + pow((point - lt).y, 2));
    float lb_distance = sqrt(pow((point - lb).x, 2) + pow((point - lb).y, 2));
    float rt_distance = sqrt(pow((point - rt).x, 2) + pow((point - rt).y, 2));
    float rb_distance = sqrt(pow((point - rb).x, 2) + pow((point - rb).y, 2));

    float distance_sum = lt_distance + lb_distance + rt_distance + rb_distance;

    // 将颜色平均到四个像素上
    window.at<cv::Vec3b>(lt.y, lt.x) += (1 - lt_distance) / distance_sum * color;
    window.at<cv::Vec3b>(lb.y, lb.x) += (1 - lb_distance) / distance_sum * color;
    window.at<cv::Vec3b>(rt.y, rt.x) += (1 - rt_distance) / distance_sum * color;
    window.at<cv::Vec3b>(rb.y, rb.x) += (1 - rb_distance) / distance_sum * color;
}

int main()
{
    cv::Mat window = cv::Mat(700, 700, CV_8UC3, cv::Scalar(0));
    cv::cvtColor(window, window, cv::COLOR_BGR2RGB);
    cv::namedWindow("Bezier Curve", cv::WINDOW_AUTOSIZE);

    cv::setMouseCallback("Bezier Curve", mouse_handler, nullptr);

    int key = -1;
    while (key != 27)
    {
        for (auto &point : control_points)
        {
            cv::circle(window, point, 3, {255, 255, 255}, 3);
        }

        if (control_points.size() == 4)
        {
            // naive_bezier(control_points, window);
            bezier(control_points, window);

            cv::imshow("Bezier Curve", window);
            cv::imwrite("my_bezier_curve.png", window);
            key = cv::waitKey(0);

            return 0;
        }

        cv::imshow("Bezier Curve", window);
        key = cv::waitKey(20);
    }

    return 0;
}
