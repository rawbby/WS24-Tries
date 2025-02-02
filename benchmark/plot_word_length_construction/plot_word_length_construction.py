#!/usr/bin/env python3
import pandas as pd
import matplotlib.pyplot as plt

def plot_construction_time():
    # Read the CSV file that contains construction time data.
    df = pd.read_csv("plot_word_length_construction_time.csv")

    plt.figure(figsize=(10, 6))
    # Plot construction_time_ns vs. word_length for each variant.
    for variant in df["variant"].unique():
        sub = df[df["variant"] == variant]
        plt.plot(sub["word_length"], sub["construction_time_ns"], marker="o", label=variant)
    plt.xlabel("Word Length")
    plt.ylabel("Construction Time (ns)")
    plt.title("Construction Time vs. Word Length")
    plt.legend()
    plt.grid(True)
    plt.savefig("plot_word_length_construction_time.png")
    plt.close()
    print("Saved plot_word_length_construction_time.png")

def plot_construction_size():
    # Read the CSV file that contains construction size data.
    df = pd.read_csv("plot_word_length_construction_size.csv")

    plt.figure(figsize=(10, 6))
    # Plot construction_size vs. word_length for each variant.
    for variant in df["variant"].unique():
        sub = df[df["variant"] == variant]
        plt.plot(sub["word_length"], sub["construction_size"], marker="o", label=variant)
    plt.xlabel("Word Length")
    plt.ylabel("Construction Size")
    plt.title("Construction Size vs. Word Length")
    plt.legend()
    plt.grid(True)
    plt.savefig("plot_word_length_construction_size.png")
    plt.close()
    print("Saved plot_word_length_construction_size.png")

def main():
    plot_construction_time()
    plot_construction_size()

if __name__ == "__main__":
    main()
