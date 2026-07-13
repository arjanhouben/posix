#pragma once

#include "arjan/posix/streambuf.hpp"

namespace arjan {
namespace posix {

template < typename stream_base, size_t buffer_size = 64 >
struct basic_fstream : stream_base
{
	using streambuf_type = basic_streambuf< typename stream_base::char_type, buffer_size >;

	basic_fstream( streambuf_type buf ) :
		stream_base( &buffer_ ),
		buffer_( std::move( buf ) ) {}
	
	private:
		streambuf_type buffer_;
};

typedef basic_fstream< std::istream > ifstream;
typedef basic_fstream< std::ostream > ofstream;
typedef basic_fstream< std::iostream > fstream;

}}