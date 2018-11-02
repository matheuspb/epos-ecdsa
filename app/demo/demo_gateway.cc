#include <smart_data.h>
#include <usb.h>

using namespace EPOS;

USB io;

template<typename T>
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
}

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

private:
    T * _data;
};

void setup()
{
    // Get epoch time from serial
    TSTP::Time epoch = 0;
    while(!io.ready_to_get());
    char c = io.get();
    if(c != 'X') {
        epoch += c - '0';
        while(!io.ready_to_get());
        c = io.get();
        while(c != 'X') {
            epoch *= 10;
            epoch += c - '0';
            while(!io.ready_to_get());
            c = io.get();
        }
        TSTP::epoch(epoch);
    }

    Alarm::delay(5000000);
}

int main()
{
    GPIO g('C', 3, GPIO::OUT);
    g.set(false);

    setup();

    g.set(true);

    // Interest center points
    TSTP::Coordinates center(300, 300, 0);

    const TSTP::Time DATA_PERIOD = 5 * 1000000;
    const TSTP::Time DATA_EXPIRY = DATA_PERIOD;
    const TSTP::Time INTEREST_EXPIRY = 2ull * 60 * 60 * 1000000;

    // Regions of interest
    TSTP::Time start = TSTP::now();
    TSTP::Time end = start + INTEREST_EXPIRY;
    TSTP::Region region(center, 50 * 100, start, end);

    // Data of interest
    Temperature data_temperature(region, DATA_EXPIRY, DATA_PERIOD);
    Smart_Data<Switch_Sensor> data_led(region, DATA_EXPIRY, 0);

    // Event-driven actuators
    Printer<Temperature> p0(&data_temperature);

    char led = 0;
    while(1) {
        Alarm::delay(1000000);
        data_led = ((led++)%2);
    }

    return 0;
}
