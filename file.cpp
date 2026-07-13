#include "catch2/catch.hpp"
#include "arjan/posix/file.hpp"
#include "arjan/posix/open.hpp"

extern const char* cmake_command;

TEST_CASE( "file" )
{
	WHEN( "using the default constructor" )
	{
		arjan::posix::file file;
		CHECK( file.get() == arjan::posix::file::invalid );
		CHECK_FALSE( file );
	}

	WHEN( "opening an existing file" )
	{
		auto file = arjan::posix::open( cmake_command, arjan::posix::open_mode::read );
		CHECK( file );
		CHECK( file.get() != arjan::posix::file::invalid );
		WHEN( "moving a valid file into a copy" )
		{
			auto copy = std::move( file );
			CHECK_FALSE( file );
			CHECK( copy );
		}
		WHEN( "comparing a valid file to an invalid file" )
		{
			CHECK( file > arjan::posix::file() );
		}
		WHEN( "resetting a valid file" )
		{
			file.reset();
			CHECK_FALSE( file );
		}
		WHEN( "releasing a valid file" )
		{
			auto fp = file.release();
			CHECK_FALSE( file );
			AND_WHEN( "reinitializing a invalid file with a valid filedescriptor" )
			{
				file.reset( fp );
				CHECK( file );
			}
		}
	}
}
