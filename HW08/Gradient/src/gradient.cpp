#include "gradient.h"
#include <string>

doubleVector Gradient::GetGradient(doubleVector& args, size_t argumentNum, scalarFunction func, doubleVector& defineDomain, double infinitesimal){
    doubleVector result;
    double partialDerivative;

    for (int i=0; i<argumentNum; i++){
        // defineDomain[0]: first arg minDomain; defineDomain[1] first arg maxDomain
        if (args[i]+  infinitesimal > defineDomain[i*2 + 1]){
            throw(std::string("don't select bounary point in defineDomain"));
        }

        double left, right;

        // partialDerivative define. partialDerivative = (f(x+infinitesimal, y) - f(x, y)) / infinitesimal
        args[i] += infinitesimal;
        left = func(args);
        args[i] -= infinitesimal;
        right = func(args);

        partialDerivative = (left - right) / infinitesimal;
        result.push_back(partialDerivative);
    }

    // gradient vector normalization
    double denominator2 = 0;
    double divdenominator2;
    std::vector<int> sign;

    for (double v : result){
        //confirm sqrt sign is right
        sign.push_back(v > 0 ? 1 : -1);

        denominator2 += v * v;
    }

    divdenominator2 = 1. / denominator2;
    for (int i=0; i<result.size(); i++){
        result[i] = sqrt(result[i] * result[i] * divdenominator2) * sign[i];
    }
    return result;
}