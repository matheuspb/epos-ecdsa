#include <smart_data.h>

using namespace EPOS;

const unsigned int DOOR_FEELINGS_EXPIRY = 10*1000000; // 10s

int main()
{
    OStream cout;
    cout << "EPOSMote III SmartDoor" << endl;
    Machine::delay(5000000);

    Angular_Rate angular_rate(0, DOOR_FEELINGS_EXPIRY, Angular_Rate::ADVERTISED);

    GPIO g('C', 3, GPIO::OUT); // EPOSMote III led pin
    char led = 0;
    while(1) {
        Alarm::delay(1000000);
        g.set((led++)%2);
    }
    return 0;
}