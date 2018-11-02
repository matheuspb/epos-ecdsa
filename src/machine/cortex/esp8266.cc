// EPOS Cortex ESP8266 Wi-Fi Board Mediator Implementation

#include <machine/cortex/esp8266.h>
#ifndef __mmod_zynq__

__BEGIN_SYS

// Class attributes

/* List of commands and responses */
//WiFi connection
static const char cmd_set_auth_method[] = "AT+SETAUTHMETHOD=";
static const char cmd_get_auth_method[] = "AT+GETAUTHMETHOD";
static const char cmd_set_ssid[] = "AT+SETSSID=";
static const char cmd_get_ssid[] = "AT+GETSSID";
static const char cmd_set_username[] = "AT+SETUSERNAME=";
static const char cmd_get_username[] = "AT+GETUSERNAME";
static const char cmd_set_password[] = "AT+SETPASSWORD=";
static const char cmd_get_password[] = "AT+GETPASSWORD";
static const char cmd_connect_wifi[] = "AT+CONNECTWIFI";
static const char cmd_get_ip[] = "AT+GETIP";
static const char cmd_wifi_status[] = "AT+WIFISTATUS";

//HTTP
static const char cmd_set_host[] = "AT+SETHOST=";
static const char cmd_get_host[] = "AT+GETHOST";
static const char cmd_set_route[] = "AT+SETROUTE=";
static const char cmd_get_route[] = "AT+GETROUTE";
static const char cmd_set_port[] = "AT+SETPORT=";
static const char cmd_get_port[] = "AT+GETPORT";
static const char cmd_http_post[] = "AT+HTTPPOST=";
static const char cmd_http_get[] = "AT+HTTPGET=";

//Time
static const char cmd_set_gmt_offset[] = "AT+SETGMTOFFSET=";
static const char cmd_get_gmt_offset[] = "AT+GETGMTOFFSET";
static const char cmd_get_timestamp[] = "AT+GETTIMESTAMP";

//Responses
static const char res_ok[] = "OK";
static const char res_err[] = "ERR";
static const char res_start[] = "+STARTESP";
static const char res_auth_method[] = "+AUTHMETHOD=";
static const char res_ssid[] = "+SSID=";
static const char res_username[] = "+USERNAME=";
static const char res_password[] = "+PASSWORD=";
static const char res_ip[] = "+IP=";
static const char res_wifi_status[] = "+WIFISTATUS=";
static const char res_host[] = "+HOST=";
static const char res_route[] = "+ROUTE=";
static const char res_port[] = "+PORT=";
static const char res_http_code[] = "+HTTPCODE=";
// static const char res_http_get[] = "+HTTPGET=";
static const char res_timestamp[] = "+TIMESTAMP=";
static const char res_gmt_offset[] = "+GMTOFFSET=";

//Command mode
static const char cmd_mode[] = "+++";

//End of msg
static const char end_of_msg[] = "\r\n";

// Methods

bool ESP8266::check_timeout(ESP8266::Timeout_Action timeout_action)
{
    if(TSC::time_stamp() > _init_timeout) {
        switch(timeout_action) {
            case ESP8266::Timeout_Action::REBOOT_MACHINE:
                CPU::int_disable();
                Machine::delay(5000000);
                Machine::reboot();
                break;
            case ESP8266::Timeout_Action::RESET_ESP:
                reset();
                // fallthrough
            case ESP8266::Timeout_Action::RETURN:
            default:
                return true;
        }
        return true;
    }
    else
        return false;
}

RTC::Microsecond ESP8266::now(Microsecond timeout)
{
    char get_timestamp[(sizeof(cmd_get_timestamp)-1) + sizeof(end_of_msg)];
    strncpy(get_timestamp, cmd_get_timestamp, (sizeof(cmd_get_timestamp)-1));
    strncpy(&get_timestamp[sizeof(cmd_get_timestamp)-1], end_of_msg, sizeof(end_of_msg));

    if(command_mode()) {
        send_command(get_timestamp, sizeof(get_timestamp));
        int ret = wait_response_1();
        if(ret > 0) {
            int timestamp_res_pos = strstr(_uart_buffer, res_timestamp);
            if(timestamp_res_pos >= 0) {
                int timestamp_pos = timestamp_res_pos + (sizeof(res_timestamp)-1);
                int timestamp_size = ret - timestamp_pos - (sizeof(end_of_msg)-1) + 1;
                char timestamp[timestamp_size];
                strncpy(timestamp, &_uart_buffer[timestamp_pos], timestamp_size-1);
                timestamp[timestamp_size-1]=0;
                return atoi(timestamp) * 1000000ULL;
            }

        }
    }
    return 0;
}

