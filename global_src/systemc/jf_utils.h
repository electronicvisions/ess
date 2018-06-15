// Author: Johannes Fieres
// Johannes's utils for speeding up programming

// Andreas Gruebl: Added mas functions on 25.05.09

#ifndef _HANNI_SHORTCUTS_
#define _HANNI_SHORTCUTS_

#include <time.h>

#include "sim_def.h"
#include "types.h"

namespace ess
{

/** Simplifies constructs of the sort
    for(vector<int>::iterator i=v.begin();i!=v.end();++i)
*/
#define foreach(I,V) for(typeof((V).begin()) I=(V).begin();I!=(V).end();++I)

/** simplifies constructs of the sort:
    for(unsigned int i=0;i<myVector.size();++i)
*/
#define forindex(I,V) for(unsigned I=0;I<(V).size();++I)

/** Current (real) relative time in microseconds.*/
long microtime();

//helper function that outputs binary numbers
ostream & binout(ostream &in, uint64 u,uint max);

//helper function that generates a binary mask from an msb bit definition
inline uint64 makemask(uint msb){
		return (1ULL<<(msb+1))-1;
	}
inline uint64 mmm(uint msb){return makemask(msb);}
inline uint64 mmw(uint width){return makemask(width-1);}

}
#endif //_HANNI_SHORTCUTS_
