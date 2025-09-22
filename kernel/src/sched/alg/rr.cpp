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

void round_robin::add_to_runqueue(tcb &tcb) { 
    runtime_queue.enqueue(&tcb); 
}

void round_robin::remove_from_runqueue(tcb &tcb)
{
	if (runtime_queue.empty()) {
		return;
	}

	runtime_queue.remove(&tcb);
	return;
}

tcb *round_robin::select_next_task(tcb *current) {
	// If current task should continue running, add it back to queue
	if (current != nullptr){
		add_to_runqueue(*current);
	}

    // Get the next task from the queue
    if (runtime_queue.empty()){
		return nullptr;
	}

	tcb *next = runtime_queue.first();
    runtime_queue.pop();
	return next;
}
