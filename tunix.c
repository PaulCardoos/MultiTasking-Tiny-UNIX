/* file: tunix.c core kernel code */

#include <stdio.h>
#include <cpu.h>
#include <gates.h>
#include "tsyscall.h"
#include "tsystm.h"
#include "proc.h"
#include "sched.h"

extern IntHandler syscall; /* the assembler envelope routine    */
extern void ustart1(void);
extern void ustart2(void);
extern void ustart3(void);
extern void schedule(int entry);
extern void finale(void);

/* kprintf is proto'd in stdio.h, but we don't need that for anything else */
//void kprintf(char *, ...);

/* functions in this file */
void debug_set_trap_gate(int n, IntHandler *inthand_addr, int debug);
void set_trap_gate(int n, IntHandler *inthand_addr);
int sysexit(int);
void k_init(void);
void shutdown(void);
void syscallc( int user_eax, int devcode, char *buff , int bufflen);
void init_process_table(void);
void process0 (void);

PEntry proctab[NPROC], *curproc; /* process table and pointer to current process */

/* Record debug info in otherwise free memory between program and stack */
/* 0x300000 = 3M, the start of the last M of user memory on the SAPC */
#define DEBUG_AREA 0x300000
char *debug_log_area = (char *)DEBUG_AREA;
char *debug_record;  /* current pointer into log area */

int zombie_processes;

#define MAX_CALL 6

/* optional part: syscall dispatch table */
static  struct sysent {
       short   sy_narg;        /* total number of arguments */
       int     (*sy_call)(int, ...);   /* handler */
} sysent[MAX_CALL];

/****************************************************************************/
/* k_init: this function for the initialize  of the kernel system*/
/****************************************************************************/

void k_init(){
  debug_record = debug_log_area; /* clear debug log */
  cli();
  ioinit();            /* initialize the deivce */
  set_trap_gate(0x80, &syscall);   /* SET THE TRAP GATE*/

  /* Note: Could set these with array initializers */
  /* Need to cast function pointer type to keep ANSI C happy */
  sysent[TREAD].sy_call = (int (*)(int, ...))sysread;
  sysent[TWRITE].sy_call = (int (*)(int, ...))syswrite;
  sysent[TEXIT].sy_call = (int (*)(int, ...))sysexit;

  sysent[TEXIT].sy_narg = 1;    /* set the arg number of function */
  sysent[TREAD].sy_narg = 3;
  sysent[TIOCTL].sy_narg = 3;
  sysent[TWRITE].sy_narg = 3;

  sti();			/* user runs with interrupts on */

  init_process_table();
  process0();
}

void init_process_table() {

	zombie_processes = 0;
	curproc = &proctab[0];

  proctab[0].p_savedregs[SAVED_EFLAGS] = 0x1 << 9;
  proctab[0].p_savedregs[SAVED_EBP] = 0;
  proctab[0].p_status = RUN;

  proctab[1].p_savedregs[SAVED_PC] = (int) &ustart1;
  proctab[1].p_savedregs[SAVED_ESP] = ESP1;
  proctab[1].p_savedregs[SAVED_EFLAGS] = 0x1 << 9;
  proctab[1].p_savedregs[SAVED_EBP] = 0;
  proctab[1].p_status = RUN;

  proctab[2].p_savedregs[SAVED_PC] = (int) &ustart2;
  proctab[2].p_savedregs[SAVED_ESP] = ESP2;
  proctab[2].p_savedregs[SAVED_EFLAGS] = 0x1 << 9;
  proctab[2].p_savedregs[SAVED_EBP] = 0;
  proctab[2].p_status = RUN;

  proctab[3].p_savedregs[SAVED_ESP] = ESP3;
  proctab[3].p_savedregs[SAVED_PC] = (int) &ustart3;
  proctab[3].p_savedregs[SAVED_EFLAGS] = 0x1 << 9;
  proctab[3].p_savedregs[SAVED_EBP] = 0;
  proctab[3].p_status = RUN;
}

void process0() {
  while (zombie_processes < NPROC-1) {
    schedule();
  }
	// for (int index = 1; index < NPROC; index++) {
	// 	while(proctab[index].p_status != ZOMBIE)
	// 		schedule(index);
	// }
  shutdown();
}

/* shut the system down */
void shutdown()
{
  kprintf("\n");
  kprintf("SHUTTING THE SYSTEM DOWN!\n");
  kprintf("Debug log from run:\n");
  kprintf("Marking kernel events as follows:\n");
  kprintf("  ^a   COM2 input interrupt, a received\n");
  kprintf("  ~d   COM2 output interrupt, ordinary char output\n");
  kprintf("  ~e   COM2 output interrupt, echo output\n");
  kprintf("  ~s   COM2 output interrupt, shutdown TX ints\n");
  kprintf("%s", debug_log_area);	/* the debug log from memory */
  kprintf("\nLEAVE KERNEL!\n\n");
  finale();		/* trap to Tutor */
}

/****************************************************************************/
/* syscallc: this function for the C part of the 0x80 trap handler          */
/* OK to just switch on the system call number here                         */
/* By putting the return value of syswrite, etc. in user_eax, it gets       */
/* popped back in sysentry.s and returned to user in eax                    */
/****************************************************************************/

void syscallc( int user_eax, int devcode, char *buff , int bufflen)
{
  int nargs;
  int syscall_no = user_eax;

  switch(nargs = sysent[syscall_no].sy_narg)
    {
    case 1:         /* 1-argument system call */
	user_eax = sysent[syscall_no].sy_call(devcode);   /* sysexit */
	break;
    case 3:         /* 3-arg system call: calls sysread or syswrite */
	user_eax = sysent[syscall_no].sy_call(devcode,buff,bufflen);
	break;
    default: kprintf("bad # syscall args %d, syscall #%d\n",
		     nargs, syscall_no);
    }
}

/****************************************************************************/
/* sysexit: this function for the exit syscall fuction */
/****************************************************************************/

int sysexit(int exit_code){
  cli();
  curproc->p_exitval = exit_code;
  curproc->p_status = ZOMBIE;
  zombie_processes++;
  sti();
  schedule();
	// if (zombie_processes < 3) {
	// 	curproc = (PEntry *)&proctab;
	// 	process0();
	// }
	// else
	// 	shutdown();
	return 0;    /* never happens, but keeps gcc happy */
}

/****************************************************************************/
/* set_trap_gate: this function for setting the trap gate */
/****************************************************************************/
extern void locate_idt(unsigned int *limitp, char ** idtp);

/* append msg to memory log */
void debug_log(char *msg)
{
    strcpy(debug_record, msg);
    debug_record +=strlen(msg);
}
