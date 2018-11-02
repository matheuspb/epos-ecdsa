#include <tstp.h>
#include <gpio.h>
#include <machine.h>
#include <smart_data.h>
#include <hydro_board.h>
#include <transducer.h>
#include <persistent_storage.h>
#include <utility/ostream.h>
#include <watchdog.h>
#include <machine/cortex/esp8266.h>

//This includes the server credentials and wifi credentials constants
// Server Credentials
// const char SERVER_CREDENTIALS_DOMAIN[]   = "...";
// const char SERVER_CREDENTIALS_USERNAME[] = "...";
// const char SERVER_CREDENTIALS_PASSWORD[] = "...";
//WiFi Credentials
// const char WIFI_CREDENTIALS_SSID[] = "...";
// const char WIFI_CREDENTIALS_USERNAME[] = "...";
// const char WIFI_CREDENTIALS_PASSWORD[] = "...";
//it is on svn:ignore, so you need to create the file
#include <credentials.key>

using namespace EPOS;

// Timeout variable
TSC::Time_Stamp _init_timeout;

// Timeout constants
const RTC::Microsecond CONNECT_TO_WIFI_TIMEOUT = 2ull * 60 * 1000000;
const RTC::Microsecond SEND_DB_SERIES_TIMEOUT = 5ull * 60 * 1000000;
const RTC::Microsecond SEND_DB_RECORD_TIMEOUT = 5ull * 60 * 1000000;

// Test time
const RTC::Microsecond INTEREST_EXPIRY = 30 * 1000000;
const RTC::Microsecond INTEREST_PERIOD = INTEREST_EXPIRY*2;
const RTC::Microsecond HTTP_SEND_PERIOD = 2ull * 60 * 1000000;
const unsigned int HTTP_SEND_PERIOD_MULTIPLY = 1;//2 * 12;

// Production time
// const RTC::Microsecond INTEREST_EXPIRY = 3ull * 60 * 1000000;
// const RTC::Microsecond INTEREST_PERIOD = 5ull * 60 * 1000000;
// const RTC::Microsecond HTTP_SEND_PERIOD = 6ull * 60 * 1000000;
// const unsigned int HTTP_SEND_PERIOD_MULTIPLY = 1;//2 * 12;

typedef Smart_Data_Common::SI_Record DB_Record; //this app does not use Digital Records
typedef Smart_Data_Common::DB_Series DB_Series;
typedef Persistent_Ring_FIFO<DB_Record> Storage;

// Server
const char HOST[] = "iot.lisha.ufsc.br";
const char ROUTE_ATTACH[] = "/api/v2_0/attach.php";
const char ROUTE_PUT[] = "/api/v2_0/put.php";

struct Credentials
{
    Credentials() {
        _size_domain = sizeof(SERVER_CREDENTIALS_DOMAIN) - 1;
        memcpy(_domain,SERVER_CREDENTIALS_DOMAIN,_size_domain);
        _size_username = sizeof(SERVER_CREDENTIALS_USERNAME) - 1;
        memcpy(_username,SERVER_CREDENTIALS_USERNAME,_size_username);
        _size_password = sizeof(SERVER_CREDENTIALS_PASSWORD) - 1;
        memcpy(_password,SERVER_CREDENTIALS_PASSWORD,_size_password);
    }
    char _size_domain;
    char _domain[sizeof(SERVER_CREDENTIALS_DOMAIN) - 1];
    char _size_username;
    char _username[sizeof(SERVER_CREDENTIALS_USERNAME) - 1];
    char _size_password;
    char _password[sizeof(SERVER_CREDENTIALS_PASSWORD) - 1];
}__attribute__((packed));

struct Attach_Payload
{
    void credentials(Credentials credentials){ _credentials = credentials; }
    void payload(DB_Series series){ _series = series; }
public:    
    Credentials _credentials;
    DB_Series _series;
}__attribute__((packed));

// TODO: find a generic way to declare the common smartdata variables
template <unsigned int S>
struct _Put_Payload
{
    static const unsigned int SIZE = S;
    void credentials(Credentials credentials){ _credentials = credentials; }
    void payload(DB_Record smartdata, unsigned int index){ _smartdata[index] = smartdata; }
public:    
    Credentials _credentials;
    DB_Record _smartdata[SIZE];
}__attribute__((packed));
typedef _Put_Payload<5> Put_Payload;

Put_Payload * put_payload; //static allocation was giving an heap out of memory error
Attach_Payload attach_payload;
ESP8266 * esp;
Water_Level_Ultrasonic_Microflex * level;
Distance * distance;

