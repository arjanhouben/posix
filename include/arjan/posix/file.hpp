#pragma once

#include <unistd.h>
#include <compare>

namespace arjan {
namespace posix {

template < typename T >
struct default_close
{
	constexpr void operator()( T f ) const noexcept
	{
		::close( f );
	};
};

template < typename Close = default_close< int > >
struct file_base
{	
	static constexpr int invalid = -1;

	constexpr inline explicit file_base( int no = invalid ) noexcept :
		no_( no ) {}
	
	constexpr inline file_base( file_base &&rhs ) noexcept :
		no_( rhs.release() ) {}
	
	constexpr inline file_base& operator = ( file_base &&rhs ) noexcept
	{
		reset( rhs.release() );
		return *this;
	}
	
	inline ~file_base() noexcept
	{
		reset();
	}

	constexpr auto operator <=> ( const file_base &rhs ) const noexcept = default;

	constexpr inline bool valid() const noexcept
	{
		return no_ != invalid;
	}

	constexpr inline explicit operator bool() const noexcept
	{
		return valid();
	}
	
	constexpr inline int get() const noexcept
	{
		return no_;
	}
	
	constexpr inline int release() noexcept
	{
		int r = invalid;
		std::swap( no_, r );
		return r;
	}
	
		inline void reset( int no = invalid ) noexcept
		{
			if ( valid() )
			{
				Close{}( no_ );
			}
			no_ = no;
		}
	
	private:

		int no_;
};

using file = file_base<>;

}}
