#ifndef _CONCRETE_FACTORY_H
#define _CONCRETE_FACTORY_H

#include "AbstractFactory.h"
#include "ConcreteProduct.h"

class BenzFactory : public AbstractFactory {

public:
    ICar* createCar() {
        return new BenzCar;
    }

    IBike* createBike() {
        return new BenzBike;
    }
};

class BMWFactory : public AbstractFactory {

public:
    ICar* createCar() {
        return new BMWCar;
    }

    IBike* createBike() {
        return new BMWBike;
    }
};

class AudiFactory : public AbstractFactory {

public:
    ICar* createCar() {
        return new AudiCar;
    }

    IBike* createBike() {
        return new AudiBike;
    }
};


#endif