// TSTP Gateway to be used with tools/eposiotgw/eposiotgw

#include <transducer.h>
#include <smart_data.h>
#include <tstp.h>
#include <utility/ostream.h>
#include <utility/predictor.h>
#include <gpio.h>
#include <semaphore.h>
#include <i2c.h>

using namespace EPOS;

//IF<Traits<USB>::enabled, USB, UART>::Result io;
OStream cout;

typedef TSTP::Coordinates Coordinates;
typedef TSTP::Region Region;

const unsigned int INTEREST_PERIOD = 2 * 1000000;
const unsigned int INTEREST_EXPIRY = 2 * INTEREST_PERIOD;
const unsigned int PRESENCE_EXPIRY = 5 * 60 * 1000000;

/*template<typename T>
void print(const T & d)
{
    bool was_locked = CPU::int_disabled();
    if(!was_locked)
        CPU::int_disable();
    if(EQUAL<T, Smart_Data_Common::DB_Series>::Result)
        io.put('S');
    else
        io.put('R');
    for(unsigned int i = 0; i < sizeof(T); i++)
        io.put(reinterpret_cast<const char *>(&d)[i]);
    for(unsigned int i = 0; i < 3; i++)
        io.put('X');
    if(!was_locked)
        CPU::int_enable();
}*/

template<typename T>
class Printer: public Smart_Data_Common::Observer
{
public:
    Printer(T * t) : _data(t) {
        _data->attach(this);
        //print(_data->db_series());
    }
    ~Printer() { _data->detach(this); }

    void update(Smart_Data_Common::Observed * obs) {
        cout << "Data : " << _data->db_record() << endl;
        //print(_data->db_record());
    }

private:
    T * _data;
};

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
    cout << "Hello, I'm the gateway!" << endl;
    Alarm::delay(5000000);

    // Interest center points
    Coordinates center_temperature0(10,10,0);

    // Regions of interest
    Region region_temperature0(center_temperature0, 0, 0, -1);

    // Data of interest
    My_Temperature temperature0(region_temperature0,INTEREST_EXPIRY, INTEREST_PERIOD, My_Temperature::PREDICTIVE, My_Temperature::Predictor_Configuration(0,0.5,0));

    // Printers to output received messages to serial
    Printer<My_Temperature> p15(&temperature0);

    Thread::self()->suspend();

    return 0;
}
