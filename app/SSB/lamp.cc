#include <transducer.h>
#include <alarm.h>

using namespace EPOS;

int main()
{
    Active_Power c0(0, 1000000, Active_Power::COMMANDED);

    Thread::self()->suspend();

    return 0;
}
