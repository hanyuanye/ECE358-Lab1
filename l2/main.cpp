#include <iostream>
#include <stdlib.h>
#include <random>
#include <math.h>
#include <vector>
#include <deque>
#include <functional>
#include <algorithm>
#include <string>
#include <fstream>
#include <chrono>

double T = 1000;
double c = 3 * pow(10, 8);
double T_PROP = 10.0 / (2.0 / 3.0 * c);
double T_TRANS = 1500.0 / 1000000.0;

bool nPersistant = false;

int transmissionAttempts = 0;
int transmitted = 0;

std::default_random_engine generator(std::chrono::system_clock::now().time_since_epoch().count());;
std::uniform_real_distribution<double> distribution(0.0, 1.0);


// Utils
double exponentialValue(double lambda) {
    double uniform = distribution(generator);
    return -(1 / lambda) * log(1 - uniform);
}

double backoff(int n) {
    std::uniform_int_distribution<int> int_distribution(0, pow(2, n));
    int randVal = int_distribution(generator);
    return (double)randVal * 512.0 / 1000000.0;
}

class Result {
public:
    int a;
    int n;
    double efficiency;
    double throughput;

    Result(double t_efficiency, double t_throughput) {
        efficiency = t_efficiency;
        throughput = t_throughput;
        a = 0;
        n = 0;
    }
};

class Node {
public:
    int lambda;
    int pos;
    int collisionCount;
    double nextFrame;

    Node(int t_lambda, int t_pos) {
        lambda = t_lambda;
        pos = t_pos;
        collisionCount = 0;
        nextFrame = exponentialValue(lambda);
    }

    void handleCollision() {
        collisionCount += 1;
        if (collisionCount > 10) {
            collisionCount = 0;
            nextFrame += exponentialValue(lambda);
        } else {
            nextFrame += backoff(collisionCount);
        }
    }

    void senderCollision(double max_delay) {
        handleCollision();
        nextFrame = max_delay;
    }

    bool senseBusy(double start, double end, int attempt) {
        if (start <= nextFrame && nextFrame < end) {
            if(nPersistant) {
                if (attempt == 1) {
                    nextFrame = end + backoff(attempt);
                } else {
                    nextFrame += backoff(attempt);
                }
            } else {
                nextFrame = end; //TODO: add backoff
            }
            return true;
        }
        return false;
    }

    void sendSuccessfully() {
        collisionCount = 0;
        nextFrame += exponentialValue(lambda);
    }
};

std::vector<Node> generateNodes(int avgPackets, int n) {
    std::vector<Node> nodes; 
    for (int i = 0; i < n; i++) {
        nodes.push_back(Node(avgPackets, i));
    }

    return nodes;
}

Result simulate(double simulationTime, std::vector<Node> nodes) {

    double timer = 0;

    while (timer < simulationTime) {
        // Retrieve next event
        int minIdx = 0;
        for (int i = 0; i < nodes.size(); i++) {
            if (nodes[i].nextFrame < nodes[minIdx].nextFrame) {
                minIdx = i;
            }
        }
        Node &minNode = nodes[minIdx];

        timer = minNode.nextFrame;
        // check collisions
        int maxCollidingDistance = -1;

        transmissionAttempts ++;
        
        bool dropPacket = false;
        
        for (auto &node: nodes) {
            if (node.pos == minNode.pos) { continue; }

            int distance = abs(minNode.pos - node.pos);
            double sendingTime = minNode.nextFrame;
            double arrivalTime = sendingTime + T_PROP * distance;
            double lastBit = arrivalTime + T_TRANS;

            // collision case
            if (node.nextFrame < arrivalTime) {
                maxCollidingDistance = std::max(distance, maxCollidingDistance);
                node.handleCollision();  
                transmissionAttempts ++;
            } else {
                if(nPersistant){
                    // do 10 attempts and drop packet if not successful after
                    for (int attempt = 1; attempt <=11; attempt ++){
                        if (attempt == 11){
                            dropPacket = true;
                            break;
                        }
                        if (!node.senseBusy(arrivalTime, lastBit, attempt)){
                            
                            break; 
                        }
                    } 
                } else {
                    node.senseBusy(arrivalTime, lastBit, 1);     
                }
            }
        }

        if (maxCollidingDistance >= 0) {
            minNode.senderCollision(minNode.nextFrame + T_TRANS + maxCollidingDistance * T_PROP); 
        } else if (!dropPacket){
            transmitted ++;
            //std::cout << transmitted << " " << minNode.nextFrame << " " << transmissionAttempts << std::endl;
            minNode.sendSuccessfully(); 
        }
    }

    double efficiency = (double)transmitted / (double)transmissionAttempts;
    double throughput = (double)transmitted * T_TRANS / simulationTime;
    return Result(efficiency, throughput);
}

Result createSimulation(int avgPackets, int numNodes) {
    transmitted = 0;
    transmissionAttempts = 0;
    auto nodes = generateNodes(avgPackets, numNodes);

    auto result = simulate(T, nodes);
    result.a = avgPackets;
    result.n = numNodes;

    std::cout << T << " " << result.a << " " << result.n << " " << result.efficiency << " " << result.throughput << std::endl;
    return result; 
}

bool isStable(double v1, double v2) {
    if (v2 < 0.005) { return true; }

    return abs((v1 - v2) / v2) < 0.04;
}

bool isStable(Result r1, Result r2) {
    return isStable(r1.efficiency, r2.efficiency) && isStable(r1.throughput, r2.throughput);    
}

void clear(std::string filename) {
    std::ofstream ofs;
    ofs.open(filename, std::ofstream::out | std::ofstream::trunc);
    ofs.close();
}

void write(std::vector<Result> results, std::string filename) {
    std::ofstream txtOut;
    txtOut.open(filename + "Ef", std::ofstream::out | std::ofstream::trunc);
    for (auto result: results) {
        txtOut << result.n << " " << result.efficiency << std::endl;
        if (result.n == 100) {
            txtOut << std::endl << std::endl;
        }
    }
    txtOut.close();

    txtOut.open(filename + "Th", std::ofstream::out | std::ofstream::trunc);
    for (auto result: results) {
        txtOut << result.n << " " << result.throughput << std::endl;
        if (result.n == 100) {
            txtOut << std::endl << std::endl;
        }
    }
    txtOut.close();
}

int sim(std::string fileName){
    std::vector<int> A {7, 10, 20};
    std::vector<int> N {20, 40, 60, 80, 100};

    auto prev = Result(0, 0);
    std::vector<Result> results;
    while(1) {
        for (auto a: A) {
            for (auto n: N) {
                auto result = createSimulation(a, n);
                results.push_back(result);
                if (result.a == 20 && result.n == 100) {
                    if (isStable(prev, result)) {
                        write(results, fileName);
                        std::cout << "Stable" << std::endl;
                        return 0;
                    } else {
                        results.clear();
                        prev = result;
                        T += 1000;
                        std::cout << "Unstable" << std::endl;
                    }
                }
            }
        }
    } 
    createSimulation(7, 20);
}

int main() {
    clear("persistentEf");
    clear("persistentTh");
    clear("NpersistentEf");
    clear("NpersistentTh");
    sim("persistent");
    
    nPersistant = true;
    
    sim("Npersistent");
}


