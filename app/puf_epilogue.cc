// EPOS PUF Key Generation Test Program

#include <persistent_storage.h>
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

  unsigned int FLASH_END = Traits<Machine>::FLASH_STORAGE_TOP;
  unsigned int ECC_FLASH_BEG = (Persistent_Storage::SIZE - 3) - (sizeof(Persistent_Storage::Word) * 2);

  /* Reading Error Correction Codes from flash */
  for (uint16_t i = input_size; i < bch_size; i++) {
    Persistent_Storage::read(ECC_FLASH_BEG++, &block[i], sizeof(uint8_t));
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

  uint8_t *flash_read;
  unsigned int ECC_KEY_FLASH_BEG = (Persistent_Storage::SIZE - 3) - (sizeof(Persistent_Storage::Word) * 18);
  cout << "Original key:" << endl;
  for (uint16_t i = 0; i < input_size; i++) {
    Persistent_Storage::read(ECC_KEY_FLASH_BEG++, flash_read, sizeof(uint8_t));
    cout << *flash_read;
  }

  cout << endl << "======== END PUF KEY GEN TESTING ========" << endl;
  while(1);
}
