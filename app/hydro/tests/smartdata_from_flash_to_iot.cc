// TSTP Gateway to be used with tools/eposiotgw/eposiotgw

#include <machine.h>
#include <smart_data.h>
#include <utility/ostream.h>

using namespace EPOS;

IF<Traits<USB>::enabled, USB, UART>::Result io;

OStream cout;

typedef Smart_Data_Common::SI_Record DB_Record;
typedef Persistent_Ring_FIFO<DB_Record> Storage;

template<typename T>
void print(const T & d)
{
    bool was_locked = CPU::int_disabled();
    if(!was_locked)
        CPU::int_disable();
    if(EQUAL<T, Smart_Data_Common::DB_Series>::Result)
        io.put('S');
    else
        io.put('R');
    for(unsigned int i = 0; i < sizeof(T); i++)
        io.put(reinterpret_cast<const char *>(&d)[i]);
    for(unsigned int i = 0; i < 3; i++)
        io.put('X');
    if(!was_locked)
        CPU::int_enable();
}

int main()
{
    // Get epoch time from serial
    unsigned long long epoch = 0;
    char c = io.get();
    if(c != 'X') {
        epoch += c - '0';
        c = io.get();
        while(c != 'X') {
            epoch *= 10;
            epoch += c - '0';
            c = io.get();
        }
    }

    Machine::delay(5000000);

    DB_Record e;
    bool popped = false;
    while(true) {
        CPU::int_disable();
        popped = Storage::pop(&e);
        CPU::int_enable();
        if(popped) {
            print(e);
            //TODO:
            //*change the tools/eposiotgw script to return the http response code.
            //*in case of the http response indicates error, the db_record is pushed back to the storage.
        }
        else {
            break;
        }
        Machine::delay(1000000);
    }

    return 0;
}
