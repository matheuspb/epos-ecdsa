// EPOS PUF Key Generation Test Program

#include <persistent_storage.h>
#include <utility/random.h>
#include <utility/bch.h>

using namespace EPOS;

OStream cout;

int main()
{
    cout << "======= BEGIN PUF KEY GEN TESTING =======" << endl;
    uint8_t* sram_bytes = PUF::read_sram();
    cout << (sram_bytes != 0 ? "read ok" : "read failed") << endl;
    for (uint8_t i = 0; i < Traits<BCH>::CHUNK_SIZE; i++) {
        cout << sram_bytes[i];
    }
    cout << endl << "======== END PUF KEY GEN TESTING ========" << endl;
    while(1);
}
