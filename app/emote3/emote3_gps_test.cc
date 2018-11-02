#include <machine.h>
#include <gpio.h>
#include <utility/ostream.h>
#include <spi.h>
#include <cpu.h>

#include <gnss.h>

#include <alarm.h>
#include <system/config.h>

using namespace EPOS;

OStream cout;

// Must enable GPS (A2035) in emote3_traits.h

int main()
{
    GPIO g('C', 3, GPIO::OUT);
    A2035 gps;
    while(1) {
        g.set(1);
        Alarm::delay(50000);
        g.clear();
        gps.parse_msg();
        if (gps.is_ready()) {
            A2035::Coordinates llh;
            TSTP::Global_Coordinates xyz;
            llh = gps.get_llh();
            xyz = gps.llh_to_xyz(llh);
            cout << gps << endl;
            cout << xyz << endl;
            
//             INE's front table
//             X :  3746.675   km
//             Y : -4237.599   km
//             Z : -2937.338   km
            TSTP::Global_Coordinates table(374667500, -423759900, -293733800);
            
            cout << "Dist: " << xyz - table << endl;
            
            cout << endl;
        }
        Alarm::delay(50000);
    }
    return 0;
}

