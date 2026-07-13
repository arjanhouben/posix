#pragma once

#include <cerrno>
#include <exception>
#include <format>
#include <source_location>
#include <string_view>
#include <system_error>
#include <cstring>
#include <functional>

#ifdef __linux__
using errno_t = int;
#else
// errno_t is a built-in type on macOS / BSD
#endif

namespace arjan {
namespace posix {

inline auto check_errno( std::source_location call_site, auto&& f, auto&&...args )
{
	const auto result = std::invoke( std::forward< decltype( f ) >( f ), std::forward< decltype( args ) >( args )... );
	if ( result == -1 )
	{
		const auto pos = std::string_view( call_site.file_name() ).find_last_of( '/' );
		const auto basename = ( pos == std::string_view::npos )
			? std::string_view( call_site.file_name() )
			: std::string_view( call_site.file_name() + pos + 1 );

		throw std::system_error( 
			errno,
			std::system_category(),
			std::format( "{}:{}: {}", basename, call_site.line(), std::strerror( errno ) )
		);
	}
	return result;
}

}}
