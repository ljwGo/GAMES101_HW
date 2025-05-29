#include <iostream>
#include <Eigen/Dense>

using namespace std;

int vectorCalc(){

    Eigen::Vector3d v3_01(1,2,3);  // the word 'd' and 'f' behind identifier represent for interger or float
    Eigen::Vector2d v2_01(2,3);
    Eigen::Vector3d v3_02(2,3,4);

    cout << v3_01 << endl;
    cout << v3_01 * 2 << endl;

    // dot 点积
    cout << v3_01.dot(v3_02) << endl;
    // cross 叉积,不知道为什么出错
    // Eigen::Vector3d v3_03 = v3_01.cross(v3_02);
    // Eigen::Vector3d v3_03 = v3_02.cross(v3_01);
    // transpose 转置
    cout << v3_01.transpose() << endl;

    return 0;   
}