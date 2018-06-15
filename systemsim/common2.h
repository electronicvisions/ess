#ifndef __COMMON_H__
#define __COMMON_H__

/*

 Copyright: TUD/UHEI 2007 - 2012
 License: GPL
 Description: Common Functions

*/

#include <sstream>
#include <string>
#include <cstdio>
#include <ctime>
#include <cassert>
#include <cstring> // required by strlen
#include <vector>
#include <cmath>
#include <cstring> // for strlen
#include <sys/time.h>

//TODO rm logger from this file, and replace by stream to stdout or stderr
#include "logger.h"

#define UNUSED(x) (static_cast<void>(x))

// FIXME: who knows std::for_each?
// convenient looping through containers with std:: iterators
#define foreach(I,V) for(typeof((V).begin()) I=(V).begin();I!=(V).end();++I)

// convenient looping through std:: random access containers with index access
#define forindex(I,V) for(unsigned I=0;I<(V).size();++I)

template <typename T>
T convert(std::string str) {
	T tmp;
	std::stringstream ss(str);
	ss >> tmp;
	return tmp;
}

template <typename T>
std::string convert(T a) {
	std::stringstream ss;
	ss << a;
	return ss.str();
}

template<typename input_T, typename output_T>
output_T convert(input_T s) {
	std::stringstream ss;
	ss << s;
	output_T tmp;
	ss>>tmp;
	return tmp;
}

template<typename T>
void boundary_check(T val, T min, T max) {
	if ( val < min || val > max)
		assert(false);
}

template<typename T>
void boundary_clip(T& val, T min, T max) {
	if (val < min)
		val = min;
	else if (val > max)
		val = max;
}

template<typename output_T>
output_T stochastic_round(output_T input, double res)
{
	UNUSED(res);
	ssize_t value = input;
	double rest  = input - value;

	if((double)rand()/RAND_MAX < rest)
	++value;
	return static_cast<output_T>(value);
}

template<typename T>
inline
int div_ceil(T numerator, T denominator)
{
	int result = static_cast<int>(numerator/denominator);
	if (result*denominator < numerator)
		++result;
	return result;
}

template<>
inline
int div_ceil<int>(int numerator, int denominator)
{
	return (numerator % denominator ) ? (numerator / denominator + 1 ) : ( numerator / denominator );
}

/// returns mean and standard deviation of all values in a vector.
/// should work with any numeric type
template< typename T > std::pair<double, double> mean_and_dev(std::vector<T>& data)
{
	if(data.size()==0)
		return std::make_pair(0,0);
	else if(data.size()==1)
		return std::make_pair(static_cast<double>(data[0]),0);
	std::vector<T>& Data = data;
	double mean = 0;
	double dev = 0;
	typename std::vector< T >::iterator it_D = Data.begin();
	typename std::vector< T >::iterator D_begin = Data.begin();
	typename std::vector< T >::iterator D_end= Data.end();

	for(;it_D<D_end;++it_D)
		mean+=(*it_D);
	mean /= Data.size();
	// transform data: value = value - mean
	for(it_D = D_begin;it_D<D_end;++it_D)
		dev += pow((*it_D) - mean, 2);
	// sum of squares
	dev = sqrt(dev/(Data.size()-1));

	return std::make_pair(mean,dev);
}

// just for printing a string (no format strings!)
inline int printf(std::string str) {
	return printf("%s", str.c_str());
}

inline int gettickcount(void){
	struct timeval tv;
	gettimeofday(&tv,NULL);
	return (int)(tv.tv_sec*1000 + (tv.tv_usec / 1000));
}

inline void newline_to_file(const char* name)
{
	FILE* fp = fopen(name, "wb");
	fputc('\n', fp);
	fclose(fp);
}

inline void append_to_file(const char* name, const char* str)
{
	Logger& log = Logger::instance();
	FILE* fp = fopen(name, "r+b");
	if( !fp ) fp = fopen(name, "w+b");
	if( !fp ) log(Logger::INFO) << "AlgController: Could not open benchmarkfile " << name << " !!!";
	else log(Logger::INFO) << "AlgController: Openend benchmarkfile " << name;
	fseek(fp, -1, SEEK_END);
	fputc(' ', fp);
	fwrite(str, strlen(str),1,fp);
	log(Logger::INFO) << " and appended " << str;
	fputc('\n', fp);
	fclose(fp);
}

#ifdef _WIN32
/// defines function drand48 for Windows.
inline double drand48()
{
	return (double)(rand()*rand())/(double)(RAND_MAX*RAND_MAX);
}

/// defines function srand48 for Windows.
inline void srand48(unsigned int set_seed)
{
	srand(set_seed);
}
#endif //_WIN32

#endif // __COMMON_H__
