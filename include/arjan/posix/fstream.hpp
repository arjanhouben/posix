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

using ifstream = basic_fstream< std::istream >;
using ofstream = basic_fstream< std::ostream >;
using fstream  = basic_fstream< std::iostream >;

}}
