#include <iostream>
#include <cmath>

using namespace std;

int radian(){

    float a = 0.2f, b = 0.1f;

    // cmath repository sin,cos,arcos,arsin base on radian rule.
    cout << a - b << endl;
    cout << sqrt(a) << endl;
    cout << a / b << endl;
    cout << acos(-1) << endl;
    cout << sin(30.0 / 180.0 * acos(-1)) << endl;

    return 0;
}
