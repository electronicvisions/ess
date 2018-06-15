//_____________________________________________
// Company      :	Tu-Dresden
// Author       :	Stefan Scholze
// Refined		: 	Matthias Ehrlich
// E-Mail   	:	scholze@iee.et.tu-dresden.de
//
// Date         :   Wed Aug 28 09:18:19 2007
// Last Change  :   mehrlich 03/27/2008
// Module Name  :   mutex
// Filename     :   mutex.h
// Project Name	:   p_facets/s_systemsim
// Description	:   manages access from different threads
//
//_____________________________________________

#ifndef __MUTEX_H__
#define __MUTEX_H__

#include <unistd.h>
#include <pthread.h>
#include <sys/time.h>
#ifdef __sun__
#include <poll.h>
#endif
#ifndef __bsdi__
#include <semaphore.h>
#endif

namespace ess
{

/// This class provides functions for atomic access based on pthreads.
class mutex
{

	mutex(const mutex&);
	const mutex& operator=(const mutex&);
protected:
    pthread_mutex_t mtx;
public:
    mutex()
	{
		pthread_mutexattr_t attr;
		pthread_mutexattr_init(&attr);
		pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
		pthread_mutex_init(&mtx, &attr);
		pthread_mutexattr_destroy(&attr);
	}
    ~mutex()			{ pthread_mutex_destroy(&mtx); }
	bool trylock() const{ return pthread_mutex_trylock(const_cast<pthread_mutex_t*>(&mtx)); }
    void enter() const 	{ pthread_mutex_lock(const_cast<pthread_mutex_t*>(&mtx)); }
    void leave() const 	{ pthread_mutex_unlock(const_cast<pthread_mutex_t*>(&mtx)); }
    void lock()	const 	{ this->enter(); }
    void unlock() const { this->leave(); }

};

} // namespace ess


#endif // __MUTEX_H__
