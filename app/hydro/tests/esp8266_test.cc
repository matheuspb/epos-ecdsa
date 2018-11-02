#include <gpio.h>
#include <machine.h>
#include <smart_data.h>
#include <tstp.h>
#include <transducer.h>
#include <utility/ostream.h>
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

typedef Smart_Data_Common::SI_Record DB_Record; //this app does not use Digital Records
typedef Smart_Data_Common::DB_Series DB_Series;

// Server
const char HOST[] = "iot.lisha.ufsc.br";
const char ROUTE_ATTACH[] = "/api/attach.php";
const char ROUTE_PUT[] = "/api/put.php";

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

template <unsigned int S>
struct _Put_Payload
{
    static const unsigned int SIZE = S;
    void credentials(Credentials credentials){ _credentials = credentials; }
    void payload(DB_Record smartdata, unsigned int index)
    {
        _smartdata[index] = smartdata; 
    }
public:    
    Credentials _credentials;
    DB_Record _smartdata[SIZE];
}__attribute__((packed));
typedef _Put_Payload<1> Put_Payload;

Put_Payload * put_payload; //static allocation was giving an heap out of memory error
Attach_Payload attach_payload;
Water_Flow_M170 * flow;

OStream cout;

int main()
{
    cout << "main()" << endl;

    // Initialize mediators
    /* TODO:
     * Transfer the uart, gpio, host and routes info to the traits 
     * Create a esp8266_init to initiate the mediator (use the same pattern as the m95 mediator)
    */
    UART uart(0, 115200, 8, 0, 1);
    GPIO rst('C', 0, GPIO::OUT);
    cout << "ESP INIT" << endl;
    ESP8266 esp(&uart, &rst);

    flow = new Water_Flow_M170(0, 1000000, static_cast<Water_Flow_M170::Mode>(Water_Flow_M170::PRIVATE | Water_Flow_M170::CUMULATIVE));
    put_payload = new Put_Payload(); //dynamic allocated to solve the heap out of memory problem

    attach_payload.payload(flow->db_series());
    bool attach_success = false;

    while(true)
    {
        int status = esp.wifi_status();
        cout << WIFI_CREDENTIALS_SSID << " wifi status: " << status << endl;
        if(!esp.connected()) {
            esp.connect(WIFI_CREDENTIALS_SSID, sizeof(WIFI_CREDENTIALS_SSID)-1, WIFI_CREDENTIALS_PASSWORD, sizeof(WIFI_CREDENTIALS_PASSWORD)-1, WIFI_CREDENTIALS_USERNAME, sizeof(WIFI_CREDENTIALS_USERNAME)-1);
        }
        if(esp.connected()){
            char ip[ESP8266::IP_MAX];
            int ip_size = esp.ip(ip);
            cout << WIFI_CREDENTIALS_SSID << " IP: " << ip << endl;
            TSTP::Time t = esp.now();
            if(t != 0) {
                cout << "Timestamp=" << t << endl;
                TSTP::epoch(t);
                if(!attach_success) {
                    bool config_endpoint = esp.config_endpoint_1(80, HOST, sizeof(HOST)-1, ROUTE_ATTACH, sizeof(ROUTE_ATTACH)-1);
                    if(config_endpoint) {
                        bool post_attach = esp.post_1(&attach_payload, sizeof(Attach_Payload));
                        cout << "attach=" << post_attach << endl;
                        if(post_attach) {
                            attach_success = true;
                        }
                    }
                }
                else {
                    bool config_endpoint = esp.config_endpoint_1(80, HOST, sizeof(HOST)-1, ROUTE_PUT, sizeof(ROUTE_PUT)-1);
                    if(config_endpoint) {
                        DB_Record r = flow->db_record();
                        cout << "r=" << r << endl;
                        put_payload->payload(r, 0);
                        bool post_put = esp.post_1(put_payload, sizeof(Put_Payload));
                        cout << "put=" << post_put << endl;
                    }
                }
            }
        }
        Machine::delay(2000000);
    }

    return 0;
}
