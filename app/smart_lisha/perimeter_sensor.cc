#include <transducer.h>
#include <thread.h>
#include <smart_data.h>

using namespace EPOS;


int main()
{
    GPIO trigger('A', 1, GPIO::OUT);
    GPIO echo('A', 3, GPIO::IN);
    Distance_Sensor distance_sensor(0, &trigger, &echo);
    Distance distance(0, 1000000, Distance::ADVERTISED);


    Vibration vibration(7, 1000000, Vibration::ADVERTISED);


    Sound sound(5, 1000000, Sound::ADVERTISED);

    Thread::self()->suspend();

    return 0;
}

