#include <iostream>
#include <stdlib.h>
#include <random>
#include <math.h>
#include <vector>
#include <numeric>

std::default_random_engine generator;
std::uniform_real_distribution<double> distribution(0.0, 1.0);

double exponentialValue(double lambda, double uniform) {
    return -(1 / lambda) * log(1 - uniform); 
}


int main() {

    std::cout << "hello world" << std::endl;
    
    std::vector<double> result;
    for (int i = 0; i < 1000; i ++) {
        double value = exponentialValue(75, distribution(generator));
        result.push_back(value);
    }


    double mean = accumulate(result.begin(), result.end(), 0.0) / result.size();
    double var = 0;

    for (int i = 0; i < result.size(); i++) {
        var += (result[i] - mean) * (result[i] - mean);
    }

    var /= result.size();

    std::cout << "Variance: " << var << ", Mean: " << mean << std::endl;
}
