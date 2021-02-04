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

enum EventType { ARRIVAL, DEPARTURE, OBSERVER };

class Event {
public:
    double timestamp;
    double serviceTime;
    EventType type;

    Event(double t_timestamp, double t_serviceTime, EventType t_type) {
        timestamp = t_timestamp;
        serviceTime = t_serviceTime;
        type = t_type;
    }

    Event(double t_timestamp, EventType t_type) {
        timestamp = t_timestamp;
        serviceTime = 0;
        type = t_type;
    }

    friend std::ostream& operator<<(std::ostream& os, const Event& event) {
        os << event.timestamp << "/" << event.serviceTime;
        return os;
    }
};

struct Result {
    double rho;
    double packetLoss;
    double queueSizeTotal;
    double idleTimeTotal;

    Result() {
        rho = 0;
        packetLoss = 0;
        queueSizeTotal = 0;
        idleTimeTotal = 0;
    };
};

double startRho = 0.25;
double endRho = 0.95;
double incrementRho = 0.10;
double arrivalRatio = 500;
double T = 1000;
double queueSize = 0;
double arrivalLambda = 0;
double lengthLambda = 0.0005;
double c = 1000000;
std::default_random_engine generator(std::chrono::system_clock::now().time_since_epoch().count());;

std::uniform_real_distribution<double> distribution(0.0, 1.0);

void printResults(Result result) {
        std::cout  << "Rho: " << result.rho << ", Packet loss: " << result.packetLoss << ", Queue Size: " << result.queueSizeTotal << ", idleTimeTotal: " << result.idleTimeTotal << std::endl;
}

double exponentialValue(double lambda, double uniform) {
    return -(1 / lambda) * log(1 - uniform); 
}

std::vector<Event> generateArrivals(int simulationTime) {
    double currTime = 0;

    std::vector<Event> arrivalEvents;
    while (currTime < simulationTime) {
        double nextArrival = exponentialValue(arrivalLambda, distribution(generator));
        double serviceTime = exponentialValue(lengthLambda, distribution(generator)) / c;
        Event newEvent(nextArrival + currTime, serviceTime, ARRIVAL);
        arrivalEvents.push_back(newEvent);
        currTime = nextArrival + currTime;
     }

    return arrivalEvents;
}

std::vector<Event> generateDepartures(std::vector<Event> arrivals, int simulationTime) {
    double currTime = 0;
    std::vector<Event> departures;

    for (auto a: arrivals) {
        double departureTime = 0;

        if (a.timestamp <= currTime) {
            departureTime = currTime + a.serviceTime;
        } else {
            departureTime = a.timestamp + a.serviceTime;
        }

        currTime = departureTime;

        departures.push_back(Event(departureTime, DEPARTURE));
    }

    return departures;
}

std::vector<Event> generateDepartures(std::vector<Event> arrivals, int simulationTime, int queueSize) {

    if (queueSize == 0) {
        return generateDepartures(arrivals, simulationTime);
    }

    std::deque<Event> eventQueue;

    for (auto c: arrivals) {
        eventQueue.push_back(c);
    }

    std::vector<Event> departures;
    std::deque<Event> packetQueue;

    double currTime = 0;
    // Simulate queue to see which packets are serviced
    // While packets in the packet queue can be serviced before next arrival
    // service them.
    //
    // On the next arrival event, attempt to add it to the packet queue.
    // If full, skip it and increment packet loss counter, otherwise add it.

    while (currTime < simulationTime && (eventQueue.size() > 0 || packetQueue.size() > 0)) {
        // Service packets in packet queue
        while (packetQueue.size() > 0 && (eventQueue.size() == 0 || packetQueue.front().serviceTime + currTime < eventQueue.front().timestamp)) {
            currTime += packetQueue.front().serviceTime;
            departures.push_back(Event(currTime, DEPARTURE));
            packetQueue.pop_front();
        }

        if (eventQueue.size() == 0) { continue; }

        Event next = eventQueue.front();
        eventQueue.pop_front();

        // packet loss
        if (packetQueue.size() == queueSize) {
            continue;
        }

        // add next packet to packet queue
        packetQueue.push_back(next);
    }

    return departures;
}

std::vector<Event>generateObservers(int simulationTime) {
    double currTime = 0;

    std::vector<Event> observerEvents;
    while (currTime < simulationTime) {
        double nextArrival = exponentialValue(arrivalLambda*5.0, distribution(generator));
        Event newEvent(nextArrival + currTime, OBSERVER);
        observerEvents.push_back(newEvent);
        currTime = nextArrival + currTime;
     }

    return observerEvents; 
}

