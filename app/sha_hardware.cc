// EPOS SHA hardware Test Program

#include <utility/ostream.h>
#include <sha.h>

using namespace EPOS;

OStream cout;

void print(uint8_t *hash) 
{   
    static const char hexdigits[] = "0123456789ABCDEF";
    int i;
    
    for (i = 0; i < 32; ++i) 
    {
      cout << (hexdigits[(hash[i] >> 4) & 0xF]);
      cout << (hexdigits[hash[i] & 0xF]);
    }
    
    cout << endl;
}

size_t strlen(const char * str)
{
    const char *s;
    for (s = str; *s; ++s) {}
    return(s - str);
}

int main()
{
    char *test = "abc";
    uint8_t *pui8Msg = (uint8_t *)test;

    cout << "Entry: " << test << "\n" << endl;

    SHA sha((uint8_t*)pui8Msg, (uint32_t)strlen((char const *)pui8Msg));
    uint8_t* hash = sha.SHACompute();
    print(hash);

    cout << "\n" << endl;

	return 0;
}