#include "AbstractFactory.h"
#include "ConcreteFactory.h"

AbstractFactory* AbstractFactory::createFactory(FactoryType type) {
    AbstractFactory* factory = nullptr;

    switch (type) {
        case FactoryType::BENZ_FACTORY:
            factory = new BenzFactory;

        case FactoryType::BMW_FACTORY:
            factory = new BMWFactory;

        case FactoryType::AUDI_FACTORY:
            factory = new AudiFactory;
    }

    return factory;
}