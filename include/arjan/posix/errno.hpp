#pragma once

#include <cerrno>
#include <exception>

#ifdef __linux__
using errno_t = int;
#else
// errno_t is a built-in type on macOS / BSD
#endif

namespace arjan {
namespace posix {

template < typename F, typename ...Args >
auto check_errno( F &&f, Args &&...args )
{
	const auto result = f( std::forward< Args >( args )... );
	if ( result == -1 )
	{
		throw std::system_error( errno, std::system_category() );
	}
	return result;
}

}}