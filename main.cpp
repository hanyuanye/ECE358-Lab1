#include <iostream>
#include <stdlib.h>
#include <random>
#include <math.h>
#include <vector>
#include <deque>

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

std::vector<Event> generateArrivals(int simulationTime) {
    double currTime = 0;

    std::vector<Event> arrivalEvents;
    while (currTime < simulationTime) {
        double nextArrival = exponentialValue(arrivalLambda, distribution(generator));
        double serviceTime = exponentialValue(lengthLambda, distribution(generator));
        Event newEvent(nextArrival + currTime, serviceTime, ARRIVAL);
        arrivalEvents.push_back(Event(nextArrival + currTime, serviceTime, ARRIVAL));
        currTime = nextArrival + currTime;
     }

    std::cout << arrivalEvents.size() << std::endl;

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

        currTime = next.timestamp;
    }

    return departures;
}

int main() {

    std::cout << "hello world" << std::endl;

    double rho = startRho;

    while (rho <= endRho) {
        lengthLambda = rho / lengthRatio;
        std::cout << lengthLambda << std::endl;
        auto arrivals = generateArrivals(1000);
        auto departures = generateDepartures(arrivals, 1000);

        std::cout << "Arrival count: " << arrivals.size() << ", departure count: " << departures.size() << std::endl;

        rho += incrementRho;
    }    
}
