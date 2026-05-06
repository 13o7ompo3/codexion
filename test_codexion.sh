#!/bin/bash

# EXHAUSTIVE Automation Script for ./codexion
# Goal: Exit Codes, Memory Leaks, Data Races, and Stress Tests.

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'
BINARY="./codexion"

if [ ! -f "$BINARY" ]; then
    echo -e "${RED}[FATAL] $BINARY not found. Run 'make' first.${NC}"
    exit 1
fi

check_command() {
    command -v "$1" >/dev/null 2>&1
}

# Helper: Check Exit Codes and Crashes
run_test() {
    local test_name="$1"
    local args="$2"
    local expected_exit="$3"
    local to="$4"
    
    echo -ne "${YELLOW}Testing: ${test_name}${NC} [Args: $args] ... "
    
    output=$(timeout "$to" $BINARY $args 2>&1)
    local exit_code=$?

    if [ $exit_code -eq 124 ]; then
        echo -e "${RED}[FAIL] TIMEOUT / DEADLOCK!${NC} Hung for $to seconds."
    elif [ $exit_code -eq 139 ] || [ $exit_code -eq 11 ]; then
        echo -e "${RED}[FAIL] SEGMENTATION FAULT!${NC}"
    elif [ $exit_code -eq 134 ] || [ $exit_code -eq 6 ]; then
        echo -e "${RED}[FAIL] ABORT / DOUBLE FREE!${NC}"
    elif [ $exit_code -ne "$expected_exit" ]; then
        echo -e "${RED}[FAIL] WRONG EXIT CODE!${NC} Expected $expected_exit, got $exit_code."
    else
        echo -e "${GREEN}[PASS]${NC}"
    fi
}

# Helper: Valgrind Memory Leak Check
run_valgrind() {
    local test_name="$1"
    local args="$2"
    echo -ne "${CYAN}MemCheck:${NC} ${test_name} ... "
    
    if ! check_command valgrind; then
        echo -e "${YELLOW}[SKIP] Valgrind not installed.${NC}"
        return
    fi

    valgrind_out=$(timeout 15 valgrind --leak-check=full --errors-for-leak-kinds=all --error-exitcode=42 $BINARY $args 2>&1)
    local v_exit=$?

    if [ $v_exit -eq 124 ]; then
        echo -e "${RED}[FAIL] TIMEOUT inside Valgrind.${NC}"
    elif [ $v_exit -eq 42 ]; then
        echo -e "${RED}[FAIL] MEMORY LEAK DETECTED!${NC}"
    else
        echo -e "${GREEN}[PASS] No Leaks.${NC}"
    fi
}

# Helper: Check for Burnout (Death) or Survival
run_burnout_test() {
    local test_name="$1"
    local args="$2"
    local expect_death="$3"
    local to=5
    
    echo -ne "${YELLOW}BurnoutCheck:${NC} ${test_name} ... "
    
    output=$(timeout "$to" $BINARY $args 2>&1)
    local exit_code=$?

    if [ $exit_code -eq 124 ]; then
        echo -e "${RED}[FAIL] TIMEOUT / DEADLOCK!${NC} Hung for $to seconds."
        return
    fi

    # Check if the output contains standard death/burnout messages
    if echo "$output" | grep -iqE "died|dead|burnout|burned out"; then
        if [ "$expect_death" -eq 1 ]; then
            echo -e "${GREEN}[PASS] Coder successfully burned out.${NC}"
        else
            echo -e "${RED}[FAIL] UNEXPECTED DEATH!${NC} Everyone should have survived."
        fi
    else
        if [ "$expect_death" -eq 1 ]; then
            echo -e "${RED}[FAIL] NO BURNOUT DETECTED!${NC} A coder should have mathematically died."
        else
            echo -e "${GREEN}[PASS] All coders survived.${NC}"
        fi
    fi
}

echo "=========================================================="
echo " PHASE 1: EXIT CODES & PARSING"
echo "=========================================================="
run_test "Not enough args" "5 800 200" 1 3
run_test "Too many args" "5 800 200 200 200 5 100 edf extra" 1 3
run_test "Negative coders" "-5 800 200 200 200 5 100 edf" 1 3
run_test "Zero coders" "0 800 200 200 200 5 100 edf" 1 3
run_test "Bad Scheduler" "5 800 200 200 200 5 100 roundrobin" 1 3
run_test "Missing Scheduler" "5 800 200 200 200 5 100" 1 3
run_test "Non-integer argument" "5 800a 200 200 200 5 100 edf" 1 3
run_test "INT_MAX Overflow" "2147483648 800 200 200 200 5 100 edf" 1 3
run_test "Valid Args (Quick Exit)" "3 500 50 50 50 2 50 edf" 0 5

echo "=========================================================="
echo " PHASE 2: TOPOLOGY & DEADLOCK CHECKS"
echo "=========================================================="
run_test "The '1 Coder' Trap" "1 300 200 200 200 5 100 edf" 0 5
run_test "Zero Execution Times" "5 100 0 0 0 10 10 edf" 0 5
run_test "Immediate Death" "5 100 200 200 200 5 100 edf" 0 5
run_test "Zero Cooldown (Hot Swap)" "4 410 200 200 0 2 0 fifo" 0 5
run_test "Zero Required Compiles" "5 800 200 200 200 0 100 edf" 0 5
run_test "Single Compile Requirement" "4 500 50 50 50 1 10 edf" 0 5

