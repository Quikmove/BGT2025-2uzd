import matplotlib.pyplot as plt
import pandas as pd
import pathlib as pl
import math
data_path = pl.Path("results/konstitucija.txt")
output_path = pl.Path("results/konstitucija.png")
if not data_path.exists():
    print("no file found")
    exit(1)
df = pd.read_csv(data_path, sep=" ", header=None, names=["Lines", "Time"])
plt.plot(df['Lines'], df['Time'], marker="o", linestyle="-", label="konstitucija.txt")
plt.xlabel("Eilučių skaičius")
plt.ylabel("Laikas")
plt.title("Konstitucija.txt eilučių skaičiaus hešavimo laikas (5 avg.)")
plt.legend()
plt.grid(True)
plt.savefig(output_path)