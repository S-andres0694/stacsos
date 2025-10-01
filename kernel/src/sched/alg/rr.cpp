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
 * Since the parameter is a reference, it is impossible for it to be `null`. Therefore, no check is required.
 *
 * @param tcb - Thread Control Block malloced object. 
 */

void round_robin::add_to_runqueue(tcb &tcb)
{		
	if (&tcb == nullptr){
		return;
	}

	// No null check because the compiler can guarantee that the pointer is actually referencing something since 
	// the parameter is a reference (-Waddress)
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
 * 					Sourced all of my research about the algorithm from here: https://www.geeksforgeeks.org/operating-systems/round-robin-scheduling-in-operating-system/
 * @return tcb* - A pointer to the task that is going to be ran next. Not used because of the aforementioned reasons.
 */

tcb *round_robin::select_next_task(tcb *current)
{
	/*
	Avoids running the rotate function on an empty list since
	the function itself does not seem to handle the edge case.
	 */

	// There are no runnable threads waiting for time, should just return. 
	if (runtime_queue.empty()) {
		return nullptr;
	}

	// Small optimization for shorter runtime if the list has just one member.
	if (runtime_queue.count() == 1) {
		return runtime_queue.first();
	}

	return runtime_queue.rotate();
}
