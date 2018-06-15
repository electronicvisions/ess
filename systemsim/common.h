/**
// Company         :   kip                      
// Author          :   Johannes Schemmel, Andreas Gruebl          
// E-Mail          :   agruebl@kip.uni-heidelberg.de
//                    			
// Filename        :   common.h                
// Project Name    :   p_facets
// Subproject Name :   s_hicann1            
//                    			
// Create Date     :   Tue Jun 24 08
// Last Change     :   Mon Aug 04 08    
// by              :   agruebl        
//------------------------------------------------------------------------


Definitions of common methods for the Stage 2 Control Software / HAL.
Taken from Stage 1 software - this file originally by Johannes Schemmel,
adapted to FACETS Stage 2 software by Andreas Gruebl

*/

#ifndef _COMMON_H_
#define _COMMON_H_

#define NCSC_INCLUDE_TASK_CALLS

//standard library includes

#include <string>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream> 
#include <vector>
#include <map>
#include <list>
#include <queue>
#include <valarray>
#include <bitset>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "pthread.h"

#include "types.h"       // type definitions from System Simulation
#include "sim_def.h"     // constant definitions from System Simulation
#include "hardware_base.h" // constants created from SystemVerilog code with genheader.sv

using namespace std;
using namespace ess;

// debug output class
//dbg(5)<<"blabla"<<endl;

extern uint randomseed;

class Debug {
	public:
	ostringstream empty;
	enum dbglvl {fatal=0,verbose=5,debug=10,all=99999};
	uint setlevel; //global output level
	Debug(uint il=verbose):setlevel(il){};
	void setLevel(uint l){setlevel=l;};
	ostream & operator() (uint ll=fatal){if(l(ll))return cout; else {empty.str(string());return empty;}};//str(string()) clears stringstream by setting empty string, flush() only commits buffer to output
	ostream & o(uint ll=fatal){if(l(ll))return cout; else {empty.str(string());return empty;}};
	bool l(uint l){return (l<=setlevel);}
};



//helper function that outputs binary numbers 
inline ostream & binout(ostream &in, uint64 u,uint max) {
	for(int i=max-1;i>=0;--i)in<<((u&(1ULL<<i))?"1":"0");
	return in;
}

// commented out, because already defined in jf_utils.h
/*
//helper function that generates a binary mask from an msb bit definition
inline uint64 makemask(uint msb){
		return (1ULL<<(msb+1))-1;
	}
inline uint64 mmm(uint msb){return makemask(msb);}
inline uint64 mmw(uint width){return makemask(width-1);}
*/


// simple statistics: standard deviation and mean of vector T
template<class T> T sdev(vector<T> &v){
	T mean=accumulate(v.begin(),v.end(),0.0)/v.size();
	T sdev=0;
	for(uint i=0;i<v.size();++i)sdev+=pow(v[i]-mean,2);
	sdev*=1.0/(v.size()-1.0);
	sdev=sqrt(sdev);
	return sdev;
}

template<class T> T mean(vector<T> &v){
	return accumulate(v.begin(),v.end(),0.0)/v.size();
}

//fir filter, res must have the same size than data and be initialized with zero
template<class T> void fir(vector<T> &res, const vector<T> &data,const vector<T> &coeff){
	T norm=accumulate(coeff.begin(),coeff.end(),0.0)/coeff.size();
	for(uint i=0;i<data.size()-coeff.size()+1;++i){
		T val=0;
		for(uint j=0;j<coeff.size();++j)val+=data[i+j]*coeff[j];
		res[i+coeff.size()/2 +1]=val/norm;
	}
}

// dump the systemc object tree
void print_systemc_object_tree(std::ostream & out);

#endif  // _COMMON_H_