void check_timeout()
{
    OStream cout;
    if(TSC::time_stamp() > _init_timeout) {
        cout << "Timeout! Reseting..." << endl;
        esp->off();
        CPU::int_disable();
        Machine::delay(5000000);
        Machine::reboot();
    }
}

void send_db_series(DB_Series s)
{
    OStream cout;
    bool ret = false;
    // Declare a timeout for sending the db_series
    _init_timeout = TSC::time_stamp() + SEND_DB_SERIES_TIMEOUT * (TSC::frequency() / 1000000);
    attach_payload.payload(s);
    do {
        check_timeout();
        bool status = esp->connected();
        if(!status) {
            cout << "Reconnecting to " << WIFI_CREDENTIALS_SSID << endl;
            status = esp->connect(WIFI_CREDENTIALS_SSID, sizeof(WIFI_CREDENTIALS_SSID)-1, WIFI_CREDENTIALS_PASSWORD, sizeof(WIFI_CREDENTIALS_PASSWORD)-1, WIFI_CREDENTIALS_USERNAME, sizeof(WIFI_CREDENTIALS_USERNAME)-1);
        }
        if(status) {
            cout << "Connected to " << WIFI_CREDENTIALS_SSID << endl;
            if(esp->config_endpoint_1(80, HOST, sizeof(HOST)-1, ROUTE_ATTACH, sizeof(ROUTE_ATTACH)-1)) {
                cout << "...Sending db_series..." << endl;
                ret = esp->post_1(&attach_payload, sizeof(Attach_Payload));
                cout << "post=" << ret << endl;
            }
        }
    } while(!ret);
}

int http_send()
{
    OStream cout;
    cout << "http_send() init" << endl;
    DB_Record e;
    bool ret = false;

    send_db_series(level->db_series());
    send_db_series(distance->db_series());

    esp->off();

    while(true) {
        for(unsigned int i = 0; i < HTTP_SEND_PERIOD_MULTIPLY; i++)
            Periodic_Thread::wait_next();
     
        cout << "http_send()" << endl;
        CPU::int_disable();
        if(Storage::pop(&e)) {
            CPU::int_enable();

            cout << "Turning ESP on" << endl;
            esp->on();
            Alarm::delay(2000000);

            bool popped_next = true;
            int db_record_index = 0;

            while(popped_next) {

                Alarm::delay(500000);
                put_payload->payload(e, db_record_index);
                ret = false;

                CPU::int_disable();
                popped_next = Storage::pop(&e);
                CPU::int_enable();

		        // Declare a timeout for sending the db_records
    	        _init_timeout = TSC::time_stamp() + SEND_DB_RECORD_TIMEOUT * (TSC::frequency() / 1000000);
            
                if((db_record_index == (Put_Payload::SIZE-1)) || !popped_next) {
                    do {
                        cout << "db_records num: " << db_record_index+1 << endl;
                        Alarm::delay(500000);
                        bool status = esp->connected();
                        if(!status) {
                            cout << "Reconnecting to " << WIFI_CREDENTIALS_SSID << endl;
                            status = esp->connect(WIFI_CREDENTIALS_SSID, sizeof(WIFI_CREDENTIALS_SSID)-1, WIFI_CREDENTIALS_PASSWORD, sizeof(WIFI_CREDENTIALS_PASSWORD)-1, WIFI_CREDENTIALS_USERNAME, sizeof(WIFI_CREDENTIALS_USERNAME)-1);
                        }
                        if(status) {
                            cout << "Connected to " << WIFI_CREDENTIALS_SSID << endl;
                            if(esp->config_endpoint_1(80, HOST, sizeof(HOST)-1, ROUTE_PUT, sizeof(ROUTE_PUT)-1)) {
                                cout << "...Sendind db_records..." << endl;
                                ret = esp->post_1(put_payload, sizeof(Put_Payload)-((Put_Payload::SIZE-(db_record_index+1))*sizeof(DB_Record)));
                                cout << "post=" << ret << endl;
                            }
                        }
                    } while(!ret && (TSC::time_stamp() < _init_timeout));

                    if(!ret) {
                        cout << "POST timeout, pushing all db_records back to the stack, and waiting for other thread cycle" << endl;
                        for(int i = 0; i <= db_record_index; i++) {
                            CPU::int_disable();
                            Storage::push(put_payload->_smartdata[i]);
                            CPU::int_enable();
                        }
                        break;
                    }
                    db_record_index = 0;
                } 
                else
                    db_record_index++;
            }
            if(ret) {
                RTC::Microsecond t = esp->now();
                if(t != 0)
                    TSTP::epoch(t);
            }
            cout << "Turning ESP off" << endl;
            esp->off();
        } else
            CPU::int_enable();
        cout << "Going to sleep..." << endl;
    }
}

