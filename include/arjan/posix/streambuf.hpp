#pragma once

#include <chrono>
#include <streambuf>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
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
		const auto n = xsgetn( begin, static_cast< std::streamsize >( buffer_.size() ) );
		if ( n > 0 )
		{
			base::setg( begin, begin, begin + n );
		}
		else if ( n < 0 )
		{
			// EOF: clear the buffer so subsequent calls also return eof()
			base::setg( begin, begin, begin );
		}
		if ( base::gptr() == base::egptr() )
		{
			return traits_type::eof();
		}
		return traits_type::to_int_type( *base::gptr() );
	}

	std::streamsize xsgetn( char_type* dst, std::streamsize count ) override
	{
		const ssize_t n = ::read( file_.get(), dst, static_cast< size_t >( count ) );
		if ( n <= 0 )
		{
			return n == 0 ? -1 : 0;
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

	int sync() noexcept override
	{
		return ::fsync( file_.get() );
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

	enum class poll_result
	{
		timeout,
		data_available
	};

	poll_result poll( std::chrono::milliseconds wait_time )
	{
		while ( true )
		{
			struct ::pollfd pfd { file_.get(), POLLIN, 0 };
			const int ret = ::poll( &pfd, 1, static_cast<int>(wait_time.count()) );
			if ( ret == 0 )
			{
				return poll_result::timeout;
			}
			else if ( ret < 0 )
			{
				if ( errno == EINTR ) continue;
				throw std::system_error(
					errno,
					std::system_category(),
					std::strerror( errno )
				);
			}
			return poll_result::data_available;
		}
	}
	
	private:
	
		file file_;
		std::array< char_type, buffer_size > buffer_;
};

using streambuf = basic_streambuf< char >;

}}