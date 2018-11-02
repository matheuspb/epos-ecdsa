#include <http.h>
#include <tstp.h>
#include <gpio.h>
#include <machine.h>
#include <smart_data.h>
#include <hydro_board.h>
#include <transducer.h>
#include <persistent_storage.h>
#include <utility/ostream.h>
#include <watchdog.h>

//This includes the server credentials and wifi credentials constants
// Server Credentials
// const char SERVER_CREDENTIALS_DOMAIN[]   = "...";
// const char SERVER_CREDENTIALS_USERNAME[] = "...";
// const char SERVER_CREDENTIALS_PASSWORD[] = "...";
//WiFi Credentials
// const char WIFI_CREDENTIALS_SSID[] = "...";
// const char WIFI_CREDENTIALS_USERNAME[] = "...";
// const char WIFI_CREDENTIALS_PASSWORD[] = "...";
//it is on svn:ignore, so you need to create this file
#include <credentials.key>

using namespace EPOS;

// Timeout variable
TSC::Time_Stamp _init_timeout;

// Timeout constants
const RTC::Microsecond SEND_DB_SERIES_TIMEOUT = 5ull * 60 * 1000000;
const RTC::Microsecond SEND_DB_RECORD_TIMEOUT = 5ull * 60 * 1000000;

// Test time
// const RTC::Microsecond INTEREST_EXPIRY = 30 * 1000000;
// const RTC::Microsecond INTEREST_PERIOD = INTEREST_EXPIRY*2;
// const RTC::Microsecond HTTP_SEND_PERIOD = 2ull * 60 * 1000000;
// const unsigned int HTTP_SEND_PERIOD_MULTIPLY = 1;//2 * 12;

// Production time
const RTC::Microsecond INTEREST_EXPIRY = 1ull * 60 * 1000000;
const RTC::Microsecond INTEREST_PERIOD = 5ull * 60 * 1000000;
const RTC::Microsecond HTTP_SEND_PERIOD = 30ull * 60 * 1000000;
const unsigned int HTTP_SEND_PERIOD_MULTIPLY = 4;//2 * 12;

typedef Smart_Data_Common::SI_Record DB_Record; //this app does not use Digital Records

//The db_record of this app will be only value and timestamp
//the rest will be put in the post url
//TODO: find a way to make it generic, so that other variables may be declared
struct DB_Record_aux {
    DB_Record_aux(){}
    DB_Record_aux(DB_Record r)
    {
        value = r.value;
        t = r.t; 
    }
public:
    double value;
    unsigned long long t;
}__attribute__((packed));

typedef Smart_Data_Common::DB_Series DB_Series;
typedef Persistent_Ring_FIFO<DB_Record_aux> Storage;

//CSDF: Common SmartData Fields
// (8 bits) -> | version | unit | error | confidence | x | y | z | dev |
enum CSDF {
    VERSION    = (1 << 7),
    UNIT       = (1 << 6),
    ERROR      = (1 << 5),
    CONFIDENCE = (1 << 4),
    X          = (1 << 3),
    Y          = (1 << 2),
    Z          = (1 << 1),
    DEV        = (1 << 0)
};
char csdf_c[4];


// Server
const char URL_ATTACH[] = "https://150.162.62.3/api/attach.php";
const char URL_PUT[] = "https://150.162.62.3/api/put.php";
const char CSDF_TAG[] = "?csdf=";
char url_put_csdf[(sizeof(URL_PUT)-1) + (sizeof(CSDF_TAG)-1) + sizeof(csdf_c)];

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
    void payload(DB_Record_aux smartdata, unsigned int index){ _smartdata[index] = smartdata; }
public:    
    Credentials _credentials;
    unsigned char version;
    // unsigned long unit;
    unsigned char error;
    unsigned char confidence;
    long x;
    long y;
    long z;
    unsigned long dev;
    DB_Record_aux _smartdata[SIZE];
}__attribute__((packed));
typedef _Put_Payload<60> Put_Payload;

Put_Payload * put_payload; //static allocation was giving an heap out of memory error
Attach_Payload attach_payload;

void check_timeout()
{
    if(TSC::time_stamp() > _init_timeout) {
        M95 * m95 = M95::get(0);
        m95->off();
        CPU::int_disable();
        Machine::delay(5000000);
        Machine::reboot();
    }
}

