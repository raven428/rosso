/*
  This file contains/describes function for signal handling.
*/

#ifndef __signal_h__
#define __signal_h__

// initialize signal handling for critical sections
void init_signal_handling(void);

// blocks signals for critical section
void start_critical_section(void);

// unblocks signals after critical section
void end_critical_section(void);

#endif // __signal_h__
