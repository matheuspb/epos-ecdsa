#include <machine/cortex/hcsr04.h>

using namespace EPOS;

int main()
{
    GPIO trigger('A', 0, GPIO::OUT);
    GPIO echo('A', 1, GPIO::IN);
    HCSR04 s(&trigger, &echo);

    OStream cout;

    while(true) {

        while(!s.ready_to_get()){}

        int distance = s.get();
        if(distance > 0)
            cout << "Distance = " << distance << "cm" << endl;
        else
            cout << "Error! " << distance << endl;
    }

    return 0;
}
