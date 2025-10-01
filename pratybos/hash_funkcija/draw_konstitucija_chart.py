import matplotlib.pyplot as plt
import pandas as pd
import os
import pathlib as pl
data_path = pl.Path("results/konstitucija.txt")
output_path = pl.Path("results/konstitucija.png")
if not data_path.exists():
    print("no file found")

    exit(1)
df = pd.read_csv(data_path, sep=" ", header=None, names=["Lines", "Time"])

plt.plot(df['Lines'], df['Time'], marker="o", linestyle="-")
plt.xlabel("Line count")
plt.ylabel("Time (s)")
plt.grid(True)
plt.savefig(output_path)