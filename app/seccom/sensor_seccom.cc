#include <gpio.h>
#include <smart_data.h>
#include <utility/ostream.h>

using namespace EPOS;

int main()
{
    // Instantiate your SmartData here
    Switch_Sensor s(0, 'C', 3, GPIO::OUT);
    Smart_Data<Switch_Sensor> my_smart_data(/*...*/);


    Thread::self()->suspend();

    return 0;
}
