#include <smart_data.h>
#include <alarm.h>
#include <i2c.h>

using namespace EPOS;

OStream cout;

class Temperature_Transducer
{
    typedef TSTP::Unit Unit;
public:
    static const unsigned int UNIT = Unit::Get_Quantity<Unit::Temperature,Unit::F32>::UNIT;
    static const int ERROR = 0; // Unknown

    static const bool INTERRUPT = false;
    static const bool POLLING = true;
public:
    typedef _UTIL::Observer Observer;
    typedef _UTIL::Observed Observed;

    typedef Dummy_Predictor Predictor;
    typedef Predictor::Configuration Predictor_Configuration;

    static void attach(Observer * obs) {}
    static void detach(Observer * obs) {}

public:
    Temperature_Transducer() {}
    static void sense(unsigned int dev, Smart_Data<Temperature_Transducer> * data) {
        data->_value = sensor.get();
    }
    static void actuate(unsigned int dev, Smart_Data<Temperature_Transducer> * data, const Smart_Data<Temperature_Transducer>::Value & command) {
        data->_value = command;
    }
    static I2C_Temperature_Sensor sensor;
};
I2C_Temperature_Sensor Temperature_Transducer::sensor;
typedef Smart_Data<Temperature_Transducer> My_Temperature;

int main()
{
    Alarm::delay(5000000);
    cout << "Hello!" << endl;

    My_Temperature t(0, 15000000, My_Temperature::PRIVATE);
    t = 10;

    while(true) {
        Alarm::delay(5000000);
        cout << "Temperature = " << t << " at " << t.location() << ", " << t.time() << endl;
    }

    return 0;
}
