/*
	mer_task.h
*/
typedef struct mer_task_struct {
	unsigned long ESP;	// holds the ESP value so that when the 
				// process is selected again it holds 
				// reference to its stack
	unsigned long need_resched; // if '1' then the process should be
				   // scheduled out.
}task_struct;
