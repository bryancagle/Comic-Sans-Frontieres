/// \file timer.h
/// \brief Interface for the timer class CTimer.

#pragma once

#include <windows.h> //needed for BOOL
#include "Defines.h"

/// The \brief The timer. 
///
/// The timer allows you to manage game events by duration, rather than
/// on a frame-by-frame basis. This simple version of the timer is based on
/// the Windows API function timeGetTime, which is notoriously inaccurate
/// but perfectly adequate for a simple game demo.

class CTimer{
  private:
		int m_nStartTime; ///< Time that timer was started.
		int m_nCurrentTime; ///< Current time.

		int m_nLastFrameStartTime; ///< Start time for previous frame.
		int m_nFrameTime; ///< Elapsed time for previous frame.

		int m_nLevelStartTime; ///< Time the current level started.
		int m_nLevelFinishTime; ///< Time the current level finished.
		BOOL m_bLevelTimerOn; ///< Whether the current level is being timed.

  public:
		CTimer(); ///< Constructor.
		void start(); ///< Start the timer.
		int time(); ///< Return the time in ms.
		bool elapsed(int &start, int interval); ///< Has interval ms elapsed since start?

		int frametime(); ///< Return the time for last frame in ms.

										 //begin and end of frame functions
		void beginframe(); ///< Beginning of animation frame.
		void endframe(); ///< End of animation frame.

		int GetLevelStartTime(); ///< Get time that the level started.
		int GetLevelElapsedTime(); ///< Get elapsed time since the level started.
		void StartLevelTimer(); ///< Start the level timer.
		void StopLevelTimer(); ///< Stop the level timer.
}; //CTimer