void ESP8266::reset()
{
    off();
    Machine::delay(500000); // 500ms delay recommended by the manual
    on();
}

bool ESP8266::connect(const char *ssid, int ssid_size, const char *pass, int pass_size, const char *username, int username_size)
{
    /* Verify size constrains */
    assert(ssid_size < SSID_MAX);
    assert(username_size < USERNAME_MAX);
    assert(pass_size < PASS_MAX);

    /* Update class variables */
    memcpy(_ssid, ssid, ssid_size);
    memcpy(_username, username, username_size);
    memcpy(_password, pass, pass_size);
    if(username_size == 0)
        _auth_method = PERSONAL;
    else
        _auth_method = ENTERPRISE;

    /* Prepare commands */
    char auth_method[2];
    itoa(_auth_method, auth_method);
    char set_auth_method[(sizeof(cmd_set_auth_method)-1) + (sizeof(auth_method)-1) + sizeof(end_of_msg)];
    strncpy(set_auth_method, cmd_set_auth_method, sizeof(cmd_set_auth_method)-1);
    strncpy(&set_auth_method[sizeof(cmd_set_auth_method)-1], auth_method, sizeof(auth_method)-1);
    strncpy(&set_auth_method[(sizeof(cmd_set_auth_method)-1) + (sizeof(auth_method)-1)], end_of_msg, sizeof(end_of_msg));

    char set_ssid[(sizeof(cmd_set_ssid)-1) + ssid_size + sizeof(end_of_msg)];
    strncpy(set_ssid, cmd_set_ssid, sizeof(cmd_set_ssid) - 1);
    strncpy(&set_ssid[sizeof(cmd_set_ssid) - 1], ssid, ssid_size);
    strncpy(&set_ssid[(sizeof(cmd_set_ssid)-1) + ssid_size], end_of_msg, sizeof(end_of_msg));

    //WPA-Enterprise only
    char set_username[(sizeof(cmd_set_username)-1) + username_size + sizeof(end_of_msg)];
    if(_auth_method == ENTERPRISE) {
        strncpy(set_username, cmd_set_username, sizeof(cmd_set_username) - 1);
        strncpy(&set_username[sizeof(cmd_set_username) - 1], username, username_size);
        strncpy(&set_username[(sizeof(cmd_set_username) - 1) + username_size], end_of_msg, sizeof(end_of_msg));
    }

    char set_password[(sizeof(cmd_set_password)-1) + pass_size + sizeof(end_of_msg)];
    strncpy(set_password, cmd_set_password, sizeof(cmd_set_password) - 1);
    strncpy(&set_password[sizeof(cmd_set_password) - 1], pass, pass_size);
    strncpy(&set_password[(sizeof(cmd_set_password)-1) + pass_size], end_of_msg, sizeof(end_of_msg));

    char connect_wifi[(sizeof(cmd_connect_wifi)-1) + sizeof(end_of_msg)];
    strncpy(connect_wifi, cmd_connect_wifi, sizeof(cmd_connect_wifi)-1);
    strncpy(&connect_wifi[sizeof(cmd_connect_wifi)-1], end_of_msg, sizeof(end_of_msg));

    if(command_mode()) {
        send_command(set_auth_method, sizeof(set_auth_method));
        if(!check_response_1()) {
            return false;
        }
    }
    else
        return false;

    if(command_mode()) {
        send_command(set_ssid, sizeof(set_ssid));
        if(!check_response_1()) {
            return false;
        }
    }
    else
        return false;

    if(_auth_method == ENTERPRISE) {
        if(command_mode()) {
            send_command(set_username, sizeof(set_username));
            if(!check_response_1()) {
                return false;
            }
        }
        else
            return false;
    }

    if(command_mode()) {
        send_command(set_password, sizeof(set_password));
        if(!check_response_1()) {
            return false;
        }
    }
    else
        return false;

    // The ESP can restart sometimes when trying to connect to an WiFi network (particularlly in ENTERPRISE auth mode).
    // The ESP sends a flag message when it restarts 
    if(command_mode()) {
        send_command(connect_wifi, sizeof(connect_wifi));
        if(!check_response_1()) {
            return false;
        }
    }
    else
        return false;

    return true;
}

