#ifndef SIGNAL_HPP
#define SIGNAL_HPP 1

/* Commands can check this variable to see if they should terminate.
 * It is set in signal handler and reset in shell.cpp. */
extern bool got_sigint;

extern void init_signals ();

#endif
