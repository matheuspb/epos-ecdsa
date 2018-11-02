#include <utility/ostream.h>
#include <rsa.h>

using namespace EPOS;

const unsigned int ITERATIONS = 50;
const unsigned int SIZE = 4;

int main()
{
	Util_Private_Key sk(2561107663, 2514426353);
	Util_Public_Key pk(2561107663, 65537);
    RSA rsa;

    Value s = rsa.sign(sk, 88);

    coutt << "Entrada para cifragem: " << endl;
    coutt << 88 << endl;

    coutt << "Resultado da cifragem: " << endl;
    coutt << s << endl;

    Value deph = rsa.verify(pk,s);

    coutt << "Resutado da decifragem: " << endl;
    coutt << deph << endl;

    coutt << "Done!" << endl;
    return 0;
}
