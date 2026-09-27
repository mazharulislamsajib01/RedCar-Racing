# RedCar Racing

A beginner-friendly 3D endless highway overtaking game developed in C++ using legacy OpenGL, GLU, and FreeGLUT.

The player controls a fast red car, changes between four lanes, avoids traffic, and earns one golden coin for every same-direction vehicle successfully overtaken.

## Project Overview

This project was developed as a university Computer Graphics Lab assignment. It demonstrates fixed-function OpenGL rendering, hierarchical modeling, transformations, procedural textures, lighting, materials, animation, collision detection, and keyboard/mouse interaction.

The project does not require any external model, texture, sound, physics, or game-engine library.

## Features

- Four-lane endless 3D highway
- Two same-direction traffic lanes
- Two opposite-direction traffic lanes
- Smooth lane changing with visual body tilt
- Three selectable red player cars:
  - Sports Car
  - Sedan
  - SUV
- Fixed-size and safely spaced traffic system
- One golden coin for every successful overtake
- Model-dependent collision detection
- Third-person chase camera
- First-person cockpit camera
- Dashboard and framed windshield
- Day and night environments
- Directional sunlight or moonlight
- Player-headlight spotlight
- Procedurally generated textures:
  - Asphalt
  - Grass
  - Building wall
  - Dashboard
- Continuously rotating wheels
- Animated golden reward coin
- Trees, buildings, streetlights, road signs, guardrails, hills, and city silhouettes
- Main menu, car-selection screen, HUD, pause mode, and Game Over screen
- No external assets required

## Technologies Used

- C++11
- Legacy OpenGL fixed-function pipeline
- GLU
- GLUT or FreeGLUT
- MinGW or Code::Blocks on Windows

## Game States

The program uses the following game states:

```cpp
enum GameState
{
    MAIN_MENU,
    CAR_SELECTION,
    PLAYING,
    PAUSED,
    GAME_OVER
};
```

## Controls

| Context | Control | Action |
|---|---|---|
| Main menu | Mouse | Highlight and select buttons |
| Main menu | Enter | Start racing |
| Main menu | Esc | Exit the application |
| Car selection | Left/Right arrow | Previous or next car |
| Car selection | Enter | Select the displayed car and start |
| Car selection | B | Return to the main menu |
| Racing | A / Left arrow | Move one lane left |
| Racing | D / Right arrow | Move one lane right |
| Racing | W / Up arrow | Increase speed |
| Racing | S / Down arrow | Decrease speed |
| Racing or paused | V | Toggle chase and cockpit cameras |
| Racing or paused | P | Pause or resume |
| Racing or paused | N | Toggle day and night |
| Racing or paused | H | Show or hide control hints |
| Racing or paused | M or Esc | Return to the main menu |
| Game Over | R | Restart the race |
| Game Over | M or Esc | Return to the main menu |

## Scoring Rules

- One golden coin is awarded after completely overtaking a same-direction vehicle.
- Each traffic-car spawn can award only one coin.
- Opposite-direction vehicles do not award coins.
- A pass that causes a collision does not award a coin.
- Recycled traffic vehicles begin a new scoring lifecycle.

The HUD displays:

- Golden coins
- Vehicles overtaken
- Current speed
- Distance travelled
- Current lane
- Selected car
- Camera mode
- Day/night mode
- Pause status

## Project Structure

```text
four-lane-highway-overtake-racing/
├── main.cpp
├── README.md
└── screenshots/          Optional screenshots
```

All rendering, game logic, traffic, textures, lighting, cameras, and user-interface code are contained in `main.cpp`.

## Required Libraries

Link the following libraries:

```text
freeglut
opengl32
glu32
gdi32
winmm
```

The essential graphics libraries are:

```text
freeglut
opengl32
glu32
```

## Build with MinGW

If FreeGLUT is already configured:

```bash
g++ -std=c++11 -O2 -Wall -Wextra -Wpedantic main.cpp -o FourLaneHighway.exe -lfreeglut -lopengl32 -lglu32 -lgdi32 -lwinmm
```

If FreeGLUT is installed in `C:\freeglut`:

```bash
g++ -std=c++11 -O2 -Wall -Wextra -Wpedantic main.cpp -o FourLaneHighway.exe -IC:\freeglut\include -LC:\freeglut\lib -lfreeglut -lopengl32 -lglu32 -lgdi32 -lwinmm
```

Copy the matching `freeglut.dll` beside `FourLaneHighway.exe` when the DLL is not available through the Windows `PATH`.

