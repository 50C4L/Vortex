#ifndef _EAGE_POINTERS_H
#define _EAGE_POINTERS_H

#include <memory>
#include <iostream>

namespace utility
{
	///
	/// Helper function to make shared pointer with thirdparty constructor and destructor functions.
	/// Thus RAII will release resources automatically.
	///
	/// @param Creator
	///  Creator function
	///
	/// @param Destructor
	///  Destructor function
	///
	/// @param Arguments
	///  Parameters for creator function
	///
	/// @return
	///  unique_ptr of the created object
	///
	template< typename Creator, typename Destructor, typename... Arguments >
	auto make_resource( Creator c, Destructor d, Arguments... args )
	{
		auto r = c( std::forward<Arguments>( args )... );
		if( !r )
		{
			throw std::system_error( errno, std::generic_category() );
		}
		return std::shared_ptr<std::decay_t<decltype( *r )>>( r, d );
	}
}

#endif // _EAGE_POINTERS_H
