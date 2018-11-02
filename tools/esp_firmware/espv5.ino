#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <time.h>
#include "FS.h"

class IoTConnection
{
    static const int SSID_MAX_SIZE = 32;
    static const int PASS_MAX_SIZE = 32;
    static const int HOST_MAX_SIZE = 128;
    static const int ROUTE_MAX_SIZE = 128;
    static const int MAX_MESSAGE_SIZE = 255;
private:
    char ssid[SSID_MAX_SIZE];
    int ssid_size;

    char pass[PASS_MAX_SIZE];
    int pass_size;

    char host[HOST_MAX_SIZE];
    int host_size;

    char route[ROUTE_MAX_SIZE];
    int route_size;

    int port;

    long gmtOffset_sec;
    int daylightOffset_sec;

    File certificate;
    File key;

    enum state_t { CONNECTED, DISCONNECTED };
    int con_state;

    unsigned long int last_time_recorded;
    unsigned long int last_response_time;

    HTTPClient client;

private:
    bool can_try_connection(){
        if(ssid_size == 0 || pass_size == 0)
            return false;
        return true;
    }

    bool is_all_setup_correctly(){
        if(host_size == 0 || route_size == 0 || port == 0)
            return false;
        return true;
    }

    void config_time(){
        configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org", "time.nist.gov");
    }

public:
    IoTConnection(){
        ssid_size = 0;
        pass_size = 0;
        host_size = 0;
        route_size = 0;
        port = 0;
        gmtOffset_sec = 8 * 3600;
        daylightOffset_sec = 0;
        con_state = DISCONNECTED;
    }

    bool connect_to_wifi(){
        if(!can_try_connection())
            return false;

        WiFi.disconnect();

        WiFi.begin(ssid, pass);
        if (WiFi.waitForConnectResult() != WL_CONNECTED) {
            con_state = DISCONNECTED;
        } else {
            con_state = CONNECTED;
        }
        return (con_state == CONNECTED);
    }

    bool disconnect_from_wifi(){
        WiFi.disconnect();
        con_state = DISCONNECTED;
    }

    int send_data(String method_t, unsigned char * message, int message_size) {
        if(!is_all_setup_correctly() || message_size >= MAX_MESSAGE_SIZE)
            return 0;

        String s_host;
        String s_route;

        for(int i = 0; i < host_size; i++) s_host += host[i];
        for(int i = 0; i < route_size; i++) s_route += route[i];

        client.begin(s_host,(uint16_t)port,s_route);

        int httpCode = client.POST((uint8_t *)message,(size_t)message_size);

        client.end();

        return httpCode;
    }

    time_t now() {
        config_time();
        time_t now = time(nullptr);
        while (now < 1514764800) { // Just to avoid wrong time value
            delay(2);
            now = time(nullptr);
        }
        return now;
    }

    bool get_status(){
        if(WiFi.status() == WL_CONNECTED)
            con_state = CONNECTED;
        else
            con_state = DISCONNECTED;
        return (con_state == CONNECTED);
    }

    void get_ip(){
        Serial.println(WiFi.localIP());
    }

    bool set_ssid(char *newssid, int ssidsize) {
        if(ssidsize > SSID_MAX_SIZE || newssid == 0)
            return false;
        ssid_size = ssidsize;
        memcpy(ssid, newssid, ssidsize);
        return true;
    }

    bool set_pass(char *newpass, int passsize) {
        if(passsize > PASS_MAX_SIZE || newpass == 0)
            return false;
        pass_size = passsize;
        memcpy(pass, newpass, passsize);
        return true;
    }

    bool set_host(char *newhost, int hostsize) {
        if(hostsize > HOST_MAX_SIZE || newhost == 0)
            return false;
        host_size = hostsize;
        memcpy(host, newhost, hostsize);
        return true;
    }

    bool set_route(char *newroute, int routesize) {
        if(routesize > ROUTE_MAX_SIZE || newroute == 0)
            return false;
        route_size = routesize;
        memcpy(route, newroute, routesize);
        return true;
    }

    bool set_port(int newport) {
        port = newport;
        return true;
    }

    bool set_gmt_offset(long new_gmt_offset) {
        gmtOffset_sec = new_gmt_offset;
        return true;
    }

    char *get_ssid() { return ssid; }
    int get_ssid_size(){ return ssid_size; }
    char *get_pass() { return pass; }
    int get_pass_size(){ return pass_size; }
    char *get_host() { return host; }
    int get_host_size(){ return host_size; }
    char *get_route() { return route; }
    int get_route_size(){ return route_size; }
    int get_port(){ return port; }
};

void act();
IoTConnection iot;

void setup() {
    Serial.begin(115200);
}

char command[32];
int command_pos = 0;

char data[4096];
int data_pos = 0;

bool is_command = true; //alternates between command or data

void command_mode() {
    is_command = true;
    data_pos = 0;
    command_pos = 0;
    Serial.read(); //remove \r\n
    Serial.read(); //remove \r\n
    Serial.print("OK");
    Serial.print('\r');
    Serial.print('\n');
}

