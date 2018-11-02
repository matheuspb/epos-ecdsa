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

    //typedef Dummy_Predictor Predictor;
    typedef Last_Value_Predictor<Smart_Data<Temperature_Transducer>> Predictor;
    typedef Predictor::Configuration Predictor_Configuration;

    //typedef DBP<Smart_Data<Temperature_Transducer>> Predictor;
    //typedef Predictor::Configuration<10,3> Predictor_Configuration;

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
    cout << "Hello, I'm a sensor!" << endl;

    My_Temperature t(0, 5000000, My_Temperature::ADVERTISED | My_Temperature::PREDICTIVE);
    Thread::self()->suspend();
    return 0;
}