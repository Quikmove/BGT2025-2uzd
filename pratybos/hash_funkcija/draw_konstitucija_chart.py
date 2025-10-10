import matplotlib.pyplot as plt
import pandas as pd
import pathlib as pl
data_path = pl.Path("results/konstitucija.txt")
output_path = pl.Path("results/konstitucija.png")
if not data_path.exists():
    print("no file found")
    exit(1)
df = pd.read_csv(data_path, sep="\s+", engine="python")
line_counts = df.iloc[:, 0]
for column in df.columns[1:]:
    plt.plot(line_counts, df[column], marker="o", linestyle="-", label=column)
plt.xlabel("Eilučių skaičius")
plt.ylabel("Laikas (s)")
plt.title("Konstitucija.txt hešavimo laikas (5 vidurkiai)")
plt.legend(title="Algoritmas")
plt.grid(True)
plt.savefig(output_path)