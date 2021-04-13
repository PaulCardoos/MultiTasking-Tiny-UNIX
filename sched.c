#include <stdio.h>
#include <cpu.h>
#include "sched.h"

extern void asmswtch(PEntry *oldproc, PEntry *newproc);
extern void debug_log(char *);

// sprintf(log,"<%c", ch);
// debug_log(log);

void schedule(int entry) {

  debug_log("|(");
  debug_log(entry-1);
  if (curproc->p_status == BLOCKED) {
    debug_log("b");
  } else if (curproc->p_status == ZOMBIE) {
    debug_log("z");
  }
  debug_log("-");

	PEntry * past_process = curproc;

	if (proctab[entry].p_status == RUN) {
    curproc = (PEntry *)proctab + entry;
  } else {
		curproc = (PEntry *)proctab;
  }

  int	saved_eflags = get_eflags();
	cli();

	asmswtch(past_process, curproc);
	set_eflags(saved_eflags);
}

void sleep() {
	int saved_eflags = get_eflags();
	cli();
	curproc->p_status = BLOCKED;
	saved_eflags = get_eflags();
}

void wakeup() {
	int saved_eflags = get_eflags();
	cli();
	for (int index = 1; index < NPROC; index++) {
		if (proctab[index].p_status == BLOCKED) {
      proctab[index].p_status == RUN;
    }
	}
	set_eflags(saved_eflags);
}
