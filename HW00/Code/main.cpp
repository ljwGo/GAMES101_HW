#include <cmath>
#include <iostream>
#include <Eigen/Dense>

using namespace std;

int main(){
    // 为了屏蔽平移矩阵的影响, 使用齐次坐标, 2维点使用3个数表示
    Eigen::Vector3f p;
    float angle, angleRadian, x, y, x_offset, y_offset;

    cout << "请输入要进行变化的点坐标(x, y)" << endl;
    // cin >> x >> y;
    x = 2; y = 1;
    p << x, y, 1; // 齐次坐标下, 0是向量, 1是点

    cout << "输入逆时针旋转的角度a" << endl;
    // cin >> angle;
    angle = 45;
    angleRadian = angle / 180 * acos(-1);
    Eigen::Matrix3f rotateMatrix;
    rotateMatrix << cos(angleRadian), -sin(angleRadian), 0, 
                    sin(angleRadian), cos(angleRadian), 0,
                    0, 0, 1;

    cout << "cos(angle): " << cos(angleRadian) << endl;

    cout << "请输入平移位移(x, y)" << endl;
    // cin >> x_offset >> y_offset; 
    x_offset = 1; y_offset = 2;

    // 平移矩阵
    Eigen::Matrix3f translateMatrix;
    translateMatrix << 1, 0, x_offset,
                    0, 1, y_offset,
                    0, 0, 1;
    // 输出变换结果
    cout << "transform: " << translateMatrix * rotateMatrix * p << endl;

    return 0;
}