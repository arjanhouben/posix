#include <filesystem>
#include <string>
#include <string_view>

#include "arjan/posix/process.hpp"
#include "arjan/posix/file.hpp"
#include "arjan/posix/fstream.hpp"
#include "arjan/posix/open.hpp"
#include "arjan/posix/pipe.hpp"

#include "catch2/catch.hpp"

#include <iterator>

extern const char* cmake_command;
extern const char* head_command;
extern const char* env_command;
extern const std::vector< std::string > current_environment;

using arjan::posix::process::process;
using arjan::posix::process::options;
using arjan::posix::process::redirects;
using arjan::posix::process::convertible_to_string;

auto process( const std::string &cmd, convertible_to_string auto &&...args )
{
	return process( arjan::posix::process::options{}, cmd, std::forward< decltype( args ) >( args )... );
}

std::string process_to_string( std::string_view cmd, convertible_to_string auto &&...args )
{
	options opts {
		{
			redirects::null,
			redirects::pipe,
			redirects::null
		}
	};
	arjan::posix::ifstream stream{
		arjan::posix::streambuf(
			process( opts, cmd, std::forward< decltype( args ) >( args )... ).cout
		)
	};
	return {
		std::istreambuf_iterator< char >{ stream },
		std::istreambuf_iterator< char >{}
	};
}

TEST_CASE( "run cmake command" )
{
	CHECK( bool( process( cmake_command ) ) );
}

TEST_CASE( "run cmake command with incorrect argument" )
{
	REQUIRE_THROWS_AS(
		process_to_string( cmake_command, "this is incorrect" ),
		arjan::posix::process::unexpected_return_code
	);
	options opts;
	opts.throw_on_unexpected_return_code = false;
	REQUIRE_FALSE( bool( process( opts, cmake_command, "this is incorrect" ) ) );
}

TEST_CASE( "run cmake command and check output" )
{
	const std::string_view test_data = "test_data";
	const auto cout_output = process_to_string( cmake_command, "-E", "echo_append", test_data );
	CHECK( cout_output == test_data );
}

TEST_CASE( "run cmake command and check input and output" )
{
	options opt;
	opt.throw_on_unexpected_return_code = false;
	opt.cin = redirects::pipe;
	opt.cout = redirects::pipe;
	auto p = process( opt, head_command, "-1" );
	CHECK( p.cin != arjan::posix::file() );
	const std::string_view test_data = "some_test_data";
	arjan::posix::ofstream(
		arjan::posix::streambuf{
			std::move( p.cin )
		}
	) << test_data << '\n';
	CHECK( p.cin == arjan::posix::file() );
	CHECK_NOTHROW( p.result.value() );
	std::string cout_output;
	std::getline( arjan::posix::ifstream( arjan::posix::streambuf( std::move( p.cout ) ) ), cout_output );
	CHECK( cout_output == test_data );
}

TEST_CASE( "run command with modified env" )
{
	const std::string_view test_environment = "my_variable=my_value";
	options opts;
	opts.cout = redirects::pipe;
	opts.environment = { std::string{ test_environment } };
	std::string result;
	arjan::posix::ifstream stream( arjan::posix::streambuf( process( opts, env_command ).cout ) );
	stream >> result;
	CHECK( result == test_environment );
}

TEST_CASE( "kill running process" )
{
	options opts;
	opts.throw_on_unexpected_return_code = false;
	auto p = process( opts, head_command );
	CHECK_FALSE( p.result.finished() );
	p.kill();
	CHECK( p.result.value() != opts.expected_return_code );
	CHECK( p.result.finished() );
}

TEST_CASE( "streambuf EAGAIN returns EOF to the stream" )
{
	GIVEN( "an empty pipe with non-blocking reads" )
	{
		arjan::posix::pipe pipe;
		pipe.open();
		REQUIRE( pipe );

		arjan::posix::streambuf sb{
			std::move( pipe[ arjan::posix::pipe::input ] )
		};
		arjan::posix::file writer( pipe[ arjan::posix::pipe::output ].release() );
		pipe[ arjan::posix::pipe::output ].reset();

		std::istream is{ &sb };
		std::string result;

		WHEN( "reading with no data available" )
		{
			is >> result;

			THEN( "the stream sets failbit" )
			{
				CHECK( is.fail() );
			}

			THEN( "the result string is empty" )
			{
				CHECK( result.empty() );
			}

			AND_WHEN( "reading again after clearing the error flags" )
			{
				is.clear();
				is >> result;

				THEN( "the stream still fails" )
				{
					CHECK( is.fail() );
				}
			}
		}
	}
}

TEST_CASE( "streambuf EOF on empty file" )
{
	GIVEN( "an empty file" )
	{
		constexpr auto path = "/tmp/streambuf_eof_test_empty";
		std::filesystem::remove( path );

		arjan::posix::file f{ arjan::posix::open( path, arjan::posix::open_mode::write | arjan::posix::open_mode::create | arjan::posix::open_mode::truncate, 0600 ) };
		REQUIRE( f );
		f.reset();

		std::string result;

		WHEN( "reading from a stream built on the file" )
		{
			arjan::posix::ifstream is(
				arjan::posix::streambuf(
					arjan::posix::open( path, arjan::posix::open_mode::read )
				)
			);
			is >> result;

			THEN( "the stream reports EOF" )
			{
				CHECK( is.eof() );
			}

			THEN( "the result string is empty" )
			{
				CHECK( result.empty() );
			}
		}

		std::filesystem::remove( path );
	}
}

TEST_CASE( "streambuf reads content then hits EOF cleanly" )
{
	GIVEN( "a file containing \"hello world\"" )
	{
		constexpr auto path = "/tmp/streambuf_eof_test_content";
		constexpr std::string_view data = "hello world";

		arjan::posix::file f{ arjan::posix::open( path, arjan::posix::open_mode::write | arjan::posix::open_mode::create | arjan::posix::open_mode::truncate, 0600 ) };
		REQUIRE( f );
		arjan::posix::streambuf sb{ std::move( f ) };
		REQUIRE( sb.xsputn( data.data(), static_cast< std::streamsize >( data.size() ) )
			== static_cast< std::streamsize >( data.size() ) );
		sb.sync();

		std::string result;

		WHEN( "reading the first word" )
		{
			arjan::posix::ifstream is(
				arjan::posix::streambuf(
					arjan::posix::open( path, arjan::posix::open_mode::read )
				)
			);
			is >> result;

			THEN( "the result is \"hello\"" )
			{
				CHECK( result == "hello" );
			}

			THEN( "the stream has not reached EOF" )
			{
				CHECK( !is.eof() );
			}

			AND_WHEN( "reading the second word" )
			{
				is >> result;

				THEN( "the result is \"world\"" )
				{
					CHECK( result == "world" );
				}

				AND_WHEN( "reading once more after clearing the stream" )
				{
					is.clear();
					result.clear();
					is >> result;

					THEN( "the stream reports EOF" )
					{
						CHECK( is.eof() );
					}

					THEN( "the result string is empty" )
					{
						CHECK( result.empty() );
					}
				}
			}
		}

		std::filesystem::remove( path );
	}
}
