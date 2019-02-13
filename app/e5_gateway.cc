#include <smart_data.h>
#include <usb.h>
#include <utility/ostream.h>

#include "smart_log.h"


using namespace EPOS;

USB io;
OStream cout;

template<typename T>
class Printer: public Smart_Data_Common::Observer
{
public:
    Printer(T * t, char* name) : _data(t), name(name) {
        _data->attach(this);
    }
    ~Printer() { _data->detach(this); }

    void update(Smart_Data_Common::Observed * obs) {
        cout << "[\"" << name << "\"," << *_data << "," << _data->time() << "]" << endl;
    }

private:
    T * _data;
    char * name;
};

int main()
{

    // Interest center points
    TSTP::Coordinates center(300, 300, 0);

    const TSTP::Time DATA_PERIOD = 5 * 1000000;
    const TSTP::Time DATA_EXPIRY = 10* 1000000;
    const TSTP::Time INTEREST_EXPIRY = 2ull * 60 * 60 * 1000000;

    // Regions of interest
    TSTP::Time start = TSTP::now();
    TSTP::Time end = start + INTEREST_EXPIRY;
    TSTP::Region region(center, 50 * 100, start, end);

    // Data of interest
    Temperature data_temperature(region, DATA_EXPIRY, DATA_PERIOD);
    Smart_Log log(region, DATA_EXPIRY);

    // Event-driven actuators
    Printer<Temperature> p0(&data_temperature, "Temperature");
    Printer<Smart_Log> p1(&log, "Log");

    while(true) {
    }

    return 0;
}
