// EPOS PUF Key Generation Test Program

#include <utility/random.h>
#include <utility/bch.h>

using namespace EPOS;

OStream cout;

int main()
{
  cout << "======= BEGIN PUF KEY GEN TESTING =======" << endl;

  char* SRAM_BEG = reinterpret_cast<char*>(Traits<Machine>::PUF_BASE);
  char* SRAM_END = reinterpret_cast<char*>(Traits<Machine>::PUF_END);

  const struct BCH_Def sample_def {
    (Traits<BCH>::SYNS_SIZE),
      (Traits<BCH>::POLY_DEGREE),
      (Traits<BCH>::ECC_BYTES),
      (Traits<BCH>::GENERATOR)
  };

  uint16_t bch_size = (Traits<BCH>::TEST_CHUNK_SIZE);
  uint16_t input_size = (Traits<BCH>::CHUNK_SIZE);

  uint8_t block[bch_size];

  // Put ECC code on
  for (uint16_t i = input_size, int j = 0; i < bch_size; i++, j++) {
    block[i] = Traits<BCH>::ECC_CODE[j];
  }

  BCH *bch = new BCH();

  cout << "Reading PUF from SRAM..." << endl;

  int index = 0;
  while(SRAM_BEG != SRAM_END) {
    block[index] = *SRAM_BEG++;
    cout << block[index];
    index++;
  }
  cout << endl;
  cout << "Done reading from SRAM!" << endl;

  cout << "Repairing code" << endl;
  bch->repair(&sample_def, block, input_size, block + input_size);

  cout << "Checking if repaired" << endl;
  bool test = bch->verify(&sample_def, block, input_size, block + input_size);
  cout << (test == 0 ? "OK" : "FAIL") << endl;

  cout << "Repaired key:" << endl;
  for (uint16_t i = 0; i < input_size; i++) {
    cout << block[i];
  }

  cout << endl;
  cout << "Original key:" << endl;
  for (uint16_t i = 0; i < input_size; i++) {
    cout << Traits<BCH>::EC_KEY[i];
  }

  cout << endl << "======== END PUF KEY GEN TESTING ========" << endl;
  while(1);
}
