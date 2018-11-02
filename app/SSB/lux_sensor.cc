#include <smart_data.h>

using namespace EPOS;

int main()
{
    Luminous_Intensity lux(6, 1000000, Luminous_Intensity::ADVERTISED);

    Thread::self()->suspend();

    return 0;
}
