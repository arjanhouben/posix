#pragma once

#include <compare>
#include <source_location>
#include <fcntl.h>
#include <unistd.h>

#include "arjan/posix/errno.hpp"

namespace arjan {
namespace posix {

struct open_mode 
{
	static constexpr auto read = O_RDONLY;
	static constexpr auto write = O_WRONLY;
	static constexpr auto read_write = O_RDWR;
	static constexpr auto create = O_CREAT;
	static constexpr auto truncate = O_TRUNC;
};

inline file open( const char *path, int m, mode_t mode = 0644 )
{
	return file( check_errno( std::source_location::current(), ::open, path, m, mode ) );
}

inline file open( const std::string &path, int m, mode_t mode = 0644 )
{
	return open( path.c_str(), m, mode );
}

}}