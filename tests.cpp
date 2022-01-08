#include "catch2/catch.hpp"
#include "arjan/posix/process.hpp"
#include "arjan/posix/fstream.hpp"

#include <iostream>
#include <iterator>

extern const std::string cmake_command;
extern const std::string head_command;
extern const std::string env_command;
extern const std::vector< std::string > current_environment;

using arjan::posix::process::process;
using arjan::posix::process::options;
using arjan::posix::process::redirects;

template < typename ...Args >
auto process( const std::string &cmd, Args &&...args )
{
	return process( arjan::posix::process::options{}, cmd, std::forward< Args >( args )... );
}

template < typename ...Args >
std::string process_to_string( const std::string &cmd, Args &&...args )
{
	options opts {
		{
			redirects::null,
			redirects::pipe,
			redirects::null
		}
	};
	arjan::posix::ifstream stream{
		process( opts, cmd, std::forward< Args >( args )... ).cout
	};
	return {
		std::istreambuf_iterator< char >{ stream },
		std::istreambuf_iterator< char >{}
	};
}

TEST_CASE( "run cmake command" )
{
	// bool is required because of Catch not calling operator bool()
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
	REQUIRE_FALSE(
		bool( process( opts, cmake_command, "this is incorrect" ) )
	);
}

TEST_CASE( "run cmake command and check output" )
{
	const std::string test_data = "test_data";
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
	const std::string test_data = "some_test_data";
	arjan::posix::ofstream( std::move( p.cin ) ) << test_data << '\n';
	CHECK( p.cin == arjan::posix::file() );
	CHECK_NOTHROW( p.result.value() );
	std::string cout_output;
	std::getline( arjan::posix::ifstream( std::move( p.cout ) ), cout_output );
	CHECK( cout_output == test_data );
}

TEST_CASE( "run command with modified env" )
{
	const std::string test_environment = "my_variable=my_value";
	options opts;
	opts.cout = redirects::pipe;
	opts.environment = { test_environment };
	std::string result;
	arjan::posix::ifstream stream( process( opts, env_command ).cout );
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