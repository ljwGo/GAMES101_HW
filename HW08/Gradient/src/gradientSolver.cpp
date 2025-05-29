#include "gradientSolver.h"

GradientSolver::GradientSolver(){};

GradientSolver::GradientSolver(double studyRate, double canAcceptError, size_t maxIterCount, double (*lossFn)(doubleVector& args))
: studyRate(studyRate), canAcceptError(canAcceptError), maxIterCount(maxIterCount), lossFn(lossFn)
{
    gradientProvider = Gradient();
};

doubleVector GradientSolver::Solver(doubleVector& initArgs, size_t argumentNum, doubleVector defineDomain){
    
    doubleVector& args = initArgs;
    doubleVector gradient;
    double previousLoss;
    double currentLoss;
    double studyRate = this->studyRate;
    
    int iter = 0;

    // is not close to target value
    currentLoss = lossFn(args);
    while (currentLoss > canAcceptError && iter <= maxIterCount){
        // get decrease gradient to find min value
        gradient = gradientProvider.GetGradient(args, argumentNum, lossFn, defineDomain, infinitesimal);
        
        // default gradient is the most speed of become big. Is is to find max value. 
        for (int i=0; i<gradient.size(); i++){
            gradient[i] = -gradient[i];
        }

        for (int i=0; i<argumentNum; i++){
            args[i] += gradient[i] * studyRate;
        }

        previousLoss = currentLoss;
        currentLoss = lossFn(args);

        if (currentLoss >= previousLoss){
            for (int i=0; i<argumentNum; i++){
                args[i] -= gradient[i] * studyRate;
            }

            studyRate *= 0.5;
        }

        iter++;
    }
    
    return args;
}
