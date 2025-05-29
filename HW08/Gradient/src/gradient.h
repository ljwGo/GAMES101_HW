#pragma
#include "CGL/CGL.h"
#include <vector>

typedef std::vector<double> doubleVector;
typedef double (*scalarFunction)(doubleVector&);

class Gradient{
public:
    doubleVector GetGradient(doubleVector& args, size_t argumentNum, scalarFunction func, doubleVector& defineDomain, double infinitesimal);
};