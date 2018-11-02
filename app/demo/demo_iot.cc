#include <smart_data.h>
#include <machine/cortex/esp8266.h>

using namespace EPOS;

typedef Smart_Data_Common::DB_Series DB_Series;
typedef Smart_Data_Common::DB_Record DB_Record;

const char DOMAIN[]   = "";
const char USERNAME[] = "";
const char PASSWORD[] = "";
const char SSID[]     = "";
const char PASS[]     = "";

const char HOST[] = "iot.lisha.ufsc.br";
const char ROUTE_CREATE[] = "/api/create.php";
const char ROUTE_PUT[] = "/api/put.php";

OStream cout;

class Temperature_Transducer
{
    typedef TSTP::Unit Unit;
public:
    // === Technicalities ===
    // We won't use these, but every Transducer must declare them
    typedef _UTIL::Observer Observer;
    typedef _UTIL::Observed Observed;

    typedef Dummy_Predictor Predictor;
    struct Predictor_Configuration : public Predictor::Configuration {};

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
    Temperature_Transducer() {}

    static void sense(unsigned int dev, Smart_Data<Temperature_Transducer> * data) {
        cout << "sense(dev=" << dev << ")" << endl;
    }

    static void actuate(unsigned int dev, Smart_Data<Temperature_Transducer> * data, const Smart_Data<Temperature_Transducer>::Value & command) {
        cout << "actuate(dev=" << dev << ", command=" << command << ")" << endl;
        data->_value = command;
    }
};

// A SmartData that personifies the transducer defined above
typedef Smart_Data<Temperature_Transducer> My_Temperature;

class IoT_Platform_Connector
{
    enum {
        CREATE,
        PUT
    };

public:
    IoT_Platform_Connector() {
        // Initialize mediators
        _uart = new UART(0, 115200, 8, 0, 1);
        _rst = new GPIO('C', 0, GPIO::OUT);
        _esp = new ESP8266(_uart, _rst, 80, HOST, sizeof(HOST) - 1, ROUTE_CREATE, sizeof(ROUTE_CREATE) - 1);
        _route = CREATE;
        _esp->connect(SSID, sizeof(SSID)-1, PASS, sizeof(PASS)-1);
        _esp->gmt_offset(0);
        TSTP::epoch(now());
        Delay(3000000);
    }

    TSTP::Time now(){
        return _esp->now();
    }

    int post(char * buffer, unsigned int buffer_size) {
        char res[255];
        int res_size = _esp->post(buffer, buffer_size, res, 255);
        res[6] = '\n';
        int http_code = atoi(res);
        return http_code;
    }

    int create(DB_Series db) {
        if(_route != CREATE)
            change_route();
        _create_payload.payload(db);
        return post(reinterpret_cast<char*>(&_create_payload), sizeof(Create_Payload));
    }

    int put(DB_Record db) {
        if(_route != PUT)
            change_route();
        _put_payload.payload(db);
        return post(reinterpret_cast<char*>(&_put_payload), sizeof(Put_Payload));
    }

private:
    void change_route() {
        if(_route == CREATE){
            _route = PUT;
            _esp->config_endpoint(80, HOST, sizeof(HOST) - 1, ROUTE_PUT, sizeof(ROUTE_PUT) - 1);
        } else {
            _route = CREATE;
            _esp->config_endpoint(80, HOST, sizeof(HOST) - 1, ROUTE_CREATE, sizeof(ROUTE_CREATE) - 1);
        }
    }

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

    struct Create_Payload
    {
        void credentials(Credentials credentials){ _credentials = credentials; }
        void payload(DB_Series series){ _series = series; }
    public:
        Credentials _credentials;
        DB_Series _series;
    }__attribute__((packed));

    struct Put_Payload
    {
        void credentials(Credentials credentials){ _credentials = credentials; }
        void payload(DB_Record smartdata){ _smartdata = smartdata; }
    public:
        Credentials _credentials;
        DB_Record _smartdata;
    }__attribute__((packed));

private:
    short int _route;
    UART * _uart;
    GPIO * _rst;
    ESP8266 * _esp;
    Put_Payload _put_payload;
    Create_Payload _create_payload;
};

int main()
{
    IoT_Platform_Connector iot;
    cout << "Hello!" << endl;

    // Instantiate the Smart_Data here <<<<
    t = 10;

    cout << "Creating series" << endl;
    // Create the data series here  <<<<

    while(true) {
        Alarm::delay(5000000);
        cout << "Temperature = " << t << " at " << t.location() << ", " << t.time() << endl;
        cout << "Posting smartdata" << endl;
        // Send the data here  <<<<
    }

    return 0;
}