Result runDes(std::vector<Event> events, int simulationTime, int size, int arrivalsSize, int observersSize) {
    std::deque<Event> queue;

    for (auto e: events) {
        queue.push_back(e);
    }

    Result result;
    int currentQueueSize = 0;
    double previousTime = 0;

    while (queue.size() > 0) {
        auto event = queue.front();
        queue.pop_front();

        if (event.timestamp >= simulationTime) { break; }

        switch (event.type) {
        case ARRIVAL:
            if (size > 0 && currentQueueSize == size) {
                result.packetLoss += 1;
            } else {
                currentQueueSize += 1;
            }
            break;
        case DEPARTURE:
            currentQueueSize -= 1;
            break;
        case OBSERVER:
            result.queueSizeTotal += currentQueueSize;
            if (currentQueueSize == 0) {
                result.idleTimeTotal += 1;
            }
            break;
        }

        previousTime = event.timestamp; 
    }

    result.packetLoss /= arrivalsSize;
    result.queueSizeTotal /= observersSize;
    result.idleTimeTotal /= observersSize;

    return result;
}

bool isStable(double v1, double v2) {
    if (v2 < 0.005) { return true; }

    return abs((v1 - v2) / v2) < 0.04;
}

bool isStable(Result r1, Result r2) {
    return isStable(r1.packetLoss, r2.packetLoss) &&
           isStable(r1.queueSizeTotal, r2.queueSizeTotal) &&
           isStable(r1.idleTimeTotal, r2.idleTimeTotal);
}

void outputGraphTxt1(std::vector<Result> results, std::string filename) {
    std::ofstream txtOut;
    txtOut.open(filename, std::ofstream::out | std::ofstream::trunc);
    for (auto result: results) {
        txtOut << result.rho << " " << result.queueSizeTotal << std::endl;
    }
    txtOut.close();
}

void outputGraphTxt2(std::vector<Result> results, std::string filename) {
    std::ofstream txtOut;
    txtOut.open(filename, std::ofstream::out | std::ofstream::trunc);
    for (auto result: results) {
        txtOut << result.rho << " " << result.idleTimeTotal << std::endl;
    }
    txtOut.close();
}

void outputGraphTxt3(std::vector<Result> results, std::string filename, bool shouldTruncate) {
    std::ofstream txtOut;
    if (shouldTruncate) {
        txtOut.open(filename, std::ofstream::out | std::ofstream::trunc);
    } else {
        txtOut.open(filename);
    }
    for (auto result: results) {
        txtOut << result.rho << " " << result.packetLoss << std::endl;
    }
    txtOut.close();
}

void outputGraphTxt4(std::vector<Result> results, std::string filename, bool shouldTruncate) {
    std::ofstream txtOut;
    if (shouldTruncate) {
        txtOut.open(filename, std::ofstream::out | std::ofstream::trunc);
    } else { 
        txtOut.open(filename);
        txtOut << std::endl;
        txtOut << std::endl;
    }
    for (auto result: results) {
        txtOut << result.rho << " " << result.queueSizeTotal << std::endl;
    }
    txtOut.close();
}

std::vector<Result> runSimulation() {
    bool stable = false;
    Result prevResult;
    std::vector<Result> results;
    T = 1000;

    while (!stable) {
        double rho = startRho;
        results.clear();
        while (rho <= endRho + 0.05) {
            std::cout << rho << std::endl;
            arrivalLambda = rho * arrivalRatio;
            auto arrivals = generateArrivals(T);
            auto departures = generateDepartures(arrivals, T, queueSize);
            auto observers = generateObservers(T);

            std::vector<Event> events = arrivals;
            events.insert(events.end(), departures.begin(), departures.end());
            events.insert(events.end(), observers.begin(), observers.end());

            std::sort(std::begin(events),
                      std::end(events),
                      [](Event a, Event b) { return a.timestamp < b.timestamp; });
            auto result = runDes(events, T, queueSize, arrivals.size(), observers.size());
            result.rho = rho;

            results.push_back(result);
            if (rho > endRho - 0.05) {
                stable = isStable(prevResult, result);
                std::cout << "Queue Size: " << queueSize << ", T: " << T << ", stable: " << stable << std::endl;
                printResults(result);
                prevResult = result;
            }

            rho += incrementRho;
        }

        T += 1000;
    }
    return results;
}

void runSimulation(int t_queueSize) {
    queueSize = t_queueSize;
    auto results = runSimulation();
    outputGraphTxt3(results, "q6_dataPacketLoss", t_queueSize == 10);
    outputGraphTxt4(results, "q6_dataEn", t_queueSize == 10);
}

int main(int argc, char* argv[]) {

    int mode = strtol(argv[1], NULL, 10);
    if (mode == 0) {
        startRho = 0.25;
        endRho = 0.95;
    } else {
        startRho = 0.5;
        endRho = 1.5;
    }

    if (mode == 1) {
        runSimulation(10);
        runSimulation(25);
        runSimulation(50);
    } else {
        auto results = runSimulation();
        outputGraphTxt1(results, "q3_data1");
        outputGraphTxt2(results, "q3_data2");
    }
}

