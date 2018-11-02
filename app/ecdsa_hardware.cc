// EPOS RSA hardware Test Program

#include <utility/ecdsa.h>
#include <utility/hwbignum.h>

using namespace EPOS;

OStream cout;

int main() 
{
    ECPoint<SecP256Info> a, b;
    cout << a << endl;
    cout << b << endl;

    cout << "Performing hand-shake" << endl;

    HWBignum d_a(new uint32_t[8], 8);
    HWBignum d_b(new uint32_t[8], 8);

    d_a.random_same_size();
    d_b.random_same_size();

    a *= d_a;
    b *= d_b;

    a *= d_b;
    b *= d_a;

    cout << a << endl;
    cout << b << endl;

    cout << "Done" << endl;

    while(1);
}
