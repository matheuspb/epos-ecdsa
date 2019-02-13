// EPOS Cortex ESP8266 Wi-Fi Board Mediator Implementation

#include <machine/cortex/esp8266.h>
#ifndef __mmod_zynq__

__BEGIN_SYS

// Class attributes

// Methods
RTC::Microsecond ESP8266::now(Microsecond timeout)
{
    command_mode();

    const char cmd[] = "AT+GETTIMESTAMP";
    send_no_wait(cmd, strlen(cmd));

    _uart->put('\r');
    _uart->put('\n');

    char time[11];
    int size = wait_response(time, 10);
    if(strcmp(time,"ERR\r\n") > 0)
    {
        reset();
        Machine::reboot();
    }
    if(size <= 0)
        return -1;
    time[size] = 0;

    return atoi(time) * 1000000ULL;
}

void ESP8266::reset()
{
    off();
    Machine::delay(500000); // 500ms delay recommended by the manual
    on();
}

void ESP8266::connect(const char *ssid, int ssid_size, const char *pass, int pass_size)
{
    assert(ssid_size < SSID_MAX);
    assert(pass_size < PASS_MAX);

    memcpy(_ssid, ssid, ssid_size);
    memcpy(_password, pass, pass_size);

    command_mode();

    char set_ssid[13 + ssid_size];
    const char temp_ssid[] = "AT+SETSSID=";
    const char end_of_line[2] = {'\r', '\n'};

    strncpy(set_ssid, temp_ssid, sizeof(temp_ssid) - 1);
    strncpy(&set_ssid[sizeof(temp_ssid) - 1], ssid, ssid_size);
    strncpy(&set_ssid[sizeof(temp_ssid) + ssid_size - 1], end_of_line, sizeof(end_of_line));

    Machine::delay(100000);
    send_data(set_ssid, sizeof(set_ssid), 1);

    char set_pass[17 + pass_size];
    const char temp_pass[] = "AT+SETPASSWORD=";

    strncpy(set_pass, temp_pass, sizeof(temp_pass) - 1);
    strncpy(&set_pass[sizeof(temp_pass) - 1], pass, pass_size);
    strncpy(&set_pass[sizeof(temp_pass) + pass_size - 1], end_of_line, sizeof(end_of_line));

    Machine::delay(100000);
    send_data(set_pass, sizeof(set_pass), 1);

    Machine::delay(100000);

    const char connect_wifi[] = "AT+CONNECTWIFI\r\n";
    _connected = send_data(connect_wifi, sizeof(connect_wifi) - 1, 1);
    if(!_connected)
    {
        reset();
        Machine::reboot();
    }
}

void ESP8266::config_endpoint(int port, const char *host, unsigned int host_size, const char *route, unsigned int route_size)
{
    assert(host_size < HOST_MAX);
    assert(route_size < ROUTE_MAX);

    _port = port;
    memcpy(_host, host, host_size);
    memcpy(_route, route, route_size);

    char set_host[13 + host_size];
    const char temp_host[] = "AT+SETHOST=";
    const char end_of_line[2] = {'\r', '\n'};

    strncpy(set_host, temp_host, sizeof(temp_host) - 1);
    strncpy(&set_host[sizeof(temp_host) - 1], host, host_size);
    strncpy(&set_host[sizeof(temp_host) + host_size - 1], end_of_line, sizeof(end_of_line));

    Machine::delay(100000);
    send_data(set_host, sizeof(set_host), 5);

    char set_route[14 + route_size];
    const char temp_route[] = "AT+SETROUTE=";

    strncpy(set_route, temp_route, sizeof(temp_route) - 1);
    strncpy(&set_route[sizeof(temp_route) - 1], route, route_size);
    strncpy(&set_route[sizeof(temp_route) + route_size - 1], end_of_line, sizeof(end_of_line));

    Machine::delay(100000);
    send_data(set_route, sizeof(set_route), 5);

    const char temp_port[] = "AT+SETPORT=";
    char port_str[33];

    itoa(port, port_str);

    int size_of_port = strlen(port_str);

    char set_port[13 + size_of_port];

    strncpy(set_port, temp_port, sizeof(temp_port) - 1);
    strncpy(&set_port[sizeof(temp_port) - 1], port_str, size_of_port);
    strncpy(&set_port[sizeof(temp_port) + size_of_port - 1], end_of_line, sizeof(end_of_line));

    Machine::delay(100000);
    send_data(set_port, sizeof(set_port), 5);
}

void ESP8266::gmt_offset(long gmt_offset_sec)
{
    const char temp_gmt[] = "AT+SETGMTOFFSET=";
    const char end_of_line[2] = {'\r', '\n'};
    char gmt_str[33];

    itoa(gmt_offset_sec, gmt_str);

    int size_of_gmt = strlen(gmt_str);

    char set_gmt[18 + size_of_gmt];

    strncpy(set_gmt, temp_gmt, sizeof(temp_gmt) - 1);
    strncpy(&set_gmt[sizeof(temp_gmt) - 1], gmt_str, size_of_gmt);
    strncpy(&set_gmt[sizeof(temp_gmt) + size_of_gmt - 1], end_of_line, sizeof(end_of_line));

    Machine::delay(100000);
    send_data(set_gmt, sizeof(set_gmt), 5);
}

