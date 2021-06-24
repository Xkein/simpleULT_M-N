
/*void switch_to(struct task_struct *next)*/

#define PARAM1 8
#define PARAM2 12
#define CONTEXT 4
#define SCHEDULING 8

.section .text
.global switch_to
switch_to:
  push %ebp
  mov %esp, %ebp /* store stack frame */

  /* make context */
	pusha
  pushf
  
  call closealarm /* close interrupt */
  /* switch context */
  mov PARAM1(%ebp), %eax /* get current thread */
  mov %esp, CONTEXT(%eax) /* store context */ 
  mov PARAM2(%ebp), %eax /* get next thread */
	mov CONTEXT(%eax), %esp /* switch next stack */

  call openalarm /* open interrupt */

  /* context changed, ready to run other thread. */
  popf
  popa

	pop %ebp
  ret