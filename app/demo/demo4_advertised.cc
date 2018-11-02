#include <smart_data.h>
#include <alarm.h>
#include <i2c.h>

using namespace EPOS;

OStream cout;

class MY_Temperature_Transducer
{
    typedef TSTP::Unit Unit;
public:
    // === Technicalities ===
    // We won't use these, but every Transducer must declare them
    typedef _UTIL::Observer Observer;
    typedef _UTIL::Observed Observed;

    typedef Dummy_Predictor Predictor;
    typedef Predictor::Configuration Predictor_Configuration;

    static void attach(Observer * obs) {}
    static void detach(Observer * obs) {}

public:
    // === Sensor characterization ===
    static const unsigned int UNIT = Unit::Get_Quantity<Unit::Temperature,Unit::F32>::UNIT;
    static const int ERROR = 0; // Unknown

    static const bool INTERRUPT = false;
    static const bool POLLING = true;

public:
    // === SmartData interaction ===
    MY_Temperature_Transducer() {}

    static void sense(unsigned int dev, Smart_Data<MY_Temperature_Transducer> * data) {
        cout << "MY_Temperature_Transducer::sense(dev=" << dev << ")" << endl;
        data->_value = sensor.get();
    }

    static void actuate(unsigned int dev, Smart_Data<MY_Temperature_Transducer> * data, const Smart_Data<MY_Temperature_Transducer>::Value & command) { }

private:
    // === Sensor engine instance ===
    static I2C_Temperature_Sensor sensor;
};
I2C_Temperature_Sensor MY_Temperature_Transducer::sensor;

// SmartData type with the transducer (MY_Temperature_Transducer) as template parameter.
typedef Smart_Data<MY_Temperature_Transducer> My_Temperature;

int main()
{
    Alarm::delay(5000000);
    cout << "Hello! I'm a sensor node" << endl;

    // Local SmartData constructor
    My_Temperature t(0, 15000000, My_Temperature::ADVERTISED); // dev=0, expiry=15s, mode=ADVERTISED

    Thread::self()->suspend();
    return 0;
}