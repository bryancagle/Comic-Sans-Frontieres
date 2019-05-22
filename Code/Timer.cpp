/// \file timer.cpp
/// \brief Code for timer class CTimer.

#include "timer.h"
#include "debug.h"

CTimer::CTimer() :
	m_nStartTime(0),
	m_nLastFrameStartTime(0),
	m_nFrameTime(0),
	m_nLevelStartTime(0),
	m_nLevelFinishTime(0),
	m_bLevelTimerOn(FALSE)
{}; //constructor

/// Start the timer from zero.

void CTimer::start(){
  m_nStartTime = timeGetTime();
} //start

/// Get the time.
/// \return The time in milliseconds.

int CTimer::time(){ 
  return timeGetTime() - m_nStartTime;
} //time

/// Get last frame time for last frame in milliseconds.

int CTimer::frametime() {
	return m_nFrameTime;
} //frametime

/// The elapsed function is a useful function for measuring repeating time 
/// intervals. Given the start and duration times, this function returns TRUE 
/// if the interval is over, and has the side-effect of resetting the start
/// time when that happens, thus setting things up for the next interval.
/// \param start Start of time interval
/// \param interval Duration of time interval
/// \return TRUE if time interval is over

bool CTimer::elapsed(int &start, int interval){
  int curtime = time(); //current time

  if(curtime >= start + interval){ //if interval is over
    start = curtime; //reset the start 
    return true; //succeed
  } //if

  else return false; //otherwise, fail
} //elapsed

	/// This is the signal that a new animation frame has begun,
	/// so that the timer can return the same time value for the
	/// duration of this frame.

void CTimer::beginframe() {
	const int t = timeGetTime();
	m_nCurrentTime = t - m_nStartTime;
	if (fabs(m_nFrameTime) > 1000)
		m_nFrameTime = 0; //safety
	m_nLastFrameStartTime = t;
} //beginframe

/// This is the signal that an animation frame has ended.

void CTimer::endframe() {
	m_nFrameTime = 0;
} //endframe

/// Get the time that the current level started.
/// \return Level start time.

int CTimer::GetLevelStartTime() {
	return m_nLevelStartTime;
} //GetLevelStartTime

/// Get amount of time spent in the current level.
/// \return Level elapsed time.

int CTimer::GetLevelElapsedTime() {
	return m_bLevelTimerOn ?
		timeGetTime() - m_nLevelStartTime :
		m_nLevelFinishTime - m_nLevelStartTime;
} //GetLevelElapsedTime

void CTimer::StartLevelTimer() {
	m_bLevelTimerOn = TRUE;
	m_nLevelStartTime = timeGetTime();
} //StartLevelTimer

/// Stop the level timer. Should be called at the end of a level.

void CTimer::StopLevelTimer() {
	m_bLevelTimerOn = FALSE;
	m_nLevelFinishTime = timeGetTime();
} //StopLevelTimer