int tstp_work()
{
    OStream cout;
    cout << "tstp_work() init" << endl;

    cout << "epoch now = " << TSTP::absolute(TSTP::now()) / 1000000 << endl;

    cout << "Going to sleep..." << endl;
    while(Periodic_Thread::wait_next()) {
        cout << "tstp_work()" << endl;

        Water_Level_Ultrasonic_Microflex::DB_Record r0 = level->db_record();
        cout << "r0 = " << r0 << endl;
        if(!level->expired()) {
            CPU::int_disable();
            Storage::push(r0);
            CPU::int_enable();
            cout << "push r0 OK" << endl;
        }

        Distance::DB_Record r1 = distance->db_record();
        cout << "r1 = " << r1 << endl;
        if(!distance->expired()) {
            CPU::int_disable();
            Storage::push(r1);
            CPU::int_enable();
            cout << "push r1 OK" << endl;
        }

        cout << "Going to sleep..." << endl;
    }

    return 0;
}

int kick()
{
    OStream cout;
    cout << "Watchdog init" << endl;

    Watchdog::enable();
    Watchdog::kick();

    while(Periodic_Thread::wait_next())
        Watchdog::kick();    

    return 0;
}

// // A Stacker, when created, sends the db_series of a smartdata.
// // It also listens to smartdata notify and push its db_records to the stack
// template<typename T>
// class Stacker: public Smart_Data_Common::Observer
// {
// public:
//     Stacker(T * t) : _data(t) {
//         _data->attach(this);
//         send_db_series(_data->db_series());
//     }
//     ~Stacker() { _data->detach(this); }

//     void update(Smart_Data_Common::Observed * obs) {
//         OStream cout;
//         CPU::int_disable();
//         DB_Record r = _data->db_record();
//         cout << r << endl;
//         Storage::push(DB_Record_aux(r));
//         CPU::int_enable();
//     }

// private:
//     T * _data;
// };

int main()
{
    OStream cout;
    cout << "main()" << endl;

    // Initialize mediators
    /* TODO:
     * Transfer the uart, gpio, host and routes info to the traits 
     * Create a esp8266_init to initiate the mediator (use the same pattern as the m95 mediator)
    */
    UART uart(0, 115200, 8, 0, 1);
    GPIO rst('C', 0, GPIO::OUT);
    esp = new ESP8266(&uart, &rst);

    esp->reset();

    TSTP::Time t = 0;
    _init_timeout = TSC::time_stamp() + CONNECT_TO_WIFI_TIMEOUT * (TSC::frequency() / 1000000);
    cout << "Connecting to " << WIFI_CREDENTIALS_SSID << endl;
    do {
        check_timeout();
        if(esp->connect(WIFI_CREDENTIALS_SSID, sizeof(WIFI_CREDENTIALS_SSID)-1, WIFI_CREDENTIALS_PASSWORD, sizeof(WIFI_CREDENTIALS_PASSWORD)-1, WIFI_CREDENTIALS_USERNAME, sizeof(WIFI_CREDENTIALS_USERNAME)-1))
            t = esp->now();
        else
            Machine::delay(1000000);
    } while(t == 0);
    cout << "Connected to " << WIFI_CREDENTIALS_SSID << endl;
    cout << "epoch=" << t << endl;
    TSTP::epoch(t);

    // Tranducers
    GPIO trigger('A', 5, GPIO::OUT); //signal from P6 in hydro board
    GPIO echo('A', 4, GPIO::IN); //signal from P5 in hydro board
    new Distance_Sensor(0, &trigger, &echo, Hydro_Board::get_relay(Hydro_Board::Port::P6));
    new Water_Level_Sensor_Ultrasonic_Microflex(1, Hydro_Board::Port::P3, Hydro_Board::Port::P3);

    //Smart data config
    distance = new Distance(0, INTEREST_EXPIRY, Distance::PRIVATE);
    level = new Water_Level_Ultrasonic_Microflex(1, INTEREST_EXPIRY, Water_Level_Ultrasonic_Microflex::PRIVATE);

    put_payload = new Put_Payload(); //dynamic allocated to solve the heap out of memory problem

    //Threads config
    Periodic_Thread tstp_worker = Periodic_Thread(INTEREST_PERIOD, tstp_work);
    Periodic_Thread http_sender = Periodic_Thread(HTTP_SEND_PERIOD, http_send);
    Periodic_Thread watchdog = Periodic_Thread(500 * 1000, kick);

    watchdog.join();
    tstp_worker.join();
    http_sender.join();

    return 0;
}
