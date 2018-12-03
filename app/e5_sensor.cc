#include "smart_log.h"
#include <smart_data.h>
#include <alarm.h>
#include <utility/random.h>
#include <utility/keypair.h>
#include <utility/hwbignum.h>


using namespace EPOS;

OStream cout;


class Temperature_Transducer
{
public:
    // === Technicalities ===
    // We won't use these, but every Transducer must declare them
    typedef _UTIL::Observer Observer;
    typedef _UTIL::Observed Observed;

    typedef Dummy_Predictor Predictor;
    struct Predictor_Configuration : public Predictor::Configuration {};

    static void attach(Observer * obs) {}
    static void detach(Observer * obs) {}

public:
    // === Sensor characterization ===
    static const unsigned int UNIT = TSTP::Unit::Temperature;
    static const unsigned int NUM = TSTP::Unit::I32;
    static const int ERROR = 0; // Unknown

    static const bool INTERRUPT = false;
    static const bool POLLING = true;

public:
    // === SmartData interaction ===
    Temperature_Transducer() = delete;

    static void sense(
            unsigned int dev,
            Smart_Data<Temperature_Transducer> * data)
    {
        cout << "sense(dev=" << dev << ")" << endl;
    }

    static void actuate(
            unsigned int dev,
            Smart_Data<Temperature_Transducer> * data,
            const Smart_Data<Temperature_Transducer>::Value & command)
    {
        cout << "actuate(dev=" << dev << ", command=" << command << ")" << endl;
        data->_value = command;
    }
};

// A SmartData that personifies the transducer defined above
typedef Smart_Data<Temperature_Transducer> My_Temperature;

int main()
{
    cout << "Hello!" << endl;

    My_Temperature t(0, 15000000, My_Temperature::ADVERTISED);
    Smart_Log log(0, 15000000, Smart_Log::ADVERTISED);
    Logger<My_Temperature, 3> p0(&t, &log);
    log = 0;
    t = 10;

    Random::seed(TSTP::now());
    while(true) {
        Alarm::delay(1000000);
        cout << "Temperature = " << t << " at " << t.location() << ", " << t.time() << endl;
        t = Random::random() % 100;
    }

    return 0;
}
