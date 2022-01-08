#pragma once

#include <chrono>
#include <sys/stat.h>
#include <sys/time.h>

namespace arjan {
namespace posix {

template < typename Target = std::chrono::steady_clock::time_point >
Target to_timepoint( timespec ts )
{
	return Target{
		std::chrono::nanoseconds( ts.tv_nsec ) + std::chrono::seconds( ts.tv_sec )
	};
}

template < typename time_point = std::chrono::steady_clock::time_point >
struct stat_result
{
	stat_result( const struct stat &stat_, errno_t errno_value_ = 0 ) :
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
		last_status_change( to_timepoint< time_point >( stat_.st_ctimespec ) ),
		last_access( to_timepoint< time_point >( stat_.st_atimespec ) ),
		last_modified( to_timepoint< time_point >( stat_.st_mtimespec ) ),
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

	type file_type() const
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

	bool exists() const
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

template < typename char_type = char, typename time_point = std::chrono::steady_clock::time_point >
stat_result< time_point > stat( const char_type *path )
{
	struct stat sb = {};
	const auto check_value_of_errno = lstat( path, &sb ) == -1;
	return stat_result< time_point >{ sb, check_value_of_errno ? errno : 0 };
}

}
}