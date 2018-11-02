#include <smart_data.h>
#include <alarm.h>
#include <i2c.h>

using namespace EPOS;

USB io;

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
        //cout << "MY_Temperature_Transducer::sense(dev=" << dev << ")" << endl;
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

template<typename T>
class Printer: public Smart_Data_Common::Observer
{
public:
    Printer(T * t) : _data(t) {
        _data->attach(this);
        print(_data->db_series());
    }
    ~Printer() { _data->detach(this); }

    void update(Smart_Data_Common::Observed * obs) {
        print(_data->db_record());
    }

    template<typename D>
    void print(const D & d)
    {
        bool was_locked = CPU::int_disabled();
        if(!was_locked)
            CPU::int_disable();
        if(EQUAL<D, Smart_Data_Common::DB_Series>::Result)
            io.put('S');
        else
            io.put('R');
        for(unsigned int i = 0; i < sizeof(D); i++)
            io.put(reinterpret_cast<const char *>(&d)[i]);
        for(unsigned int i = 0; i < 3; i++)
            io.put('X');
        if(!was_locked)
            CPU::int_enable();
    }

private:
    T * _data;
};

void setup()
{
    // Get epoch time from serial
    TSTP::Time epoch = 0;
    unsigned int bytes = 0;
    char c = 0;
    do {
        while(!io.ready_to_get());
        c = io.get();
    } while(c != 'X');
    while(bytes < sizeof(TSTP::Time)){
        while(!io.ready_to_get());
        c = io.get();
        epoch |= (static_cast<unsigned long long>(c) << ((bytes++)*8));
    }
    TSTP::epoch(epoch);
    Alarm::delay(5000000);
}

int main()
{
    GPIO g('C', 3, GPIO::OUT); // EPOSMote III led pin
    g.set(false); // turn off led
    setup();
    g.set(true); // turn on led

    // Local SmartData constructor
    My_Temperature t(0, 1500000, My_Temperature::PRIVATE, 5000000); // dev=0, expiry=15s, mode=PRIVATE, period=5s
    Printer<My_Temperature> p(&t);

    Thread::self()->suspend();
    return 0;
}