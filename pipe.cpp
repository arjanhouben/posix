#include <unistd.h>
#include <string_view>

#include "catch2/catch.hpp"
#include "arjan/posix/pipe.hpp"

TEST_CASE( "pipe" )
{
	WHEN( "using the default constructor" )
	{
		arjan::posix::pipe p;
		CHECK_FALSE( bool( p ) );
		CHECK( p[ arjan::posix::pipe::input ].get() == arjan::posix::file::invalid );
		CHECK( p[ arjan::posix::pipe::output ].get() == arjan::posix::file::invalid );
	}

	WHEN( "opening a pipe" )
	{
		arjan::posix::pipe p;
		p.open();
		CHECK( bool( p ) );
		CHECK( p[ arjan::posix::pipe::input ].get() != arjan::posix::file::invalid );
		CHECK( p[ arjan::posix::pipe::output ].get() != arjan::posix::file::invalid );
		CHECK( p[ arjan::posix::pipe::input ].get() != p[ arjan::posix::pipe::output ].get() );

		WHEN( "closing an opened pipe" )
		{
			p.close();
			CHECK_FALSE( bool( p ) );
			CHECK( p[ arjan::posix::pipe::input ].get() == arjan::posix::file::invalid );
			CHECK( p[ arjan::posix::pipe::output ].get() == arjan::posix::file::invalid );
		}

		WHEN( "writing to the pipe and reading from it" )
		{
			std::string_view test_data = "hello from pipe";
			::write( p[ arjan::posix::pipe::output ].get(), test_data.data(), test_data.size() );
			std::string buf( 256, 0 );
			const ssize_t n = ::read( p[ arjan::posix::pipe::input ].get(), buf.data(), buf.size() );
			CHECK( n > 0 );
			CHECK( std::string_view( buf.data(), static_cast< std::string_view::size_type >( n ) ) == test_data );
		}
	}
}
