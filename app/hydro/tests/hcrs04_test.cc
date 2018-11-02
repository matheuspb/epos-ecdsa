#include <transducer.h>
#include <thread.h>
#include <smart_data.h>
#include <hydro_board.h>

using namespace EPOS;

OStream cout;

int main()
{
    GPIO trigger('A', 5, GPIO::OUT);
    GPIO echo('A', 4, GPIO::IN); //signal from P6 in hydro board
    Distance_Sensor distance_sensor(0, &trigger, &echo, Hydro_Board::get_relay(Hydro_Board::Port::P6));
    Distance distance(0, 1000000, Distance::PRIVATE);
    while(true)
    {
    	Machine::delay(2000000);
    	cout << "Distance: " << distance << endl;
    }

    return 0;
}

