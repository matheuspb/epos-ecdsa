// EPOS RSA hardware Test Program

#include <utility/ecdsa.h>
#include <utility/hwbignum.h>

using namespace EPOS;

OStream cout;

int main() 
{
    cout << "========== BEGIN ECDSA TESTING ==========" << endl;
    ECPoint<SecP256Info> a, b;

    HWBignum d_a(new uint32_t[8], 8);
    HWBignum d_b(new uint32_t[8], 8);

    d_a.random_same_size();
    d_b.random_same_size();

    a *= d_a;
    b *= d_b;

    cout << "Point addition testing: ";
    bool test = (a + b) == (b + a);
    cout << (test ? "OK" : "FAIL") << endl;

    cout << "Performing hand-shake: ";
    test = (a *= d_b) == (b *= d_a);
    cout << (test ? "OK" : "FAIL") << endl;

    cout << "=========== END ECDSA TESTING ===========" << endl;
    while(1);
}
