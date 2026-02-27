# Codexion

Master the race for resources before the deadline masters you.

Codexion is a concurrency and multi-threading project that simulates a group of coders competing for shared USB dongles to compile their code. It is a modern, strictly-timed variation of Dijkstra's classic Dining Philosophers problem, implementing both First-In-First-Out (FIFO) and Earliest Deadline First (EDF) scheduling.

## 🚀 Instructions

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

## 📚 Resources

**Classic References**

This project relies heavily on low-level operating system concepts and algorithmic theory.  Below are the foundational concepts and documentation referenced during development:
- The Dining Philosophers Problem: Edsger W. Dijkstra's original concurrency problem, which serves as the theoretical foundation for this project's resource-sharing challenge.
- POSIX Threads (pthread) Documentation: Linux manual pages for `pthread_create`, `pthread_join`, `pthread_mutex_lock`, `pthread_mutex_init`, `pthread_cond_init`, `pthread_cond_wait`, and `pthread_cond_broadcast`.
- Youtube play list explaining Threads: [link](www.youtube.com/playlist?list=PLfqABt5AS4FmuQf70psXrsMLEDQXNkLq2)

**AI Usage Declaration**
- **Debugging Hardware-Level Concurrency**: Identifying and explaining deep OS-level phenomena such as "Thread Creation Lag" and "Deadlock by Silence," and providing the theoretical solutions.
- **Documentation**: Assisting in structuring and formatting this README.md.

## 🧠 Visual Program Structure

The program is structured around a central Global Arbiter that manages resource distribution, a Monitor that acts as the executioner, and the Coders who compete for time.

```
[ MAIN THREAD ]
      │
      ├─► Parses Arguments & Initializes t_sim, t_coder, and t_dongle arrays.
      │
      ├─► Spawns Coder Threads (1 to N)
      │     └─► Coders wait at the `start_cond` barrier.
      │
      ├─► Spawns Monitor Thread
      │     └─► Monitor waits at the `start_cond` barrier.
      │
      └─► Records `start_time`, sets `threads_ready = 1`, and fires `start_cond` Broadcast!
            │
            ▼
[ THE SIMULATION LOOP ]
      ┌─────────────────────────┐             ┌─────────────────────────┐
      │ MONITOR THREAD          │             │ CODER THREADS (x N)     │
      │ 1. Lock state_mutex     │             │ 1. Lock state_mutex     │
      │ 2. Read timestamps      │◄──(Reads)───┤ 2. Update timestamps    │
      │ 3. Check for burnout    │             │ 3. Enter Global Queue   │
      │ 4. If dead: set active=0│             │ 4. Wait for Arbiter     │
      │ 5. Broadcast to Arbiter │──(Wakes)───►│ 5. Compile / Debug      │
      └─────────────────────────┘             └───────────┬─────────────┘
                                                          │
                                                          ▼
                                              ┌─────────────────────────┐
                                              │ THE GLOBAL ARBITER      │
                                              │ - Sorts by EDF / FIFO   │
                                              │ - Checks Cooldowns      │
                                              │ - Grants Both Dongles   │
                                              └─────────────────────────┘
```

## ⚙️ Thread Synchronization Mechanisms

Thread-safe communication between the Coders, the Monitor, and the Shared Resources is achieved using POSIX Mutexes and Condition Variables to prevent data races and minimize CPU overhead.

- **The State Mutex (state_mutex):** Protects all read/write operations to critical simulation variables, including the is_active flag, the global priority queue, and each coder's last_compile_start timestamp. This ensures the Monitor never reads a partially updated timestamp while a coder is modifying it.

- **The Write Mutex (write_mutex):** Serializes all terminal output. A thread must acquire this lock before printing to stdout, ensuring that timestamped logs never interleave or corrupt each other.

- **The Thread Barrier (start_cond):**  A condition variable used to synchronize the launch of all threads. To prevent "Thread Creation Lag," all newly spawned threads sleep on this condition variable until the main thread has fully initialized the environment and sends a global broadcast to wake them simultaneously.

