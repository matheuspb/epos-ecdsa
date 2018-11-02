// EPOS MLSS Utility 

#ifndef __mlss_h
#define __mlss_h

#include <sha.h>
#include <utility/ostream.h>

__BEGIN_UTIL

#define n 9
#define d 2

using namespace EPOS;

class MLSS
{

private:
	int cff[n][n];

public:

	MLSS(){
		cff = {
        {1,0,0,1,0,0,1,0,0},
        {0,1,0,0,1,0,0,1,0},
        {0,0,1,0,0,1,0,0,1},
        {1,0,0,0,0,1,0,1,0},
        {0,1,0,1,0,0,0,0,1},
        {0,0,1,0,1,0,1,0,0},
        {1,0,0,0,1,0,0,0,1},
        {0,0,1,1,0,0,0,1,0},
        {0,1,0,0,0,1,1,0,0},
    	};
	};

	~MLSS(){};

	void sign(char *msg){};
	void verify(char *sig, char *msg){};
	void genBlocks(char *msg){};
};

__END_UTIL

#endif /*__mlss_h*/
