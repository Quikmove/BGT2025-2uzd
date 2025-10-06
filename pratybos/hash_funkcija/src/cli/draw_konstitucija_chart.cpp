#include <draw.h>
#include <cstdlib>
// Uses Matplot++ to draw the same chart that the Python script produced.
// Reads a space-separated two-column file (Lines Time) at
// `results/konstitucija.txt` and writes `results/konstitucija.png`.

int draw_konstitucija_results() {
  return system("uv run draw_konstitucija_chart.py");
}
