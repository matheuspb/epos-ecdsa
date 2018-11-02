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

IF<Traits<USB>::enabled, USB, UART>::Result io;

typedef TSTP::Coordinates Coordinates;
typedef TSTP::Region Region;

const unsigned int INTEREST_PERIOD = 30 * 1000000;
const unsigned int INTEREST_EXPIRY = 2 * INTEREST_PERIOD;
const unsigned int PRESENCE_EXPIRY = 5 * 60 * 1000000;
const unsigned int DOOR_FEELINGS_EXPIRY = 30*1000000;

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

class Presence_Transform
{
public:
    void apply(Active_Power * destination, Presence * source) {
        Presence::Value p = *source;
        if(p)
            *destination = 1;
    }
};

typedef Smart_Data<Dummy_Transducer<TSTP::Unit::Get_Quantity<TSTP::Unit::Angular_Speed,TSTP::Unit::F32>::UNIT>> Dummy_Gyroscope;
template<typename T>
class Door_Feelings_Transform: public Smart_Data_Common::Observer
{
    static const unsigned int INFINITE = -1;
public:
    Door_Feelings_Transform(T * t) : _data(t) {
        _data->attach(this);
        _gyro = new Dummy_Gyroscope(0,INFINITE,Dummy_Gyroscope::PRIVATE);
        print(_gyro->db_series());
    }
    ~Door_Feelings_Transform() { _data->detach(this); }

    void update(Smart_Data_Common::Observed * obs) {
        Angular_Rate::Value v = (*_data);
        Angular_Rate::Time t = _data->time();

        Angular_Rate_Analyzer::Data d = *(v.data<Angular_Rate_Analyzer::Data>());
        Dummy_Gyroscope::DB_Record db;

        for(int i = 0; i < Angular_Rate_Analyzer::Data::SIZE; i++){
            t+=(d.m[i].offset * 1000);

            (*_gyro) = d.m[i].x;
            db = _gyro->db_record();
            db.t = t;
            db.dev = 0;
            print(db);

            (*_gyro) = d.m[i].y;
            db = _gyro->db_record();
            db.t = t;
            db.dev = 1;
            print(db);

            (*_gyro) = d.m[i].z;
            db = _gyro->db_record();
            db.t = t;
            db.dev = 2;
            print(db);
        }
    }

private:
    Angular_Rate * _data;
    Dummy_Gyroscope * _gyro;
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

    // Interest center points
    Coordinates center_outlet0(460-730, -250-80, -15);
    Coordinates center_outlet1(-5-730, -30-80, -15);
    //Coordinates center_lights1(305-730, -170-80, 220);
    Coordinates center_lux0(-720,-90, 0);
    Coordinates center_door0(-200,150,200);
    Coordinates center_presence0(-720,-100,0);
    Coordinates center_perimeter_sensor(-160, 452, -73);
    Coordinates center_temperature0(-200,100,0);

    // Regions of interest
    Region region_outlet0(center_outlet0, 0, 0, -1);
    Region region_outlet1(center_outlet1, 0, 0, -1);
    //Region region_lights1(center_lights1, 0, 0, -1);
    Region region_lux0(center_lux0, 0, 0, -1);
    Region region_door0(center_door0, 0, 0, -1);
    Region region_presence0(center_presence0, 0, 0, -1);
    Region region_perimeter_sensor(center_perimeter_sensor, 0, 0, -1);
    Region region_temperature0(center_temperature0, 0, 0, -1);

    // Data of interest
    Active_Power data_outlet0(region_outlet0, INTEREST_EXPIRY, INTEREST_PERIOD);
    Active_Power data_outlet1(region_outlet1, INTEREST_EXPIRY, INTEREST_PERIOD);
    //Active_Power data_lights1(region_lights1, INTEREST_EXPIRY, INTEREST_PERIOD);
    Luminous_Intensity data_lux0(region_lux0, INTEREST_EXPIRY, INTEREST_PERIOD);
    Presence data_presence0(region_presence0, PRESENCE_EXPIRY, 0);
    Distance distance(region_perimeter_sensor, INTEREST_EXPIRY, INTEREST_PERIOD);
    Vibration vibration(region_perimeter_sensor, INTEREST_EXPIRY, INTEREST_PERIOD);
    Sound sound(region_perimeter_sensor, INTEREST_EXPIRY, INTEREST_PERIOD);
    I2C_Temperature temperature0(region_temperature0,INTEREST_EXPIRY, INTEREST_PERIOD);
    I2C_Temperature temperature1(region_lux0,INTEREST_EXPIRY, INTEREST_PERIOD);
    I2C_Temperature temperature2(0,INTEREST_EXPIRY,I2C_Temperature::PRIVATE,INTEREST_PERIOD);


    Angular_Rate door_angular_rate(region_door0, DOOR_FEELINGS_EXPIRY, 0);

    // Printers to output received messages to serial
    Printer<Active_Power> p1(&data_outlet0);
    Printer<Active_Power> p2(&data_outlet1);
    //Printer<Active_Power> p3(&data_lights1);
    Printer<Luminous_Intensity> p4(&data_lux0);
    Printer<Presence> p5(&data_presence0);
    Printer<Distance> p6(&distance);
    Printer<Vibration> p7(&vibration);
    Printer<Sound> p8(&sound);
    Printer<I2C_Temperature> p9(&temperature0);
    Printer<I2C_Temperature> p10(&temperature1);
    Printer<I2C_Temperature> p11(&temperature2);
    Door_Feelings_Transform<Angular_Rate> df_transf(&door_angular_rate);

    char led = 0;
    while(1) {
        Alarm::delay(1000000);
        g.set((led++)%2);
    }

    return 0;
}
