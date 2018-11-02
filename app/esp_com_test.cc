#include <utility/ostream.h>
#include <utility/string.h>
#include <alarm.h>
#include <uart.h>
#include <machine/cortex/esp8266.h>

using namespace EPOS;
OStream cout;

const unsigned int UNITS = 2;

class Data_Record
{
public:
    Data_Record() {
        for(int i = 0; i < 84; i++)
            a[i] = 0;
    }

    char a[84];
};

int main()
{
    Delay(2000000);

    UART uart(1, 115200, 8, 0, 1);
    GPIO rst('B', 3, GPIO::OUT);

    char host[] = "iot.lisha.ufsc.br";
    char route[] = "/api/put.php";

    ESP8266 esp(&uart, &rst, 443, host, sizeof(host) - 1, route, sizeof(route) - 1);

    esp.connect("LishaJoinville", 14, "12345678", 8);
    cout << "Connected to SSID " << esp.ssid() << " with pass " << esp.pass() << endl;
    cout << "IP is: " << esp.ip() << endl << endl;

    cout << "Timestamp is: " << esp.now() << endl;
    cout << "Host is: " << esp.host() << endl;
    cout << "Route is: " << esp.route() << endl;
    cout << "Port is: " << esp.port() << endl;

    Data_Record aux;
    char res[255];

    cout << endl << "Posting..." << endl;
    int res_size = esp.post(&aux, sizeof(aux), res);

    res[res_size] = '\0';

    cout << "Res size: " << res_size << endl;
    cout << "Response: " << res << endl;

    while(1) {

    }

    return 0;
}
