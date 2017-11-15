#ifndef ABSTRACT_FACTORY_H
#define ABSTRACT_FACTORY_H

#include <string>
#include "AbstractProduct.h"

class AbstractFactory {

public:
    enum FactoryType {
        BENZ_FACTORY,
        BMW_FACTORY,
        AUDI_FACTORY
    };

    virtual ICar* createCar() = 0;
    virtual IBike* createBike() = 0;
    static AbstractFactory* createFactory(FactoryType type);

};

#endif