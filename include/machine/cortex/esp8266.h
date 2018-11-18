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
    static const unsigned int SSID_MAX = 64;
    static const unsigned int PASS_MAX = 64;
    static const unsigned int HOST_MAX = 64;
    static const unsigned int ROUTE_MAX = 64;
    static const unsigned int IP_MAX = 16;
    static const unsigned int DEFAULT_TIMEOUT = 20000000;

    typedef RTC::Microsecond Microsecond;

    enum Timeout_Action {
        RETURN,
        RESET_ESP,
        REBOOT_MACHINE
    };

    static const Timeout_Action TIMEOUT_ACTION = REBOOT_MACHINE;

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

    void connect(const char *ssid, int ssid_size, const char *pass, int pass_size);

    void config_endpoint(int port, const char *host, unsigned int host_size, const char *route, unsigned int route_size);

    void gmt_offset(long gmt_offset_sec);

    bool command_mode();

    int post(const void * data, unsigned int data_size, char * res, unsigned int res_size);

    int get(char * res, unsigned int res_size);

    bool connected();

    const char * ssid() { return _ssid; }
    const char * pass() { return _password; }
    const char * host() { return _host; }
    const char * route() { return _route; }
    int port(){ return _port; }
    const char * ip();

private:
    void flush_serial();

    int send_no_wait(const char *command, unsigned int size);

    bool send_data(const char * data, unsigned int size, unsigned int attempts = 1);

    int wait_response(char * response, unsigned int response_size, Microsecond timeout = DEFAULT_TIMEOUT);

    bool check_response(const char * expected, Microsecond timeout = DEFAULT_TIMEOUT);

    int timeout();

    GPIO * _rstkey;
    UART * _uart;
    TSC::Time_Stamp _last_send;
    TSC::Time_Stamp _init_timeout;

    char _ssid[SSID_MAX];
    char _password[PASS_MAX];
    char _host[HOST_MAX];
    char _route[ROUTE_MAX];
    char _ip[IP_MAX];
    int _port;

    bool _connected;
};

__END_SYS

#endif
#endif
