#pragma once

#include <future>
#include <utility>
#include <cstdlib>
#include <vector>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#ifdef __linux__
#include <wait.h>
#endif

#include "arjan/posix/errno.hpp"
#include "arjan/posix/file.hpp"
#include "arjan/posix/pipe.hpp"

namespace arjan {
namespace posix {
namespace process {

struct result_value
{
	inline result_value( int expected, std::future< int > &&f ) :
		expected_( expected ),
		future_( std::move( f ) ),
		value_( std::numeric_limits< int >::max() ) {}
	
	inline ~result_value() noexcept( false )
	{
		if ( future_.valid() )
		{
			if ( exception_count == std::uncaught_exceptions() )
			{
				future_.get();
			}
		}
	}

	result_value( result_value&& ) = default;

	inline explicit operator bool() 
	{
		return expected_ == value();
	}
	
	inline bool finished() const noexcept
	{
		if ( future_.valid() )
		{
			return future_.wait_for( std::chrono::seconds( 0 ) ) == std::future_status::ready;
		}
		return true;
	}
	
	inline int expected() const noexcept
	{
		return expected_;
	}
	
	inline int value()
	{
		if ( future_.valid() )
		{
			value_ = future_.get();
		}
		return value_;
	}
	
	private:
	
		const int expected_;
		std::future< int > future_;
		int value_;
		const int exception_count = std::uncaught_exceptions();
};

enum class signal
{
	abort = SIGABRT,
	alarm = SIGALRM,
	bus_error = SIGBUS,
	child_stopped = SIGCHLD,
	continue_ = SIGCONT,
	emulator_trap = SIGEMT,
	floating_point_exception = SIGFPE,
	hangup = SIGHUP,
	illegal_instruction = SIGILL,
	interrupt = SIGINT,
	io_possible = SIGIO,
	kill = SIGKILL,
	broken_pipe = SIGPIPE,
#ifdef SIGPOLL
	poll = SIGPOLL,
#endif
	profiling_timer = SIGPROF,
#ifdef SIGPWR
	power_failure = SIGPWR,
#endif
	quit = SIGQUIT,
	segmentation_faul = SIGSEGV,
	stop = SIGSTOP,
	bad_system_call = SIGSYS,
	termination = SIGTERM,
	breakpoint = SIGTRAP,
	terminal_input = SIGTTIN,
	terminal_output = SIGTTOU,
	urgent = SIGURG,
	user_1 = SIGUSR1,
	user_2 = SIGUSR2,
	virtual_alarm = SIGVTALRM,
	CPU_time_limit = SIGXCPU,
};

struct handle
{
	explicit operator bool()
	{
		return static_cast< bool >( result );
	}

	void kill( signal s = signal::kill )
	{
		check_errno( ::kill, pid, static_cast< std::underlying_type_t< signal > >( s ) );
	}

	const int pid;
	result_value result;
	posix::file cin, cout, cerr;
};

enum class redirects
{
	parent,
	pipe,
	null
};

struct options : std::array< redirects, 3 >
{
	redirects &cin = std::array< redirects, 3 >::operator[]( STDIN_FILENO ), 
		&cout = std::array< redirects, 3 >::operator[]( STDOUT_FILENO ), 
		&cerr = std::array< redirects, 3 >::operator[]( STDERR_FILENO );

	std::vector< std::string > environment;
	
	int expected_return_code = 0;
	bool throw_on_unexpected_return_code = true;
};

template < typename Container, typename F >
void for_each( Container &c, F &&f )
{
	std::for_each( std::begin( c ), std::end( c ), std::forward< F >( f ) );
}

struct unexpected_return_code : std::exception
{
	unexpected_return_code( int c ) noexcept :
		code( c ) {}
		
	int code;
	const char* what() const noexcept override
	{
		return "unexpected_return_code";
	}
};

template < typename T = std::vector< std::string > >
T environment( char **environment )
{
	T result;
	while ( *environment )
	{
		result.push_back( *environment++ );
	}
	return result;
}

inline process::handle process( process::options options_, std::string cmd, std::vector< std::string > arguments = {} )
{
	constexpr auto input = posix::pipe::input;
	constexpr auto output = posix::pipe::output;

	std::array< posix::pipe, 3 > pipes;

	constexpr std::array< size_t, 3 > FILE_DESCRIPTORS = {
		STDIN_FILENO,
		STDOUT_FILENO,
		STDERR_FILENO
	};

	static_assert( STDERR_FILENO < FILE_DESCRIPTORS.size() );

	const auto get_direction = [&]( auto id )
	{
		return ( id == STDIN_FILENO ) ? input : output;
	};

	for ( auto pipe_id : FILE_DESCRIPTORS )
	{
		switch ( options_[ pipe_id ] )
		{
			case process::redirects::pipe:
				pipes[ pipe_id ].open();
				break;
			case process::redirects::null:
				pipes[ pipe_id ][ input ] = posix::file( "/dev/null", posix::file::mode::read );
				pipes[ pipe_id ][ output ] = posix::file( "/dev/null", posix::file::mode::write );
				break;
			case process::redirects::parent:
				break;
		}
	}
	
	const auto pid = check_errno( fork );
	if ( pid == 0 )
	{	
		for ( auto pipe_id : FILE_DESCRIPTORS )
		{
			if ( pipes[ pipe_id ] )
			{
				check_errno( close, static_cast< int >( pipe_id ) );
				check_errno( 
					dup2, 
					pipes[ pipe_id ][ get_direction( pipe_id ) ].get(),
					static_cast< int >( pipe_id )
				);
				pipes[ pipe_id ].close();
			}
		};

		std::vector< char* > parameters, environment;
		cmd.push_back( 0 );
		parameters.push_back( &cmd[ 0 ] );
		const auto append_null_terminator = []( auto &storage )
		{
			return [&]( auto &str )
			{
				str.push_back( 0 );
				storage.push_back( &str[ 0 ] );
			};
		};
		for_each( arguments, append_null_terminator( parameters ) );
		for_each( options_.environment, append_null_terminator( environment ) );
		parameters.push_back( nullptr );
		environment.push_back( nullptr );
		execve( cmd.c_str(), parameters.data(), environment.data() );
		exit( -1 );
	}
	
	constexpr auto get_status = []( int pid, options opt )
	{
		int status = 0;
		waitpid( pid, &status, 0 );
		if ( status != opt.expected_return_code )
		{
			if ( opt.throw_on_unexpected_return_code )
			{
				throw unexpected_return_code{ status };
			}
		}
		return status;
	};
	
	for ( auto pipe_id : FILE_DESCRIPTORS )
	{
		pipes[ pipe_id ][ get_direction( pipe_id ) ].reset();
	}
	
	return {
		pid,
		{
			options_.expected_return_code,
			std::async( get_status, pid, options_ )
		},
		std::move( pipes[ STDIN_FILENO ][ output ] ),
		std::move( pipes[ STDOUT_FILENO ][ input ] ),
		std::move( pipes[ STDERR_FILENO ][ input ] )
	};
}

template < typename ...Args >
process::handle process( process::options options_, const std::string &cmd, Args ...args )
{
	return process( options_, cmd, std::vector< std::string >{ args... } );
}

}}}