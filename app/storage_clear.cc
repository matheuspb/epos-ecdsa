#include <smart_data.h>
#include <persistent_storage.h>
#include <utility/ostream.h>

using namespace EPOS;

typedef Smart_Data_Common::SI_Record DB_Record;
typedef Persistent_Ring_FIFO<DB_Record> Storage;

int main()
{
    OStream cout;

    Storage::clear(); cout << "storage cleared" << endl; while(true);

    return 0;
}
