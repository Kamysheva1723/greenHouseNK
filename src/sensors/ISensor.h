#ifndef ISENSOR_H
#define ISENSOR_H

class ISensor {
public:
    virtual bool readSensor() = 0;
    virtual ~ISensor() = default;

};


#endif // ISENSOR_H
