#ifndef __SIZED_QUEUE_H__
#define __SIZED_QUEUE_H__

#include <queue>
#include "sim_def.h"

namespace ess
{

/// This class providew a queue with limited size (i.e. a FIFO with limited size)
/// This class has the same interface as class heap_mem.
template<class T> class sized_queue
{
private:
	std::queue<T> _queue;
	size_t _max_size;
public:
	/// constructor with parameter for memory depth
	///\param m: integer with memory depth
	explicit sized_queue (size_t m = DELAY_MEM_DEPTH)
	{
		_max_size=m;
	}

	// function declarations
	bool insert(const T&);
	void get(T&);
	void view_next(T&);
	unsigned int num_available();
	bool empty();
};

/// Function to insert data in heap at correct position.
///\param value: gets element to insert
///\return true, if insertion was succesful false otherwise
template <class T>
bool sized_queue<T>::insert(const T& value)
{
	if (_queue.size() < _max_size) {
		_queue.push(value);
		return true;
	}
	else {
		return false;
	}
}

/// Function to get data of heap root.
/// Resort heap memory by inserting last element at first position.
///\param value: returns first element
template <class T>
void sized_queue<T>::get(T& value)
{
	value = _queue.front();
	_queue.pop();
}

/// Function to view heap root data.
/// No remove from heap, only view the first element
///\param value: shows first memory element
template <class T>
void sized_queue<T>::view_next(T& value)
{
	value = _queue.front();
}

/// Zero while heap memory contains data.
template <class T>
bool sized_queue<T>::empty()
{
	return ( _queue.size() == 0 );
}


/// Returns number of containing elements.
template <class T>
unsigned int sized_queue<T>::num_available()
{
	return _queue.size();
}

} // end namespace ess

#endif // __SIZED_QUEUE_H__
