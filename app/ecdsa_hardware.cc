// EPOS ECDSA hardware Test Program

#include <ecdsa.h>
#include <utility/hwbignum.h>

using namespace EPOS;

OStream cout;
typedef SecP256Info Curve;
typedef ECPoint<Curve> Point;
typedef ECDSA<Curve> DSA;

int main()
{
    cout << "========== BEGIN ECDSA TESTING ==========" << endl;
    Point a, b;

    HWBignum d_a(new uint32_t[Curve::size], Curve::size*4);
    HWBignum d_b(new uint32_t[Curve::size], Curve::size*4);

    d_a.random_same_size();
    d_b.random_same_size();

    a *= d_a;
    b *= d_b;

    cout << "Sanity test: " << ((a == b) ? "FAIL" : "OK") << endl;

    cout << "Point addition testing: ";
    bool test = (a + b) == (b + a);
    cout << (test ? "OK" : "FAIL") << endl;

    cout << "Performing hand-shake: ";
    test = (a *= d_b) == (b *= d_a);
    cout << (test ? "OK" : "FAIL") << endl;

    cout << "Copy constructor testing: ";
    Point c(a);
    test = a == c;
    cout << (test ? "OK" : "FAIL") << endl;

    cout << "ECDSA testing: ";
    DSA dsa;
    dsa.gen_key_pair();
    DSA::Signature s = dsa.sign(d_a);
    test = dsa.verify(d_a, s);
    cout << (test ? "OK" : "FAIL") << endl;

    cout << "=========== END ECDSA TESTING ===========" << endl;
    while(1);
}
