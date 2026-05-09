*This project has been created as part of the 42 curriculum by obahya.*
# Codexion

Master the race for resources before the deadline masters you.

Codexion is a concurrency and multi-threading project that simulates a group of coders competing for shared USB dongles to compile their code. It is a modern, strictly-timed variation of Dijkstra's classic Dining Philosophers problem, implementing both First-In-First-Out (FIFO) and Earliest Deadline First (EDF) scheduling.

## \# Instructions

### Compilation & Installation

The project is written in standard C and utilizes a Makefile for compilation. It strictly compiles with -Wall -Wextra -Werror and links the POSIX threads library (-pthread).

To install and compile the project, run the following commands in your terminal:

```
# Clone the repository
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

- `required_compiles`: Simulation stops if all coders reach this number.

- `dongle_cooldown`: Mandatory wait time (in ms) before a released dongle can be taken again.

- `scheduler_type`: The arbitration policy (fifo or edf).

**Example Usage:**

To run a simulation with 3 coders, a 300ms burnout limit, 100ms debug time, 100ms cooldown, requiring 3 compiles each, using Earliest Deadline First scheduling:

	./codexion 3 300 0 100 0 3 100 edf

## \# The Survival Math (Minimum Burnout Time)

To guarantee that no Coder starves, the simulation parameters must satisfy strict mathematical thresholds. Because each Coder requires two dongles, a maximum of `⌊N/2⌋` Coders can compile simultaneously. This forces the Global Arbiter to divide the room into "Execution Shifts."

Whether the simulation can run infinitely without a burnout depends on two independent bottlenecks: **The Hardware Topology** and **The Lifecycle Minimum**.

### 1. The Hardware Topology (The Shift Bottleneck)
The `time_to_burnout` must be large enough to allow every other shift in the room to finish their compiles and hardware cooldowns before a starving coder's timer reaches zero.

* **For an EVEN number of Coders (2 Shifts):**
    With an even number of coders, the room divides perfectly into Group A and Group B. Group A compiles while Group B waits. Therefore, a coder must survive long enough for two full compile cycles to complete.
    > `time_to_burnout >= 2 * (time_to_compile + dongle_cooldown)`

* **For an ODD number of Coders (3 Shifts):**
    With an odd number of coders, the geometry of the room prevents a perfect 50/50 split. A single dongle will always be left unused during the first two shifts, requiring a mandatory 3rd shift to cycle the "leftover" coder.
    > `time_to_burnout >= 3 * (time_to_compile + dongle_cooldown )`

### 2. The Lifecycle Minimum (The Sleep & Refactor Bottleneck)
A Coder's burnout timer does not reset until they successfully *start* their next compile. Therefore, they must mathematically be able to survive their current compile, their mandatory debug sleep, and their refactor phase before they can even attempt to queue up for hardware again.
> `time_to_burnout >= time_to_compile + time_to_debug + time_to_refactor`

### EDF Arbitration & T=0 Stagger

Standard simulations suffer from OS scheduler drift, requiring inflated burnout times. Codexion eliminates this using an **Earliest Deadline First (EDF) Priority Queue**. The Waiter dynamically assigns hardware to the thread closest to burnout, allowing the simulation to operate safely at strict mathematical minimums.

**The T=0 Stagger Optimization (Maximum Throughput)**
At the exact start of the simulation (`T=0`), every single coder hits the queue with the exact same EDF priority (0 compiles done, and identical burnout deadlines). If all `N` coders request hardware simultaneously, it creates a massive "Thundering Herd" CPU collision, even though only `⌊N/2⌋` coders can physically compile. 

Because EDF allows us to arbitrarily break ties between identical priorities, we have the mathematical right to manually shape the first shift. To achieve maximum throughput and eliminate initial lock contention, the engine applies a **Staggered Start**. 

Before entering the queue for the first time, all even-numbered coders are forced to yield and sleep for:
> `(time_to_compile + dongle_cooldown) / 2`

**The Mathematical Proof:**
* **Shift A** (odd coders) takes the hardware at `T=0`. They will finish compiling and release the hardware at exactly `T = time_to_compile`.
* By forcing **Shift B** (even coders) to sleep for `(time_to_compile + dongle_cooldown) / 2`, we place their wake-up time at the exact midpoint of the system's operational cycle. 
* This mathematical offset guarantees that Shift B never requests hardware while Shift A is holding it, creating a perfect, zero-collision rhythm.

This desynchronizes threads into flawless alternating execution shifts. Group A instantly acquires hardware with zero contention, while Group B wakes up and queues at the exact moment Group A releases their dongles.

## \# Resources

**Classic References**

This project relies heavily on low-level operating system concepts and algorithmic theory.  Below are the foundational concepts and documentation referenced during development:
- The Dining Philosophers Problem: Edsger W. Dijkstra's original concurrency problem, which serves as the theoretical foundation for this project's resource-sharing challenge.
- Youtube play list explaining Threads: [Playlist](https://www.youtube.com/playlist?list=PLfqABt5AS4FmuQf70psXrsMLEDQXNkLq2)
- OS - Three Easy Pieces book, Concurency Part II page 280 [PDF](https://raw.githubusercontent.com/Areadrill/HaPOS/master/Operating%20Systems%20-%20Three%20Easy%20Pieces.pdf)
- POSIX Threads (pthread) Documentation: Linux manual pages for `pthread_create`, `pthread_join`, `pthread_mutex_lock`, `pthread_mutex_init`, `pthread_cond_init`, `pthread_cond_wait`, and `pthread_cond_broadcast`.

**AI Usage Declaration**
- **Architectural Auditing**: Used AI to perform strict, line-by-line synchronization audits, and ensuring Single Responsibility Principles for all POSIX mutexes.
- **Deep-OS Debugging**: Assisted in diagnosing Linux kernel-level threading phenomena, including Thread Creation Lag, Context-Switching storms, and analyzing lock-free atomic assembly interactions to resolve ThreadSanitizer/Helgrind false positives.
- **Documentation**: Assisting in structuring and formatting this README.md.

## \# Visual Program Structure


The program abandons the traditional "philosophers" array model in favor of a highly scalable **Master-Worker Architecture**. It is structured around a central Waiter (Global Arbiter), a hardware-accurate Timer (Sleep Room), an executioner (Monitor), and the Coders who compete for CPU time.

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

┌─────────────────────────────────────────────────────────────────────┐
│ WAITER THREAD (The Global Arbiter)                                  │
│ 1. Yields CPU      -> Empty queue or dongel in use or in cooldown.  │
│ 2. Full Sweep      -> Traverses the queue in a single O(N) pass.    |
|                       reserve dongsles for high priority coders,    |
|                       in case of unavalable dongles.                │
│ 3. Assign Hardware -> Checks cooldowns, maps dongles.               │
│ 4. Wake Coders     -> Signals specific `queue_cond` for ready nodes.│
└─────────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────────┐
│ TIMER THREAD (The Sleep Room)                                       │
│ 1. Poll Min-Heap   -> Checks the root node for the next wake-up.    │
| 2. Sleep           -> Empty heap or the coder at the root           |
|                       has `wake_up_time` > `current_time`           |
│ 3. Zeno's Spinlock -> Halves `usleep` continuously to beat OS lag.  │
│ 4. Evict & Wake    -> Pops the node and broadcasts `sleep_cond`.    │
└─────────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────────┐
│ MONITOR THREAD (The Executioner)                                    │
│ 1. Isolate State   -> Locks `state_mutex` to safely read deadlines. │
│ 2. End simulation  -> If `current_time > deadline` or               |
|                       all coders done the required compiles,        |
|                       then sets active = 0.                         │
│ 3. Fire Alarm      -> Broadcasts shutdown to Waiter, Timer & Coders.│
└─────────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────────┐
│ CODER THREADS (The Workers x N)                                     │
│ 1. Request dongles -> Yields to Waiter, sleeps on `queue_cond`.     │
│ 3. Release dongles -> Drops dongles, signals Waiter to re-evaluate. │
│ 2. Sleep           -> Yields to Timer, sleeps on `sleep_cond`.      │
└─────────────────────────────────────────────────────────────────────┘
```

## \# Thread Synchronization Mechanisms

Thread-safe communication is achieved using tightly scoped POSIX Mutexes and Condition Variables. Every lock follows the Single Responsibility Principle to prevent "Scope Bleed" and micro-delays.

- **The Queue Mutex (queue_mutex)**: Protects the Master's Arbitration Queue and dongle states (in_use, available_at). It strictly guards the hardware resource mapping and prevents queue corruption.

- **The State Mutex (state_mutex)**: The absolute source of truth for the is_active and coders_remaining flags. By isolating this flag from the I/O and Queue mutexes, the Monitor can instantly shut down the simulation without being blocked by a slow thread writing to the terminal.

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
