#include <cstdlib>
#include <iostream>
int main() {
    int code = std::system("uv run draw_konstitucija_chart.py");
    if(code !=0 ) {
        std::cout << "something went wrong. error code: " << code << '\n';
    } 
    return code;
}