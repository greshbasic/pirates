#define _USE_MATH_DEFINES
#include "Drone.h"

#include <cmath>
#include <limits>

#include "AstarStrategy.h"
#include "BeelineStrategy.h"
#include "BfsStrategy.h"
#include "DfsStrategy.h"
#include "DijkstraStrategy.h"
#include "JumpDecorator.h"
#include "Package.h"
#include "SimulationModel.h"
#include "SpinDecorator.h"

Drone::Drone(const JsonObject& obj) : IEntity(obj) { available = true; }

Drone::~Drone() {
  if (toPackage) delete toPackage;
  if (toFinalDestination) delete toFinalDestination;
}

void Drone::notifyPirates() {
  for (auto pirate : model->the_pirates) {
    pirate->getNextTarget();
  }
  std::cout << "Pirates have been notified" << std::endl;
}

void Drone::notifyRobot() {
  model->the_robot->canPickUpPackage = true;
  std::cout << "Robot has been notified" << std::endl;
}

void Drone::getNextDelivery() {
  if (model && model->scheduledDeliveries.size() > 0) {
    package = model->scheduledDeliveries.front();
    model->scheduledDeliveries.pop_front();
    model->newestDelivery = package;

    if (package) {
      std::string message = getName() + " heading to: " + package->getName();
      notifyObservers(message);
      available = false;
      pickedUp = false;

      Vector3 packagePosition = package->getPosition();
      Vector3 finalDestination = package->getDestination();

      toPackage = new BeelineStrategy(position, packagePosition);

      std::string strat = package->getStrategyName();
      if (strat == "astar") {
        toFinalDestination = new JumpDecorator(new AstarStrategy(
            packagePosition, finalDestination, model->getGraph()));
      } else if (strat == "dfs") {
        toFinalDestination =
            new SpinDecorator(new JumpDecorator(new DfsStrategy(
                packagePosition, finalDestination, model->getGraph())));
      } else if (strat == "bfs") {
        toFinalDestination =
            new SpinDecorator(new SpinDecorator(new BfsStrategy(
                packagePosition, finalDestination, model->getGraph())));
      } else if (strat == "dijkstra") {
        toFinalDestination =
            new JumpDecorator(new SpinDecorator(new DijkstraStrategy(
                packagePosition, finalDestination, model->getGraph())));
      } else {
        toFinalDestination =
            new BeelineStrategy(packagePosition, finalDestination);
      }
    }
  }
}

void Drone::update(double dt) {
  if (available) getNextDelivery();

  if (package) {
    if (package->stolen) {
      delete toPackage;
      toPackage = nullptr;
      delete toFinalDestination;
      toFinalDestination = nullptr;
      package = nullptr;
      available = true;
      pickedUp = false;
    }

    if (toPackage) {
      toPackage->move(this, dt);

      if (toPackage->isCompleted() && !package->stolen) {
        package->canBeStolen = false;
        std::string message = getName() + " picked up: " + package->getName();
        notifyObservers(message);
        delete toPackage;
        toPackage = nullptr;
        pickedUp = true;
      }
    } else if (toFinalDestination) {
      toFinalDestination->move(this, dt);

      if (package && pickedUp) {
        package->setPosition(position);
        package->setDirection(direction);
      }

      if (toFinalDestination->isCompleted()) {
        std::string message = getName() + " dropped off: " + package->getName();
        package->canBeStolen = true;
        notifyObservers(message);
        notifyPirates();
        notifyRobot();
        delete toFinalDestination;
        toFinalDestination = nullptr;
        package->handOff();
        package = nullptr;
        available = true;
        pickedUp = false;
      }
    }
  }
}
