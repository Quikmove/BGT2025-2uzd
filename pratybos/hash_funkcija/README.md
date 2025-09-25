# Hešavimo funkcija

## Rekvizitai

- C++ kompiliatorius, palaikantis C++17 standartą
- CMake

## Paleidimas

`# Unix/MacOS`
- `mkdir -p build/`
- `cmake ..`
- `cmake --build .`
- `./main [--input <input> | --file <file_path>]`


`# Windows(netestuota, ymmw)`
- `dir build/`
- `cd build/`
- `cmake ..`
- `cmake --build .`
- `main.exe [--input <input> | --file <file_path>]`


`# Pavyzdys`
- `./main --input a`
- `./main --file ../konstitucija.txt`

## Algoritmo idėja
- ### Pipeline
(*incomplete*)
- input -> data = (input.size() > 512bit ? suspaudimas(input, 512bit) : ispletimas(input, 512bit))

