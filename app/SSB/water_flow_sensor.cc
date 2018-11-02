#include <transducer.h>
#include <watchdog.h>

using namespace EPOS;

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

    IF<Traits<Hydro_Board>::enabled && Traits<Hydro_Board>::P7_enabled, bool, void>::Result p7_check; // Traits<Hydro_Board>::P7_enabled && Traits<Hydro_Board>::enabled must be true

    Water_Flow water_flow(0, 1000000, Water_Flow::ADVERTISED);

    watchdog->join();

    return 0;
}
