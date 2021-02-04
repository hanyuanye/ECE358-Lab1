#include <iostream>
#include <stdlib.h>
#include <random>
#include <math.h>
#include <vector>

double startRho = 0.25;
double endRho = 0.95;
double incrementRho = 0.10;
double lengthRatio = 0.002;
double T = 1000;
double arrivalLambda = 75;
double lengthLambda = 0;
std::default_random_engine generator;
std::uniform_real_distribution<double> distribution(0.0, 1.0);

double exponentialValue(double lambda, double uniform) {
    return -(1 / lambda) * log(1 - uniform); 
}

double generateEvents(int simulationTime) {
    double currTime = 0;

    std::vector<double> arrivalEvents;
    while (currTime < simulationTime) {
        double nextArrival = exponentialValue(arrivalLambda, distribution(generator));
        double transmissionTime = exponentialValue(lengthLambda, distribution(generator)); 
        arrivalEvents.push_back(nextArrival + currTime);
        currTime = nextArrival + currTime;

//        std::cout << "Arrival time: " << currTime << ", Service time: " << transmissionTime << std::endl;
    }

    std::cout << arrivalEvents.size() << std::endl;
}

int main() {

    std::cout << "hello world" << std::endl;

    double rho = startRho;

    while (rho <= endRho) {
        lengthLambda = rho / lengthRatio;
        generateEvents(1000);

        rho += incrementRho;
    }    
}
