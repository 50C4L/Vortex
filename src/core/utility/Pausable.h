#ifndef _PAUSABLE_H
#define _PAUSABLE_H

namespace utility
{
	///
	/// Mixin for systems that can freeze and resume without losing state.
	/// Pause() is idempotent; override OnPauseChanged() for side effects
	/// such as silencing audio voices.
	///
	class Pausable
	{
	public:
		virtual ~Pausable() = default;

		void Pause( bool paused )
		{
			if( mPaused == paused )
			{
				return;
			}
			mPaused = paused;
			OnPauseChanged( paused );
		}

		bool IsPaused() const { return mPaused; }

	protected:
		virtual void OnPauseChanged( bool /*paused*/ ) {}

	private:
		bool mPaused = false;
	};
}

#endif // _PAUSABLE_H