void loop() {
    if(Serial.available() != 0){ //if received a command or data
        while(Serial.available() != 0){ //read until it finds an end
            if(is_command) {
                command[command_pos] = Serial.read();
                if(command[command_pos] == '=') {
                    is_command = false;
                    continue;
                }
                if(command[command_pos] == '\n' && command[command_pos - 1] == '\r') {
                    act();
                    command_pos = 0;
                    break;
                }
                if(command[command_pos] == '+' && command[command_pos-1] == '+' && command[command_pos-2] == '+') {
                    command_mode();
                    continue;
                }
                command_pos++;
            } else {
                data[data_pos] = Serial.read();
                if(data[data_pos] == '\n' && data[data_pos - 1] == '\r') {
                    act();
                    break;
                }
                if(data[data_pos] == '+' && data[data_pos-1] == '+' && data[data_pos-2] == '+') {
                    command_mode();
                    continue;
                }
                data_pos++;
            }
        }
    }
}

void act(){
    if(command[0] != 'A' || command[1] != 'T' || command[2] != '+')
        return;

    int action_size = command_pos - 4;
    int data_size = data_pos - 1;

    if(action_size < 1)
        return;

    char action[action_size];

    for(int i = 3; i < command_pos; i++){
        action[i - 3] = command[i];
    }

    if(strncmp(action,"SETSSID",action_size) == 0){
        if(data_size == 0)
            return;

        bool result = iot.set_ssid(data, data_size);

        if(result)
            Serial.print("OK");
        else
            Serial.print("ERR");
    } else if(strncmp(action,"SETPASSWORD",action_size) == 0){
        if(data_size == 0)
            return;

        bool result = iot.set_pass(data, data_size);

        if(result)
            Serial.print("OK");
        else
            Serial.print("ERR");

    } else if(strncmp(action,"SETHOST",action_size) == 0){
        if(data_size == 0)
            return;

        bool result = iot.set_host(data, data_size);

        if(result)
            Serial.print("OK");
        else
            Serial.print("ERR");

    } else if(strncmp(action,"SETROUTE",action_size) == 0){
        if(data_size == 0)
            return;

        bool result = iot.set_route(data, data_size);

        if(result)
            Serial.print("OK");
        else
            Serial.print("ERR");

    } else if(strncmp(action,"SETPORT",action_size) == 0){
        if(data_size == 0)
            return;

        String temp;

        for(int i = 0; i < data_size; i++)
            temp += data[i];

        bool result = iot.set_port(temp.toInt());

        if(result)
            Serial.print("OK");
        else
            Serial.print("ERR");

    } else if(strncmp(action,"SETGMTOFFSET",action_size) == 0){
        if(data_size == 0) return;
        long gmt_offset = atol(data);
        bool result = iot.set_gmt_offset(gmt_offset);
        if(result)
            Serial.print("OK");
        else
            Serial.print("ERR");

    } else if(strncmp(action,"GETSSID",action_size) == 0){

        char temp[iot.get_ssid_size()];

        memcpy(temp, iot.get_ssid(), iot.get_ssid_size());
        Serial.print(temp);

    } else if(strncmp(action,"GETPASS",action_size) == 0){

        char temp[iot.get_pass_size() + 1];

        memcpy(temp, iot.get_pass(), iot.get_pass_size());
        temp[iot.get_pass_size()] = '\0';

        Serial.print(temp);

    } else if(strncmp(action,"GETHOST",action_size) == 0){

        char temp[iot.get_host_size()];

        memcpy(temp, iot.get_host(), iot.get_host_size());
        Serial.print(temp);

    } else if(strncmp(action,"GETROUTE",action_size) == 0){

        char temp[iot.get_route_size()];

        memcpy(temp, iot.get_route(), iot.get_route_size());

        Serial.print(temp);

    } else if(strncmp(action,"GETPORT",action_size) == 0){

        Serial.print(iot.get_port());

    } else if(strncmp(action,"RESPONSETIME",action_size) == 0){

        Serial.println("OK");

    } else if(strncmp(action,"CONNECTWIFI",action_size) == 0){
        bool result = iot.connect_to_wifi();

        if(result)
            Serial.print("OK");
        else
            Serial.print("ERR");

    } else if(strncmp(action,"DISCONNECTWIFI",action_size) == 0){

        bool result = iot.disconnect_from_wifi();

        if(result)
            Serial.print("OK");
        else
            Serial.print("ERR");

    } else if(strncmp(action,"GETIP",action_size) == 0){

        iot.get_ip();

    } else if(strncmp(action,"CONNECTIONSTATUS",action_size) == 0){

        bool is_connected = iot.get_status();
        Serial.print(is_connected);

    } else if(strncmp(action,"GETTIMESTAMP",action_size) == 0){

        if(!iot.get_status()) {
            Serial.print("ERR");
        } else {
            Serial.print(iot.now());
        }

    } else if(strncmp(action,"GETHEAPSIZE",action_size) == 0){

        Serial.print(ESP.getFreeHeap());

    } else if(strncmp(action,"SENDPOST",action_size) == 0){
        if(data_size == 0) {
            Serial.print("ERR");
        }
        int res = iot.send_data(String("POST"), (unsigned char *) data, data_size);

        Serial.print(res);
        Serial.print('\r');
        Serial.print('\n');
    } else if(strncmp(action,"SENDGET",action_size) == 0){

    } else {
        Serial.print("INVALIDCOMMAND");
    }

    Serial.print('\r');
    Serial.print('\n');
}