int http_send()
{
    OStream cout;
    cout << "http_send() init" << endl;
    DB_Record_aux e;
    M95 * m95 = M95::get(0);
    m95->off();
    int ret = 0;
    while(true) {
        for(unsigned int i = 0; i < HTTP_SEND_PERIOD_MULTIPLY; i++)
            Periodic_Thread::wait_next();
     
        cout << "http_send()" << endl;
        CPU::int_disable();
        if(Storage::pop(&e)) {
            CPU::int_enable();
            cout << "Turning GPRS on" << endl;
            m95->on();
            bool popped_next = true;
            int db_record_index = 0;

            while(popped_next) {

                Alarm::delay(500000);
                put_payload->payload(e, db_record_index);
                ret = 0;

                CPU::int_disable();
                popped_next = Storage::pop(&e);
                CPU::int_enable();

		        // Declare a timeout for sending the db_records
    	        _init_timeout = TSC::time_stamp() + SEND_DB_RECORD_TIMEOUT * (TSC::frequency() / 1000000);
            
                if((db_record_index == (Put_Payload::SIZE-1)) || !popped_next) {
                    do {
                        cout << "db_records num: " << db_record_index+1 << endl;
                        Alarm::delay(500000);
                        ret = Quectel_HTTP::post(url_put_csdf, put_payload, sizeof(Put_Payload)-((Put_Payload::SIZE-(db_record_index+1))*sizeof(DB_Record_aux)));
                        cout << "post = " << ret << endl;
                    } while((ret <= 0) && (TSC::time_stamp() < _init_timeout));

                    if(ret <= 0) {
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
            if(ret > 0) {
                RTC::Microsecond t = m95->now();
                if(t)
                    TSTP::epoch(t);
            }
            cout << "Turning GPRS off" << endl;
            m95->off();
        } else
            CPU::int_enable();
        cout << "Going to sleep..." << endl;
    }
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

void send_db_series(DB_Series s)
{
    OStream cout;
    int ret = 0;
    // Declare a timeout for sending the db_series
    _init_timeout = TSC::time_stamp() + SEND_DB_SERIES_TIMEOUT * (TSC::frequency() / 1000000);
    attach_payload.payload(s);
    do {
        check_timeout();
        cout << "...Sending db_series..." << endl;
        ret = Quectel_HTTP::post(URL_ATTACH, &attach_payload, sizeof(Attach_Payload));
        cout << "post db_series: " << ret << endl;
    } while(ret <= 0);
}

// A Stacker, when created, sends the db_series of a smartdata.
// It also listens to smartdata notify and push its db_records to the stack
template<typename T>
class Stacker: public Smart_Data_Common::Observer
{
public:
    Stacker(T * t) : _data(t) {
        _data->attach(this);
        send_db_series(_data->db_series());
    }
    ~Stacker() { _data->detach(this); }

    void update(Smart_Data_Common::Observed * obs) {
        OStream cout;
        CPU::int_disable();
        DB_Record r = _data->db_record();
        cout << r << endl;
        Storage::push(DB_Record_aux(r));
        CPU::int_enable();
    }

private:
    T * _data;
};

void config_put_url(CSDF csdf)
{
    strncpy(url_put_csdf, URL_PUT, sizeof(URL_PUT)-1);
    strcat(url_put_csdf, CSDF_TAG);
    strcat(url_put_csdf, itoa(csdf, csdf_c));
}

int main()
{
    OStream cout;
    cout << "main()" << endl;

    Alarm::delay(5000000);

    M95 * m95 = M95::get(0);

    TSTP::Time t = m95->now();
    TSTP::epoch(t);

    // Tranducers
    new Pressure_Sensor_Keller_Capacitive(0, Hydro_Board::Port::P3, Hydro_Board::Port::P3);
    // new Water_Turbidity_Sensor_OBS(0, Hydro_Board::Port::P6, Hydro_Board::Port::P6);
    new Water_Turbidity_Sensor_OBS(1, Hydro_Board::Port::P6, Hydro_Board::Port::P5);

    // Smartdata config
    Pressure_Keller_Capacitive level(0, INTEREST_EXPIRY, Pressure_Keller_Capacitive::PRIVATE, INTEREST_PERIOD);
    // Water_Turbidity_OBS turbidity_OBS_high(0, INTEREST_EXPIRY, Water_Turbidity_OBS::PRIVATE, INTEREST_PERIOD);
    Water_Turbidity_OBS turbidity_OBS_low(1, INTEREST_EXPIRY, Water_Turbidity_OBS::PRIVATE, INTEREST_PERIOD);

    // Stackers: sends the db_series of its smartdata and push its db_records to the stack
    Stacker<Pressure_Keller_Capacitive> stacker1(&level);
    // Stacker<Water_Turbidity_OBS> stacker2(&turbidity_OBS_high);
    Stacker<Water_Turbidity_OBS> stacker3(&turbidity_OBS_low);

    //By here, the csdf would be already defined (taking into account the types of the smartdatas)
    config_put_url(static_cast<CSDF>(VERSION|ERROR|CONFIDENCE|X|Y|Z|DEV));

    put_payload = new Put_Payload(); //dynamic allocated to solve the heap out of memory problem
    put_payload->version = 17;
    // put_payload->unit = level.unit(); 
    put_payload->error = level.error();
    put_payload->confidence = 0;
    put_payload->x = level.location().x;
    put_payload->y = level.location().y;
    put_payload->z = level.location().z;
    put_payload->dev = 0;

    //Threads config
    Periodic_Thread * http_sender = new Periodic_Thread(HTTP_SEND_PERIOD, http_send);
    Periodic_Thread * watchdog = new Periodic_Thread(500 * 1000, kick);

    watchdog->join();
    http_sender->join();
    Thread::self()->suspend();
}
