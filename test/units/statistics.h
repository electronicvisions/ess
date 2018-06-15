#ifndef __statistics_h__
#define __statistics_h__

/*

 Copyright: TUD/UHEI 2007 - 2011
 License: GPL
 Description: routing statistic functions

*/

#include <vector>
#include <numeric>
#include <cmath>

/** returns mean and standard deviation of all values in a vector.
 * should work with any numeric type
 */
template< typename T >
std::pair<double, double> mean_and_dev(std::vector<T>& data)
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

#endif //__statistics_h__
