# Codexion

Master the race for resources before the deadline masters you.

Codexion is a concurrency and multi-threading project that simulates a group of coders competing for shared USB dongles to compile their code. It is a modern, strictly-timed variation of Dijkstra's classic Dining Philosophers problem, implementing both First-In-First-Out (FIFO) and Earliest Deadline First (EDF) scheduling.

## \# Instructions

### Compilation & Installation

The project is written in standard C and utilizes a Makefile for compilation. It strictly compiles with -Wall -Wextra -Werror and links the POSIX threads library (-pthread).

To install and compile the project, run the following commands in your terminal:

```
# Clone the repository (replace with your actual repo link)
git clone https://github.com/13o7ompo3/codexion.git
cd codexion

# Compile the executable
make
```

**Available Makefile rules**:
```
make or make all: Compiles the codexion executable.

make clean: Removes the generated .o object files.

make fclean: Removes the object files and the executable.

make re: Performs a full recompilation (fclean followed by all).

```

### Execution

	./codexion <number_of_coders> <time_to_burnout> <time_to_compile> <time_to_debug> <time_to_refactor> <required_compiles> <dongle_cooldown> <scheduler_type>

**Arguments Breakdown:**

- `number_of_coders`: The amount of coders (and dongles) at the table.

- `time_to_burnout`: Milliseconds a coder can survive without starting a compilation.

- `time_to_compile`: Milliseconds spent compiling (requires holding 2 dongles).

- `time_to_debug`: Milliseconds spent debugging.

- `time_to_refactor`: Milliseconds spent refactoring.

- `required_compiles`: Simulation stops if all coders reach this number (use -1 for infinite).

- `dongle_cooldown`: Mandatory wait time (in ms) before a released dongle can be taken again.

- `scheduler_type`: The arbitration policy (fifo or edf).

**Example Usage:**

To run a simulation with 3 coders, a 300ms burnout limit, 100ms debug time, 100ms cooldown, requiring 3 compiles each, using Earliest Deadline First scheduling:

	./codexion 3 300 0 100 0 3 100 edf

## \# Resources

**Classic References**