echo "=========================================================="
echo " PHASE 3: VALGRIND MEMORY LEAKS"
echo "=========================================================="
run_valgrind "Parser Fail Leak Check" "5 800 200"
run_valgrind "Standard Execution Leak Check" "3 500 50 50 50 2 50 edf"
run_valgrind "Immediate Death Leak Check" "5 100 200 200 200 5 100 edf"

echo "=========================================================="
echo " PHASE 4: STRESS TEST (10 ITERATIONS)"
echo "=========================================================="
echo -e "${YELLOW}Running Avalanche test 10 times to catch rare context-switch races...${NC}"
STRESS_PASS=1
for i in {1..10}; do
    timeout 5 $BINARY 5 100 0 0 0 10 10 edf > /dev/null 2>&1
    if [ $? -ne 0 ]; then
        echo -e "${RED}[FAIL] Stress Test crashed or hung on iteration $i!${NC}"
        STRESS_PASS=0
        break
    fi
done
if [ $STRESS_PASS -eq 1 ]; then
    echo -e "${GREEN}[PASS] Survived 10 high-contention iterations.${NC}"
fi

echo "=========================================================="
echo " PHASE 5: BURNOUT / SURVIVAL CHECKS"
echo "=========================================================="
# 1 coder, needs 2 dongles. Mathematically doomed.
run_burnout_test "1 Coder Starvation" "1 300 200 200 200 5 100 edf" 1
# 3 coders, 1 can compile at a time (200ms). Coder 3 will wait 400ms. Burnout is 300ms.
run_burnout_test "Mathematical Starvation (3 Coders)" "3 300 200 100 100 5 100 edf" 1
# Compile takes 500ms, Burnout is 200ms. Instant death while holding dongles.
run_burnout_test "Immediate Death (Burnout < Compile)" "5 200 500 100 100 5 100 edf" 1
# Plentiful time, everyone should easily survive their 3 required compiles.
run_burnout_test "Comfortable Survival" "5 800 200 100 100 3 50 edf" 0
run_burnout_test "Tight but Survivable" "4 410 200 100 100 2 0 fifo" 0

echo "=========================================================="
echo " PHASE 6: MATHEMATICAL BURNOUT EDGE CASES"
echo "=========================================================="
# Edge Case 1: Lifecycle Dominant (t_burnout vs t_compile + t_debug + t_refactor)
# N=4, t_comp=50, t_debug=100, t_refactor=100. Cycle = 250. W=2, Contention = 120. 
# Burnout must exceed 250. Delay = 4/10 = 0ms.
run_burnout_test "Lifecycle Limit Survival" "4 255 50 100 100 3 10 edf" 0
run_burnout_test "Lifecycle Limit Death" "4 245 50 100 100 3 10 edf" 1

# Edge Case 2: Contention Dominant - Even Coders (W = 2)
# N=4, t_comp=100, t_cool=10. W*(110) = 220. Cycle = 200. Delay = 0ms.
run_burnout_test "Contention Limit Survival (Even N=4)" "4 225 100 50 50 3 10 edf" 0
run_burnout_test "Contention Limit Death (Even N=4)" "4 215 100 50 50 3 10 edf" 1

# Edge Case 3: Contention Dominant - Odd Coders (W = 3)
# N=5, t_comp=100, t_cool=10. W*(110) = 330. Cycle = 200. Delay = 0ms.
run_burnout_test "Contention Limit Survival (Odd N=5)" "5 335 100 50 50 3 10 edf" 0
run_burnout_test "Contention Limit Death (Odd N=5)" "5 325 100 50 50 3 10 edf" 1

# Edge Case 4: High Contention & OS Context-Switch Delay (1ms per 10 coders)
# N=41 (Odd -> W=3). t_comp=50, t_cool=10. Contention = 3 * 60 = 180.
# Delay Buffer = 41 / 10 = 4ms. Target Survival Threshold = 180 + 4 = 184.
run_burnout_test "OS Delay Buffer Survival (41 coders, 4ms delay)" "41 185 50 25 25 3 10 edf" 0
run_burnout_test "OS Delay Buffer Death (41 coders, starved)" "41 179 50 25 25 3 10 edf" 1

echo "=========================================================="
echo " PHASE 7: LARGE SCALE & SCHEDULER COMPARISONS"
echo "=========================================================="
run_test "Massive Scale (100 Coders - EDF)" "100 1000 20 20 20 2 10 edf" 0 10
run_test "Massive Scale (100 Coders - FIFO)" "100 1000 20 20 20 2 10 fifo" 0 10
run_burnout_test "Massive Scale Starvation (150 Coders)" "150 100 50 50 50 3 10 edf" 1
run_burnout_test "Infinite Compiles but Starves" "3 300 200 100 100 100 100 edf" 1
run_valgrind "Massive Scale Leak Check (50 Coders)" "50 800 20 20 20 1 10 edf"

echo "=========================================================="
echo " TESTING COMPLETE"
echo "=========================================================="
