# 👾 Space Invaders — C++/ SDL2

A faithful recreation of the classic **Space Invaders** arcade game built entirely in C++ using SDL2, with modern additions including levels, high scores, particle effects and a pixel-art star field.

---

## 🎮 Features

- **20 progressive levels** — aliens get faster, fire more frequently and in greater numbers as you advance
- **3 alien types** — Squid (30pts), Crab (20pts), Octopus (10pts) — each with pixel-art sprites and 2-frame animation
- **Mystery UFO** — appears periodically for 50–200 bonus points
- **4 destructible shields** — pixel-accurate damage, eroded by both player and alien bullets
- **Particle explosion effects** — colour-coded per alien type
- **Animated star field** — multi-layer parallax background
- **Screen flash feedback** — hit confirmation and damage indicators
- **Lives system** — 3 lives, 2-second invincibility after being hit
- **Pause screen** — [P] to pause/resume

---

## 🕹️ Controls

| Key | Action |
|-----|--------|
| `←` `→` or `A` `D` | Move ship |
| `SPACE` or `Z` | Fire |
| `P` | Pause / Resume |
| `ESC` | Return to title |
| `H` (title screen) | View high scores |

**High score name entry:**
- `↑` / `↓` — cycle letter
- `→` / `ENTER` — next character
- `←` — previous character
- `ENTER` on third letter — save score

---

## 🛠️ Building

### Prerequisites

**Ubuntu / Debian:**
```bash
sudo apt install libsdl2-dev libsdl2-ttf-dev build-essential
```

**macOS (Homebrew):**
```bash
brew install sdl2 sdl2_ttf
```

**Windows (vcpkg):**
```bash
vcpkg install sdl2 sdl2-ttf
```

### Compile

```bash
g++ space_invaders.cpp -o space_invaders \
    $(sdl2-config --cflags --libs) \
    -lSDL2_ttf \
    -std=c++ \
    -O2
```

### Run

```bash
./space_invaders

---

## 🏗️ Architecture

All game logic lives in `space_invaders.cpp` as a single `Game` struct with clearly separated subsystems:

| Subsystem | Description |
|-----------|-------------|
| `updatePlayer()` | Keyboard movement, shoot cooldown, invincibility timer |
| `updateAliens()` | Grid movement, edge detection, drop logic, adaptive speed, AI firing |
| `updateBullets()` | Bullet translation, off-screen culling |
| `updateUFO()` | Timed spawn, horizontal flight, collision |
| `updateCollisions()` | Player bullets vs aliens/UFO/shields, alien bullets vs player/shields |
| `Shield::hitTest()` | Per-pixel collision and erosion on destructible shields |
| `renderHighScores()` | Top-10 table loaded from binary file |
| `renderHighScoreEntry()` | 3-letter arcade-style name entry |

### Level Scaling

Each level increases difficulty:
- **Move period** decreases (aliens step more frequently)
- **Fire rate** increases per alien per frame
- **Max simultaneous alien bullets** increases (capped at 8)
- **Alien speed** scales up
- When fewer aliens remain, the surviving aliens move faster automatically

---

## 🎨 Tech

- **Language:** C++
- **Renderer:** SDL2 hardware-accelerated 2D

---

## 📬 Author

**Aashir Gurung (veqzii)**
- GitHub: [github.com/veqzii](https://github.com/veqzii)
- Portfolio: [veqzii.github.io](https://veqzii.github.io)
