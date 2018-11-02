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
        /* TODO: Get value from sensor engine */
        /* TIP: You can access Smart_Data private attributes since the transducer is a friend class */
    }

    static void actuate(unsigned int dev, Smart_Data<MY_Temperature_Transducer> * data, const Smart_Data<MY_Temperature_Transducer>::Value & command) { }

private:
    // === Sensor engine instance ===
    static I2C_Temperature_Sensor sensor;
};
I2C_Temperature_Sensor MY_Temperature_Transducer::sensor;

// SmartData type with the transducer (MY_Temperature_Transducer) as template parameter
typedef Smart_Data<MY_Temperature_Transducer> My_Temperature;

int main()
{
    Alarm::delay(5000000);
    cout << "Hello! I'm a standalone sensor node" << endl;

    // Local SmartData constructor
    My_Temperature t(/*dev*/, /*expiry*/, /*mode*/); /* TODO: Complete the constructor parameters */

    while(true) {
        Alarm::delay(5000000); // 5s
        cout << "Temperature = " << /* TODO: use operator Value() */
             << " at " << /* TODO: use location() method */
             << ", " << /* TODO: use time() method */ << endl;
    }

    return 0;
}