## Code::Blocks Setup

1. Create a new **C++ Console Application**.
2. Replace the generated source file with `main.cpp`.
3. Open **Project → Build options**.
4. Under **Search directories → Compiler**, add the FreeGLUT include directory.
5. Under **Search directories → Linker**, add the FreeGLUT library directory.
6. Under **Linker settings**, add:

```text
freeglut
opengl32
glu32
gdi32
winmm
```

7. Under **Compiler settings → Other options**, add:

```text
-std=c++11
```

8. Copy the matching `freeglut.dll` beside the executable.
9. Build and run the project.

The compiler, FreeGLUT library, and DLL must all use the same 32-bit or 64-bit architecture.

## Computer Graphics Concepts Demonstrated

| Concept | Implementation |
|---|---|
| Object transformations | `glTranslatef`, `glRotatef`, and `glScalef` |
| Hierarchical modeling | Car body, cabin, spoiler, lights, and child wheels |
| Matrix stack | `glPushMatrix` and `glPopMatrix` |
| Perspective projection | `gluPerspective` |
| Viewing transformation | `gluLookAt` |
| Multiple cameras | Third-person and first-person cockpit views |
| Lighting | Directional `GL_LIGHT0` and spotlight `GL_LIGHT1` |
| Materials | Ambient, diffuse, specular, emission, and shininess |
| Texturing | Procedurally generated OpenGL textures |
| Animation | Traffic, wheels, car preview, and reward coin |
| Interaction | Keyboard, special-key, mouse, hover, and click callbacks |
| Mixed rendering | Perspective 3D world and orthographic 2D interfaces |

## Collision Detection

Collision detection compares the X and Z distances between the player and every active traffic vehicle.

The collision dimensions depend on the selected player model and NPC car model:

```text
abs(playerX - trafficX) < combined width threshold
abs(playerZ - trafficZ) < combined length threshold
```

A collision immediately changes the state to `GAME_OVER`.

## Overtaking Detection

Negative Z represents forward travel.

For a traffic vehicle:

```text
relativeZ = trafficZ - playerZ
```

- Negative `relativeZ`: the traffic car is ahead.
- Positive `relativeZ`: the traffic car is behind.

A coin is awarded only when:

1. The vehicle travels in the same direction.
2. It was previously ahead of the player.
3. The player becomes fully clear in front.
4. No collision occurred.
5. The vehicle has not already awarded a coin.

Collision detection runs before overtaking detection, preventing a crashed pass from awarding a coin.

## Traffic System

The game uses a fixed traffic pool:

- Seven same-direction vehicles
- Three opposite-direction vehicles

Safe spawning checks:

- Minimum spacing between vehicles in the same lane
- Cross-lane traffic alignment
- Available overtaking gaps
- Prevention of impossible four-lane traffic walls

NPC vehicles also slow down when approaching a slower car in the same lane.

## Cameras

### Third-Person Camera

The default camera follows the player from above and behind while looking forward along the highway.

### First-Person Camera

The first-person camera is positioned inside the selected car. A dashboard, steering wheel, gauges, windshield frame, and pillars create a cockpit view while keeping the highway visible through the windshield.

Press `V` to switch camera modes.

## Day and Night Modes

Press `N` to toggle the environment.

### Day Mode

- Light-blue gradient sky
- Bright sunlight
- Clear roadside scenery
- Dim headlights

### Night Mode

- Dark-blue gradient sky
- Moon and stars
- Cooler moonlight
- Bright headlights
- Emissive streetlights


## Troubleshooting

| Problem | Solution |
|---|---|
| `GL/glut.h` is missing | Add the FreeGLUT include directory to the compiler search path. |
| `cannot find -lfreeglut` | Add the FreeGLUT library directory to the linker search path. |
| `freeglut.dll` is missing | Copy the matching DLL beside the executable. |
| `undefined reference to gluPerspective` | Link the `glu32` library. |
| `undefined reference to glutInit` | Link `freeglut` after the source/object file. |
| Wrong file format | Match the compiler and FreeGLUT 32/64-bit architecture. |
| Black textures | Use `GL_LINEAR` filtering because no mipmaps are generated. |
| Scene is dark | Ensure lighting is configured after `gluLookAt`. |

## Possible Future Improvements

- Persistent high scores
- More road layouts
- Weather effects
- Additional car models
- Optional roadside bonus coins
- Difficulty-selection menu
- Local multiplayer controls
- Sound effects using an optional platform API


