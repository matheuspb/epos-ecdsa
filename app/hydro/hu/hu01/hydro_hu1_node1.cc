#include <alarm.h>
#include <transducer.h>

using namespace EPOS;

int kick()
{
    OStream cout;
    cout << "Watchdog init" << endl;

    Watchdog::enable();
    Watchdog::kick();

    while(Periodic_Thread::wait_next())
        Watchdog::kick();    

    return 0;
}

int main()
{
    Water_Flow_WSTAR flow(0, 1000000, static_cast<Water_Flow_WSTAR::Mode>(Water_Flow_WSTAR::ADVERTISED | Water_Flow_WSTAR::CUMULATIVE));

    Periodic_Thread * watchdog = new Periodic_Thread(500 * 1000, kick);

    watchdog->join();

    Thread::self()->suspend();

    return 0;
}
