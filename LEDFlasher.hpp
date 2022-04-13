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
 * @file LEDFlasher.hpp
 * @author Adrian Loeff (aloeff@arizona.edu)
 * @brief The LEDFlasher class is used to flash the Teensy LED in a specified pattern.
 * @version 1.0
 * @date 2021-12-23
 *
 * @copyright Copyright (c) 2021
 *
 */


#ifndef __LEDFLASHER_HPP__
#define __LEDFLASHER_HPP__


#include <stdbool.h>
#include <stdarg.h>
#include <iostream>


class LEDFlasher
{
public:
  LEDFlasher();
  ~LEDFlasher();

  // Set up the hardware to drive the LED.  This should only be called once.
  void init_hardware( void );

  // Manually turn the LED on or off, overriding any loaded sequence.  The
  // loaded sequence will resume the next time update_LED() is called.
  void set_state( bool turn_on );

  // Return the current state of the LED: true for on, false for off.
  bool get_state( void );

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
  bool load_sequence( int *times, int num_times );

  // Drive the LED on or off based on the loaded timing sequence and the
  // current elapsed time since the sequence was loaded.  This function should
  // be called frequently, at least as often as the desired timing accuracy.
  // Returns true if the sequence is still running, or false if the sequence
  // has halted (reached a negative time) or no sequence is loaded.  If
  // LED_is_on is non-NULL, the current state of the LED will be returned in
  // that parameter.  The LED won't be driven if the sequence has halted.
  bool update_LED( bool *LED_is_on );

private:

  int           *m_times;
  int            m_num_times;
  bool           m_new_sequence;
  bool           m_halting_sequence;
  unsigned long  m_sequence_time;
  unsigned long  m_start_time;
  bool           m_sequence_halted;
  bool           m_LED_is_on;
};


#endif  // __LEDFLASHER_HPP__
