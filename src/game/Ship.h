#ifndef _SHIP_H
#define _SHIP_H

namespace vortex
{
	class Ship
	{
	public:
		///
		/// Constructor
		///
		Ship();

		///
		/// Destructor
		///
		virtual ~Ship();

		///
		/// Render the ship
		///
		void Render();
	};
}

#endif // _SHIP_H