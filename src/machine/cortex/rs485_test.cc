#include <utility/ostream.h>
#include <rs485.h>
#include <alarm.h>

using namespace EPOS;

int main()
{
    OStream cout;

    RS485 rs485(1, 19200, 8, 0, 1);
    char buf[16];

    while(true) {

        cout << "Write" << endl;
        
        buf[0] = 0x01;
        buf[1] = 0x11;
        buf[2] = 0xC0;
        buf[3] = 0x2C;
        rs485.write(buf, 4);
        Delay(10000);

        int res = rs485.read(&buf, 16);
        for(int i = 0; i < res; i++)
        	cout << hex << (int)buf[i];
        cout << endl;
        Delay(1000000);
    }

    return 0;
}
