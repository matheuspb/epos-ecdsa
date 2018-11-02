
#include <system/config.h>

#if !defined(__cortex_a2035_h) && defined(__GNSS_H)
#define __cortex_gps_h

#include <machine.h>
#include <gnss.h>
#include <alarm.h>

__BEGIN_SYS


class A2035: public GNSS_Common
{
public:
    A2035() : _spi(0, Traits<CPU>::CLOCK, SPI::FORMAT_MOTO_1, SPI::MASTER, 1000000, 8),
    _gps_on_off('C', 6, GPIO::OUT),
    _gps_pps('C', 5, GPIO::IN),
    _gps_reset('D', 5, GPIO::OUT),
    _gps_wakeup('D', 4, GPIO::OUT)
    {
        _gps_reset.set(1);
        _gps_wakeup.set(1);
        _gps_on_off.clear();
        Alarm::delay(200000); //200ms
        _gps_on_off.set(1);
        Alarm::delay(200000); //200ms
        _gps_on_off.clear();
    }

private:
    friend class GNSS_Common;
    
    char get_byte() {
        _spi.put_data_non_blocking(0);
        while(_spi.is_busy()) ;
        return (char) _spi.get_data_non_blocking();
    }
    
    SPI _spi;
    GPIO _gps_on_off;
    GPIO _gps_pps;
    GPIO _gps_reset;
    GPIO _gps_wakeup;
};

__END_SYS

#endif
