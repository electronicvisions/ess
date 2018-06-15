//_____________________________________________
// Company      :	Tu-Dresden
// Author       :	Stefan Scholze
// E-Mail   	:	scholze@iee.et.tu-dresden.de
//
// Date         :   Wed Jan 17 09:18:19 2007
// Last Change  :   mehrlich 03/27/2008
// Module Name  :   heap_mem
// Filename     :   heap_mem.h
// Project Name	:   p_facets/s_systemsim
// Description	:   sorted memory with heap algorithmus
//
//_____________________________________________

#ifndef __HEAP_MEM_H__
#define __HEAP_MEM_H__

#include "sim_def.h"

namespace ess
{

/// This class providew a sorted memory with heap algorithmus.
template<class T> class heap_mem
{
public:
	// instantiations
	T *memory;					///<pointer to variable type
	//data
	unsigned int start_pointer; ///<lowest array element
	unsigned int end_pointer;	///<current last array element
	unsigned int max;			///<highest memory place

	/// standard constructor with standard depth from sim_def.h element DELAY_MEM_DEPTH
	heap_mem ()
	{
        start_pointer = 1;
		end_pointer = 1;
		max=DELAY_MEM_DEPTH;
		memory = new T[DELAY_MEM_DEPTH+1];
	}

	/// constructor with parameter for memory depth
	///\param m: integer with memory depth
	heap_mem (unsigned int m)
	{
        start_pointer = 1;
		end_pointer = 1;
		max=m;
		memory = new T[m+1];
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
bool heap_mem<T>::insert(const T& value)
{
//	printf("Start insert: %i, %i\n",end_pointer,max);
	unsigned int end_safe;

	if(start_pointer == end_pointer)
	{
    	memory[end_pointer] = value;
		++end_pointer;
	} else if(end_pointer>max)
	{
		return false;
	} else if((((memory[end_pointer>>1] & 0xfffff)-(value& 0xfffff))>>19) & 0x1)
	{
     	memory[end_pointer] = value;
		++end_pointer;
	} else
	{
        memory[end_pointer] = memory[end_pointer>>1];
		end_safe = end_pointer;
		end_pointer = end_pointer>>1;
        insert(value);
        end_pointer = end_safe + 1;
	}
	return true;
}

/// Function to get data of heap root.
/// Resort heap memory by inserting last element at first position.
///\param value: returns first element
template <class T>
void heap_mem<T>::get(T& value)
{
//	printf("Start get: %i, %i\n",end_pointer,max);
	unsigned int i=1;
	T temp;

    value = memory[start_pointer];
    --end_pointer;
    memory[start_pointer] = memory[end_pointer];
	if(start_pointer != end_pointer)
	{
		while (i<=((end_pointer-1)>>1))
		{
			if (((((memory[i<<1] & 0xfffff)-(memory[(i<<1)+1] & 0xfffff))>>19) & 0x1) || (end_pointer-1 == (i<<1)))
			{
				if ((((memory[i<<1] & 0xfffff)-(memory[i] & 0xfffff))>>19) & 0x1)
                {
					temp = memory[i];
//					printf("get1: mem[%i]=%.8X > mem[%i]=%.8X (endp = %i) (temp= %.8x)\n",i,memory[i],i<<1,memory[i<<1],end_pointer,temp);
					memory[i] = memory[i<<1];
					memory[i<<1] = temp;
				}
			} else
			{
				if ((((memory[(i<<1)+1] & 0xfffff)-(memory[i] & 0xfffff))>>19) & 0x1)
				{
                	temp = memory[i];
//					printf("get2: mem[%i]=%.8X > mem[%i]=%.8X (endp = %i) (temp= %.8x)\n",i,memory[i],(i<<1)+1,memory[(i<<1)+1],end_pointer,temp);
					memory[i] = memory[(i<<1)+1];
					memory[(i<<1)+1] = temp;
				}
			}
			++i;
		}
	}
}

/// Function to view heap root data.
/// No remove from heap, only view the first element
///\param value: shows first memory element
template <class T>
void heap_mem<T>::view_next(T& value)
{
	value = memory[start_pointer];
}

/// Zero while heap memory contains data.
template <class T>
bool heap_mem<T>::empty()
{
	return (start_pointer == end_pointer);
}


/// Returns number of containing elements.
template <class T>
unsigned int heap_mem<T>::num_available()
{
	return (end_pointer-1);
}

} // end namespace ess

#endif // __HEAP_MEM_H__
