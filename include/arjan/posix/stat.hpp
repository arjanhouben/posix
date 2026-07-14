#pragma once

#include <chrono>
#include <sys/stat.h>
#include <sys/time.h>

#include "arjan/posix/errno.hpp"

#ifdef __linux__
#define ST_TIMESPEC st_ctim
#define ST_ATIMESPEC st_atim
#define ST_MTIMESPEC st_mtim
#else
#define ST_TIMESPEC st_ctimespec
#define ST_ATIMESPEC st_atimespec
#define ST_MTIMESPEC st_mtimespec
#endif

namespace arjan {
namespace posix {

template < typename Target = std::chrono::steady_clock::time_point >
constexpr Target to_timepoint( timespec ts ) noexcept
{
	return Target{
		std::chrono::nanoseconds( ts.tv_nsec ) + std::chrono::seconds( ts.tv_sec )
	};
}

template < typename time_point = std::chrono::steady_clock::time_point >
struct stat_result
{
	constexpr stat_result( const struct stat &stat_, errno_t errno_value_ = 0 ) noexcept :
		id_of_device_containing_the_file( stat_.st_dev ),
		inode_number( stat_.st_ino ),
		type_and_mode( stat_.st_mode ),
		number_of_hard_links( stat_.st_nlink ),
		user_id_of_owner( stat_.st_uid ),
		group_id_of_owner( stat_.st_gid ),
		device_id( stat_.st_rdev ),
		size_in_bytes( stat_.st_size ),
		block_size( stat_.st_blksize ),
		number_of_512B_blocks( stat_.st_blocks ),
		last_status_change( to_timepoint< time_point >( stat_.ST_TIMESPEC ) ),
		last_access( to_timepoint< time_point >( stat_.ST_ATIMESPEC ) ),
		last_modified( to_timepoint< time_point >( stat_.ST_MTIMESPEC ) ),
		errno_value( errno_value_ ) {}

	enum class type
	{
		unknown = 0,
		block_device = S_IFBLK,
		character_device = S_IFCHR,
		directory = S_IFDIR,
		pipe = S_IFIFO,
		symlink = S_IFLNK,
		regular = S_IFREG,
		socket = S_IFSOCK
	};

	constexpr type file_type() const noexcept
	{
		switch ( type_and_mode & S_IFMT )
		{
			case static_cast< std::underlying_type_t< type > >( type::block_device ):
				return type::block_device;
			case static_cast< std::underlying_type_t< type > >( type::character_device ):
				return type::character_device;
			case static_cast< std::underlying_type_t< type > >( type::directory ):
				return type::directory;
			case static_cast< std::underlying_type_t< type > >( type::pipe ):
				return type::pipe;
			case static_cast< std::underlying_type_t< type > >( type::symlink ):
				return type::symlink;
			case static_cast< std::underlying_type_t< type > >( type::regular ):
				return type::regular;
			case static_cast< std::underlying_type_t< type > >( type::socket ):
				return type::socket;
			default:
				return type::unknown;
		}
	}

	constexpr bool exists() const noexcept
	{
		if ( errno_value )
		{
			if ( errno_value == ENOENT )
			{
				return false;
			}
		}
		return true;
	}

	dev_t id_of_device_containing_the_file;
	ino_t inode_number;
	mode_t type_and_mode;
	nlink_t number_of_hard_links;
	uid_t user_id_of_owner;
	gid_t group_id_of_owner;
	dev_t device_id;
	off_t size_in_bytes;
	blksize_t block_size;
	blkcnt_t number_of_512B_blocks;

	time_point last_status_change,
		last_access,
		last_modified;

	errno_t errno_value;
};

template < typename time_point = std::chrono::steady_clock::time_point >
stat_result< time_point > stat( const char *path )
{
	struct stat sb = {};
	const auto check_value_of_errno = lstat( path, &sb ) == -1;
	return stat_result< time_point >{ sb, check_value_of_errno ? errno : 0 };
}

template < typename time_point = std::chrono::steady_clock::time_point >
stat_result< time_point > stat( int fd )
{
	struct stat sb = {};
	const auto check_value_of_errno = fstat( fd, &sb ) == -1;
	return stat_result< time_point >{ sb, check_value_of_errno ? errno : 0 };
}

}
}