- **The Global Arbiter (arbiter_cond):** A single condition variable that manages the sleep/wake cycles of all coders. Instead of coders polling the CPU, they sleep on this variable. Whenever a coder releases dongles, they broadcast a signal to wake the queue so the next coder can evaluate their priority and the dongle cooldowns.

## 🛡️ Blocking Cases Handled (Deadlock & Starvation Prevention)

This simulation successfully prevents all forms of deadlocks and thread starvation through a centralized arbitration system.

- **Hold-and-Wait Prevention:** Coders are strictly forbidden from acquiring a single dongle and waiting for the second. The Global Arbiter enforces an atomic `take_both_dongles` policy. A coder only locks the dongle mutexes if both are free and off cooldown; otherwise, they yield.

- **Earliest Deadline First (EDF) Tie-Breakers:** To prevent starvation when two coders have the exact same burnout deadline (a common occurrence at T=0), the priority queue falls back to evaluating `compiles_done`. The coder with the fewest completed compiles is placed ahead in the queue, forcing aggressive threads to yield to starving threads.

- **The 1-Coder Edge Case:** Handled gracefully by forcing a single coder to take their only available dongle and yield the thread, waiting for the inevitable burnout without attempting to double-lock the same mutex.

## 🚧 The Journey: Problems Encountered & Solved

Building a mathematically perfect, microsecond-accurate concurrency system revealed several hardware-level ghosts and logical traps. Here is how they were solved:

### 1. The Odd-Number Topology Trap & Empty-Queue Race

**The Problem:** Initially, deadlocks were avoided using an "Even/Odd" acquisition logic (evens grab right, odds grab left). However, with an odd number of coders (e.g., 3), this created an asymmetric topology where two coders fought over the same initial dongle while another dongle was completely ignored, leading to rapid starvation. Furthermore, coders were racing for individual dongle queues. A coder could lock an empty queue and steal a dongle before the EDF scheduler even had a chance to compare them against a starving coder.

**The Solution:** The architecture was rewritten to use a Global Arbiter. Individual dongle queues were destroyed. All coders now enter a single global priority queue. No one is allowed to touch a dongle until the Arbiter confirms they are the highest priority coder whose required resources are free.

### 2. Thread Creation Lag

**The Problem:** When compile/debug/refactor times were set to 0, the OS took longer to physically allocate memory for Coder 2's thread than it took Coder 1 to execute its entire lifecycle. Coder 1 would grab the dongles, compile, release them, and queue up for round 2 before Coder 2 even existed in the OS scheduler.

**The Solution:** Implementation of the "Starting Gun." Threads are now forcefully halted using a `pthread_cond_wait` barrier immediately after creation. They are only allowed to proceed when the main thread sends a `pthread_cond_broadcast`, guaranteeing a perfectly synchronized T=0 start time.

### 3. Cooldown Deadlocks & Zombie Threads

**The Problem:** If a coder woke up, saw the dongles were free but still on a mandatory cooldown timer, they would go back to sleep via pthread_cond_wait. Because the dongles were no longer held by anyone, no thread would ever broadcast a wake-up signal when the cooldown timer naturally expired, resulting in permanent deadlocks. Additionally, if the monitor stopped the simulation while coders were sleeping, the coders never woke up to see the exit flag, creating zombie threads.

**The Solution:** 1. The monitor thread was updated to broadcast a final wake-up signal to the Global Arbiter before exiting.
2. The Arbiter logic was updated to use a Cooldown Polling mechanism. If a coder is highest priority and the dongles are free but on cooldown, the coder uses usleep(500) to step out of the queue briefly and check the clock, rather than going into a deep cond_wait sleep.

### 4. The Razor's Edge (Hardware Lag)

**The Problem:** If the time_to_burnout perfectly matched the sum of the cycle times (e.g., 300ms burnout with a 100ms cycle rate), a coder would die at exactly 300ms. The Monitor thread would wake up at T=300 and instantly evaluate the burnout as true before the coder thread had the microsecond needed to lock the mutex and update its timestamp.

**The Solution:** The burnout condition was adjusted from >= to >. This provides a 1-millisecond grace period, perfectly absorbing the hardware-level context-switching lag and allowing the coder thread to register its compilation right on the mathematical deadline.