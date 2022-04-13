/*******************************************************************************
Copyright 2021
Steward Observatory Engineering & Technical Services, University of Arizona

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*******************************************************************************/

/**
 * @file LEDFlasher.cpp
 * @author Adrian Loeff (aloeff@arizona.edu)
 * @brief The LEDFlasher class is used to flash the Teensy LED in a specified pattern.
 * @version 1.0
 * @date 2021-12-23
 *
 * @copyright Copyright (c) 2021
 *
 */


#include <Arduino.h>

#include "LEDFlasher.hpp"


const int ledPin = 13;  // Pin number of diagnostic LED


// LEDFlasher class methods

LEDFlasher::LEDFlasher()
{
  m_times            = NULL;
  m_num_times        = 0;
  m_new_sequence     = false;
  m_halting_sequence = false;
  m_sequence_time    = 0;
  m_start_time       = 0;
  m_sequence_halted  = false;
  m_LED_is_on        = false;
}


LEDFlasher::~LEDFlasher()
{
}


// Set up the hardware to drive the LED.  This should only be called once.
void LEDFlasher::init_hardware( void )
{
  pinMode( ledPin, OUTPUT );
}


// Manually turn the LED on or off, overriding any loaded sequence.  The
// loaded sequence will resume the next time update_LED() is called.
void LEDFlasher::set_state( bool turn_on )
{
  // Remember the new state
  m_LED_is_on = turn_on;

  // Set the LED to the correct state
  digitalWrite( ledPin, m_LED_is_on ? HIGH : LOW );
}


// Return the current state of the LED: true for on, false for off.
bool LEDFlasher::get_state()
{
  return m_LED_is_on;
}


// Load a new timing sequence, replacing any existing timing sequence.  Each
// element of the times array represents a time in milliseconds for the LED
// to be on or off, with the first element corresponding to the first on time
// then alternating off, on, off, etc.  Elements specifying zero time will be
// skipped.  To start the sequence, call update_LED().  A negative time will
// halt the sequence, with the LED remaining in the state specified by the
// previous element; otherwise the sequence will wrap back to the start when
// it reaches the end and continue indefinitely or until another sequence is
// loaded.  Note that only a pointer to the times array is stored internally
// - the caller must maintain the array for the lifetime of the sequence and
// should not change it.  Returns false if the sequence is empty or the total
// sequence time is zero.
bool LEDFlasher::load_sequence( int *times, int num_times )
{
  // Store a pointer to the new sequence
  m_times     = times;
  m_num_times = num_times;

  // The sequence is not yet running
  m_new_sequence    = true;
  m_sequence_halted = false;

  // An empty sequence is invalid
  if ( m_times == NULL || m_num_times <= 0 )
    return false;

  // Calculate the total time in the sequence
  m_halting_sequence = false;
  m_sequence_time    = 0;
  int interval;
  for ( interval = 0; interval < m_num_times; interval++ )
  {
    long interval_time = m_times[ interval ];

    // Have we reached a halt indicator?
    if ( interval_time < 0 )
    {
      m_halting_sequence = true;
      break;
    }

    m_sequence_time += interval_time;
  }

  // A sequence with zero time is invalid
  if ( m_sequence_time == 0 )
    return false;

  // The valid sequence has been loaded successfully
  return true;
}


// Drive the LED on or off based on the loaded timing sequence and the
// current elapsed time since the sequence was loaded.  This function should
// be called frequently, at least as often as the desired timing accuracy.
// Returns true if the sequence is still running, or false if the sequence
// has halted (reached a negative time) or no sequence is loaded.  If
// LED_is_on is non-NULL, the current state of the LED will be returned in
// that parameter.  The LED won't be driven if the sequence has halted.
bool LEDFlasher::update_LED( bool *LED_is_on )
{
  // Check that the loaded sequence is valid
  if ( m_times == NULL || m_num_times <= 0 || m_sequence_time == 0 )
    return false;

  // If the sequence has been halted, report the last LED state
  if ( m_sequence_halted )
  {
    if ( LED_is_on != NULL )
      *LED_is_on = m_LED_is_on;

    return false;
  }

  // Get the current time
  unsigned long current_time = millis();

  // If a new sequence has been loaded, start it
  if ( m_new_sequence )
  {
    m_start_time   = current_time;
    m_new_sequence = false;
  }

  // Calculate the elapsed time since the start of the sequence
  unsigned long elapsed_time = current_time - m_start_time;

  // For repeating sequences, calculate elapsed time since the start of the
  // last cycle, and reset the start time
  if ( !m_halting_sequence )
  {
    elapsed_time %= m_sequence_time;
    m_start_time = current_time - elapsed_time;
  }

  // Find the current interval in the sequence
  unsigned long cumulative_time = 0;
  int last_non_zero_interval = -1;
  int interval;
  for ( interval = 0; interval < m_num_times; interval++ )
  {
    long interval_time = m_times[ interval ];

    // Have we reached a halt indicator?
    if ( interval_time < 0 )
    {
      // Halt the sequence at the last non-zero interval (if any)
      m_sequence_halted = true;
      interval = last_non_zero_interval;
      break;
    }
    else if ( interval_time > 0 )
      last_non_zero_interval = interval;

    // Calculate the cumulative time to the end of this interval
    cumulative_time += interval_time;

    // Are we in the current interval?
    if ( elapsed_time < cumulative_time )
      // Stop searching
      break;
  }

  // Set LED state based on the last non-zero interval, if any
  if ( last_non_zero_interval >= 0 )
    // Even intervals are on, odd intervals are off
    m_LED_is_on = ( last_non_zero_interval % 2 ) == 0;

  // Set the LED to the correct state
  digitalWrite( ledPin, m_LED_is_on ? HIGH : LOW );

  // Report the state of the LED, if desired
  if ( LED_is_on != NULL )
    *LED_is_on = m_LED_is_on;

  // Return an indication of whether the sequence is still running
  return !m_sequence_halted;
}
