#include <transducer.h>
#include <tstp.h>
#include <utility/ostream.h>
#include <watchdog.h>

using namespace EPOS;

OStream cout;

class RFID_Transform
{
    static const unsigned int DOOR_OPEN_TIME = 15 * 1000000;
    typedef RFID_Sensor::Data Data;    

public:
    RFID_Transform(GPIO * beep, GPIO * led) : _beep(beep), _led(led), _last_uid(0), _last_time(0), _clean_count(0), _clean_time(0) { }

    void apply(int * o, RFID * _rfid) 
    {
        RFID_Transform::signal(0, _led, list_size(), 1);
        unsigned int temp = *_rfid;
        unsigned int uid = ((Data)temp).uid();

        if(_rfid->expired())
            RFID_Transform::signal(_beep, _led, 10, 1);

        if(uid != 0) {
            if(list_size() == 0){
                // Save master card
                RFID_Transform::signal(_beep, _led, 1, 10);
                push_cache(uid, 0);
            } else {                
                bool denied = true;

                unsigned int i;
                for(i = 0; i < list_size(); i++) {
                    unsigned int cache_uid = 0;
                    read_cache(&cache_uid, i);

                    if(cache_uid == uid) {
                        denied = false;
                        break;
                    }
                }

                if(i == 0){
                    // Is the master card
                    if(TSTP::now() < (_last_time + (3*1000000)) && _last_uid != 0){
                        push_cache(_last_uid, list_size());
                        _last_time = _last_uid = 0;
                    } else {
                        if(TSTP::now() < (_clean_time + (2*1000000))){
                            _clean_count++;
                            if(_clean_count > 10){
                                unsigned int ret = 0; Persistent_Storage::write(0, &ret, sizeof(unsigned int));
                                RFID_Transform::signal(_beep, _led, 1, 30);
                                _clean_count = 0;
                            }
                        }else{
                            _clean_count = 0;
                            //RFID_Transform::signal(_beep, _led, list_size(), 3);
                        }
                        _clean_time = TSTP::now();
                    }

                } else {
                    if (denied) {
                        RFID_Transform::signal(_beep, _led, 5, 1);
                        _last_time = TSTP::now();
                        _last_uid = uid;
                    }
                }
            }
        }
    }

    static void signal(GPIO * beep, GPIO * led, int i = 1, int y = 1)
    {
        for(int count=0; count < i; count++) {
            if(beep) beep->set(false);
            if(led)  led->set(true);
            for(volatile int t=0;t<0x1ffff * y;t++);
            if(beep) beep->set(true);
            if(led)  led->set(false);
            for(volatile int t=0;t<0xffff;t++);
        }
    }

private:
    // Flash handling methods

    static const unsigned int SIZE_ALIGNED = (((sizeof(unsigned int) + sizeof(Persistent_Storage::Word) - 1) / sizeof(Persistent_Storage::Word)) * sizeof(Persistent_Storage::Word));
    static const unsigned int FLASH_LIMIT = Persistent_Storage::SIZE / SIZE_ALIGNED - 1;

    unsigned int list_size() {
        unsigned int ret;
        Persistent_Storage::read(0, &ret, sizeof(unsigned int));
        if(ret > FLASH_LIMIT) {
            ret = 0;
            Persistent_Storage::write(0, &ret, sizeof(unsigned int));
        }
        return ret;
    }

    void read_cache(unsigned int * d, unsigned int flash_block) {
        Persistent_Storage::read(sizeof(Persistent_Storage::Word) + flash_block * SIZE_ALIGNED, d, sizeof(unsigned int));
    }

    unsigned int push_cache(const unsigned int & d, unsigned int flash_block) {
        bool pushed = true;
        unsigned int addr = sizeof(Persistent_Storage::Word) + flash_block * SIZE_ALIGNED;
        if(addr > FLASH_LIMIT) {
            flash_block = Random::random() % FLASH_LIMIT;
            addr = sizeof(Persistent_Storage::Word) + flash_block * SIZE_ALIGNED;
            pushed = false;
            RFID_Transform::signal(_beep, _led, 20, 1);
        }

        Persistent_Storage::write(addr, &d, sizeof(unsigned int));

        if(pushed) {
            unsigned int b = flash_block + 1;
            Persistent_Storage::write(0, &b, sizeof(unsigned int));
            RFID_Transform::signal(_beep, _led, 1, 8);
            RFID_Transform::signal(_beep, _led, 2, 3);
        }

        return flash_block;
    }

private:
    GPIO * _beep;
    GPIO * _led;

    // last pending card
    unsigned int _last_uid;
    TSTP::Microsecond _last_time;
    
    unsigned int _clean_count;
    TSTP::Microsecond _clean_time;
};

int kick()
{
    Watchdog::enable();
    Watchdog::kick();

    while(Periodic_Thread::wait_next())
        Watchdog::kick();    

    return 0;
}
 

int main()
{
    //Periodic_Thread * watchdog = new Periodic_Thread(500 * 1000, kick);

    //Persistent_Ring_FIFO<Smart_Data_Common::DB_Record>::clear();
    //unsigned int ret = 0; Persistent_Storage::write(0, &ret, sizeof(unsigned int));// while(true);
    
    GPIO gpio_beep('A', 1, GPIO::OUT);    
    GPIO gpio_led('C', 3, GPIO::OUT);

    gpio_led.set(false);

    GPIO in0('D', 2, GPIO::IN);
    GPIO in1('D', 4, GPIO::IN);
    
    RFID_Sensor rfid_sensor(1, 0, &in0, &in1);
    RFID rfid(1, 2*1000000, RFID::PRIVATE);
    
    int out_dummy;
    
    RFID_Transform rfid_transform(&gpio_beep, &gpio_led);
    Actuator<int, RFID_Transform, RFID> door_actuator(&out_dummy, &rfid_transform, &rfid);
    
    //watchdog->join();

    Thread::self()->suspend();

    return 0;
}
