#include "AbstractFactory.h"
#include "AbstractProduct.h"

#include <iostream>

int main(void)
{
    AbstractFactory* benzFactory = AbstractFactory::createFactory(AbstractFactory::FactoryType::BENZ_FACTORY);
    ICar* benzCar = benzFactory->createCar();
    std::cout << "car is: " << benzCar->getName();

    return 0;
}