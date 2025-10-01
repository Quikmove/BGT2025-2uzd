#include <cstdlib>
inline int draw_konstitucija_results() {
  int code = std::system("uv run draw_konstitucija_chart.py");
  return code;
}