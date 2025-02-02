#!/usr/bin/env python3
import os
import pandas as pd
import matplotlib.pyplot as plt

def plot_csv(csv_file, output_file, title):
    """
    Reads the CSV file, groups the data by 'variant', and plots
    word_length (x-axis) vs. query_time_ns (y-axis). Saves the plot to output_file.
    """
    try:
        df = pd.read_csv(csv_file)
    except Exception as e:
        print(f"Error reading {csv_file}: {e}")
        return

    df = df.iloc[:, :3]
    df.columns = ["word_length", "variant", "query_time_ns"]

    plt.figure(figsize=(10, 6))
    # Plot query_time_ns vs word_length for each variant.
    for variant in df["variant"].unique():
        sub = df[df["variant"] == variant]
        plt.plot(sub["word_length"], sub["query_time_ns"], marker="o", label=variant)

    plt.xlabel("Word Length")
    plt.ylabel("Query Time (ns)")
    plt.title(title)
    plt.legend()
    plt.grid(True)
    plt.savefig(output_file)
    plt.close()
    print(f"Plot saved as {output_file}")

def main():
    # List of CSV files produced by your benchmark function
    # and the corresponding titles for the plots.
    files = [
        ("plot_word_length_insert_already_inserted.csv", "Word Length: Insert Already Inserted"),
        ("plot_word_length_insert_random.csv", "Word Length: Insert Random"),
        ("plot_word_length_contain_already_inserted.csv", "Word Length: Contain Already Inserted"),
        ("plot_word_length_contain_random.csv", "Word Length: Contain Random"),
        ("plot_word_length_remove_already_inserted.csv", "Word Length: Remove Already Inserted"),
        ("plot_word_length_remove_random.csv", "Word Length: Remove Random"),
    ]

    for csv_file, title in files:
        # Create an output PNG filename based on the CSV filename.
        output_file = os.path.splitext(csv_file)[0] + ".png"
        plot_csv(csv_file, output_file, title)

if __name__ == "__main__":
    main()
