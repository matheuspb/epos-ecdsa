#include <gpio.h>
#include <machine.h>
#include <smart_data.h>
#include <transducer.h>
#include <tstp.h>


using namespace EPOS;
OStream cout;

int main()
{
    //Transducer
	new Lux_Sensor(new ADC(ADC::SINGLE_ENDED_ADC4));

    //SmartData
    new Lux(0, 1000000, Lux::ADVERTISED);
    new I2C_Temperature(0, 1000000, I2C_Temperature::ADVERTISED);
    
    GPIO g('C',3, GPIO::OUT);
    char led = 0;
    while(1) {
        Alarm::delay(1000000);
        g.set((led++)%2);
    }
    // Thread::self()->suspend();

    return 0;
}
