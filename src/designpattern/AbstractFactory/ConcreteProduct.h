#ifndef _CONCRETE_PRODUCT_H
#define _CONCRETE_PRODUCT_H

#include "AbstractProduct.h"
#include <string>

/// cars
class BenzCar : public ICar {
    std::string getName() {
        return "Benz Car";
    }
};

class BMWCar : public ICar { 
    std::string getName() {
        return "BMW Car";
    }
};

class AudiCar :  public ICar {
    std::string getName() {
        return "Audi Car";
    }
};

/// bikes
class BenzBike : public IBike {
    std::string getName() {
        return "Benz Bike";
    }
};

class BMWBike : public IBike {
    std::string getName() {
        return "BMW Bike";
    }
};

class AudiBike : public IBike {
    std::string getName() {
        return "Audi Bike";
    }
};



#endif