bool ESP8266::config_endpoint_1(int port, const char *host, unsigned int host_size, const char *route, unsigned int route_size)
{
    //Verify size constrains
    assert(host_size < HOST_MAX);
    assert(route_size < ROUTE_MAX);

    //Update variables
    _port = port;
    memcpy(_host, host, host_size);
    memcpy(_route, route, route_size);

    //Prepare commands
    char set_host[(sizeof(cmd_set_host)-1) + host_size + sizeof(end_of_msg)];
    strncpy(set_host, cmd_set_host, sizeof(cmd_set_host)-1);
    strncpy(&set_host[sizeof(cmd_set_host)-1], host, host_size);
    strncpy(&set_host[(sizeof(cmd_set_host)-1) + host_size], end_of_msg, sizeof(end_of_msg));

    char set_route[(sizeof(cmd_set_route)-1) + route_size + sizeof(end_of_msg)];
    strncpy(set_route, cmd_set_route, sizeof(cmd_set_route)-1);
    strncpy(&set_route[sizeof(cmd_set_route)-1], route, route_size);
    strncpy(&set_route[(sizeof(cmd_set_route)-1) + route_size], end_of_msg, sizeof(end_of_msg));

    char port_str[PORT_MAX];
    itoa(port, port_str);
    int size_of_port = strlen(port_str);
    char set_port[(sizeof(cmd_set_port)-1) + size_of_port + sizeof(end_of_msg)];
    strncpy(set_port, cmd_set_port, sizeof(cmd_set_port)-1);
    strncpy(&set_port[sizeof(cmd_set_port)-1], port_str, size_of_port);
    strncpy(&set_port[(sizeof(cmd_set_port)-1) + size_of_port], end_of_msg, sizeof(end_of_msg));

    //Send commands
    if(command_mode()) {
        send_command(set_host, sizeof(set_host));
        if(!check_response_1())
            return false;
    }
    else
        return false;

    if(command_mode()) {
        send_command(set_route, sizeof(set_route));
        if(!check_response_1())
            return false;
    }
    else
        return false;

    if(command_mode()) {
        send_command(set_port, sizeof(set_port));
        if(!check_response_1())
            return false;
    }
    else
        return false;

    return true;
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

    command_mode();
    send_data(set_host, sizeof(set_host), 5);

    char set_route[14 + route_size];
    const char temp_route[] = "AT+SETROUTE=";

    strncpy(set_route, temp_route, sizeof(temp_route) - 1);
    strncpy(&set_route[sizeof(temp_route) - 1], route, route_size);
    strncpy(&set_route[sizeof(temp_route) + route_size - 1], end_of_line, sizeof(end_of_line));

    command_mode();
    send_data(set_route, sizeof(set_route), 5);

    const char temp_port[] = "AT+SETPORT=";
    char port_str[33];

    itoa(port, port_str);

    int size_of_port = strlen(port_str);

    char set_port[13 + size_of_port];

    strncpy(set_port, temp_port, sizeof(temp_port) - 1);
    strncpy(&set_port[sizeof(temp_port) - 1], port_str, size_of_port);
    strncpy(&set_port[sizeof(temp_port) + size_of_port - 1], end_of_line, sizeof(end_of_line));

    command_mode();
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

    command_mode();
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

    // char command[(sizeof(cmd_mode)-1) + sizeof(end_of_msg)-1];
    // strncpy(command, cmd_mode, sizeof(cmd_mode)-1);
    // strncpy(&command[(sizeof(cmd_mode)-1)], end_of_msg, sizeof(end_of_msg)-1);

    // send_command(command, sizeof(command));

    // return check_response("OK\r\n");
    return check_response_1();
}

int ESP8266::post(const void * data, unsigned int data_size, char * res, unsigned int res_size)
{
    char request[(sizeof(cmd_http_post)-1) + data_size + sizeof(end_of_msg)]; //-1 for \0 and +2 for \r\n
    strncpy(request, cmd_http_post, sizeof(cmd_http_post)-1);
    memcpy(&request[sizeof(cmd_http_post)-1], data, data_size);
    strncpy(&request[(sizeof(cmd_http_post)-1) + data_size], end_of_msg, sizeof(end_of_msg));

    if(command_mode()) {
        db<ESP8266>(WRN) << request << endl;
        send_command(request, sizeof(request));
        int ret = wait_response_1();

        if(ret >= 0) {
            int http_code_pos = strstr(_uart_buffer, res_http_code);
            if(http_code_pos >= 0) {
                int http_code_size = ret - http_code_pos - (sizeof(res_http_code)-1) - (sizeof(end_of_msg)-1) + 1;
                char http_code_c[http_code_size];
                strncpy(http_code_c, &_uart_buffer[http_code_pos + (sizeof(res_http_code)-1)], http_code_size-1);
                http_code_c[http_code_size-1] = 0;
                return atoi(http_code_c);
            }
        }
    }
    return -1;
}

