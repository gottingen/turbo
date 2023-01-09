
#ifndef TESTING_SSTREAM_WORKAROUND_H_
#define TESTING_SSTREAM_WORKAROUND_H_

// defining private as public makes it fail to compile sstream with gcc5.x like this:
// "error: ‘struct std::__cxx11::basic_stringbuf<_CharT, _Traits, _Alloc>::
// __xfer_bufptrs’ redeclared with different access"

#ifdef private
# undef private
# include <sstream>
# define private public
#else

# include <sstream>

#endif

#endif  //  TESTING_SSTREAM_WORKAROUND_H_
