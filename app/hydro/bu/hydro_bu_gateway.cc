#include <http.h>
#include <tstp.h>
#include <gpio.h>
#include <machine.h>
#include <smart_data.h>
#include <transducer.h>
#include <persistent_storage.h>
#include <utility/ostream.h>
#include <watchdog.h>

using namespace EPOS;

// Timeout variable
TSC::Time_Stamp _init_timeout;
const RTC::Microsecond SEND_DB_SERIES_TIMEOUT = 5ull * 60 * 1000000;
const RTC::Microsecond SEND_DB_RECORD_TIMEOUT = 5ull * 60 * 1000000;

// Test time
// const RTC::Microsecond INTEREST_EXPIRY = 1ull * 60 * 1000000;
// const RTC::Microsecond HTTP_SEND_PERIOD = 2ull * 60 * 1000000;
// const RTC::Microsecond INTEREST_PERIOD = INTEREST_EXPIRY / 2;
// const unsigned int HTTP_SEND_PERIOD_MULTIPLY = 1;//2 * 12;

// Production time
const RTC::Microsecond INTEREST_EXPIRY = 5ull * 60 * 1000000;
const RTC::Microsecond HTTP_SEND_PERIOD = 30ull * 60 * 1000000;
const RTC::Microsecond INTEREST_PERIOD = INTEREST_EXPIRY / 2;
const unsigned int HTTP_SEND_PERIOD_MULTIPLY = 4;//2 * 12;

typedef Smart_Data_Common::SI_Record DB_Record;
typedef Smart_Data_Common::DB_Series DB_Series;
typedef Persistent_Ring_FIFO<DB_Record> Storage;

// Credentials
const char DOMAIN[]   = "...";
const char USERNAME[] = "...";
const char PASSWORD[] = "...";

// Server
const char HOST[] = "iot.lisha.ufsc.br";
const char ROUTE_ATTACH[] = "/api/attach.php";
const char ROUTE_PUT[] = "/api/put.php";

struct Credentials
{
    Credentials() {
        _size_domain = sizeof(DOMAIN) - 1;
        memcpy(_domain,DOMAIN,_size_domain);
        _size_username = sizeof(USERNAME) - 1;
        memcpy(_username,USERNAME,_size_username);
        _size_password = sizeof(PASSWORD) - 1;
        memcpy(_password,PASSWORD,_size_password);
    }
    char _size_domain;
    char _domain[sizeof(DOMAIN) - 1];
    char _size_username;
    char _username[sizeof(USERNAME) - 1];
    char _size_password;
    char _password[sizeof(PASSWORD) - 1];
}__attribute__((packed));

struct Attach_Payload
{
    void credentials(Credentials credentials){ _credentials = credentials; }
    void payload(DB_Series series){ _series = series; }
public:    
    Credentials _credentials;
    DB_Series _series;
}__attribute__((packed));

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
typedef _Put_Payload<100> Put_Payload;

Put_Payload * put_payload; //static allocation was giving an heap out of memory error
Attach_Payload attach_payload;

Rain * pluviometer;

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

    DB_Record e;
    M95 * m95 = M95::get(0);
    int ret = 0;

    // Declare a timeout for sending the db_series
    _init_timeout = TSC::time_stamp() + SEND_DB_SERIES_TIMEOUT * (TSC::frequency() / 1000000);

    attach_payload.payload(pluviometer->db_series());
    do {
        check_timeout();
        cout << "...Sending pluviometer db_series..." << endl;
        ret = Quectel_HTTP::post("https://150.162.62.3/api/attach.php", &attach_payload, sizeof(Attach_Payload));
        cout << "post pluviometer db_series: " << ret << endl;
    } while(ret <= 0);
    ret = 0;

    m95->off();

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

                cout << "Popped: " << e << endl;
                put_payload->payload(e, db_record_index);
                ret = 0;

                CPU::int_disable();
                popped_next = Storage::pop(&e);
                CPU::int_enable();

		        // Declare a timeout for sending the db_records
    	        _init_timeout = TSC::time_stamp() + SEND_DB_RECORD_TIMEOUT * (TSC::frequency() / 1000000);
            
                if((db_record_index == (Put_Payload::SIZE-1)) || !popped_next) {
                    do {
    		            cout << "...Sendind db_records..." << endl;
                        ret = Quectel_HTTP::post("https://150.162.62.3/api/put.php", put_payload, sizeof(Credentials) + (db_record_index+1)*sizeof(DB_Record));
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

int tstp_work()
{
    OStream cout;
    cout << "tstp_work() init" << endl;

    cout << "epoch now = " << TSTP::absolute(TSTP::now()) / 1000000 << endl;

    cout << "Going to sleep..." << endl;
    while(Periodic_Thread::wait_next()) {

        cout << "tstp_work()" << endl;
        cout << "sleeping 10 seconds..." << endl;
        Alarm::delay(10000000);
        cout << "...woke up" << endl;

        Rain::DB_Record r0 = pluviometer->db_record();
        cout << "r0 = " << r0 << endl;

        CPU::int_disable();
        if(!pluviometer->expired()) {
            Storage::push(r0);
            cout << "push r0 OK" << endl;
        }
        CPU::int_enable();
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

int main()
{
    OStream cout;

    cout << "main()" << endl;

    put_payload = new Put_Payload(); //dynamic allocated to solve the heap out of memory problem

    // Storage::clear(); cout << "storage cleared" << endl; while(true);

    Alarm::delay(5000000);

    M95 * m95 = M95::get(0);

    TSTP::Time t = m95->now();
    TSTP::epoch(t);

    pluviometer = new Rain(0, 10000000, static_cast<Rain::Mode>(Rain::PRIVATE | Rain::CUMULATIVE));

    //Threads config
    Periodic_Thread * tstp_worker = new Periodic_Thread(INTEREST_EXPIRY, tstp_work);
    Periodic_Thread * http_sender = new Periodic_Thread(HTTP_SEND_PERIOD, http_send);

    Periodic_Thread * watchdog = new Periodic_Thread(500 * 1000, kick);

    cout << "threads created. Joining." << endl;

    watchdog->join();
    tstp_worker->join();
    http_sender->join();

    return 0;
}
