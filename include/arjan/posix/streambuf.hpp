#pragma once

#include <streambuf>
#include <fcntl.h>
#include <unistd.h>
#include "arjan/posix/file.hpp"
#include "arjan/posix/errno.hpp"

namespace arjan {
namespace posix {

template < typename char_type = char, size_t buffer_size = 64 >
struct basic_streambuf : std::basic_streambuf< char_type >
{
	using base = std::basic_streambuf< char_type >;
	using int_type = typename base::int_type;
	using traits_type = typename base::traits_type;
	
	explicit basic_streambuf( file f )  :
		base(),
		file_( std::move( f ) )
	{
		base::setg(
			buffer_.data(),
			buffer_.data() + buffer_.size(),
			buffer_.data() + buffer_.size()
		);

		const auto flags = check_errno( std::source_location::current(), fcntl, file_.get(), F_GETFL, 0 );
		check_errno( std::source_location::current(), fcntl, file_.get(), F_SETFL, flags | O_NONBLOCK );
	}

	auto reset( posix::file &&fileno = file() ) noexcept
	{
		std::swap( file_, fileno );
		return std::move( fileno );
	}

	int_type underflow() override
	{
		const auto begin = buffer_.data();
		const auto read = xsgetn( begin, static_cast< std::streamsize >( buffer_.size() ) );
		if ( read > 0 )
		{
			base::setg( begin, begin, begin + read );
		}
		if ( base::gptr() == base::egptr() )
		{
			return traits_type::eof();
		}
		return traits_type::to_int_type( *base::gptr() );
	}

	std::streamsize xsgetn( char_type* dst, std::streamsize count ) override
	{
		const auto n = ::read( file_.get(), dst, static_cast< size_t >( count ) );
		if ( n == -1 )
		{
			return 0;
		}
		return n;
	}

	std::streamsize xsputn( const char_type* src, std::streamsize count ) override
	{
		const auto n = ::write( file_.get(), src, static_cast< size_t >( count ) );
		if ( n == -1 )
		{
			return 0;
		}
		return n;
	}

	int_type overflow( int_type inp = traits_type::eof() ) override
	{
		auto c = traits_type::to_char_type( inp );
		if ( xsputn( &c, 1 ) )
		{
			return traits_type::to_int_type( c );
		}
		return traits_type::eof();
	}
	
	private:
	
		file file_;
		std::array< char_type, buffer_size > buffer_;
};

using streambuf = basic_streambuf< char >;

}}