This project relies heavily on low-level operating system concepts and algorithmic theory.  Below are the foundational concepts and documentation referenced during development:
- The Dining Philosophers Problem: Edsger W. Dijkstra's original concurrency problem, which serves as the theoretical foundation for this project's resource-sharing challenge.
- POSIX Threads (pthread) Documentation: Linux manual pages for `pthread_create`, `pthread_join`, `pthread_mutex_lock`, `pthread_mutex_init`, `pthread_cond_init`, `pthread_cond_wait`, and `pthread_cond_broadcast`.
- Youtube play list explaining Threads: [Playlist](https://www.youtube.com/playlist?list=PLfqABt5AS4FmuQf70psXrsMLEDQXNkLq2)

**AI Usage Declaration**
- **Architectural Auditing**: Used AI to perform strict, line-by-line synchronization audits, identifying scope bleeds (like "Ghost Nodes") and ensuring Single Responsibility Principles for all POSIX mutexes.
- **Deep-OS Debugging**: Assisted in diagnosing Linux kernel-level threading phenomena, including Thread Creation Lag, Context-Switching storms, and analyzing lock-free atomic assembly interactions to resolve ThreadSanitizer/Helgrind false positives.
- **Documentation**: Assisting in structuring and formatting this README.md.

## \# Visual Program Structure

The program abandons the traditional "philosophers" array model in favor of a highly scalable Master-Worker Architecture. It is structured around a central Waiter (Global Arbiter), a hardware-accurate Timer (Sleep Room), an executioner (Monitor), and the Coders who compete for CPU time.

```
[ MAIN THREAD ]
      │
      ├─► Parses Arguments & Initializes memory safely via `calloc`.
      ├─► Spawns Coders, Monitor, Waiter, and Timer threads.
      │     └─► All threads sleep at the `start_cond` barrier.
      └─► Records `start_time` and fires `start_cond` Broadcast!
            │
            ▼
[ THE CONCURRENCY ENGINE ]
┌─────────────────┐       ┌─────────────────┐       ┌─────────────────┐
│ MONITOR THREAD  │       │ WAITER (Master) │       │ TIMER (Sleep)   │
│ 1. Scans Coders │       │ 1. Sweeps Queue │       │ 1. Manages Heap │
│ 2. Checks time  │       │ 2. Checks EDF   │       │ 2. Precise wait │
│ 3. Fires Alarm! │       │ 3. Gives Dongles│       │ 3. Wakes Coders │
└────────┬────────┘       └────────┬────────┘       └────────┬────────┘
         │                         │                         │
         ▼                         ▼                         ▼
┌─────────────────────────────────────────────────────────────────────┐
│ CODER THREADS (x N)                                                 │
│ 1. Request hardware   -> Yields control to the Waiter.              │
│ 2. Compile / Debug    -> Yields control to the Timer (Sleep Room).  │
│ 3. Release hardware   -> Wakes the Waiter to evaluate the Queue.    │
└─────────────────────────────────────────────────────────────────────┘
```

## \# Thread Synchronization Mechanisms

Thread-safe communication is achieved using tightly scoped POSIX Mutexes and Condition Variables. Every lock follows the Single Responsibility Principle to prevent "Scope Bleed" and micro-delays.

- **The Queue Mutex (queue_mutex)**: Protects the Master's Arbitration Queue and dongle states (in_use, available_at). It strictly guards the hardware resource mapping and prevents queue corruption.

- **The State Mutex (state_mutex)**: The absolute source of truth for the is_active flag. By isolating this flag from the I/O and Queue mutexes, the Monitor can instantly shut down the simulation without being blocked by a slow thread writing to the terminal.

- **The Sleep Mutex (sleep_mutex)**: Protects the Timer's Min-Heap and wake-up times, ensuring the Sleep Room can precisely track which coder needs to wake up next without locking the main execution queue.

- **The Write Mutex (write_mutex)**: Serializes all terminal output. A thread must acquire this lock before printing to stdout, ensuring that timestamped logs never interleave or corrupt.

- **The Thread Barrier (start_cond)**: A condition variable used to synchronize the launch of all threads. To prevent OS-level "Thread Creation Lag," all newly spawned threads sleep on this variable until the main thread has fully initialized the environment and sends a global broadcast.

## \# Blocking Cases Handled (Deadlock & Starvation Prevention)

This simulation successfully prevents all forms of deadlocks and thread starvation through a centralized arbitration system.

- **Hold-and-Wait Prevention:** Coders are strictly forbidden from acquiring a single dongle and waiting for the second. The Waiter enforces an atomic `take_both_dongles` policy. A coder only receives hardware if both required dongles are free and off cooldown.

- **Earliest Deadline First (EDF) Tie-Breakers:** To prevent starvation when two coders have the exact same burnout deadline (a common occurrence at T=0), the priority queue falls back to evaluating `compiles_done`. The coder with the fewest completed compiles is placed ahead in the queue, forcing aggressive threads to yield to starving threads.

- **The 1-Coder Edge Case:** Handled gracefully by forcing a single coder to take their only available dongle and yield the thread, waiting for the inevitable burnout.

## \# The Journey: Problems Encountered & Solved

Building a mathematically perfect, microsecond-accurate concurrency system revealed several hardware-level ghosts and logical traps. Here is how they were solved:

### 1. The Odd-Number Topology Trap & Empty-Queue Race

**The Problem:** Initially, deadlocks were avoided using an "Even/Odd" acquisition logic. However, with an odd number of coders, this created an asymmetric topology where two coders fought over the same initial dongle, leading to rapid starvation. Furthermore, coders could lock an empty individual dongle queue before the EDF scheduler could compare them against a starving coder.

**The Solution:** The architecture was rewritten into a Master-Worker model. Individual dongle queues were destroyed. All coders now enter a single global priority queue. No one touches a dongle until the Waiter explicitly confirms they are the highest-priority coder.

### 2. Thread Creation Lag

**The Problem:** When compile/debug/refactor/cooldown times were set to 0, the OS took longer to physically allocate memory for Coder 2's thread than it took Coder 1 to execute its entire lifecycle. Coder 1 would grab the dongles, compile, release them, and queue up for round 2 before Coder 2 even existed in the OS scheduler.

**The Solution:** Implementation of the "Starting Gun." Threads are now forcefully halted using a `pthread_cond_wait` barrier immediately after creation. They are only allowed to proceed when the main thread sends a `pthread_cond_broadcast`, guaranteeing a perfectly synchronized T=0 start time.

### 3. The OS Scheduler Lag & The Min-Heap Timer

**The Problem:**  Standard implementations rely on usleep for thread delays. However, the Linux Completely Fair Scheduler (CFS) often oversleeps by several milliseconds depending on CPU load, causing Coders to die even when the math proved they should survive. Furthermore, forcing a Timer thread to iterate over an array of 200 coders every millisecond to check who should wake up next caused massive CPU waste.

**The Solution:** The implementation of a dedicated Sleep Room powered by a Min-Heap and a Zeno's Paradox Spinlock.

- *The Min-Heap*: Sleeping coders are pushed into a Min-Heap ordered by their wake-up times. This guarantees the Timer thread instantly knows exactly who wakes up next in O(1) time without scanning the entire system.

- *The Spinlock*: Instead of one massive usleep, the Timer thread pops the next coder from the Heap and continuously halves the remaining sleep time as it approaches the deadline. This defeats the OS minimum sleep lag, achieving microsecond precision without burning 100% of the CPU core on an active while loop.
