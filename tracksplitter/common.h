#ifndef __COMMON_H__
#define __COMMON_H__

#include <exception>

#define DECL_ERROR(_name_)	class _name_ : public std::exception {};
#define DECL_ERROR_DESC(_name_, _desc_)	\
	class _name_ : public std::exception {	\
		public: \
		virtual const char *what() const throw() {	\
			 return _desc_;	\
		}	\
	};



#endif //__COMMON_H__