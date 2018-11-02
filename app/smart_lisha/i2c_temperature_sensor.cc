#include <smart_data.h>
#include <alarm.h>

using namespace EPOS;

int main()
{
    Alarm::delay(5000000);
    I2C_Temperature t(0, 5000000, I2C_Temperature::ADVERTISED);

    GPIO g('C', 3, GPIO::OUT);
    g.set(false);
    char led = 0;
    while(1) {
        Alarm::delay(1000000);
        g.set((led++)%2);
    }
    //Thread::self()->suspend();
    return 0;
}