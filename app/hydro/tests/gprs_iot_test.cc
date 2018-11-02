// EPOS cout Test Program

#include <http.h>
#include <tstp.h>
#include <gpio.h>
#include <machine.h>
#include <smart_data.h>
#include <hydro_board.h>
#include <transducer.h>
#include <persistent_storage.h>
#include <utility/ostream.h>

using namespace EPOS;

TSC::Time_Stamp _init_timeout;
const RTC::Microsecond SEND_DB_SERIES_TIMEOUT = 5ull * 60 * 1000000;
const RTC::Microsecond SEND_DB_RECORD_TIMEOUT = 5ull * 60 * 1000000;

typedef Smart_Data_Common::SI_Record DB_Record;
typedef Smart_Data_Common::DB_Series DB_Series;
typedef Persistent_Ring_FIFO<DB_Record> Storage;

static const unsigned int SMARTDATA_PAYLOAD_SIZE = 12;
static const unsigned int SMARTDATA_STORAGE_SIZE = 24;

/* End Point */
const char HOST[] = "iot.lisha.ufsc.br";
const char ROUTE_ATTACH[] = "/api/attach.php";
const char ROUTE_PUT[] = "/api/put.php";

// Credentials
const char DOMAIN[]   = "...";
const char USERNAME[] = "...";
const char PASSWORD[] = "...";

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
typedef _Put_Payload<SMARTDATA_PAYLOAD_SIZE> Put_Payload;

Put_Payload * put_payload; //static allocation was giving an heap out of memory error
Attach_Payload attach_payload;

OStream cout;

void check_timeout()
{
    if(TSC::time_stamp() > _init_timeout) {
        CPU::int_disable();
        Machine::delay(5000000);
        Machine::reboot();
    }
}

int main()
{
	cout << "main()" << endl;
	put_payload = new Put_Payload();

	// Machine::delay(5000000);
	
	M95 * m95 = M95::get(0);
	TSTP::Time t = m95->now();
    TSTP::epoch(t);

    Water_Flow_M170 flow(0, 500000, static_cast<Water_Flow_M170::Mode>(Water_Flow_M170::PRIVATE | Water_Flow_M170::CUMULATIVE));

    int ret = 0;
    _init_timeout = TSC::time_stamp() + SEND_DB_SERIES_TIMEOUT * (TSC::frequency() / 1000000);
    attach_payload.payload(flow.db_series());
    do {
        check_timeout();
        cout << "...Sending flow db_series..." << endl;
        ret = Quectel_HTTP::post("https://150.162.62.3/api/attach.php", &attach_payload, sizeof(Attach_Payload));
        cout << "post flow db_series: " << ret << endl;
    } while(ret <= 0);
    ret = 0;

    for(int i=0; i < SMARTDATA_STORAGE_SIZE; i++) {
    	Water_Flow_M170::DB_Record r0 = flow.db_record();
    	cout << "r0=" << r0 << endl;
    	// put_payload->payload(e, i);
        if(!flow.expired()) {
            CPU::int_disable();
            Storage::push(r0);
            CPU::int_enable();
            cout << "push r0 OK" << endl;
        }
    	Machine::delay(1000000);
    }

    cout << "Sleeping for 10s..." << endl;
    Machine::delay(10000000);

    // _init_timeout = TSC::time_stamp() + SEND_DB_RECORD_TIMEOUT * (TSC::frequency() / 1000000);
   	// do {
    //     check_timeout();
    //     cout << "...Sending flow db_records..." << endl;
    //     ret = Quectel_HTTP::post("https://150.162.62.3/api/put.php", put_payload, sizeof(Put_Payload));
    //     cout << "post flow db_records: " << ret << endl;
    // } while(ret <= 0);

    DB_Record e;
    CPU::int_disable();
    bool popped_next = Storage::pop(&e);
    CPU::int_enable();
    int db_record_index = 0;
    while(popped_next) {

        cout << "Popped: " << e << endl;
        put_payload->payload(e, db_record_index);
        cout << "Added to payload" << endl;
        ret = 0;

        CPU::int_disable();
        popped_next = Storage::pop(&e);
        CPU::int_enable();
        cout << "Popped next=" << popped_next << endl;

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
        else
            db_record_index++;
    }

    return 0;
}