//_____________________________________________
// Company      :	Tu-Dresden
// Author       :	Stefan Scholze
// Refinement	: 	Matthias Ehrlich
// E-Mail   	:	scholze@iee.et.tu-dresden.de
//
// Date         :   Wed Aug 28 09:18:19 2007
// Last Change  :   mehrlich 03/27/2008
// Module Name  :   mutexfile
// Filename     :   mutexfile.h
// Project Name	:   p_facets/s_systemsim
// Description	:   structure with locked access to file
//
//_____________________________________________

#ifndef __MUTEXFILE_H__
#define __MUTEXFILE_H__

#include <stdio.h>
#include <stdlib.h>

#include "mutex.h"

namespace ess
{
/// This struct provides atomic file access utilizing mutexes.
struct mutexfile {
	FILE *fp;
	mutex mx;

	private:
		mutexfile(const mutexfile&);
        const mutexfile& operator=(const mutexfile&);

	public:
		mutexfile(const char *name, const char *access)
		: fp(fopen(name,access))
		{}

		mutexfile()
		: fp(NULL)
		{}

		~mutexfile()
		{
			this->close();
		}

		void close()
		{
         	if(fp) fclose(fp);
		}

		bool open(const char *name, const char *access)
		{
			this->close();
            fp=fopen(name,access);
			return fp;
		}

		FILE* aquire_fp()
		{
         	mx.lock();
			return fp;
		}

		void release_fp(FILE *f)
		{
         	if(f == fp)
				mx.unlock();
		}
};

} // namespace ess

#endif // __MUTEXFILE_H__
