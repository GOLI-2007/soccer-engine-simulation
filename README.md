# ⚽ Soccer Engine Simulation

A 2D football game simulation developed in C, featuring a 6v6 match environment, custom game logic, and autonomous player behavior.

This project was developed as part of a university programming assignment, with a focus on understanding game engine architecture, low-level C programming, and AI-based decision-making.

## 🎮 Project Overview

Soccer Engine is a lightweight football simulation built using SDL2. It provides the core game engine, including player and ball entities, rendering, physics, and match management.

The project focuses on two important parts of the game logic:

- **Referee System:** Responsible for enforcing game rules and validating player actions.
- **Coach System:** Responsible for player behavior and team decision-making.

## 🛠️ Technologies

- **Programming Language:** C (C99)
- **Graphics:** SDL2, SDL_ttf, SDL_image
- **Build System:** CMake
- **Physics:** Custom 2D vector-based kinematics

## 📂 Project Structure

```text
.
├── cmake/
├── engine/
│   ├── assets/
│   ├── core/
│   ├── entities/
│   ├── game/
│   ├── graphics/
│   └── logic/
│       ├── coach.c
│       ├── coach.h
│       ├── referee.c
│       └── referee.h
├── CMakeLists.txt
├── main.c
└── README.md
```

## 💻 My Contributions

My main contributions focused on implementing and modifying the game logic in the following files:

### 1. Referee Logic (`engine/logic/referee.c`)

Worked on the referee system, which is responsible for checking player actions and enforcing the rules of the game.

### 2. Coach Logic (`engine/logic/coach.c`)

Worked on the coach system, which handles player behavior and decision-making during the match.

These contributions were based on the original project specifications.

## 🚀 Installation & Setup

### Prerequisites

- GCC or Clang
- CMake
- SDL2
- SDL2_ttf
- SDL2_image

### Build

The project uses CMake as its build system.

```bash
cmake -S . -B build
cmake --build build
```

Additional SDL2 setup may be required depending on your operating system.

## 🎓 Project Context

This project was completed as part of a university programming course.

The original project structure and game engine were provided as the starting point. My work focused on the referee and coach logic.

## 🙏 Acknowledgements

Thanks to the original project authors, [MatinB02](https://github.com/MatinB02) and [Mani Ebrahimi](https://github.com/maniebra), for providing the initial game engine and project structure.

---

**Developed as a university programming project.**
