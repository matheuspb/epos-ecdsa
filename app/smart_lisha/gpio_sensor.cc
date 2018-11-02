#include <transducer.h>
#include <thread.h>
#include <tstp.h>
#include <utility/ostream.h>

using namespace EPOS;

int main()
{
    OStream cout;

    cout << "GPIO Sensor" << endl;
    cout << "Here = " << TSTP::here() << endl;

    Switch_Sensor presence_sensor(0, 'B', 4, GPIO::IN, GPIO::UP, GPIO::BOTH);
    Presence presence(0, 1000000, Presence::ADVERTISED);

    Thread::self()->suspend();

    return 0;
}
