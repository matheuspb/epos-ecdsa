#include <thread.h>
#include <smart_data.h>

using namespace EPOS;

int main()
{
    // Instantiate your SmartData here
    Switch_Sensor s(0, 'C', 3, GPIO::OUT);
    Smart_Data<Switch_Sensor> my_led(0, 1000000, Smart_Data<Switch_Sensor>::COMMANDED);

    Thread::self()->suspend();

    return 0;
}
