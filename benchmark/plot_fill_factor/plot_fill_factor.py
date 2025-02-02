#!/usr/bin/env python3
import pandas as pd
import matplotlib.pyplot as plt

def plot_fill_factor(csv_file, output_file, title):
    """
    Reads the CSV file, plots query_time_ns vs. num_words for each variant,
    and saves the plot to output_file.
    """
    try:
        df = pd.read_csv(csv_file)
    except Exception as e:
        print(f"Error reading {csv_file}: {e}")
        return

    plt.figure(figsize=(10, 6))
    # Plot a line for each variant.
    for variant in df['variant'].unique():
        sub = df[df['variant'] == variant]
        plt.plot(sub['num_words'], sub['query_time_ns'], marker='o', label=variant)
    plt.xlabel("Number of Words")
    plt.ylabel("Query Time (ns)")
    plt.title(title)
    plt.legend()
    plt.grid(True)
    plt.savefig(output_file)
    plt.close()
    print(f"Plot saved as {output_file}")

def main():
    # Define the list of CSV files, output PNG filenames, and titles.
    experiments = [
        ("plot_fill_factor_insert.csv", "plot_fill_factor_insert.png", "Fill Factor: Insert Benchmark"),
        ("plot_fill_factor_contains.csv", "plot_fill_factor_contains.png", "Fill Factor: Contains Benchmark"),
        ("plot_fill_factor_remove.csv", "plot_fill_factor_remove.png", "Fill Factor: Remove Benchmark")
    ]

    for csv_file, output_file, title in experiments:
        plot_fill_factor(csv_file, output_file, title)

if __name__ == "__main__":
    main()
