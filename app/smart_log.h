#include <smart_data.h>
#include <alarm.h>
#include <rsa.h>
#include <sha.h>
#include <semaphore.h>
#include <sha-init.h>
#include <sha-int-cfg.h>
#include <thread.h>
#include <utility/array.h>
#include <utility/keypair.h>
#include <utility/ecdsa.h>
#include <utility/hwbignum.h>

__BEGIN_SYS

typedef unsigned char byte_t;
typedef SecP256Info Curve;
typedef ECPoint<Curve> Point;
typedef ECDSA<Curve> DSA;

class Log_Transducer
{
public:
    // === Technicalities ===
    // We won't use these, but every Transducer must declare them
    typedef _UTIL::Observer Observer;
    typedef _UTIL::Observed Observed;

    typedef Dummy_Predictor Predictor;
    struct Predictor_Configuration : public Predictor::Configuration {};

    static void attach(Observer * obs) {}
    static void detach(Observer * obs) {}

public:
    // === Sensor characterization ===
    static const unsigned int UNIT = 0 << 31 | 5 << 16 | 8 << 0;
    static const unsigned int NUM = TSTP::Unit::I64;
    static const int ERROR = 0; // Unknown

    static const bool INTERRUPT = false;
    static const bool POLLING = true;

public:
    // === SmartData interaction ===
    static void sense(
            unsigned int dev,
            Smart_Data<Log_Transducer> * data)
    {
    }

    static void actuate(
            unsigned int dev,
            Smart_Data<Log_Transducer> * data,
            const Smart_Data<Log_Transducer>::Value & command)
    {
        data->_value = command;
    }
};

typedef Smart_Data<Log_Transducer> Smart_Log;

template<typename T, size_t NUM_LOGS>
class Logger: public Smart_Data_Common::Observer
{
public:
    Logger(T * t, Smart_Log* log):
        thread_semaphore(0),
        _data(t),
        log(log),
        logs_size(0),
        logs(),
        thread{new Thread(&send_log, this)},
        sending{false}
    {
        _data->attach(this);
        thread->priority(Thread::IDLE);
    }

    ~Logger() { _data->detach(this); }

    void update(Smart_Data_Common::Observed * obs) {
        if (sending) {
            return;
        }

        if (logs_size == 0) {
            initial_timestamp = _data->time();
        }

        logs[logs_size] = *_data;
        ++logs_size;

        if (logs_size == NUM_LOGS) {
            final_timestamp = _data->time();
            // if (thread)
            //     delete thread;
            sending = true;
            thread_semaphore.v();
        }
    }

private:
    static int send_log(Logger* _logger) {
        Logger& logger = *_logger;

        while (true) {
            logger.thread_semaphore.p();
            
            *logger.log = logger.initial_timestamp;
            *logger.log = logger.final_timestamp;

            SHA sha((uint8_t*)logger.logs, (uint32_t)(NUM_LOGS*sizeof(long long)));
            uint8_t* hash = logger.convertToBigEndian(sha.SHACompute());

            DSA ecdsa;
            ecdsa.gen_key_pair();
            HWBignum bighash = HWBignum((uint32_t*)hash, 32);
            DSA::Signature cryptographed_hash = ecdsa.sign(bighash);
        
            const int bus_size = 64; // I32 = 32, I64 = 64
            const int sends = 8 * 128 / bus_size;

            for (int i = 0; i < sends; i++) {
                long long value = 0;

                const int parts_size = bus_size / 8;
                for (int j = 0; j < parts_size; ++j) {
                    long long part = ((uint8_t *) &cryptographed_hash)[i * parts_size + j];
                    value |= part << (j*8);
                }
                *logger.log = value;
            }

            // TODO: separa em métodos
            logger.logs_size = 0;
            logger.sending = false;
        }
        return 0;
    }

    uint8_t* convertToBigEndian(uint8_t* hash){
        uint8_t* shares = new uint8_t[32];
        uint32_t cont = 0;
        for(int i = 31; i>=0; i--){
            shares[i] = hash[cont];
            cont++;
        }
        return shares;
    
    }

    Semaphore thread_semaphore;
    T * _data;
    Smart_Log * log;
    int logs_size;
    Thread * thread;
    bool sending;
    long long initial_timestamp;
    long long final_timestamp;
    long long logs[NUM_LOGS];
};

__END_SYS
