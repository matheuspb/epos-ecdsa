#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

extern "C" {
#include "user_interface.h"
#include "wpa2_enterprise.h"
}

#include <time.h>
#include "FS.h"

enum auth_t { PERSONAL, ENTERPRISE };

class IoTConnection
{
    static const int SSID_MAX_SIZE = 64;
    static const int USERNAME_MAX_SIZE = 64;
    static const int PASS_MAX_SIZE = 64;
    static const int HOST_MAX_SIZE = 128;
    static const int ROUTE_MAX_SIZE = 128;
    static const int MAX_MESSAGE_SIZE = 255;
    static const long int DEFAULT_TIMEOUT = 40 * 1000;
private:
    char ssid[SSID_MAX_SIZE];
    int ssid_size;

    char username[SSID_MAX_SIZE];
    int username_size;
    
    char pass[PASS_MAX_SIZE];
    int pass_size;

    char host[HOST_MAX_SIZE];
    int host_size;

    char route[ROUTE_MAX_SIZE];
    int route_size;

    int port;

    long gmtOffset_sec;
    int daylightOffset_sec;

//    File certificate;
//    File key;

    enum state_t { CONNECTED, DISCONNECTED };
    int con_state;

    int auth_method;

    unsigned long int last_time_recorded;
    unsigned long int last_response_time;
    unsigned long int init_timeout;

