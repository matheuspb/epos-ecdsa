// EPOS Cortex ESP8266 Wi-Fi Board Mediator Declarations

#include <system/config.h>
#ifndef __mmod_zynq__
#ifndef __esp8266_h
#define __esp8266_h

#include <machine.h>
#include <machine/cortex/uart.h>
#include <machine/cortex/gpio.h>
#include <utility/string.h>

__BEGIN_SYS

class ESP8266
{
public:
    static const unsigned int SSID_MAX = 64;
    static const unsigned int PASS_MAX = 64;
    static const unsigned int USERNAME_MAX = 64;
    static const unsigned int HOST_MAX = 128;
    static const unsigned int ROUTE_MAX = 128;
    static const unsigned int PORT_MAX = 6;
    static const unsigned int IP_MAX = 16;
    static const unsigned int UART_BUFFER_SIZE = 1024;
    static const unsigned int DEFAULT_TIMEOUT = 60000000;

    typedef RTC::Microsecond Microsecond;

    enum Timeout_Action {
        RETURN,
        RESET_ESP,
        REBOOT_MACHINE
    };

    enum Auth_Method {
        PERSONAL,
        ENTERPRISE
    };

    // static const Timeout_Action TIMEOUT_ACTION = REBOOT_MACHINE;

    enum WiFi_Status {
        WL_IDLE_STATUS = 0,     //when Wi-Fi is in process of changing between statuses
        WL_NO_SSID_AVAIL = 1,   //in case configured SSID cannot be reached
        WL_CONNECTED = 3,       //after successful connection is established
        WL_CONNECT_FAILED = 4,  //if password is incorrect
        WL_DISCONNECTED = 6     //if module is not configured in station mode
    };

public:
    ESP8266(UART *uart, GPIO *power) : _rstkey(power), _uart(uart), _connected(false) { }

    ESP8266(UART *uart, GPIO *power, int port, const char *host, unsigned int host_size, const char *route, unsigned int route_size) : _rstkey(power), _uart(uart), _connected(false) {
        config_endpoint(port, host, host_size, route, route_size);
    }

    ~ESP8266() {}

    void on() { _rstkey->set(1); }
    void off() { _rstkey->set(0); }

    RTC::Microsecond now(Microsecond timeout = DEFAULT_TIMEOUT);

    int send(const char * data, unsigned int size) {
        return send_data(data, size);
    }

    void reset();

    // int connect(const char *ssid, int ssid_size, const char *pass, int pass_size, const char *username = "", int username_size = 0);

    bool connect(const char *ssid, int ssid_size, const char *pass, int pass_size, const char *username = "", int username_size = 0);

    void config_endpoint(int port, const char *host, unsigned int host_size, const char *route, unsigned int route_size);

    bool config_endpoint_1(int port, const char *host, unsigned int host_size, const char *route, unsigned int route_size);

    void gmt_offset(long gmt_offset_sec);

    bool command_mode();

    int post(const void * data, unsigned int data_size, char * res, unsigned int res_size);

    bool post_1(const void * data, unsigned int data_size, char * res = 0, unsigned int res_size = 0);

    int get(char * res, unsigned int res_size);

    int wifi_status();

    bool connected();

    void flush_serial();

    const char * ssid() { return _ssid; }
    const char * pass() { return _password; }
    const char * host() { return _host; }
    const char * route() { return _route; }
    int port(){ return _port; }
    int ip(char * ip);

private:
    // void flush_serial();

    int send_no_wait(const char *command, unsigned int size);

    void send_command(const char *command, unsigned int size);

    bool send_data(const char * data, unsigned int size, unsigned int attempts = 1);

    int wait_response(char * response, unsigned int response_size, Microsecond timeout = DEFAULT_TIMEOUT);

    int wait_response_1(Microsecond timeout = DEFAULT_TIMEOUT);

    bool check_response(const char * expected, Microsecond timeout = DEFAULT_TIMEOUT);

    bool check_response_1(Microsecond timeout = DEFAULT_TIMEOUT);

    bool check_timeout(Timeout_Action timeout_action = REBOOT_MACHINE);

    GPIO * _rstkey;
    UART * _uart;
    TSC::Time_Stamp _last_send;
    TSC::Time_Stamp _init_timeout;

    int _auth_method;
    char _ssid[SSID_MAX];
    char _username[SSID_MAX];
    char _password[PASS_MAX];
    char _host[HOST_MAX];
    char _route[ROUTE_MAX];
    char _ip[IP_MAX];
    int _port;

    char _uart_buffer[UART_BUFFER_SIZE];

    bool _connected;
};

__END_SYS

#endif
#endif
