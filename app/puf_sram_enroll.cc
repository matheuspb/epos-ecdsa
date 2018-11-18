// EPOS cout Test Program

#include <utility/ostream.h>
#include <utility/random.h>
#include <utility/bch.h>
#include <usb.h>
using namespace EPOS;

OStream cout;

int main()
{
    cout << "PUF SRAM START:" << Traits<Machine>::PUF_BASE << endl;
    cout << "PUF SRAM END:" << Traits<Machine>::PUF_END << endl;

    USB usb;
    while(!USB::ready_to_get());
    USB::get();

    char* SRAM_BEG = reinterpret_cast<char*>(Traits<Machine>::PUF_BASE);
    char* SRAM_END = reinterpret_cast<char*>(Traits<Machine>::PUF_END);

    const struct BCH_Def sample_def {
      (Traits<BCH>::SYNS_SIZE),
      (Traits<BCH>::POLY_DEGREE),
      (Traits<BCH>::ECC_BYTES),
      (Traits<BCH>::GENERATOR)
    };

    uint8_t block[256];
    BCH *bch = new BCH();

    for (uint8_t i = 0; i < 256; i++)
      block[i] = Random::random();

    cout << "Testing BCH..." << endl;
    cout << "Result from yield:" <<
                 bch->verify(sample_def, block, 256, block + 256) <<
                 endl;

    //Change this to read SRAM
    ////////////////////////////////////
    while(SRAM_BEG != SRAM_END) {
        USB::put(*SRAM_BEG++);
    }
    ////////////////////////////////////
    while(true);
    return 0;
}
