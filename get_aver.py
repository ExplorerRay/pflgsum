#!/usr/bin/env python3

import sys
import subprocess
import statistics

def main():
    if len(sys.argv) != 3:
        print("Usage: ./get_aver.py <node_num> <iter>")
        sys.exit(1)

    node_num = int(sys.argv[1])
    iter_num = int(sys.argv[2])

    times = []

    for i in range(iter_num):
        result = subprocess.run(['srun', '-N', str(node_num), '-p', 'cc1', './pflgsum', 'test_cases/de_maillog'], capture_output=True, text=True)
        if result.returncode != 0:
            print(f"Error running srun: {result.stderr}")
            sys.exit(1)
        times.append(float(result.stdout.splitlines()[0]))

    total_time = sum(times)
    average_time = statistics.mean(times)
    stdev_time = statistics.stdev(times) if len(times) > 1 else 0.0

    # Identify outliers (using 2 standard deviations from the mean as a simple heuristic)
    outliers = [time for time in times if abs(time - average_time) > 2 * stdev_time]

    # Filter out the outliers
    filtered_times = [time for time in times if time not in outliers]
    filtered_total_time = sum(filtered_times)
    filtered_average_time = statistics.mean(filtered_times) if filtered_times else 0.0
    filtered_stdev_time = statistics.stdev(filtered_times) if len(filtered_times) > 1 else 0.0

    print(f"Total time: {total_time}")
    print(f"Average time: {average_time}")
    print(f"Standard deviation: {stdev_time}")
    print(f"Outliers: {outliers}")
    print(f"Filtered total time: {filtered_total_time}")
    print(f"Filtered average time: {filtered_average_time}")
    print(f"Filtered standard deviation: {filtered_stdev_time}")

if __name__ == "__main__":
    main()
