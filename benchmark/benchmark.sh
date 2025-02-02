#!/bin/bash
# This script:
#   1. Stops TLP to disable power saving.
#   2. Sets all CPU cores to the "performance" governor.
#   3. Uses taskset to bind the benchmark process to a specified core.
#   4. Runs the benchmark with high (real-time) scheduling priority.
#   5. Restarts TLP when finished.
#
# Requirements:
#   - cpupower (sudo pacman -S cpupower)
#   - TLP installed (if you use it)
#   - taskset is part of util-linux.
#
# Adjust the CORE variable below to select the core(s) to bind.

# Stop TLP (if enabled)
echo "Stopping TLP..."
sudo systemctl stop tlp

# Set CPU frequency governor to performance for all cores.
echo "Setting CPU governor to performance..."
sudo cpupower frequency-set --governor performance

# Specify the CPU core(s) to bind to. (For example, core 1.)
CORE="1"
echo "Running benchmark on core(s): $CORE"

# Run the benchmark using taskset to bind to the specified core.
# Use chrt to set real-time priority (SCHED_FIFO with priority 99).
# Adjust the command "./benchmark" if your executable is in a different location.
taskset -c $CORE sudo chrt -f 99 ./benchmark

# Restart TLP after benchmark
echo "Benchmark complete. Restarting TLP..."
sudo systemctl start tlp

echo "Done."
