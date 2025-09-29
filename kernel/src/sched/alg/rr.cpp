/* SPDX-License-Identifier: MIT */

/* StACSOS - Kernel
 *
 * Copyright (c) University of St Andrews 2024
 * Tom Spink <tcs6@st-andrews.ac.uk>
 */
#include <stacsos/kernel/sched/alg/rr.h>

// *** COURSEWORK NOTE *** //
// This will be where you are implementing your round-robin scheduling algorithm.
// Please edit this file in any way you see fit.  You may also edit the rr.h
// header file.

using namespace stacsos::kernel::sched;
using namespace stacsos::kernel::sched::alg;

/**
 * @brief Adds a process to the runnable threads queue.
 *
 * I made the assumption that if the thread control block was null, it should not be added
 * as it might be a useless node and could cause issues within the code.
 *
 * @param tcb - Thread Control Block malloced object. As previously mentioned, if the object is null, it is not placed upon the runqueue.
 */

void round_robin::add_to_runqueue(tcb &tcb)
{		
	// No null check because the compiler can guarantee that the pointer is actually referencing something. (-Waddress)
	runtime_queue.enqueue(&tcb);
}

/**
 * @brief Removes a passed Thread Control Block element from the runqueue.
 *
 * @param tcb  - The passed Thread Control Block.
 */

void round_robin::remove_from_runqueue(tcb &tcb)
{
	// Standard empty runtime_queue check to avoid any issues.
	if (runtime_queue.empty()) {
		return;
	}

	// By checking on the code, we know this will fail silently so no membership check is required
	runtime_queue.remove(&tcb);
	return;
}

/**
 * @brief Called by the CPU scheduler when it is time to run another task.
 * In this case, by implementing the round-robin algorithm, it:
 * 	1. Takes the element from the front of the list
 * 	2. Places said element in the back of the list
 * 	3. Returns it, as it is the next task that should run.
 * For this, I chose the rotate() function since it implements such behaviour in a concise and expressive manner
 *
 * @param current - The task which is currently running. It is ignored in this function because in our given documentation
 * 					the current task has no role to play. In a more advanced algorithm, we would determine a quantum time
 * 					to run. If it finished during said time, then it leaves the system. If not, it would go back to the end of the queue. 
 * @return tcb* - A pointer to the task that is going to be ran next. 
 */

tcb *round_robin::select_next_task(tcb *current)
{
	/*
	Avoids running the rotate function on an empty list since
	the function itself does not seem to handle the edge case.
	 */

	if (runtime_queue.empty()) {
		return nullptr;
	}

	return runtime_queue.rotate();
}
