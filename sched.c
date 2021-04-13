#include <stdio.h>
#include <cpu.h>
#include "sched.h"

extern void asmswtch(PEntry *oldproc, PEntry *newproc);
extern void debug_log(char *);

// sprintf(log,"<%c", ch);
// debug_log(log);

void schedule(int entry) {

	PEntry * prev_process = curproc;

	if (proctab[entry].p_status == RUN) {
    curproc = (PEntry *)proctab + entry;
  } else {
		curproc = (PEntry *)proctab;
    debug_log("|(3z-0)");
  }

  int	saved_eflags = get_eflags();
	cli();

	asmswtch(prev_process, curproc);

	set_eflags(saved_eflags);
}

void sleep(WaitCode event) {
	int saved_eflags = get_eflags();
	cli();
	curproc->p_status = BLOCKED;
  curproc->p_waitcode = event;
  saved_eflags = get_eflags();
  schedule();
}

void wakeup(WaitCode event) {
	int saved_eflags = get_eflags();
	cli();
	for (int index = 1; index < NPROC; index++) {
		if (proctab[index].p_status == BLOCKED && proctab[k].p_waitcode==event) {
      proctab[index].p_status == RUN;
    }
	}
	set_eflags(saved_eflags);
}
