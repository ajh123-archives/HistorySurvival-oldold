
# History Survival

A simple 3d multiplayer game using
* [raylib](https://github.com/raysan5/raylib/tree/master/examples)!
* [entt](https://github.com/skypjack/entt)

## Run and build Locally with [CMAKE](cmake.org)

Clone the project

```bash
  git clone --recurse-submodules https://github.com/ajh123-development/HistorySurvival
```

Go to the project directory

```bash
  cd HistorySurvival
```

Compile and run the client
```bash
  mkdir build
  cd build
  cmake ..
  cmake --build .

  #Runing
  cd ..
  ./build/game-client
  #Or on windows use if using vs 2022 or > as the cmake generator
  ./build/Debug/game-client
```

Compile and run the server
```bash
  mkdir build
  cd build
  cmake -D BUIlD_SERVER=ON ..
  cmake --build .

  #Runing
  cd ..
  ./build/game-server
  #Or on windows use if using vs 2022 or > as the cmake generator
  ./build/Debug/game-server
```

## Authors

- [@ajh123](https://www.github.com/ajh123)