    HTTPClient client;
    
private:
    bool can_try_connection(){
        if((auth_method == PERSONAL && (ssid_size == 0 || pass_size == 0)) || (auth_method == ENTERPRISE && (ssid_size == 0 || pass_size == 0 || username_size == 0)))
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

    bool check_timeout() {
        if (millis() > init_timeout) {
            return true;
        }
        return false;
    }

public:
    IoTConnection(){
        ssid_size = 0;
        pass_size = 0;
        host_size = 0;
        route_size = 0;
        port = 0;
        gmtOffset_sec = 0;
        daylightOffset_sec = 0;
        con_state = DISCONNECTED;
        auth_method = PERSONAL;
    }

    bool connect_to_wifi() {
        if(!can_try_connection())
            return false;

        while (disconnect_from_wifi() == false) { delay(10); }
        if(auth_method == PERSONAL) {
            WiFi.begin(ssid, pass);
        }
        else if(auth_method == ENTERPRISE) {
            wifi_station_set_wpa2_enterprise_auth(1);
            wifi_station_set_enterprise_identity((uint8*)username, username_size);
            wifi_station_set_enterprise_username((uint8*)username, username_size);
            wifi_station_set_enterprise_password((uint8*)pass, pass_size);
            wifi_station_set_enterprise_new_password((uint8*)pass, pass_size); 
            WiFi.begin(ssid, 0);
        }

        init_timeout = millis() + DEFAULT_TIMEOUT;
        while(WiFi.status() != WL_CONNECTED) { 
            if(check_timeout())
                break;
            delay(0);
        }
        if(WiFi.status() != WL_CONNECTED)
            return false;
        else
            return true;
    }

    bool disconnect_from_wifi() {

        bool discon = WiFi.disconnect();
        // Clean up to be sure no old data is still inside
        wifi_station_clear_cert_key();
        wifi_station_clear_enterprise_ca_cert();
        wifi_station_clear_enterprise_identity();
        wifi_station_clear_enterprise_username();
        wifi_station_clear_enterprise_password();
        wifi_station_clear_enterprise_new_password();
        
        return discon;
    }

    //ToDo: LISHA's IoT API implements the GET method as a POST.
    // change this method to acommodate that.
    int http_post(unsigned char * message, int message_size) {
        if(!is_all_setup_correctly() || message_size >= MAX_MESSAGE_SIZE)
            return -1;

        String s_host;
        String s_route;

        for(int i = 0; i < host_size; i++) s_host += host[i];
        for(int i = 0; i < route_size; i++) s_route += route[i];

        client.begin(s_host,(uint16_t)port,s_route);

        int httpCode = client.POST((uint8_t *)message,(size_t)message_size);

        client.end();

        return httpCode;
    }

    int http_get(String &payload) {
        if(!is_all_setup_correctly())
            return -1;
            
        String s_host;
        String s_route;

        for(int i = 0; i < host_size; i++) s_host += host[i];
        for(int i = 0; i < route_size; i++) s_route += route[i];
        
        client.begin(s_host,(uint16_t)port,s_route);

        int httpCode = client.GET();

        payload = client.getString();

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

//    void get_ip(){
//        Serial.print(WiFi.localIP());
//    }

    bool set_auth_method(int auth_type) {
        if (auth_type != PERSONAL && auth_type != ENTERPRISE)
            return false;
            
        auth_method = auth_type;
        return true;
    }

    bool set_ssid(char *newssid, int ssidsize) {
        if(ssidsize > SSID_MAX_SIZE || newssid == 0)
            return false;
        ssid_size = ssidsize;
        memcpy(ssid, newssid, ssidsize);
        return true;
    }

    bool set_username(char *newusername, int usernamesize) {
        if(usernamesize > USERNAME_MAX_SIZE || newusername == 0)
            return false;
        username_size = usernamesize;
        memcpy(username, newusername, usernamesize);
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

    int get_auth_method() { return auth_method; }
    char *get_ssid() { return ssid; }
    int get_ssid_size(){ return ssid_size; }
    char *get_username() { return username; }
    int get_username_size() { return username_size; }
    char *get_pass() { return pass; }
    int get_pass_size(){ return pass_size; }
    char *get_host() { return host; }
    int get_host_size(){ return host_size; }
    char *get_route() { return route; }
    int get_route_size(){ return route_size; }
    int get_port(){ return port; }
};

void act();
IoTConnection* iot;

void setup() {
    Serial.begin(115200);
    Serial.setDebugOutput(false);
    iot = new IoTConnection();
    WiFi.mode(WIFI_STA);
    delay(5000);
    Serial.print("+STARTESP\r\n");
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

    if(strncmp(action,"SETAUTHMETHOD",action_size) == 0){
        if(data_size == 0)
            return;

        String temp;

        for(int i = 0; i < data_size; i++)
            temp += data[i];

        bool result = iot->set_auth_method(temp.toInt());

        if(result)
            Serial.print("OK");
        else
            Serial.print("ERR");

    } else if(strncmp(action,"SETSSID",action_size) == 0){
        if(data_size == 0)
            return;

        bool result = iot->set_ssid(data, data_size);

        if(result)
            Serial.print("OK");
        else
            Serial.print("ERR");

    } else if(strncmp(action,"SETPASSWORD",action_size) == 0){
        if(data_size == 0)
            return;

        bool result = iot->set_pass(data, data_size);

        if(result)
            Serial.print("OK");
        else
            Serial.print("ERR");

    } else if(strncmp(action,"SETUSERNAME",action_size) == 0){
        if(data_size == 0)
            return;

        bool result = iot->set_username(data, data_size);

        if(result)
            Serial.print("OK");
        else
            Serial.print("ERR");

    } else if(strncmp(action,"SETHOST",action_size) == 0){
        if(data_size == 0)
            return;

        bool result = iot->set_host(data, data_size);

        if(result)
            Serial.print("OK");
        else
            Serial.print("ERR");

    } else if(strncmp(action,"SETROUTE",action_size) == 0){
        if(data_size == 0)
            return;

        bool result = iot->set_route(data, data_size);

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

        bool result = iot->set_port(temp.toInt());

        if(result)
            Serial.print("OK");
        else
            Serial.print("ERR");

    } else if(strncmp(action,"SETGMTOFFSET",action_size) == 0){
        if(data_size == 0) return;
        long gmt_offset = atol(data);
        bool result = iot->set_gmt_offset(gmt_offset);
        if(result)
            Serial.print("OK");
        else
            Serial.print("ERR");

    } else if(strncmp(action,"GETAUTHMETHOD",action_size) == 0){

        Serial.print("+AUTHMETHOD=");
        Serial.print(iot->get_auth_method());

    } else if(strncmp(action,"GETSSID",action_size) == 0){

        char temp[iot->get_ssid_size()];

        memcpy(temp, iot->get_ssid(), iot->get_ssid_size());
        Serial.print(temp);

    } else if(strncmp(action,"GETPASS",action_size) == 0){

        char temp[iot->get_pass_size() + 1];

        memcpy(temp, iot->get_pass(), iot->get_pass_size());
        temp[iot->get_pass_size()] = '\0';

        Serial.print(temp);

    } else if(strncmp(action,"GETHOST",action_size) == 0){

        char temp[iot->get_host_size()];

        memcpy(temp, iot->get_host(), iot->get_host_size());
        Serial.print(temp);

    } else if(strncmp(action,"GETROUTE",action_size) == 0){

        char temp[iot->get_route_size()];

        memcpy(temp, iot->get_route(), iot->get_route_size());

        Serial.print(temp);

    } else if(strncmp(action,"GETPORT",action_size) == 0){

        Serial.print(iot->get_port());

    } else if(strncmp(action,"RESPONSETIME",action_size) == 0){

        Serial.print("OK");

    } else if(strncmp(action,"CONNECTWIFI",action_size) == 0){

        bool result = iot->connect_to_wifi();

        if(result)
            Serial.print("OK");
        else
            Serial.print("ERR");

    } else if(strncmp(action,"DISCONNECTWIFI",action_size) == 0){

        bool result = iot->disconnect_from_wifi();

        if(result)
            Serial.print("OK");
        else
            Serial.print("ERR");

    } else if(strncmp(action,"GETIP",action_size) == 0){

          Serial.print("+IP=");
          Serial.print(WiFi.localIP());
//        iot->get_ip();

    } else if(strncmp(action,"WIFISTATUS",action_size) == 0){

        Serial.print("+WIFISTATUS=");
        Serial.print(WiFi.status());
    } 
    
    else if(strncmp(action,"GETTIMESTAMP",action_size) == 0){

        if(WiFi.status() != WL_CONNECTED) {
            Serial.print("ERR");
        } else {
            Serial.print("+TIMESTAMP=");
            Serial.print(iot->now());
        }

    } else if(strncmp(action,"GETHEAPSIZE",action_size) == 0){

        Serial.print(ESP.getFreeHeap());

    } else if(strncmp(action,"HTTPPOST",action_size) == 0){
        if(data_size == 0) {
            Serial.print("ERR");
        }
        int res = iot->http_post((unsigned char *) data, data_size);

        Serial.print("+HTTPCODE=");
        Serial.print(res);

    } else if(strncmp(action,"HTTPGET",action_size) == 0){

        String payload;
        int res = iot->http_get(payload);
        Serial.print(payload);

    } else if(strncmp(action,"DEEPSLEEP",action_size) == 0){
        ESP.deepSleep(0); //put the ESP in deep sleep mode until the RST pin goes to LOW
    } else {
        Serial.print("INVALIDCOMMAND");
    }

    Serial.print('\r');
    Serial.print('\n');
}
