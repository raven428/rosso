/*
  This file contains/describes function for signal handling.
*/

#include "signal.h"

#include <stdlib.h>
#include <signal.h>
#include "mallocv.h"

sigset_t blocked_signals_set;

void init_signal_handling(void) {
/*
  initialize signal handling for critical sections
*/
  sigfillset(&blocked_signals_set);
}

void start_critical_section(void) {
/*
  blocks signals for critical section
*/
  sigprocmask(SIG_BLOCK, &blocked_signals_set, NULL);
}

void end_critical_section(void) {
/*
  unblocks signals after critical section
*/
  sigprocmask(SIG_UNBLOCK, &blocked_signals_set, NULL);
}

