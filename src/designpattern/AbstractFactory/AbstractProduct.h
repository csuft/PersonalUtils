#ifndef ABSTRACT_PRODUCT_H
#define ABSTRACT_PRODUCT_H

#include <string>

class ICar {
public:
    virtual std::string getName() = 0;
};

class IBike {
public:
    virtual std::string getName() = 0;
};


#endif