bool ESP8266::post_1(const void * data, unsigned int data_size, char * res, unsigned int res_size)
{
    char request[(sizeof(cmd_http_post)-1) + data_size + sizeof(end_of_msg)]; //-1 for \0 and +2 for \r\n
    strncpy(request, cmd_http_post, sizeof(cmd_http_post)-1);
    memcpy(&request[sizeof(cmd_http_post)-1], data, data_size);
    strncpy(&request[(sizeof(cmd_http_post)-1) + data_size], end_of_msg, sizeof(end_of_msg));

    if(command_mode()) {
        // db<ESP8266>(WRN) << request << endl;
        send_command(request, sizeof(request));
        int ret = wait_response_1();

        if(ret >= 0) {
            int http_code_pos = strstr(_uart_buffer, res_http_code);
            if(http_code_pos >= 0) {
                int http_code_size = ret - http_code_pos - (sizeof(res_http_code)-1) - (sizeof(end_of_msg)-1) + 1;
                char http_code_c[http_code_size];
                strncpy(http_code_c, &_uart_buffer[http_code_pos + (sizeof(res_http_code)-1)], http_code_size-1);
                http_code_c[http_code_size-1] = 0;
                int http_code = atoi(http_code_c);
                if((http_code >= 200) && (http_code < 300))
                    return true;
            }
        }
    }
    return false;
}

int ESP8266::get(char * res, unsigned int res_size)
{
    char http_get[(sizeof(cmd_http_get)-1) + sizeof(end_of_msg)];
    strncpy(http_get, cmd_http_get, sizeof(cmd_http_get)-1);
    strncpy(&http_get[sizeof(cmd_http_get)-1], end_of_msg, sizeof(end_of_msg));

    command_mode();
    send_command(http_get, sizeof(http_get));

    // FIXME: This is stopping at \r\n. How do we know the size of the response?
    int ret = wait_response_1();

    strncpy(res, _uart_buffer, sizeof(_uart_buffer));

    return ret;
}

int ESP8266::wifi_status()
{
    char wifi_status[(sizeof(cmd_wifi_status)-1) + sizeof(end_of_msg)];
    strncpy(wifi_status, cmd_wifi_status, sizeof(cmd_wifi_status)-1);
    strncpy(&wifi_status[sizeof(cmd_wifi_status)-1], end_of_msg, sizeof(end_of_msg));
       
    if(command_mode()) {
        send_command(wifi_status, sizeof(wifi_status));
        wait_response_1();
        int msg_pos = strstr(_uart_buffer, res_wifi_status);
        if(msg_pos >= 0) {
            return _uart_buffer[msg_pos + (sizeof(res_wifi_status)-1)] - '0';
        }
    }
    return -1;
}

bool ESP8266::connected() {
    int status = wifi_status();
    if(status == ESP8266::WiFi_Status::WL_CONNECTED)
        return true;
    else
        return false;
}

int ESP8266::ip(char * ip)
{
    char get_ip[(sizeof(cmd_get_ip)-1) + sizeof(end_of_msg)];
    strncpy(get_ip, cmd_get_ip, sizeof(cmd_get_ip)-1);
    strncpy(&get_ip[sizeof(cmd_get_ip)-1], end_of_msg, sizeof(end_of_msg));

    if(command_mode()) {
        send_command(get_ip, sizeof(get_ip));

        int ret = wait_response_1();
        if(ret < 0)
            return -1; //Timeout
        int ip_res_pos = strstr(_uart_buffer, res_ip);
        if(ip_res_pos < 0)
            return -2; //Another response but res_ip
        int ip_pos = ip_res_pos + (sizeof(res_ip)-1);
        int ip_size = ret - ip_pos - (sizeof(end_of_msg)-1);
        for(int i = 0; i < ip_size; i++)
            _ip[i] = _uart_buffer[ip_pos + i];
        _ip[ip_size] = 0;
        memcpy(ip, _ip, ip_size+1);
        return ip_size;
    }
    return -1;
}

