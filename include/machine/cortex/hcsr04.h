// EPOS ARM Cortex HC-SR04 Ultrasonic Sensor Mediator Declarations

#include <system/config.h>
#ifndef __cortex_hcsr04_h_
#define __cortex_hcsr04_h_

#include <tsc.h>
#include <gpio.h>

__BEGIN_SYS

class HCSR04
{
    static const unsigned int TRIGGER_PULSE_TIME = 15;
    static const unsigned int TIMEOUT = 24000;
    static const unsigned int COOLDOWN = 100000;

public:
    HCSR04(GPIO * trigger, GPIO * echo) : _trigger(trigger), _echo(echo), _transition(0) {
        _trigger->set(false);
    }

    bool ready_to_get() { return TSC::time_stamp() > _transition; }

    int get() {
        TSC::Time_Stamp trigger_pulse_time, echo_pulse_time, timeout, t0, t1;
        int distance;
        bool problem = false;

        trigger_pulse_time = TSC::time_stamp() + (TRIGGER_PULSE_TIME * (TSC::frequency() / 1000000));
        timeout = trigger_pulse_time + (TIMEOUT * (TSC::frequency() / 1000000));

        _trigger->set(true);
        while((TSC::time_stamp()) < trigger_pulse_time);

        _trigger->set(false);
        while(!_echo->get()) {
            if(TSC::time_stamp() >= timeout) {
                problem = true;
                break;
            }
        }

        if(!problem) {
            t0 = TSC::time_stamp();
            while(_echo->get()) {
                if(TSC::time_stamp() >= timeout) {
                    problem = true;
                    break;
    
                }
            }
            t1 = TSC::time_stamp();
        }
        _transition = TSC::time_stamp() + (COOLDOWN * (TSC::frequency() / 1000000));

        if(problem)
            return -1;
        else {
            echo_pulse_time = (t1 - t0) / (TSC::frequency() / 1000000);
            distance = (echo_pulse_time * 34000) / 2000000; 
            return distance;
        }
    }

private:
    GPIO * _trigger;
    GPIO * _echo;
    TSC::Time_Stamp _transition;
};

__END_SYS

#endif
