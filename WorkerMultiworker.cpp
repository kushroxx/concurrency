// Reconstructed Interview Question (Formal Version):
// Design and implement a worker system with the following requirements:

// There should be a Worker class responsible for executing individual units of work.

// There should be a MultiWorker class which manages multiple Worker instances.

// Tasks (works) should be added to the MultiWorker.

// Workers should process tasks concurrently (use threads, a thread pool, or whatever makes sense).

// Once all tasks are completed, a single final callback should be triggered to signal “all work is done.”

// Constraints:
// There may be multiple tasks submitted over time.

// If a new set of tasks is submitted after the previous one is completed, they should be processed again with the callback at the end.

// Avoid premature callback invocation before all work completes.

// Handle thread safety appropriately.

// ✅ Clarifying Questions You Could Ask in Interview:
// ❓ Do I need to guarantee ordering of task execution, or can they run in parallel without order?

// ❓ Do tasks have results I need to collect, or is success/failure enough?

// ❓ Do I need to support cancellation of tasks or graceful shutdown?

// ❓ Do you expect each submission batch to have its own final callback, or a single one globally?

// ✅ If the Interviewer Wants More Depth:
// Add timeouts for task execution.

// Allow partial failure handling or collecting results.

// Support dynamic task addition while tasks are running (streaming vs batch).

// Require support for cancellation of pending/running tasks.

