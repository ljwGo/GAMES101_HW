#include <Eigen/Dense>
#include <iostream>

using namespace std;

int matrixCalc(){

    // 2 * 2方阵
    Eigen::Matrix2d m2_01;
    m2_01 << 1, 2,
            3, 4;

    // 3 方阵
    Eigen::Matrix3d m3_01;
    m3_01 << 1, 2, 3,                
            4, 5, 6, 
            7, 8, 9;

    // 4 × 5矩阵
    Eigen::MatrixXd mx_01(4, 5);

    mx_01 << 1, 2, 3, 4, 5,
            6, 7, 8, 9, 10,
            11, 12, 13, 14, 15,
            16, 17, 18, 19, 20;

    Eigen::Vector2d v2_01(1, 1);
    Eigen::Vector3d v3_01(1, 1, 1);

    cout << m2_01 << endl;
    cout << m3_01 << endl;
    cout << mx_01 << endl;


    // 矩阵 × 向量, 保证矩阵和向量元素的类型相同, 比如都是int或者float, 否则会出错
    cout << m2_01 * v2_01 << endl;
    cout << m3_01 * v3_01 << endl;
    // 矩阵 × 矩阵
    cout << m2_01 * m2_01 << endl;
    // 矩阵 × 数
    cout << m2_01 * 2 << endl;

    return 0;
}