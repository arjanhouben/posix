#include "catch2/catch.hpp"

#include "arjan/posix/fstream.hpp"
#include "arjan/posix/stat.hpp"
#include "arjan/posix/open.hpp"

extern const std::string cmake_command;

using open_mode = arjan::posix::open_mode;

constexpr auto test_path = "/tmp/stream_test.txt";
constexpr auto test_line = "this is a test";

TEST_CASE( "streams" )
{
	WHEN( "using a streambuf with default constructors" )
	{
		CHECK_THROWS(
			arjan::posix::ofstream(
				arjan::posix::streambuf(
					arjan::posix::file{}
				)
			)
		);
	}
	WHEN( "creating a file and write a string to it" )
	{
		std::remove( test_path );
		{
			arjan::posix::ofstream(
				arjan::posix::streambuf(
					arjan::posix::open( test_path, open_mode::write | open_mode::create | open_mode::truncate )
				)
			) << test_line;
		}

		CHECK( arjan::posix::stat( test_path ).exists() );
		AND_WHEN( "we try to open the file and read the contents into a string" )
		{
			arjan::posix::ifstream ifstream(
				arjan::posix::streambuf(
					arjan::posix::open( test_path, arjan::posix::open_mode::read )
				)
			);
			std::string line;
			CHECK( std::getline( ifstream, line ) );
			THEN( "the string should match the line we put in" )
			{
				CHECK( line == test_line );
			}
		}
		std::remove( test_path );
	}
}
