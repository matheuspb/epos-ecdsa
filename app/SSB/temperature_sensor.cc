#include <smart_data.h>
#include <watchdog.h>

using namespace EPOS;

OStream cout;

int kick()
{
    Watchdog::enable();
    Watchdog::kick();

    while(Periodic_Thread::wait_next())
        Watchdog::kick();

    return 0;
}

int main()
{
    Periodic_Thread * watchdog = new Periodic_Thread(500 * 1000, kick);

    Temperature temperature(6, 1000000, Temperature::ADVERTISED);
    while (1) {
        Alarm::delay(1000000);
        cout << "Val=" << temperature << endl;
    }

    watchdog->join();

    return 0;
}
