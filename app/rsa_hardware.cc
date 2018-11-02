// EPOS RSA hardware Test Program

#include <rsa.h>
#include <sha.h>
#include <sha-init.h>
#include <sha-int-cfg.h>
#include <utility/random.h>
#include <utility/keypair.h>
#include <utility/hwbignum.h>

using namespace EPOS;

OStream cout;

int main() 
{
	cout << "----- RSA Test -----" << endl;

// Key pair of 1024 bits

//	HWBignum pk = HWBignum(65537);
//	uint32_t* _n = new uint32_t[32];
//	_n[31] = 0x1B9ECFDF;
//	_n[30] = 0xDFB5B415;
//	_n[29] = 0x9A6F9784;
//	_n[28] = 0xE479947F;
//	_n[27] = 0xCF1559E9;
//	_n[26] = 0xFA77AF56;
//	_n[25] = 0x53C72E17;
//	_n[24] = 0x5AE784A8;
//	_n[23] = 0xCE1D9A03;
//	_n[22] = 0x991380F7;
//	_n[21] = 0xFCA2478C;
//	_n[20] = 0xB0423E2D;
//	_n[19] = 0xDF9E1B24;
//	_n[18] = 0x0D12030C;
//	_n[17] = 0xE09C7EBD;
//	_n[16] = 0xEB763746;
//	_n[15] = 0x9E2A1399;
//	_n[14] = 0xBDFEC827;
//	_n[13] = 0x5797EAB3;
//	_n[12] = 0x741E0366;
//	_n[11] = 0xE8314F5B;
//	_n[10] = 0xAD4FF69C;
//	_n[9] = 0x724E24EF;
//	_n[8] = 0x532072E9;
//	_n[7] = 0xCD64F593;
//	_n[6] = 0x8A53A693;
//	_n[5] = 0x092F5DE7;
//	_n[4] = 0x9982A455;
//	_n[3] = 0x706A2D4F;
//	_n[2] = 0x0A3F6997;
//	_n[1] = 0x6BCFC2CE;
//	_n[0] = 0x6CEE1231;
//
//	uint32_t* _d = new uint32_t[32];
//
//	_d[31] = 0x124AC42A;
//	_d[30] = 0xE5128678;
//	_d[29] = 0x84360167;
//	_d[28] = 0xFE945277;
//	_d[27] = 0xAEA2DA50;
//	_d[26] = 0x6E1393A8;
//	_d[25] = 0x9D855D0E;
//	_d[24] = 0x14A31DA5;
//	_d[23] = 0x5A141AAA;
//	_d[22] = 0xE5571863;
//	_d[21] = 0xA409A94F;
//	_d[20] = 0x10705E93;
//	_d[19] = 0xACF41DCF;
//	_d[18] = 0x6B649166;
//	_d[17] = 0x96DEADC5;
//	_d[16] = 0x83CA5FE2;
//	_d[15] = 0xCB6F9D9E;
//	_d[14] = 0x698A2470;
//	_d[13] = 0x637B8F78;
//	_d[12] = 0x50E79528;
//	_d[11] = 0x42400710;
//	_d[10] = 0x66080A72;
//	_d[9] = 0x3050E9CA;
//	_d[8] = 0x9100CE4C;
//	_d[7] = 0x7CF74E49;
//	_d[6] = 0x1AEA32B8;
//	_d[5] = 0xD89578C8;
//	_d[4] = 0xAEBDA9D8;
//	_d[3] = 0x07617B3B;
//	_d[2] = 0xFB203824;
//	_d[1] = 0x319F3995;
//	_d[0] = 0x95A23E8D;
//
//	HWBignum nbig = HWBignum(_n, 32*4);
//	HWBignum skbig = HWBignum(_d, 32*4);
//
//	KeyPair kp = KeyPair(nbig, skbig,pk);

	KeyPair kp = KeyPair();
	cout << "Trying to generate RSA key pair of size: " << (KeyPair::_128BITS*32)*2 << "bits.." << endl;
	kp.generateKeyPair(KeyPair::_128BITS);


	RSA rsa;
	Number plaintext = Number(200);
	cout << "Plaintext:" << endl;
	plaintext.printResult(cout);
	Number cifrado = rsa.sign(kp.getPrivateKey(), plaintext);

	cout << "Encrypted:" <<endl;
	cifrado.printResult(cout);

	Number decrypt = rsa.decrypt(kp.getPublicKey(), cifrado);
	cout << "Decrypted:" <<endl;
	decrypt.printResult(cout);
	if(decrypt == plaintext)
		cout << "Plaintext and decrypted are the same." << endl;

    while(1);

}
