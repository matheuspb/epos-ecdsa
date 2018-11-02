#include <smart_data.h>
#include <utility/ostream.h>
#include <alarm.h>

using namespace EPOS;

const TSTP::Time DATA_PERIOD = 1 * 1000000;
const TSTP::Time DATA_EXPIRY = 2 * DATA_PERIOD;
const TSTP::Time INTEREST_EXPIRY = 2ull * 60 * 60 * 1000000;

IF<Traits<USB>::enabled, USB, UART>::Result io;

template<typename T>
class Printer: public Smart_Data_Common::Observer
{
public:
    Printer(T * t) : _data(t) {
        _data->attach(this);
        print(_data->db_series());
    }
    ~Printer() { _data->detach(this); }

    template<typename Record>
    void print(const Record & d)
    {
        bool was_locked = CPU::int_disabled();
        if(!was_locked)
            CPU::int_disable();
        if(EQUAL<Record, Smart_Data_Common::DB_Series>::Result)
            io.put('S');
        else
            io.put('R');
        for(unsigned int i = 0; i < sizeof(Record); i++)
            io.put(reinterpret_cast<const char *>(&d)[i]);
        for(unsigned int i = 0; i < 3; i++)
            io.put('X');
        if(!was_locked)
            CPU::int_enable();
    }

    void update(Smart_Data_Common::Observed * obs) {
        print(_data->db_record());
    }

private:
    T * _data;
};

int main()
{
    // Get epoch time from serial
    TSTP::Time epoch = 0;
    char c = io.get();
    if(c != 'X') {
        epoch += c - '0';
        c = io.get();
        while(c != 'X') {
            epoch *= 10;
            epoch += c - '0';
            c = io.get();
        }
        TSTP::epoch(epoch);
    }

    GPIO led('C', 3, GPIO::OUT);

    // Interest center points
    TSTP::Coordinates center(0, 600, 100);

    // Regions of interest
    TSTP::Time start = TSTP::now();
    TSTP::Time end = start + INTEREST_EXPIRY;
    TSTP::Region region(center, 5000, start, end);

    // Data of interest
    Switch data_switch(region, DATA_EXPIRY, DATA_PERIOD);
    Temperature data_temperature(region, DATA_EXPIRY, DATA_PERIOD);

    // Event-driven actuators
    Printer<Temperature> p11(&data_temperature);

    // Time-triggered actuators
    while(TSTP::now() < end) {
        Alarm::delay(DATA_PERIOD);

        Switch::Value state = data_switch;

        data_switch = !state;
        led.set(!state);
    }

    return 0;
}
