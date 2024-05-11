#include "PirateFactory.h"

IEntity* PirateFactory::createEntity(const JsonObject& entity) {
  std::string type = entity["type"];
  if (type.compare("pirate") == 0) {
    std::cout << "Pirate Created" << std::endl;
    return new Pirate(entity);
  }
  return nullptr;
}

