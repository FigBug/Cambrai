# Cambrai

[![Build macOS](https://github.com/FigBug/Cambrai/actions/workflows/build_macos.yml/badge.svg)](https://github.com/FigBug/Cambrai/actions/workflows/build_macos.yml)
[![Build Linux](https://github.com/FigBug/Cambrai/actions/workflows/build_linux.yml/badge.svg)](https://github.com/FigBug/Cambrai/actions/workflows/build_linux.yml)
[![Build Windows](https://github.com/FigBug/Cambrai/actions/workflows/build_windows.yml/badge.svg)](https://github.com/FigBug/Cambrai/actions/workflows/build_windows.yml)

A multiplayer tank battle game for up to 4 players. Battle in an arena with destructible obstacles, mines, turrets, portals, and more.

## Features

- **Local Multiplayer**: 2-4 players with gamepad or keyboard/mouse support
- **5-Round Matches**: Players select and place obstacles, then battle through 5 rounds
- **12 Obstacle Types**: Walls, mines, auto-turrets, portals, powerups, electromagnets, and more
- **Tank Combat**: Health system, shell physics with splash damage, and destruction animations
- **Two Aim Modes**: Crosshair mode or direct turret rotation

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

### Title Screen
| Action | Keyboard | Gamepad |
|--------|----------|---------|
| Change player count | Up/Down | D-pad Up/Down |
| Adjust volume | Left/Right | D-pad Left/Right |
| Toggle aim mode | Tab | R3 (right stick click) |
| Start game | Any key | A/B/X/Y |

### Selection Phase
| Action | Keyboard | Gamepad |
|--------|----------|---------|
| Navigate grid | Arrow keys / WASD | D-pad / Left stick |
| Select obstacle | Enter / Space | A |

### Placement Phase
| Action | Keyboard | Gamepad |
|--------|----------|---------|
| Move obstacle | Arrow keys / WASD | Left stick |
| Rotate obstacle | Q / E | Bumpers |
| Place obstacle | Enter / Left click | A |

### Combat - Crosshair Mode (default)
| Action | Keyboard/Mouse | Gamepad |
|--------|----------------|---------|
| Move forward/back | W/S | Left stick Y |
| Rotate tank | A/D | Left stick X |
| Aim crosshair | Mouse | Right stick |
| Fire | Left click | Right trigger |

### Combat - Rotation Mode
| Action | Keyboard | Gamepad |
|--------|----------|---------|
| Move forward/back | W/S | Left stick Y |
| Rotate tank | A/D | Left stick X |
| Rotate turret | Q/E | Right stick X |
| Fire | Left click | Right trigger |

Rotation mode has faster reload (3s vs 7s) but requires manual turret aiming.

## Gameplay

1. **Selection Phase**: Each player selects an obstacle type from a grid
2. **Placement Phase**: Each player places their selected obstacle on the battlefield
3. **Combat**: Destroy enemy tanks to score points (1 point per kill, 1 point for surviving)
4. **Victory**: Highest score after 5 rounds wins

## Obstacles

| Obstacle | Description |
|----------|-------------|
| Solid Wall | Indestructible barrier |
| Breakable Wall | Can be destroyed by shells |
| Reflective Wall | Bounces shells |
| Ricochet Wall | Splits shells into 5 projectiles |
| Mine | Instant kill after 2-second arm time |
| Auto Turret | Autonomous defense turret |
| Pit | Traps tanks for 15 seconds |
| Portal | Teleports tanks (maintains speed) |
| Flag | Capture for 2 points |
| Health Pack | Restores tank health |
| Electromagnet | Pulls tanks and bends shell paths |
| Fan | Pushes tanks with wind |

## Dependencies

- [raylib](https://github.com/raysan5/raylib) - Graphics
- [nlohmann/json](https://github.com/nlohmann/json) - Configuration

## License

MIT License - see [LICENSE.txt](LICENSE.txt)
