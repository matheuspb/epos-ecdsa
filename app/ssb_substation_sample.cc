#include <machine.h>
#include <alarm.h>
#include <smart_data.h>
#include <machine/cortex/esp8266.h>

using namespace EPOS;

typedef Smart_Data_Common::DB_Series DB_Series;
typedef Smart_Data_Common::DB_Record DB_Record;

const unsigned int SAMPLING_INTERVAL = 1000000;

const char DOMAIN[]   = "substation";
const char USERNAME[] = "substation";
const char PASSWORD[] = "1Y6GB8pb";

const char HOST[] = "iot.lisha.ufsc.br";
const char ROUTE_ATTACH[] = "/api/attach.php";
const char ROUTE_PUT[] = "/api/put.php";

const unsigned int SIZE_DOMAIN = sizeof(DOMAIN) - 1;
const unsigned int SIZE_USERNAME = sizeof(USERNAME) - 1;
const unsigned int SIZE_PASSWORD = sizeof(PASSWORD) - 1;
const unsigned int SIZE_CREDENTIALS = 1 + SIZE_DOMAIN + 1 + SIZE_USERNAME + 1 + SIZE_PASSWORD;

void post(ESP8266 * esp, char * buffer, unsigned int buffer_size)
{
    char res[255];
    int res_size = esp->post(buffer, buffer_size, res, 255);
    if (res_size <= 0 || res[0] == 'O' || (res_size > 9 && res[9] == '4')) {
        esp->reset();
        Machine::reboot();
    }
}

template<typename T>
void post_series(T * smart_data, ESP8266 * esp, char * buffer, TSTP::Time t0, int x, int y, int z)
{
    DB_Series db_series = smart_data->db_series();
    db_series.t0 = t0;
    db_series.x = x;
    db_series.y = y;
    db_series.z = z;
    memcpy(&buffer[SIZE_CREDENTIALS], &db_series, sizeof(DB_Series));
    post(esp, buffer, SIZE_CREDENTIALS + sizeof(DB_Series));
}

template<typename T>
void read_post_record(T * smart_data, ESP8266 * esp, char * buffer, int x, int y, int z)
{
    typename T::Value aux;
    aux = *smart_data; // Triggers a transducer reading
    DB_Record db_record = smart_data->db_record();
    db_record.x = x;
    db_record.y = y;
    db_record.z = z;

    memcpy(&buffer[SIZE_CREDENTIALS], &db_record, sizeof(DB_Record));
    post(esp, buffer, SIZE_CREDENTIALS + sizeof(DB_Record));
}

int main()
{
    // Initialize credentials
    char buffer[SIZE_CREDENTIALS + (sizeof(DB_Series) > sizeof(DB_Record) ? sizeof(DB_Record) : sizeof(DB_Series))];
    unsigned int pos = 0;
    buffer[pos++] = SIZE_DOMAIN;
    memcpy(&buffer[pos], DOMAIN, SIZE_DOMAIN);
    pos += SIZE_DOMAIN;
    buffer[pos++] = SIZE_USERNAME;
    memcpy(&buffer[pos], USERNAME, SIZE_USERNAME);
    pos += SIZE_USERNAME;
    buffer[pos++] = SIZE_PASSWORD;
    memcpy(&buffer[pos], PASSWORD, SIZE_PASSWORD);

    // Initialize mediators
    UART uart(0, 115200, 8, 0, 1);
    GPIO rst('C', 0, GPIO::OUT);
    ESP8266 esp(&uart, &rst, 443, HOST, sizeof(HOST) - 1, ROUTE_ATTACH, sizeof(ROUTE_ATTACH) - 1);
    esp.connect("Fotovoltaica-Loggers", 20, "fotovoltaica", 12);

    Delay(2000000);

    SSB_Substation_Sensor s(1, 19200, 8, 0, 1); // We need this once to configure RS485

    // Get epoch time from WiFi network
    TSTP::Time t = esp.now();
    if(t < 0) {
        esp.reset();
        Machine::reboot();
    }
    t -= 28800000000LL; // The network is returning a constantly off timestamp
    TSTP::epoch(t);

    // Instantiate Smart_Data
    SSB_Current current(0, SAMPLING_INTERVAL, SSB_Current::PRIVATE);
    SSB_Voltage_R voltage_r(0, SAMPLING_INTERVAL, SSB_Voltage_R::PRIVATE);
    SSB_Voltage_S voltage_s(0, SAMPLING_INTERVAL, SSB_Voltage_S::PRIVATE);
    SSB_Voltage_T voltage_t(0, SAMPLING_INTERVAL, SSB_Voltage_T::PRIVATE);
    SSB_Active_Power active_power(0, SAMPLING_INTERVAL, SSB_Active_Power::PRIVATE);
    SSB_Average_Power_Factor average_power_factor(0, SAMPLING_INTERVAL, SSB_Average_Power_Factor::PRIVATE);
    
    TSTP::Time t0 = TSTP::absolute(TSTP::now()); 

    // Post series to IoT
    post_series(&current, &esp, buffer, t0, 75287364, 696317740, 30250);
    post_series(&voltage_r, &esp, buffer, t0, 75287365, 696317741, 30251);
    post_series(&voltage_s, &esp, buffer, t0, 75287366, 696317742, 30252);
    post_series(&voltage_t, &esp, buffer, t0, 75287367, 696317743, 30253);
    post_series(&active_power, &esp, buffer, t0, 75287368, 696317744, 30254);
    post_series(&average_power_factor, &esp, buffer, t0, 75287369, 696317745, 30255);

    // Switch to data point route
    esp.config_endpoint(443, HOST, sizeof(HOST) - 1, ROUTE_PUT, sizeof(ROUTE_PUT) - 1);

    // Read data from transducers and post data records to IoT
    while(true) {
        Alarm::delay(SAMPLING_INTERVAL);

        read_post_record(&current, &esp, buffer, 75287364, 696317740, 30250);
        read_post_record(&voltage_r, &esp, buffer, 75287364, 696317741, 30251);
        read_post_record(&voltage_s, &esp, buffer, 75287364, 696317742, 30252);
        read_post_record(&voltage_t, &esp, buffer, 75287364, 696317743, 30253);
        read_post_record(&active_power, &esp, buffer, 75287364, 696317744, 30254);
        read_post_record(&average_power_factor, &esp, buffer, 75287364, 696317745, 30255);
    }

    return 0;
}
