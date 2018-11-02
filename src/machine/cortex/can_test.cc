#include <machine.h>
#include <alarm.h>
#include <can.h>
#include <gpio.h>
#include <cpu.h>
#include <utility/ostream.h>

using namespace EPOS;

int main()
{
    OStream cout;

    CAN can;
    can.filter(CAN::LISTEN_ONLY);

    while(true) {
        char data[8];

        Alarm::delay(10);
        while(!can.ready_to_get());
        unsigned int id = can.get_id();
        can.get(data, 8);

        if((id | 0x00FF) == 0x04FF) {
            cout << id << " ";

            for(unsigned int i = 0; i < 8; i++) {
                cout << hex << (unsigned int) data[i];
                cout << " ";
            }
            cout<<"   ";
        }
    }

    return 0;
}
