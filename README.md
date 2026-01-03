# Cambrai

[![Build macOS](https://github.com/FigBug/Cambrai/actions/workflows/build_macos.yml/badge.svg)](https://github.com/FigBug/Cambrai/actions/workflows/build_macos.yml)
[![Build Linux](https://github.com/FigBug/Cambrai/actions/workflows/build_linux.yml/badge.svg)](https://github.com/FigBug/Cambrai/actions/workflows/build_linux.yml)
[![Build Windows](https://github.com/FigBug/Cambrai/actions/workflows/build_windows.yml/badge.svg)](https://github.com/FigBug/Cambrai/actions/workflows/build_windows.yml)

A multiplayer tank battle game for up to 4 players. Battle in an arena with destructible obstacles, mines, turrets, portals, and more.

## Features

- **Local Multiplayer**: Up to 4 players with gamepad or keyboard/mouse support
- **10-Round Matches**: Players place obstacles, then battle through 10 rounds
- **11 Obstacle Types**: Walls, mines, auto-turrets, portals, powerups, electromagnets, and more
- **Tank Combat**: Health system, shell physics with splash damage, and destruction animations

## Building

### Prerequisites

- CMake 3.16+
- C++20 compatible compiler
- Git (for submodules)

### Clone and Build

```bash
git clone --recursive https://github.com/your-username/Cambrai.git
cd Cambrai
./run_cmake.sh
```

Then build with your platform's IDE/toolchain:

#### ![macOS](https://img.shields.io/badge/macOS-000000?style=flat&logo=apple&logoColor=white)
Open `build/Cambrai.xcodeproj` in Xcode

#### ![Windows](https://img.shields.io/badge/Windows-0078D6?style=flat&logo=windows&logoColor=white)
Open `build/Cambrai.sln` in Visual Studio

#### ![Linux](https://img.shields.io/badge/Linux-FCC624?style=flat&logo=linux&logoColor=black)
Run `cmake --build build`

## Controls

### Gamepad
- Left stick: Move/rotate tank
- Right stick: Aim crosshair
- Right trigger: Fire

### Keyboard/Mouse
- WASD: Move forward/back, rotate
- Mouse: Aim crosshair
- Left click: Fire

## Gameplay

1. **Placement Phase**: Each player places one randomly-assigned obstacle
2. **Combat**: Destroy enemy tanks to score points
3. **Powerups**: Collect speed, damage, or armor boosts
4. **Victory**: Highest score after 10 rounds wins

## Obstacles

| Obstacle | Description |
|----------|-------------|
| Solid Wall | Indestructible barrier |
| Breakable Wall | Can be destroyed by shells |
| Reflective Wall | Bounces shells |
| Mine | Instant kill after 2-second arm time |
| Auto Turret | Autonomous defense turret |
| Pit | Traps tanks for 15 seconds |
| Portal | Teleports tanks |
| Flag | Capture for 5 points |
| Powerup | Speed, damage, or armor boost |
| Electromagnet | Pulls tanks toward it |
| Fan | Pushes tanks with wind |

## Dependencies

- [raylib](https://github.com/raysan5/raylib) - Graphics
- [nlohmann/json](https://github.com/nlohmann/json) - Configuration

## License

MIT License - see [LICENSE.txt](LICENSE.txt)