bool ESP8266::command_mode()
{
    flush_serial();
    _uart->put('+');
    _uart->put('+');
    _uart->put('+');
    _uart->put('\r');
    _uart->put('\n');

    return check_response("OK\r\n");
}

int ESP8266::post(const void * data, unsigned int data_size, char * res, unsigned int res_size)
{
    command_mode();

    char command[] = "AT+SENDPOST=";

    char request[sizeof(command) - 1 + 2 + data_size]; //-1 for \0 and +2 for \r\n
    strncpy(request, command, sizeof(command) - 1);

    memcpy(&request[sizeof(command) - 1], data, data_size);

    request[sizeof(command) - 1 + data_size] = '\r';
    request[sizeof(command) + data_size] = '\n';

    db<ESP8266>(WRN) << request << endl;
    send_no_wait(request, sizeof(request));

    // FIXME: This is stopping at \r\n. How do we know the size of the response?
    int ret = wait_response(res, res_size);

    return ret;
}

int ESP8266::get(char * res, unsigned int res_size)
{
    command_mode();

    char command[] = "AT+SENDGET";

    send_no_wait(command, sizeof(command));

    // FIXME: This is stopping at \r\n. How do we know the size of the response?
    int ret = wait_response(res, res_size);

    command_mode();

    return ret;
}

bool ESP8266::connected()
{
    send_no_wait("AT+CONNECTIONSTATUS\r\n", 21);

    _connected = check_response("1\r\n");

    return _connected;
}

const char * ESP8266::ip()
{
    command_mode();

    const char cmd[] = "AT+GETIP";
    send_no_wait(cmd, strlen(cmd));

    _uart->put('\r');
    _uart->put('\n');

    int size = wait_response(_ip, IP_MAX);
    if(size <= 2)
        size = 2;
    _ip[size - 2] = 0; // Discard '\r\n'

    return _ip;
}

void ESP8266::flush_serial()
{
    db<ESP8266>(WRN) << "flushing serial: ";
    while(_uart->ready_to_get()) {
        char c = _uart->get();
        db<ESP8266>(WRN) << c;
        Machine::delay(100);
    }
    db<ESP8266>(WRN) << endl;
}

int ESP8266::send_no_wait(const char *command, unsigned int size)
{
    flush_serial();
    for(unsigned int i = 0; i < size; i++)
        _uart->put(command[i]);

    return 1;
}

bool ESP8266::send_data(const char * data, unsigned int size, unsigned int attempts)
{
    // db<ESP8266>(TRC) << "Sending data, size is " << size << " bytes." << endl;
    db<ESP8266>(WRN) << "Sending data:" << endl;
    for(unsigned int i=0;i<size;i++)
        db<ESP8266>(WRN) << data[i];
    db<ESP8266>(WRN) << endl;
    bool response = false;
    flush_serial();

    while(!response && attempts > 0) {

        for(unsigned int i = 0; i < size; i++)
            _uart->put(data[i]);

        _last_send = TSC::time_stamp();

        response = check_response("OK\r\n");

        attempts--;
        command_mode();
        Machine::delay(200000);
    }
    db<ESP8266>(WRN) << response << "\n\n";
    return response;
}

int ESP8266::timeout()
{
    switch(TIMEOUT_ACTION) {
        case REBOOT_MACHINE:
            Machine::reboot();
            break;
        case RESET_ESP:
            reset();
            // fallthrough
        case RETURN:
        default:
            return -1;
    }
    return -1;
}

int ESP8266::wait_response(char * response, unsigned int response_size, Microsecond timeout_time)
{
    TSC::Time_Stamp tmt = timeout_time * TSC::frequency();
    TSC::Time_Stamp t0 = TSC::time_stamp();

    unsigned int i;
    for(i = 0; i < response_size; i++) {
        while(!_uart->ready_to_get())
            if(TSC::time_stamp() - t0 > tmt)
                return timeout();

        response[i] = _uart->get();
        db<ESP8266>(WRN) << response[i];
        if(response[i] == '\n' && response[i-1] == '\r')
            return i+1;
    }
    return i;
}

bool ESP8266::check_response(const char * expected, Microsecond timeout_time)
{
    bool ret = true;
    TSC::Time_Stamp tmt = timeout_time * TSC::frequency();
    TSC::Time_Stamp t0 = TSC::time_stamp();

    unsigned int sz = strlen(expected);
    for(unsigned int i = 0; i < sz; i++) {
        char c;
        while(!_uart->ready_to_get())
            if(TSC::time_stamp() - t0 > tmt) {
                timeout();
                return false;
            }

        c = _uart->get();
        if(c != expected[i]) {
            ret = false;
            break;
        }
    }

    return ret;
}

__END_SYS
#endif