void ESP8266::flush_serial()
{
    // db<ESP8266>(WRN) << "flushing serial: ";
    while(_uart->ready_to_get()) {
        char c = _uart->get();
        db<ESP8266>(WRN) << c;
        Machine::delay(70);
    }
    // db<ESP8266>(WRN) << endl;
}

void ESP8266::send_command(const char * command, unsigned int size) {
    // db<ESP8266>(WRN) << "Sending command:" << command << endl;
    flush_serial();
    for(unsigned int i = 0; i < size; i++) {
        _uart->put(command[i]);
    }
}

int ESP8266::send_no_wait(const char *command, unsigned int size)
{
    db<ESP8266>(WRN) << "Sending command:" << endl;
    for(unsigned int i=0;i<size;i++)
        db<ESP8266>(WRN) << command[i];
    db<ESP8266>(WRN) << endl;
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

int ESP8266::wait_response_1(Microsecond timeout_time)
{
    _init_timeout = TSC::time_stamp() + timeout_time * (TSC::frequency() / 1000000);

    memset(_uart_buffer, 0, ESP8266::UART_BUFFER_SIZE);

    unsigned int i;
    for(i = 0; i < ESP8266::UART_BUFFER_SIZE; i++) {
        while(!_uart->ready_to_get())
            if(check_timeout())
                return -1;

        _uart_buffer[i] = _uart->get();
        // db<ESP8266>(WRN) << _uart_buffer[i];
        if((_uart_buffer[i] == '\n' && _uart_buffer[i-1] == '\r') || (_uart_buffer[i] == 0))
            return i+1; //ignores flag char 0 of strings
    }
    return i;
}

int ESP8266::wait_response(char * response, unsigned int response_size, Microsecond timeout_time)
{
    _init_timeout = TSC::time_stamp() + timeout_time * (TSC::frequency() / 1000000);

    unsigned int i;
    for(i = 0; i < response_size; i++) {
        while(!_uart->ready_to_get())
            if(check_timeout())
                return -1;

        response[i] = _uart->get();
        // db<ESP8266>(WRN) << response[i];
        if(response[i] == '\n' && response[i-1] == '\r')
            return i+1;
    }
    return i;
}

bool ESP8266::check_response(const char * expected, Microsecond timeout_time)
{
    bool ret = true;
    _init_timeout = TSC::time_stamp() + timeout_time * (TSC::frequency() / 1000000);

    unsigned int sz = strlen(expected);
    for(unsigned int i = 0; i < sz; i++) {
        char c;
        while(!_uart->ready_to_get())
            if(check_timeout())
                return false;

        c = _uart->get();
        db<ESP8266>(WRN) << c;
        if(c != expected[i]) {
            ret = false;
            break;
        }
    }

    return ret;
}

/* Check if the response is OK or ERR */
bool ESP8266::check_response_1(Microsecond timeout_time)
{
    char ok[(sizeof(res_ok)-1) + sizeof(end_of_msg)];
    strncpy(ok, res_ok, sizeof(res_ok)-1);
    strncpy(&ok[sizeof(res_ok)-1], end_of_msg, sizeof(end_of_msg));

    char err[(sizeof(res_err)-1) + sizeof(end_of_msg)];
    strncpy(err, res_err, sizeof(res_err)-1);
    strncpy(&err[sizeof(res_err)-1], end_of_msg, sizeof(end_of_msg));

    bool ret = true;
    unsigned int i = 0;
    unsigned int err_index = 0;
    unsigned int rst_index = 0;
    unsigned int len = strlen(ok);

    _init_timeout = TSC::time_stamp() + timeout_time * (TSC::frequency() / 1000000);

    while(i < len) {

        while(!_uart->ready_to_get())
            if(check_timeout())
                return false;

        char c = _uart->get();
        // db<ESP8266>(WRN) << c;
        if(c == ok[i])
            i++;
        else if(c == ok[0])
            i = 1;
        else
            i = 0;     

        //Check if is error msg
        if(c == err[err_index]) {
            err_index++;
            if(err_index >= strlen(err)) {
                ret = false;
                break;
            }
        }
        else if(c == err[0])
            err_index = 1;
        else
            err_index = 0;

        //Check if is reset msg
        if(c == res_start[rst_index]) {
            rst_index++;
            if(rst_index >= strlen(res_start)) {
                ret = false;
                break;
            }
        }
        else if(c == res_start[0])
            rst_index = 1;
        else
            rst_index = 0;
    }

    return ret;
}

__END_SYS
#endif
