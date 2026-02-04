#include <iostream>
#include <vector>
#include <cmath>
#include <random>
#include <iomanip>
#include <memory>
#include <stdexcept>
#include <string>
#include <sstream>
#include <windows.h> 
#include <thread> 
#include <mutex> 

using namespace std;

namespace patch {
    template <typename T>
    string to_string(const T& n) {
        std::ostringstream stm;
        stm << n;
        return stm.str();
    }
}
using namespace patch;

// ГЛОБАЛЬНІ НАЛАШТУВАННЯ

const double SIM_WIDTH = 100.0;
const double SIM_HEIGHT = 100.0;

double randomDouble(double min, double max) {
    thread_local random_device rd;
    thread_local mt19937 gen(rd());
    uniform_real_distribution<> dis(min, max);
    return dis(gen);
}

// КЛАСИ ПОМИЛОК

class SimulationException : public runtime_error {
public:
    SimulationException(const string& msg) : runtime_error(msg) {}
};

class OutOfBoundsException : public SimulationException {
public:
    OutOfBoundsException(double x, double y)
        : SimulationException("COORDINATE ERROR: (" + patch::to_string(x) + ", " + patch::to_string(y) + ")") {
    }
};

class PhysicsParameterException : public SimulationException {
public:
    PhysicsParameterException(const string& paramName, double value)
        : SimulationException("PHYSICS ERROR: '" + paramName + "': " + patch::to_string(value)) {
    }
};

// БАЗОВИЙ КЛАС ENTITY

class Entity {
protected:
    double x, y;
    double targetX, targetY;
    double speed;
    bool isMoving;
    string displayName;
    string lastThreadID;

public:
    Entity(double startX, double startY, double v, string name)
        : x(startX), y(startY), speed(v), displayName(name), isMoving(true) {

        lastThreadID = "Main";

        if (v < 0) throw PhysicsParameterException("Speed", v);
        if (startX < 0 || startX > SIM_WIDTH || startY < 0 || startY > SIM_HEIGHT) {
            throw OutOfBoundsException(startX, startY);
        }
    }

    virtual ~Entity() = default;

    void move() {
        stringstream ss;
        ss << this_thread::get_id();
        lastThreadID = ss.str();

        this_thread::sleep_for(chrono::milliseconds(10));

        if (!isMoving) return;

        double dx = targetX - x;
        double dy = targetY - y;
        double distance = sqrt(dx * dx + dy * dy);

        if (distance <= speed) {
            x = targetX;
            y = targetY;
            isMoving = false;
        }
        else {
            double ratio = speed / distance;
            x += dx * ratio;
            y += dy * ratio;
        }
    }

    bool isInRect(double px, double py, double minX, double maxX, double minY, double maxY) {
        return (px >= minX && px <= maxX && py >= minY && py <= maxY);
    }

    void printStatus() const {
        cout << left << setw(18) << displayName
            << " | Pos: (" << fixed << setprecision(1) << x << ", " << y << ") "
            << " | Target: (" << targetX << ", " << targetY << ") "
            << " | " << (isMoving ? "Moving" : "Arrived")
            << " [Th:" << lastThreadID << "]"
            << endl;
    }

    bool getIsMoving() const { return isMoving; }
};

// РЕАЛІЗАЦІЯ

class LegalEntity : public Entity {
public:
    LegalEntity(double startX, double startY, double v)
        : Entity(startX, startY, v, "Legal Entity") {
        double minX = 0, maxX = SIM_WIDTH / 2.0;
        double minY = 0, maxY = SIM_HEIGHT / 2.0;
        if (isInRect(startX, startY, minX, maxX, minY, maxY)) {
            isMoving = false; targetX = startX; targetY = startY;
        }
        else {
            targetX = randomDouble(minX, maxX); targetY = randomDouble(minY, maxY); isMoving = true;
        }
    }
};

class PhysicalEntity : public Entity {
public:
    PhysicalEntity(double startX, double startY, double v)
        : Entity(startX, startY, v, "Physical Entity") {
        double minX = SIM_WIDTH / 2.0, maxX = SIM_WIDTH;
        double minY = SIM_HEIGHT / 2.0, maxY = SIM_HEIGHT;
        if (isInRect(startX, startY, minX, maxX, minY, maxY)) {
            isMoving = false; targetX = startX; targetY = startY;
        }
        else {
            targetX = randomDouble(minX, maxX); targetY = randomDouble(minY, maxY); isMoving = true;
        }
    }
};

int main() {
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);

    vector<unique_ptr<Entity> > entities;
    double defaultSpeed = 8.0;

    cout << "=== SIMULATION START (Multithreaded) ===" << endl;

    try {
        entities.push_back(unique_ptr<Entity>(new LegalEntity(90, 90, defaultSpeed)));
        entities.push_back(unique_ptr<Entity>(new PhysicalEntity(10, 10, defaultSpeed)));
        entities.push_back(unique_ptr<Entity>(new LegalEntity(80, 20, defaultSpeed)));
        entities.push_back(unique_ptr<Entity>(new PhysicalEntity(20, 80, defaultSpeed)));
    }
    catch (const SimulationException& e) {
        cerr << e.what() << endl;
    }

    int step = 0;
    bool active = true;

    while (active && step < 15) {
        cout << "\nSTEP " << step << ":" << endl;

        vector<thread> workers;
        for (size_t i = 0; i < entities.size(); ++i) {
            workers.push_back(thread([&entities, i]() {
                entities[i]->move();
                }));
        }

        for (auto& t : workers) {
            if (t.joinable()) t.join();
        }

        active = false;
        for (const auto& e : entities) {
            e->printStatus();
            if (e->getIsMoving()) active = true;
        }

        step++;
    }

    cout << "\n=== SIMULATION FINISHED ===" << endl;
    system("pause");
    return 0;
}