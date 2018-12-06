// EPOS PUF Key Generation Test Program

#include <utility/bch.h>

using namespace EPOS;

OStream cout;

int main()
{
    cout << "======= BEGIN PUF KEY GEN TESTING =======" << endl;
    PUF::bootstrap();
    cout << "======== END PUF KEY GEN TESTING ========" << endl;
    while(1);
}
