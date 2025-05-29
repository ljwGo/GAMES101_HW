#pragma
#include "gradient.h"

class GradientSolver{
public:
    GradientSolver();
    GradientSolver(double studyRate, double canAcceptError, size_t maxIterCount, double (*lossFn)(doubleVector& args));
    doubleVector Solver(doubleVector& initArgs, size_t argumentNum, doubleVector defineDomain);
private:
    double infinitesimal = 1e-8;
    double studyRate;
    double canAcceptError;
    size_t maxIterCount;
    Gradient gradientProvider;
    double (*lossFn)(doubleVector& args);
};