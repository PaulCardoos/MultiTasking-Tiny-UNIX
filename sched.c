#include <stdio.h>
#include <cpu.h>
#include "sched.h"
#include "proc.h"

extern void asmswtch(PEntry *oldproc, PEntry *newproc);
extern void debug_log(char *);

// sprintf(log,"<%c", ch);
// debug_log(log);

void schedule() {

	PEntry * prev_process = curproc;

	for (int i = 1; i<NPROC; i++) {
    if (proctab[i].p_status == RUN) {
      curproc = &proctab[i];
      break;
    } else {
      curproc = &proctab[0];
    }
  }
	asmswtch(prev_process, curproc);
}

void sleep(WaitCode event) {
	// int saved_eflags = get_eflags();
	// cli();
	curproc->p_status = BLOCKED;
  curproc->p_waitcode = event;
  // saved_eflags = get_eflags();
  schedule();
}

void wakeup(WaitCode event) {
	// int saved_eflags = get_eflags();
	// cli();
	for (int index = 1; index < NPROC; index++) {
		if ((proctab[index].p_status==BLOCKED) && (proctab[index].p_waitcode==event)) {
      proctab[index].p_status = RUN;
    }
	}
	// set_eflags(saved_eflags);
}
