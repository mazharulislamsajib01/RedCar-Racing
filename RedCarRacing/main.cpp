#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

#include <GL/glut.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <string>

// -----------------------------------------------------------------------------
// Four-Lane Highway Overtake Racing
// Legacy OpenGL, GLU and GLUT/FreeGLUT university project.
//
// Coordinate rule:
//     Forward movement is toward negative Z.
// -----------------------------------------------------------------------------

const float PI = 3.14159265358979323846f;

const int INITIAL_WINDOW_WIDTH = 1100;
const int INITIAL_WINDOW_HEIGHT = 700;
const int TIMER_INTERVAL_MS = 16;

const int LANE_COUNT = 4;
const float LANE_X[LANE_COUNT] = {-6.0f, -2.0f, 2.0f, 6.0f};

const float ROAD_LEFT = -8.0f;
const float ROAD_RIGHT = 8.0f;

const float MIN_PLAYER_SPEED = 0.35f;
const float START_PLAYER_SPEED = 0.65f;
const float MAX_PLAYER_SPEED = 1.00f;
const float WORLD_SPEED_SCALE = 30.0f;

const int SAME_TRAFFIC_COUNT = 7;
const int OPPOSITE_TRAFFIC_COUNT = 3;
const int MAX_TRAFFIC =
    SAME_TRAFFIC_COUNT + OPPOSITE_TRAFFIC_COUNT;

enum GameState
{
    MAIN_MENU,
    CAR_SELECTION,
    PLAYING,
    PAUSED,
    GAME_OVER
};

enum CarModel
{
    SPORTS_CAR = 0,
    SEDAN = 1,
    SUV = 2,
    CAR_MODEL_COUNT = 3
};

enum TrafficDirection
{
    SAME_DIRECTION = -1,
    OPPOSITE_DIRECTION = 1
};

enum TextureId
{
    TEX_ASPHALT = 0,
    TEX_GRASS = 1,
    TEX_WALL = 2,
    TEX_DASHBOARD = 3,
    TEXTURE_COUNT = 4
};

enum ButtonId
{
    BUTTON_NONE,
    BUTTON_START,
    BUTTON_CARS,
    BUTTON_QUIT,
    BUTTON_PREVIOUS,
    BUTTON_NEXT,
    BUTTON_BACK,
    BUTTON_SELECT
};

struct Color
{
    float r;
    float g;
    float b;
};

struct Rect
{
    float x;
    float y;
    float width;
    float height;
};

struct TrafficCar
{
    int id;
    bool active;
    int lane;
    float x;
    float z;
    float speed;
    int direction;
    int model;
    Color color;
    bool coinAwarded;
    bool wasAheadOfPlayer;
    bool collidedWithPlayer;
    float wheelRotation;
};

const Color NPC_COLORS[] = {
    {0.10f, 0.25f, 0.85f},   // Blue
    {0.88f, 0.88f, 0.88f},   // White
    {0.95f, 0.72f, 0.08f},   // Yellow
    {0.35f, 0.38f, 0.42f},   // Gray
    {0.08f, 0.52f, 0.20f},   // Green
    {0.035f, 0.035f, 0.045f} // Black
};

const int NPC_COLOR_COUNT =
    static_cast<int>(sizeof(NPC_COLORS) / sizeof(NPC_COLORS[0]));

// -----------------------------------------------------------------------------
// Global game state
// -----------------------------------------------------------------------------

GameState gameState = MAIN_MENU;

int windowWidth = INITIAL_WINDOW_WIDTH;
int windowHeight = INITIAL_WINDOW_HEIGHT;

int mouseX = 0;
int mouseY = 0;
int hoveredButton = BUTTON_NONE;

int selectedCarModel = SPORTS_CAR;
int previewCarModel = SPORTS_CAR;

float previewRotation = 0.0f;
float previewWheelRotation = 0.0f;

float playerX = LANE_X[1];
float playerZ = 0.0f;
float playerSpeed = START_PLAYER_SPEED;
float playerTilt = 0.0f;
float playerWheelRotation = 0.0f;

int targetLane = 1;

float cameraFollowX = LANE_X[1];

bool firstPersonCamera = false;
bool nightMode = false;
bool showControls = true;

int coinCount = 0;
int overtakeCount = 0;

float distanceTravelled = 0.0f;
float rewardMessageTimer = 0.0f;
float rewardCoinRotation = 0.0f;

TrafficCar traffic[MAX_TRAFFIC];
int nextTrafficId = 1;
float respawnRetryTimer = 0.0f;

GLuint textures[TEXTURE_COUNT] = {0, 0, 0, 0};
GLUquadric *sharedQuadric = 0;

bool glResourcesReady = false;
int previousTimerMilliseconds = 0;

// -----------------------------------------------------------------------------
// Function declarations
// -----------------------------------------------------------------------------

float clampFloat(float value, float minimum, float maximum);
float randomFloat(float minimum, float maximum);
int randomInt(int minimum, int maximum);
int positiveModulo(int value, int modulus);

float carWidth(int model);
float carLength(int model);
float cockpitEyeHeight(int model);
const char *carModelName(int model);

void setMaterial(
    float red,
    float green,
    float blue,
    float specularStrength,
    float shininess
);

void setEmissiveMaterial(
    float red,
    float green,
    float blue,
    float emissionStrength
);

void initializeTextures();
void initializeOpenGL();
void cleanupGLResources();
void quitGame();

void drawScaledCube(
    float x,
    float y,
    float z,
    float width,
    float height,
    float length
);

void drawCylinderY(float radius, float height, int slices);

void drawWheel(
    float rotationAngle,
    float radius,
    float tireThickness
);

void drawFourWheels(
    float halfWidth,
    float wheelY,
    float frontZ,
    float rearZ,
    float rotationAngle,
    float radius,
    float tireThickness
);

void drawSportsHood();
void drawSportsCar(const Color &paint, float wheelAngle);
void drawSedan(const Color &paint, float wheelAngle);
void drawSUV(const Color &paint, float wheelAngle);

void drawCarByModel(
    int model,
    const Color &paint,
    float wheelAngle
);

void drawGoldenCoin(float rotationAngle);

void drawTree(
    float x,
    float z,
    float scale,
    unsigned int variation
);

void drawStreetlight(float x, float z);
void drawGuardrailSegment(float z);

void drawTexturedBuilding(
    float x,
    float z,
    float width,
    float height,
    float length,
    const Color &color
);

void drawControlBooth(float x, float z);
void drawDistanceSign(float x, float z, int distanceNumber);

void drawRoad();
void drawLaneMarkings();
void drawRoadsideEnvironment();
void drawHillsAndCity();

void setupLights();
void setupPreviewLights();

void drawTraffic();
void drawPlayerCar();
void drawRewardCoinInWorld();

void beginOverlay();
void endOverlay();
void drawSkyBackground();

int bitmapTextWidth(void *font, const std::string &text);

void drawText2D(
    float x,
    float y,
    const std::string &text,
    void *font,
    float red,
    float green,
    float blue
);

void drawCenteredText2D(
    float centerX,
    float y,
    const std::string &text,
    void *font,
    float red,
    float green,
    float blue
);

void drawButton(
    const Rect &rect,
    const std::string &label,
    bool hovered
);

bool pointInsideRect(int x, int y, const Rect &rect);

void getMainMenuRects(
    Rect &startRect,
    Rect &carsRect,
    Rect &quitRect
);

void getSelectionRects(
    Rect &previousRect,
    Rect &nextRect,
    Rect &backRect,
    Rect &selectRect
);

void updateHoveredButton();

void drawMainMenu();
void drawPreviewPlatform();
void drawCarSelection();

void drawDashboardAndWindshield();
void drawHUD();
void drawPauseOverlay();
void drawGameOverOverlay();
void drawGameScene();

bool isSpawnPositionSafe(
    int lane,
    float candidateZ,
    int slotToIgnore
);

bool findSafeSpawnPosition(
    int lane,
    int direction,
    int slotIndex,
    float &safeZ
);

bool spawnTrafficCar(int slotIndex, int direction);

void resetGame();
void requestLaneChange(int laneDelta);
void updatePlayer(float deltaTime);
void updateTraffic(float deltaTime);
void checkCollisions();
void checkOvertakes();
void recycleTraffic();
void updateAnimations(float deltaTime);

void display();
void reshape(int width, int height);
void keyboard(unsigned char key, int x, int y);
void specialKeys(int key, int x, int y);
void mouse(int button, int state, int x, int y);
void passiveMouseMotion(int x, int y);
void timer(int value);

// -----------------------------------------------------------------------------
// General helpers
// -----------------------------------------------------------------------------

float clampFloat(float value, float minimum, float maximum)
{
    return std::max(minimum, std::min(value, maximum));
}

float randomFloat(float minimum, float maximum)
{
    const float unit =
        static_cast<float>(std::rand()) /
        static_cast<float>(RAND_MAX);

    return minimum + (maximum - minimum) * unit;
}

int randomInt(int minimum, int maximum)
{
    return minimum + std::rand() % (maximum - minimum + 1);
}

int positiveModulo(int value, int modulus)
{
    int result = value % modulus;

    if (result < 0)
    {
        result += modulus;
    }

    return result;
}

float carWidth(int model)
{
    if (model == SPORTS_CAR)
    {
        return 2.55f;
    }

    if (model == SUV)
    {
        return 2.50f;
    }

    return 2.25f;
}

float carLength(int model)
{
    if (model == SPORTS_CAR)
    {
        return 5.80f;
    }

    if (model == SUV)
    {
        return 5.90f;
    }

    return 5.65f;
}

float cockpitEyeHeight(int model)
{
    if (model == SPORTS_CAR)
    {
        return 1.55f;
    }

    if (model == SUV)
    {
        return 2.15f;
    }

    return 1.80f;
}

const char *carModelName(int model)
{
    if (model == SPORTS_CAR)
    {
        return "Red Sports Car";
    }

    if (model == SUV)
    {
        return "Red SUV";
    }

    return "Red Sedan";
}

// -----------------------------------------------------------------------------
// Materials and textures
// -----------------------------------------------------------------------------

void setMaterial(
    float red,
    float green,
    float blue,
    float specularStrength,
    float shininess
)
{
    const GLfloat ambient[] = {
        red * 0.24f,
        green * 0.24f,
        blue * 0.24f,
        1.0f
    };

    const GLfloat diffuse[] = {
        red,
        green,
        blue,
        1.0f
    };

    const GLfloat specular[] = {
        specularStrength,
        specularStrength,
        specularStrength,
        1.0f
    };

    const GLfloat emission[] = {
        0.0f,
        0.0f,
        0.0f,
        1.0f
    };

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emission);

    glMaterialf(
        GL_FRONT_AND_BACK,
        GL_SHININESS,
        clampFloat(shininess, 0.0f, 128.0f)
    );
}

void setEmissiveMaterial(
    float red,
    float green,
    float blue,
    float emissionStrength
)
{
    setMaterial(red, green, blue, 0.25f, 32.0f);

    const GLfloat emission[] = {
        red * emissionStrength,
        green * emissionStrength,
        blue * emissionStrength,
        1.0f
    };

    glMaterialfv(
        GL_FRONT_AND_BACK,
        GL_EMISSION,
        emission
    );
}

void initializeTextures()
{
    const int textureSize = 64;

    unsigned char asphalt[textureSize * textureSize * 3];
    unsigned char grass[textureSize * textureSize * 3];
    unsigned char wall[textureSize * textureSize * 3];
    unsigned char dashboard[textureSize * textureSize * 3];

    for (int y = 0; y < textureSize; ++y)
    {
        for (int x = 0; x < textureSize; ++x)
        {
            const int index = (y * textureSize + x) * 3;

            // Asphalt: deterministic gray noise.
            const int roadNoise =
                (x * 17 + y * 31 + (x * y) % 19) % 34;

            const unsigned char roadValue =
                static_cast<unsigned char>(48 + roadNoise);

            asphalt[index] = roadValue;
            asphalt[index + 1] = roadValue;
            asphalt[index + 2] =
                static_cast<unsigned char>(roadValue + 3);

            // Grass: varied green noise.
            const int grassNoise =
                (x * 11 + y * 23 + (x ^ y) * 3) % 46;

            grass[index] =
                static_cast<unsigned char>(25 + grassNoise / 4);

            grass[index + 1] =
                static_cast<unsigned char>(92 + grassNoise);

            grass[index + 2] =
                static_cast<unsigned char>(28 + grassNoise / 3);

            // Brick wall with mortar.
            const bool mortar =
                (y % 16 < 2) ||
                ((x + ((y / 16) % 2) * 8) % 16 < 2);

            if (mortar)
            {
                wall[index] = 170;
                wall[index + 1] = 165;
                wall[index + 2] = 150;
            }
            else
            {
                const int brickNoise = (x * 5 + y * 7) % 28;

                wall[index] =
                    static_cast<unsigned char>(135 + brickNoise);

                wall[index + 1] =
                    static_cast<unsigned char>(68 + brickNoise / 3);

                wall[index + 2] =
                    static_cast<unsigned char>(45 + brickNoise / 4);
            }

            // Dashboard: dark padded square pattern.
            const bool seam =
                (x % 16 == 0) || (y % 16 == 0);

            const int dashNoise = (x * 13 + y * 9) % 14;

            const unsigned char dashValue =
                static_cast<unsigned char>(
                    seam ? 24 : 42 + dashNoise
                );

            dashboard[index] = dashValue;
            dashboard[index + 1] =
                static_cast<unsigned char>(dashValue + 4);

            dashboard[index + 2] =
                static_cast<unsigned char>(dashValue + 7);
        }
    }

    const unsigned char *imageData[TEXTURE_COUNT] = {
        asphalt,
        grass,
        wall,
        dashboard
    };

    glGenTextures(TEXTURE_COUNT, textures);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    for (int i = 0; i < TEXTURE_COUNT; ++i)
    {
        glBindTexture(GL_TEXTURE_2D, textures[i]);

        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_MIN_FILTER,
            GL_LINEAR
        );

        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_MAG_FILTER,
            GL_LINEAR
        );

        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_WRAP_S,
            GL_REPEAT
        );

        glTexParameteri(
            GL_TEXTURE_2D,
            GL_TEXTURE_WRAP_T,
            GL_REPEAT
        );

        glTexEnvi(
            GL_TEXTURE_ENV,
            GL_TEXTURE_ENV_MODE,
            GL_MODULATE
        );

        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGB,
            textureSize,
            textureSize,
            0,
            GL_RGB,
            GL_UNSIGNED_BYTE,
            imageData[i]
        );
    }

    glBindTexture(GL_TEXTURE_2D, 0);
}

void initializeOpenGL()
{
    glClearColor(0.34f, 0.67f, 0.92f, 1.0f);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_NORMALIZE);

    glShadeModel(GL_SMOOTH);

    glHint(
        GL_PERSPECTIVE_CORRECTION_HINT,
        GL_NICEST
    );

    sharedQuadric = gluNewQuadric();

    if (sharedQuadric != 0)
    {
        gluQuadricNormals(sharedQuadric, GLU_SMOOTH);
        gluQuadricTexture(sharedQuadric, GL_TRUE);
    }

    initializeTextures();
    glResourcesReady = true;
}

void cleanupGLResources()
{
    if (!glResourcesReady)
    {
        return;
    }

    glDeleteTextures(TEXTURE_COUNT, textures);

    if (sharedQuadric != 0)
    {
        gluDeleteQuadric(sharedQuadric);
        sharedQuadric = 0;
    }

    glResourcesReady = false;
}

void quitGame()
{
    cleanupGLResources();
    std::exit(0);
}

// -----------------------------------------------------------------------------
// Reusable geometry
// -----------------------------------------------------------------------------

void drawScaledCube(
    float x,
    float y,
    float z,
    float width,
    float height,
    float length
)
{
    glPushMatrix();

    glTranslatef(x, y, z);
    glScalef(width, height, length);

    glutSolidCube(1.0f);

    glPopMatrix();
}

void drawCylinderY(float radius, float height, int slices)
{
    if (sharedQuadric == 0)
    {
        return;
    }

    glPushMatrix();

    // A GLU cylinder normally extends along +Z.
    // Rotate it so that it extends upward along +Y.
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);

    gluCylinder(
        sharedQuadric,
        radius,
        radius,
        height,
        slices,
        1
    );

    glPopMatrix();
}

void drawWheel(
    float rotationAngle,
    float radius,
    float tireThickness
)
{
    glPushMatrix();

    // Rolling motion is around the wheel axle, the X axis.
    glRotatef(rotationAngle, 1.0f, 0.0f, 0.0f);

    // Rotating spokes make wheel motion visually noticeable.
    setMaterial(0.62f, 0.65f, 0.70f, 0.85f, 90.0f);

    for (int spoke = 0; spoke < 3; ++spoke)
    {
        glPushMatrix();

        glRotatef(
            static_cast<float>(spoke) * 60.0f,
            1.0f,
            0.0f,
            0.0f
        );

        drawScaledCube(
            0.0f,
            0.0f,
            0.0f,
            0.10f,
            radius * 1.25f,
            0.10f
        );

        glPopMatrix();
    }

    // GLUT torus starts with its axle along Z.
    // Rotate it to place the axle along X.
    glPushMatrix();

    glRotatef(90.0f, 0.0f, 1.0f, 0.0f);

    setMaterial(0.025f, 0.025f, 0.030f, 0.20f, 20.0f);

    glutSolidTorus(
        tireThickness,
        radius - tireThickness,
        12,
        24
    );

    glPopMatrix();

    setMaterial(0.55f, 0.58f, 0.64f, 0.90f, 96.0f);
    glutSolidSphere(radius * 0.27f, 14, 10);

    glPopMatrix();
}

void drawFourWheels(
    float halfWidth,
    float wheelY,
    float frontZ,
    float rearZ,
    float rotationAngle,
    float radius,
    float tireThickness
)
{
    const float wheelX[2] = {-halfWidth, halfWidth};
    const float wheelZ[2] = {frontZ, rearZ};

    for (int side = 0; side < 2; ++side)
    {
        for (int axle = 0; axle < 2; ++axle)
        {
            glPushMatrix();

            glTranslatef(
                wheelX[side],
                wheelY,
                wheelZ[axle]
            );

            drawWheel(
                rotationAngle,
                radius,
                tireThickness
            );

            glPopMatrix();
        }
    }
}

// -----------------------------------------------------------------------------
// Car models
// -----------------------------------------------------------------------------

void drawSportsHood()
{
    const float left = -1.20f;
    const float right = 1.20f;

    const float frontZ = -2.85f;
    const float rearZ = -0.75f;

    const float bottomY = 0.68f;
    const float frontTopY = 0.92f;
    const float rearTopY = 1.25f;

    glBegin(GL_QUADS);

    // Sloped top
    glNormal3f(0.0f, 0.99f, -0.14f);

    glVertex3f(left, rearTopY, rearZ);
    glVertex3f(right, rearTopY, rearZ);
    glVertex3f(right, frontTopY, frontZ);
    glVertex3f(left, frontTopY, frontZ);

    // Front
    glNormal3f(0.0f, 0.0f, -1.0f);

    glVertex3f(left, bottomY, frontZ);
    glVertex3f(left, frontTopY, frontZ);
    glVertex3f(right, frontTopY, frontZ);
    glVertex3f(right, bottomY, frontZ);

    // Rear
    glNormal3f(0.0f, 0.0f, 1.0f);

    glVertex3f(left, bottomY, rearZ);
    glVertex3f(right, bottomY, rearZ);
    glVertex3f(right, rearTopY, rearZ);
    glVertex3f(left, rearTopY, rearZ);

    // Left side
    glNormal3f(-1.0f, 0.0f, 0.0f);

    glVertex3f(left, bottomY, frontZ);
    glVertex3f(left, bottomY, rearZ);
    glVertex3f(left, rearTopY, rearZ);
    glVertex3f(left, frontTopY, frontZ);

    // Right side
    glNormal3f(1.0f, 0.0f, 0.0f);

    glVertex3f(right, bottomY, rearZ);
    glVertex3f(right, bottomY, frontZ);
    glVertex3f(right, frontTopY, frontZ);
    glVertex3f(right, rearTopY, rearZ);

    // Bottom
    glNormal3f(0.0f, -1.0f, 0.0f);

    glVertex3f(left, bottomY, frontZ);
    glVertex3f(right, bottomY, frontZ);
    glVertex3f(right, bottomY, rearZ);
    glVertex3f(left, bottomY, rearZ);

    glEnd();
}

void drawSportsCar(const Color &paint, float wheelAngle)
{
    setMaterial(
        paint.r,
        paint.g,
        paint.b,
        0.95f,
        105.0f
    );

    // Low main body.
    drawScaledCube(
        0.0f,
        0.80f,
        0.10f,
        2.45f,
        0.58f,
        5.25f
    );

    drawSportsHood();

    // Small sports cabin.
    drawScaledCube(
        0.0f,
        1.38f,
        0.45f,
        1.72f,
        0.78f,
        2.05f
    );

    // Dark glass surfaces.
    setMaterial(0.055f, 0.13f, 0.19f, 0.92f, 100.0f);

    drawScaledCube(
        0.0f,
        1.50f,
        -0.63f,
        1.58f,
        0.43f,
        0.08f
    );

    drawScaledCube(
        -0.88f,
        1.48f,
        0.40f,
        0.07f,
        0.45f,
        1.40f
    );

    drawScaledCube(
        0.88f,
        1.48f,
        0.40f,
        0.07f,
        0.45f,
        1.40f
    );

    // Rear spoiler.
    setMaterial(
        paint.r * 0.72f,
        paint.g * 0.72f,
        paint.b * 0.72f,
        0.90f,
        90.0f
    );

    drawScaledCube(
        -0.75f,
        1.28f,
        2.45f,
        0.11f,
        0.62f,
        0.11f
    );

    drawScaledCube(
        0.75f,
        1.28f,
        2.45f,
        0.11f,
        0.62f,
        0.11f
    );

    drawScaledCube(
        0.0f,
        1.58f,
        2.45f,
        2.25f,
        0.15f,
        0.42f
    );

    // Headlights.
    setEmissiveMaterial(
        1.0f,
        0.92f,
        0.63f,
        nightMode ? 0.90f : 0.24f
    );

    drawScaledCube(
        -0.70f,
        0.92f,
        -2.88f,
        0.48f,
        0.22f,
        0.08f
    );

    drawScaledCube(
        0.70f,
        0.92f,
        -2.88f,
        0.48f,
        0.22f,
        0.08f
    );

    // Rear lights.
    setEmissiveMaterial(0.90f, 0.02f, 0.01f, 0.55f);

    drawScaledCube(
        -0.72f,
        0.91f,
        2.74f,
        0.42f,
        0.20f,
        0.08f
    );

    drawScaledCube(
        0.72f,
        0.91f,
        2.74f,
        0.42f,
        0.20f,
        0.08f
    );

    drawFourWheels(
        1.28f,
        0.55f,
        -1.72f,
        1.72f,
        wheelAngle,
        0.55f,
        0.15f
    );
}

void drawSedan(const Color &paint, float wheelAngle)
{
    setMaterial(
        paint.r,
        paint.g,
        paint.b,
        0.82f,
        88.0f
    );

    // Lower body.
    drawScaledCube(
        0.0f,
        0.84f,
        0.0f,
        2.18f,
        0.60f,
        5.20f
    );

    // Separate hood.
    drawScaledCube(
        0.0f,
        1.14f,
        -1.78f,
        2.06f,
        0.42f,
        1.72f
    );

    // Separate passenger cabin.
    drawScaledCube(
        0.0f,
        1.50f,
        0.02f,
        1.92f,
        0.92f,
        2.25f
    );

    // Separate trunk.
    drawScaledCube(
        0.0f,
        1.10f,
        1.92f,
        2.05f,
        0.42f,
        1.25f
    );

    setMaterial(0.06f, 0.14f, 0.20f, 0.88f, 92.0f);

    // Front and rear windows.
    drawScaledCube(
        0.0f,
        1.61f,
        -1.13f,
        1.72f,
        0.52f,
        0.07f
    );

    drawScaledCube(
        0.0f,
        1.61f,
        1.18f,
        1.72f,
        0.52f,
        0.07f
    );

    // Side windows.
    drawScaledCube(
        -0.99f,
        1.62f,
        0.0f,
        0.06f,
        0.55f,
        1.62f
    );

    drawScaledCube(
        0.99f,
        1.62f,
        0.0f,
        0.06f,
        0.55f,
        1.62f
    );

    setEmissiveMaterial(
        1.0f,
        0.92f,
        0.65f,
        nightMode ? 0.86f : 0.20f
    );

    drawScaledCube(
        -0.62f,
        0.97f,
        -2.64f,
        0.42f,
        0.22f,
        0.08f
    );

    drawScaledCube(
        0.62f,
        0.97f,
        -2.64f,
        0.42f,
        0.22f,
        0.08f
    );

    setEmissiveMaterial(0.88f, 0.02f, 0.01f, 0.48f);

    drawScaledCube(
        -0.63f,
        0.96f,
        2.64f,
        0.40f,
        0.20f,
        0.08f
    );

    drawScaledCube(
        0.63f,
        0.96f,
        2.64f,
        0.40f,
        0.20f,
        0.08f
    );

    drawFourWheels(
        1.14f,
        0.54f,
        -1.74f,
        1.73f,
        wheelAngle,
        0.50f,
        0.14f
    );
}

void drawSUV(const Color &paint, float wheelAngle)
{
    setMaterial(
        paint.r,
        paint.g,
        paint.b,
        0.78f,
        78.0f
    );

    // High lower body and large boxy upper cabin.
    drawScaledCube(
        0.0f,
        1.02f,
        0.0f,
        2.42f,
        0.74f,
        5.42f
    );

    drawScaledCube(
        0.0f,
        1.75f,
        0.25f,
        2.22f,
        1.30f,
        3.75f
    );

    // Front engine section.
    drawScaledCube(
        0.0f,
        1.32f,
        -2.15f,
        2.28f,
        0.60f,
        1.15f
    );

    // Roof rails.
    setMaterial(
        paint.r * 0.62f,
        paint.g * 0.62f,
        paint.b * 0.62f,
        0.68f,
        70.0f
    );

    drawScaledCube(
        -0.82f,
        2.47f,
        0.30f,
        0.10f,
        0.10f,
        3.15f
    );

    drawScaledCube(
        0.82f,
        2.47f,
        0.30f,
        0.10f,
        0.10f,
        3.15f
    );

    setMaterial(0.045f, 0.12f, 0.18f, 0.90f, 95.0f);

    // Large windshield.
    drawScaledCube(
        0.0f,
        1.91f,
        -1.67f,
        1.93f,
        0.76f,
        0.08f
    );

    // Large side windows.
    drawScaledCube(
        -1.13f,
        1.91f,
        0.28f,
        0.06f,
        0.76f,
        2.62f
    );

    drawScaledCube(
        1.13f,
        1.91f,
        0.28f,
        0.06f,
        0.76f,
        2.62f
    );

    setEmissiveMaterial(
        1.0f,
        0.92f,
        0.66f,
        nightMode ? 0.92f : 0.23f
    );

    drawScaledCube(
        -0.69f,
        1.16f,
        -2.76f,
        0.48f,
        0.25f,
        0.08f
    );

    drawScaledCube(
        0.69f,
        1.16f,
        -2.76f,
        0.48f,
        0.25f,
        0.08f
    );

    setEmissiveMaterial(0.90f, 0.02f, 0.01f, 0.52f);

    drawScaledCube(
        -0.70f,
        1.17f,
        2.76f,
        0.45f,
        0.25f,
        0.08f
    );

    drawScaledCube(
        0.70f,
        1.17f,
        2.76f,
        0.45f,
        0.25f,
        0.08f
    );

    drawFourWheels(
        1.27f,
        0.69f,
        -1.79f,
        1.80f,
        wheelAngle,
        0.66f,
        0.18f
    );
}

void drawCarByModel(
    int model,
    const Color &paint,
    float wheelAngle
)
{
    if (model == SPORTS_CAR)
    {
        drawSportsCar(paint, wheelAngle);
    }
    else if (model == SUV)
    {
        drawSUV(paint, wheelAngle);
    }
    else
    {
        drawSedan(paint, wheelAngle);
    }
}

// -----------------------------------------------------------------------------
// Golden coin
// -----------------------------------------------------------------------------

void drawGoldenCoin(float rotationAngle)
{
    glPushMatrix();

    glRotatef(
        rotationAngle,
        0.0f,
        1.0f,
        0.0f
    );

    setMaterial(1.0f, 0.67f, 0.04f, 1.0f, 105.0f);

    // glColor is also set because HUD rendering disables lighting.
    glColor3f(1.0f, 0.72f, 0.05f);

    if (sharedQuadric != 0)
    {
        // Coin edge.
        gluCylinder(
            sharedQuadric,
            1.0f,
            1.0f,
            0.22f,
            32,
            2
        );

        // Back face with outward-facing normal.
        glPushMatrix();
        glRotatef(180.0f, 0.0f, 1.0f, 0.0f);
        gluDisk(sharedQuadric, 0.0f, 1.0f, 32, 2);
        glPopMatrix();

        // Front face.
        glPushMatrix();
        glTranslatef(0.0f, 0.0f, 0.22f);
        gluDisk(sharedQuadric, 0.0f, 1.0f, 32, 2);
        glPopMatrix();

        // Raised rim.
        glPushMatrix();
        glTranslatef(0.0f, 0.0f, 0.11f);

        glutSolidTorus(
            0.07f,
            0.91f,
            10,
            32
        );

        glPopMatrix();
    }
    else
    {
        glutSolidTorus(0.20f, 0.80f, 12, 32);
    }

    glPopMatrix();
}

// -----------------------------------------------------------------------------
// Roadside environment
// -----------------------------------------------------------------------------

void drawTree(
    float x,
    float z,
    float scale,
    unsigned int variation
)
{
    glPushMatrix();

    glTranslatef(x, 0.0f, z);
    glScalef(scale, scale, scale);

    setMaterial(0.28f, 0.16f, 0.07f, 0.12f, 12.0f);

    glPushMatrix();
    glTranslatef(0.0f, 0.0f, 0.0f);
    drawCylinderY(0.26f, 2.8f, 12);
    glPopMatrix();

    const float greenOffset =
        static_cast<float>(variation % 7u) * 0.015f;

    setMaterial(
        0.06f,
        0.40f + greenOffset,
        0.09f,
        0.10f,
        12.0f
    );

    glPushMatrix();
    glTranslatef(0.0f, 3.25f, 0.0f);
    glutSolidSphere(1.45f, 15, 12);
    glPopMatrix();

    setMaterial(
        0.04f,
        0.31f + greenOffset,
        0.07f,
        0.08f,
        10.0f
    );

    glPushMatrix();
    glTranslatef(0.0f, 4.18f, 0.0f);
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    glutSolidCone(1.18f, 2.15f, 15, 5);
    glPopMatrix();

    glPopMatrix();
}

void drawStreetlight(float x, float z)
{
    glPushMatrix();

    glTranslatef(x, 0.0f, z);

    const float towardRoad = x < 0.0f ? 1.0f : -1.0f;

    setMaterial(0.35f, 0.38f, 0.42f, 0.72f, 72.0f);

    drawCylinderY(0.10f, 6.2f, 14);

    drawScaledCube(
        towardRoad * 0.72f,
        6.15f,
        0.0f,
        1.55f,
        0.11f,
        0.11f
    );

    glPushMatrix();

    glTranslatef(
        towardRoad * 1.45f,
        5.98f,
        0.0f
    );

    setEmissiveMaterial(
        1.0f,
        0.83f,
        0.45f,
        nightMode ? 1.0f : 0.10f
    );

    glScalef(0.42f, 0.16f, 0.30f);
    glutSolidSphere(1.0f, 12, 8);

    glPopMatrix();

    // Reset emission immediately.
    setMaterial(0.35f, 0.38f, 0.42f, 0.72f, 72.0f);

    glPopMatrix();
}

void drawGuardrailSegment(float z)
{
    setMaterial(0.55f, 0.58f, 0.62f, 0.82f, 80.0f);

    drawScaledCube(
        -9.05f,
        0.72f,
        z,
        0.13f,
        0.20f,
        38.0f
    );

    drawScaledCube(
        9.05f,
        0.72f,
        z,
        0.13f,
        0.20f,
        38.0f
    );

    for (int post = -1; post <= 1; ++post)
    {
        const float postZ =
            z + static_cast<float>(post) * 13.0f;

        drawScaledCube(
            -9.05f,
            0.38f,
            postZ,
            0.16f,
            0.78f,
            0.16f
        );

        drawScaledCube(
            9.05f,
            0.38f,
            postZ,
            0.16f,
            0.78f,
            0.16f
        );
    }
}

void drawTexturedBuilding(
    float x,
    float z,
    float width,
    float height,
    float length,
    const Color &color
)
{
    setMaterial(
        color.r,
        color.g,
        color.b,
        0.28f,
        28.0f
    );

    drawScaledCube(
        x,
        height * 0.5f,
        z,
        width,
        height,
        length
    );

    // Draw the road-facing wall with the procedural brick texture.
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textures[TEX_WALL]);

    setMaterial(0.82f, 0.52f, 0.32f, 0.25f, 24.0f);

    const float roadFaceX =
        x < 0.0f
            ? x + width * 0.5f + 0.006f
            : x - width * 0.5f - 0.006f;

    glBegin(GL_QUADS);

    if (x < 0.0f)
    {
        glNormal3f(1.0f, 0.0f, 0.0f);
    }
    else
    {
        glNormal3f(-1.0f, 0.0f, 0.0f);
    }

    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(
        roadFaceX,
        0.0f,
        z - length * 0.5f
    );

    glTexCoord2f(3.0f, 0.0f);
    glVertex3f(
        roadFaceX,
        0.0f,
        z + length * 0.5f
    );

    glTexCoord2f(3.0f, 3.0f);
    glVertex3f(
        roadFaceX,
        height,
        z + length * 0.5f
    );

    glTexCoord2f(0.0f, 3.0f);
    glVertex3f(
        roadFaceX,
        height,
        z - length * 0.5f
    );

    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);

    // Roof.
    setMaterial(0.23f, 0.08f, 0.055f, 0.22f, 18.0f);

    drawScaledCube(
        x,
        height + 0.18f,
        z,
        width + 0.45f,
        0.35f,
        length + 0.45f
    );
}

void drawControlBooth(float x, float z)
{
    const Color boothColor = {0.76f, 0.71f, 0.58f};

    drawTexturedBuilding(
        x,
        z,
        4.2f,
        3.3f,
        5.2f,
        boothColor
    );

    const float roadSide =
        x < 0.0f ? x + 2.12f : x - 2.12f;

    setMaterial(0.04f, 0.15f, 0.20f, 0.84f, 88.0f);

    drawScaledCube(
        roadSide,
        2.15f,
        z,
        0.08f,
        1.30f,
        2.75f
    );

    setMaterial(0.30f, 0.32f, 0.34f, 0.52f, 58.0f);

    drawScaledCube(
        roadSide,
        0.84f,
        z + 1.65f,
        0.12f,
        1.62f,
        0.92f
    );
}

void drawDistanceSign(
    float x,
    float z,
    int distanceNumber
)
{
    setMaterial(0.32f, 0.34f, 0.37f, 0.62f, 68.0f);

    glPushMatrix();
    glTranslatef(x, 0.0f, z);
    drawCylinderY(0.09f, 2.7f, 12);
    glPopMatrix();

    setMaterial(0.04f, 0.42f, 0.20f, 0.28f, 32.0f);

    drawScaledCube(
        x,
        3.05f,
        z,
        2.55f,
        1.35f,
        0.16f
    );

    std::ostringstream label;
    label << "KM " << distanceNumber;

    glPushAttrib(
        GL_ENABLE_BIT |
        GL_CURRENT_BIT
    );

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    glColor3f(1.0f, 1.0f, 1.0f);

    glRasterPos3f(
        x - 0.85f,
        3.00f,
        z + 0.095f
    );

    const std::string text = label.str();

    for (std::size_t i = 0; i < text.size(); ++i)
    {
        glutBitmapCharacter(
            GLUT_BITMAP_HELVETICA_18,
            text[i]
        );
    }

    glPopAttrib();
}

// -----------------------------------------------------------------------------
// Highway
// -----------------------------------------------------------------------------

void drawLaneMarkings()
{
    const float farZ = playerZ - 520.0f;
    const float nearZ = playerZ + 150.0f;

    setMaterial(0.96f, 0.96f, 0.94f, 0.35f, 28.0f);

    // Dashed separators between lanes 0/1 and 2/3.
    const float firstDash =
        std::floor(farZ / 12.0f) * 12.0f;

    for (float z = firstDash; z < nearZ; z += 12.0f)
    {
        const float dashCenter = z + 3.0f;

        drawScaledCube(
            -4.0f,
            0.026f,
            dashCenter,
            0.15f,
            0.028f,
            6.0f
        );

        drawScaledCube(
            4.0f,
            0.026f,
            dashCenter,
            0.15f,
            0.028f,
            6.0f
        );
    }

    // Solid white road-edge markings.
    drawScaledCube(
        -7.80f,
        0.026f,
        (farZ + nearZ) * 0.5f,
        0.16f,
        0.028f,
        nearZ - farZ
    );

    drawScaledCube(
        7.80f,
        0.026f,
        (farZ + nearZ) * 0.5f,
        0.16f,
        0.028f,
        nearZ - farZ
    );

    // Double yellow divider between lane 1 and lane 2.
    setMaterial(1.0f, 0.72f, 0.05f, 0.38f, 35.0f);

    drawScaledCube(
        -0.19f,
        0.030f,
        (farZ + nearZ) * 0.5f,
        0.13f,
        0.032f,
        nearZ - farZ
    );

    drawScaledCube(
        0.19f,
        0.030f,
        (farZ + nearZ) * 0.5f,
        0.13f,
        0.032f,
        nearZ - farZ
    );
}

void drawRoad()
{
    const float farZ = playerZ - 520.0f;
    const float nearZ = playerZ + 150.0f;

    // Textured grass on both sides.
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textures[TEX_GRASS]);

    setMaterial(0.20f, 0.62f, 0.20f, 0.08f, 10.0f);

    glBegin(GL_QUADS);

    glNormal3f(0.0f, 1.0f, 0.0f);

    glTexCoord2f(0.0f, farZ * 0.08f);
    glVertex3f(-65.0f, -0.04f, farZ);

    glTexCoord2f(8.0f, farZ * 0.08f);
    glVertex3f(-9.15f, -0.04f, farZ);

    glTexCoord2f(8.0f, nearZ * 0.08f);
    glVertex3f(-9.15f, -0.04f, nearZ);

    glTexCoord2f(0.0f, nearZ * 0.08f);
    glVertex3f(-65.0f, -0.04f, nearZ);

    glTexCoord2f(0.0f, farZ * 0.08f);
    glVertex3f(9.15f, -0.04f, farZ);

    glTexCoord2f(8.0f, farZ * 0.08f);
    glVertex3f(65.0f, -0.04f, farZ);

    glTexCoord2f(8.0f, nearZ * 0.08f);
    glVertex3f(65.0f, -0.04f, nearZ);

    glTexCoord2f(0.0f, nearZ * 0.08f);
    glVertex3f(9.15f, -0.04f, nearZ);

    glEnd();

    // Textured asphalt.
    glBindTexture(GL_TEXTURE_2D, textures[TEX_ASPHALT]);

    setMaterial(0.34f, 0.35f, 0.37f, 0.24f, 25.0f);

    glBegin(GL_QUADS);

    glNormal3f(0.0f, 1.0f, 0.0f);

    glTexCoord2f(0.0f, farZ * 0.10f);
    glVertex3f(ROAD_LEFT, 0.0f, farZ);

    glTexCoord2f(4.0f, farZ * 0.10f);
    glVertex3f(ROAD_RIGHT, 0.0f, farZ);

    glTexCoord2f(4.0f, nearZ * 0.10f);
    glVertex3f(ROAD_RIGHT, 0.0f, nearZ);

    glTexCoord2f(0.0f, nearZ * 0.10f);
    glVertex3f(ROAD_LEFT, 0.0f, nearZ);

    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);

    // Road shoulders.
    setMaterial(0.50f, 0.48f, 0.43f, 0.16f, 18.0f);

    drawScaledCube(
        -8.55f,
        -0.005f,
        (farZ + nearZ) * 0.5f,
        1.10f,
        0.04f,
        nearZ - farZ
    );

    drawScaledCube(
        8.55f,
        -0.005f,
        (farZ + nearZ) * 0.5f,
        1.10f,
        0.04f,
        nearZ - farZ
    );

    drawLaneMarkings();
}

void drawRoadsideEnvironment()
{
    const int baseSegment =
        static_cast<int>(
            std::floor(playerZ / 40.0f)
        );

    for (int offset = -12; offset <= 4; ++offset)
    {
        const int segmentIndex = baseSegment + offset;
        const float segmentZ =
            static_cast<float>(segmentIndex) * 40.0f;

        const unsigned int hash =
            static_cast<unsigned int>(segmentIndex) *
            1664525u +
            1013904223u;

        const int pattern =
            positiveModulo(segmentIndex, 10);

        drawGuardrailSegment(segmentZ + 20.0f);

        const float leftTreeX =
            -13.0f - static_cast<float>(hash % 6u);

        const float rightTreeX =
            13.0f +
            static_cast<float>((hash / 7u) % 7u);

        drawTree(
            leftTreeX,
            segmentZ + 7.0f +
                static_cast<float>(hash % 9u),
            0.82f +
                static_cast<float>(hash % 4u) * 0.08f,
            hash
        );

        drawTree(
            rightTreeX,
            segmentZ + 27.0f -
                static_cast<float>((hash / 11u) % 8u),
            0.86f +
                static_cast<float>((hash / 5u) % 4u) * 0.08f,
            hash + 13u
        );

        if (positiveModulo(segmentIndex, 2) == 0)
        {
            drawStreetlight(
                -9.75f,
                segmentZ + 8.0f
            );

            drawStreetlight(
                9.75f,
                segmentZ + 28.0f
            );
        }

        if (pattern == 0)
        {
            const Color houseColor = {
                0.63f,
                0.57f,
                0.49f
            };

            drawTexturedBuilding(
                -17.0f,
                segmentZ + 22.0f,
                6.2f,
                4.2f,
                7.0f,
                houseColor
            );
        }

        if (pattern == 4)
        {
            const Color buildingColor = {
                0.48f,
                0.54f,
                0.60f
            };

            drawTexturedBuilding(
                18.0f,
                segmentZ + 18.0f,
                7.0f,
                5.5f,
                8.0f,
                buildingColor
            );
        }

        if (pattern == 7)
        {
            drawControlBooth(
                -13.2f,
                segmentZ + 18.0f
            );
        }

        if (positiveModulo(segmentIndex, 5) == 1)
        {
            drawDistanceSign(
                10.6f,
                segmentZ + 12.0f,
                std::abs(segmentIndex) * 2
            );
        }
    }
}

void drawHillsAndCity()
{
    // Distant hills move with the player to remain on the horizon.
    setMaterial(
        nightMode ? 0.08f : 0.24f,
        nightMode ? 0.12f : 0.38f,
        nightMode ? 0.18f : 0.24f,
        0.05f,
        8.0f
    );

    const float hillZ = playerZ - 370.0f;

    for (int i = 0; i < 5; ++i)
    {
        const float x =
            -48.0f + static_cast<float>(i) * 24.0f;

        glPushMatrix();

        glTranslatef(
            x,
            -1.0f,
            hillZ - static_cast<float>(i % 2) * 15.0f
        );

        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);

        glutSolidCone(
            17.0f,
            17.0f + static_cast<float>(i % 3) * 3.0f,
            18,
            5
        );

        glPopMatrix();
    }

    // Simple city silhouettes.
    const float cityZ = playerZ - 320.0f;

    for (int i = 0; i < 10; ++i)
    {
        const float side = i < 5 ? -1.0f : 1.0f;
        const int sideIndex = i % 5;

        const float x =
            side *
            (24.0f + static_cast<float>(sideIndex) * 5.0f);

        const float height =
            7.0f + static_cast<float>((i * 3) % 9);

        setMaterial(
            nightMode ? 0.07f : 0.34f,
            nightMode ? 0.09f : 0.39f,
            nightMode ? 0.14f : 0.43f,
            0.12f,
            16.0f
        );

        drawScaledCube(
            x,
            height * 0.5f,
            cityZ -
                static_cast<float>(sideIndex) * 6.0f,
            4.0f,
            height,
            5.0f
        );
    }
}

// -----------------------------------------------------------------------------
// Fixed-function lights
// -----------------------------------------------------------------------------

void setupLights()
{
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHT1);
    glEnable(GL_NORMALIZE);

    const GLfloat dayGlobalAmbient[] = {
        0.23f,
        0.23f,
        0.21f,
        1.0f
    };

    const GLfloat nightGlobalAmbient[] = {
        0.045f,
        0.055f,
        0.085f,
        1.0f
    };

    glLightModelfv(
        GL_LIGHT_MODEL_AMBIENT,
        nightMode
            ? nightGlobalAmbient
            : dayGlobalAmbient
    );

    glLightModeli(
        GL_LIGHT_MODEL_LOCAL_VIEWER,
        GL_TRUE
    );

    // GL_LIGHT0: directional sunlight or moonlight.
    const GLfloat sunPosition[] = {
        -0.35f,
        1.0f,
        -0.25f,
        0.0f
    };

    const GLfloat sunAmbient[] = {
        0.10f,
        0.10f,
        0.10f,
        1.0f
    };

    const GLfloat sunDiffuse[] = {
        0.98f,
        0.93f,
        0.82f,
        1.0f
    };

    const GLfloat moonDiffuse[] = {
        0.32f,
        0.39f,
        0.58f,
        1.0f
    };

    const GLfloat sunSpecular[] = {
        0.88f,
        0.88f,
        0.86f,
        1.0f
    };

    glLightfv(GL_LIGHT0, GL_POSITION, sunPosition);
    glLightfv(GL_LIGHT0, GL_AMBIENT, sunAmbient);

    glLightfv(
        GL_LIGHT0,
        GL_DIFFUSE,
        nightMode ? moonDiffuse : sunDiffuse
    );

    glLightfv(
        GL_LIGHT0,
        GL_SPECULAR,
        sunSpecular
    );

    // GL_LIGHT1: spotlight representing the player headlights.
    const GLfloat headlightPosition[] = {
        playerX,
        1.15f,
        playerZ - 2.25f,
        1.0f
    };

    const GLfloat headlightDirection[] = {
        0.0f,
        -0.09f,
        -1.0f
    };

    const GLfloat noAmbient[] = {
        0.0f,
        0.0f,
        0.0f,
        1.0f
    };

    const GLfloat dayHeadlight[] = {
        0.16f,
        0.15f,
        0.12f,
        1.0f
    };

    const GLfloat nightHeadlight[] = {
        1.0f,
        0.88f,
        0.62f,
        1.0f
    };

    glLightfv(
        GL_LIGHT1,
        GL_POSITION,
        headlightPosition
    );

    glLightfv(
        GL_LIGHT1,
        GL_SPOT_DIRECTION,
        headlightDirection
    );

    glLightfv(
        GL_LIGHT1,
        GL_AMBIENT,
        noAmbient
    );

    glLightfv(
        GL_LIGHT1,
        GL_DIFFUSE,
        nightMode
            ? nightHeadlight
            : dayHeadlight
    );

    glLightfv(
        GL_LIGHT1,
        GL_SPECULAR,
        nightHeadlight
    );

    glLightf(GL_LIGHT1, GL_SPOT_CUTOFF, 28.0f);
    glLightf(GL_LIGHT1, GL_SPOT_EXPONENT, 15.0f);

    glLightf(
        GL_LIGHT1,
        GL_CONSTANT_ATTENUATION,
        1.0f
    );

    glLightf(
        GL_LIGHT1,
        GL_LINEAR_ATTENUATION,
        0.025f
    );

    glLightf(
        GL_LIGHT1,
        GL_QUADRATIC_ATTENUATION,
        0.003f
    );
}

void setupPreviewLights()
{
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glDisable(GL_LIGHT1);

    const GLfloat previewAmbient[] = {
        0.20f,
        0.20f,
        0.20f,
        1.0f
    };

    const GLfloat previewDiffuse[] = {
        1.0f,
        0.95f,
        0.88f,
        1.0f
    };

    const GLfloat previewSpecular[] = {
        1.0f,
        1.0f,
        1.0f,
        1.0f
    };

    const GLfloat previewPosition[] = {
        6.0f,
        8.0f,
        7.0f,
        1.0f
    };

    glLightfv(
        GL_LIGHT0,
        GL_AMBIENT,
        previewAmbient
    );

    glLightfv(
        GL_LIGHT0,
        GL_DIFFUSE,
        previewDiffuse
    );

    glLightfv(
        GL_LIGHT0,
        GL_SPECULAR,
        previewSpecular
    );

    glLightfv(
        GL_LIGHT0,
        GL_POSITION,
        previewPosition
    );
}

// -----------------------------------------------------------------------------
// Traffic and player drawing
// -----------------------------------------------------------------------------

void drawTraffic()
{
    for (int i = 0; i < MAX_TRAFFIC; ++i)
    {
        if (!traffic[i].active)
        {
            continue;
        }

        glPushMatrix();

        glTranslatef(
            traffic[i].x,
            0.0f,
            traffic[i].z
        );

        if (traffic[i].direction == OPPOSITE_DIRECTION)
        {
            glRotatef(
                180.0f,
                0.0f,
                1.0f,
                0.0f
            );
        }

        drawCarByModel(
            traffic[i].model,
            traffic[i].color,
            traffic[i].wheelRotation
        );

        glPopMatrix();
    }
}

void drawPlayerCar()
{
    const Color playerRed = {
        0.84f,
        0.018f,
        0.025f
    };

    glPushMatrix();

    glTranslatef(playerX, 0.0f, playerZ);

    // Small body roll while changing lanes.
    glRotatef(
        playerTilt,
        0.0f,
        0.0f,
        1.0f
    );

    drawCarByModel(
        selectedCarModel,
        playerRed,
        playerWheelRotation
    );

    glPopMatrix();
}

void drawRewardCoinInWorld()
{
    if (rewardMessageTimer <= 0.0f)
    {
        return;
    }

    const float bounce =
        std::sin(rewardMessageTimer * 8.0f) * 0.25f;

    glPushMatrix();

    glTranslatef(
        playerX,
        3.3f + bounce,
        playerZ - 5.0f
    );

    glScalef(0.52f, 0.52f, 0.52f);

    drawGoldenCoin(rewardCoinRotation);

    glPopMatrix();
}

// -----------------------------------------------------------------------------
// Orthographic overlay helpers
// -----------------------------------------------------------------------------

void beginOverlay()
{
    glPushAttrib(GL_ALL_ATTRIB_BITS);

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    glOrtho(
        0.0,
        static_cast<double>(windowWidth),
        0.0,
        static_cast<double>(windowHeight),
        -100.0,
        100.0
    );

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
}

void endOverlay()
{
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glMatrixMode(GL_MODELVIEW);

    glPopAttrib();
}

void drawSkyBackground()
{
    beginOverlay();

    glBegin(GL_QUADS);

    if (nightMode)
    {
        glColor3f(0.025f, 0.040f, 0.095f);
        glVertex2f(0.0f, 0.0f);
        glVertex2f(
            static_cast<float>(windowWidth),
            0.0f
        );

        glColor3f(0.005f, 0.010f, 0.035f);
        glVertex2f(
            static_cast<float>(windowWidth),
            static_cast<float>(windowHeight)
        );

        glVertex2f(
            0.0f,
            static_cast<float>(windowHeight)
        );
    }
    else
    {
        glColor3f(0.55f, 0.80f, 0.97f);
        glVertex2f(0.0f, 0.0f);
        glVertex2f(
            static_cast<float>(windowWidth),
            0.0f
        );

        glColor3f(0.16f, 0.52f, 0.90f);
        glVertex2f(
            static_cast<float>(windowWidth),
            static_cast<float>(windowHeight)
        );

        glVertex2f(
            0.0f,
            static_cast<float>(windowHeight)
        );
    }

    glEnd();

    if (nightMode)
    {
        glPointSize(2.2f);
        glColor3f(0.92f, 0.94f, 1.0f);

        glBegin(GL_POINTS);

        for (int i = 0; i < 80; ++i)
        {
            const float starX =
                static_cast<float>(
                    (i * 97 + 41) %
                    std::max(1, windowWidth)
                );

            const float starY =
                static_cast<float>(windowHeight) * 0.42f +
                static_cast<float>(
                    (i * 53 + 17) %
                    std::max(
                        1,
                        static_cast<int>(
                            windowHeight * 0.55f
                        )
                    )
                );

            glVertex2f(starX, starY);
        }

        glEnd();
    }

    const float circleX =
        static_cast<float>(windowWidth) * 0.82f;

    const float circleY =
        static_cast<float>(windowHeight) * 0.82f;

    const float radius =
        nightMode ? 31.0f : 38.0f;

    if (nightMode)
    {
        glColor3f(0.86f, 0.89f, 0.97f);
    }
    else
    {
        glColor3f(1.0f, 0.86f, 0.28f);
    }

    glBegin(GL_TRIANGLE_FAN);

    glVertex2f(circleX, circleY);

    for (int i = 0; i <= 36; ++i)
    {
        const float angle =
            2.0f * PI * static_cast<float>(i) / 36.0f;

        glVertex2f(
            circleX + std::cos(angle) * radius,
            circleY + std::sin(angle) * radius
        );
    }

    glEnd();

    endOverlay();
}

int bitmapTextWidth(void *font, const std::string &text)
{
    int width = 0;

    for (std::size_t i = 0; i < text.size(); ++i)
    {
        width += glutBitmapWidth(font, text[i]);
    }

    return width;
}

void drawText2D(
    float x,
    float y,
    const std::string &text,
    void *font,
    float red,
    float green,
    float blue
)
{
    glColor3f(red, green, blue);
    glRasterPos2f(x, y);

    for (std::size_t i = 0; i < text.size(); ++i)
    {
        glutBitmapCharacter(font, text[i]);
    }
}

void drawCenteredText2D(
    float centerX,
    float y,
    const std::string &text,
    void *font,
    float red,
    float green,
    float blue
)
{
    const int width =
        bitmapTextWidth(font, text);

    drawText2D(
        centerX - static_cast<float>(width) * 0.5f,
        y,
        text,
        font,
        red,
        green,
        blue
    );
}

void drawButton(
    const Rect &rect,
    const std::string &label,
    bool hovered
)
{
    glEnable(GL_BLEND);

    glBlendFunc(
        GL_SRC_ALPHA,
        GL_ONE_MINUS_SRC_ALPHA
    );

    if (hovered)
    {
        glColor4f(0.88f, 0.16f, 0.12f, 0.94f);
    }
    else
    {
        glColor4f(0.08f, 0.13f, 0.21f, 0.90f);
    }

    glBegin(GL_QUADS);

    glVertex2f(rect.x, rect.y);
    glVertex2f(rect.x + rect.width, rect.y);
    glVertex2f(
        rect.x + rect.width,
        rect.y + rect.height
    );
    glVertex2f(rect.x, rect.y + rect.height);

    glEnd();

    glLineWidth(2.0f);

    if (hovered)
    {
        glColor3f(1.0f, 0.86f, 0.30f);
    }
    else
    {
        glColor3f(0.70f, 0.78f, 0.90f);
    }

    glBegin(GL_LINE_LOOP);

    glVertex2f(rect.x, rect.y);
    glVertex2f(rect.x + rect.width, rect.y);
    glVertex2f(
        rect.x + rect.width,
        rect.y + rect.height
    );
    glVertex2f(rect.x, rect.y + rect.height);

    glEnd();

    drawCenteredText2D(
        rect.x + rect.width * 0.5f,
        rect.y + rect.height * 0.5f - 6.0f,
        label,
        GLUT_BITMAP_HELVETICA_18,
        1.0f,
        1.0f,
        1.0f
    );

    glDisable(GL_BLEND);
}

bool pointInsideRect(int x, int y, const Rect &rect)
{
    return
        static_cast<float>(x) >= rect.x &&
        static_cast<float>(x) <= rect.x + rect.width &&
        static_cast<float>(y) >= rect.y &&
        static_cast<float>(y) <= rect.y + rect.height;
}

void getMainMenuRects(
    Rect &startRect,
    Rect &carsRect,
    Rect &quitRect
)
{
    const float buttonWidth =
        std::min(
            420.0f,
            static_cast<float>(windowWidth) * 0.72f
        );

    const float buttonHeight = 62.0f;
    const float gap = 18.0f;

    const float x =
        (
            static_cast<float>(windowWidth) -
            buttonWidth
        ) * 0.5f;

    const float middleY =
        static_cast<float>(windowHeight) * 0.45f;

    startRect.x = x;
    startRect.y = middleY + buttonHeight + gap;
    startRect.width = buttonWidth;
    startRect.height = buttonHeight;

    carsRect.x = x;
    carsRect.y = middleY;
    carsRect.width = buttonWidth;
    carsRect.height = buttonHeight;

    quitRect.x = x;
    quitRect.y = middleY - buttonHeight - gap;
    quitRect.width = buttonWidth;
    quitRect.height = buttonHeight;
}

void getSelectionRects(
    Rect &previousRect,
    Rect &nextRect,
    Rect &backRect,
    Rect &selectRect
)
{
    previousRect.width = 95.0f;
    previousRect.height = 58.0f;
    previousRect.x =
        std::max(
            20.0f,
            static_cast<float>(windowWidth) * 0.08f
        );
    previousRect.y =
        static_cast<float>(windowHeight) * 0.43f;

    nextRect.width = 95.0f;
    nextRect.height = 58.0f;
    nextRect.x =
        static_cast<float>(windowWidth) -
        previousRect.x -
        nextRect.width;
    nextRect.y = previousRect.y;

    backRect.x = 28.0f;
    backRect.y = 24.0f;
    backRect.width = 145.0f;
    backRect.height = 54.0f;

    selectRect.width =
        std::min(
            330.0f,
            static_cast<float>(windowWidth) * 0.45f
        );

    selectRect.height = 54.0f;

    selectRect.x =
        (
            static_cast<float>(windowWidth) -
            selectRect.width
        ) * 0.5f;

    selectRect.y = 24.0f;
}

void updateHoveredButton()
{
    hoveredButton = BUTTON_NONE;

    if (gameState == MAIN_MENU)
    {
        Rect startRect;
        Rect carsRect;
        Rect quitRect;

        getMainMenuRects(
            startRect,
            carsRect,
            quitRect
        );

        if (pointInsideRect(mouseX, mouseY, startRect))
        {
            hoveredButton = BUTTON_START;
        }
        else if (pointInsideRect(mouseX, mouseY, carsRect))
        {
            hoveredButton = BUTTON_CARS;
        }
        else if (pointInsideRect(mouseX, mouseY, quitRect))
        {
            hoveredButton = BUTTON_QUIT;
        }
    }
    else if (gameState == CAR_SELECTION)
    {
        Rect previousRect;
        Rect nextRect;
        Rect backRect;
        Rect selectRect;

        getSelectionRects(
            previousRect,
            nextRect,
            backRect,
            selectRect
        );

        if (pointInsideRect(mouseX, mouseY, previousRect))
        {
            hoveredButton = BUTTON_PREVIOUS;
        }
        else if (pointInsideRect(mouseX, mouseY, nextRect))
        {
            hoveredButton = BUTTON_NEXT;
        }
        else if (pointInsideRect(mouseX, mouseY, backRect))
        {
            hoveredButton = BUTTON_BACK;
        }
        else if (pointInsideRect(mouseX, mouseY, selectRect))
        {
            hoveredButton = BUTTON_SELECT;
        }
    }
}

// -----------------------------------------------------------------------------
// Menu and selection screens
// -----------------------------------------------------------------------------

void drawMainMenu()
{
    beginOverlay();

    glEnable(GL_BLEND);

    glBlendFunc(
        GL_SRC_ALPHA,
        GL_ONE_MINUS_SRC_ALPHA
    );

    const float panelWidth =
        std::min(
            620.0f,
            static_cast<float>(windowWidth) * 0.90f
        );

    const float panelHeight =
        std::min(
            520.0f,
            static_cast<float>(windowHeight) * 0.88f
        );

    const float panelX =
        (
            static_cast<float>(windowWidth) -
            panelWidth
        ) * 0.5f;

    const float panelY =
        (
            static_cast<float>(windowHeight) -
            panelHeight
        ) * 0.5f;

    glColor4f(0.025f, 0.045f, 0.075f, 0.82f);

    glBegin(GL_QUADS);

    glVertex2f(panelX, panelY);
    glVertex2f(panelX + panelWidth, panelY);
    glVertex2f(
        panelX + panelWidth,
        panelY + panelHeight
    );
    glVertex2f(panelX, panelY + panelHeight);

    glEnd();

    glDisable(GL_BLEND);

    drawCenteredText2D(
        static_cast<float>(windowWidth) * 0.5f,
        static_cast<float>(windowHeight) - 105.0f,
        "FOUR-LANE HIGHWAY",
        GLUT_BITMAP_TIMES_ROMAN_24,
        1.0f,
        0.83f,
        0.20f
    );

    drawCenteredText2D(
        static_cast<float>(windowWidth) * 0.5f,
        static_cast<float>(windowHeight) - 140.0f,
        "OVERTAKE RACING",
        GLUT_BITMAP_TIMES_ROMAN_24,
        1.0f,
        1.0f,
        1.0f
    );

    drawCenteredText2D(
        static_cast<float>(windowWidth) * 0.5f,
        static_cast<float>(windowHeight) - 178.0f,
        "Legacy OpenGL Computer Graphics Project",
        GLUT_BITMAP_HELVETICA_18,
        0.68f,
        0.79f,
        0.94f
    );

    Rect startRect;
    Rect carsRect;
    Rect quitRect;

    getMainMenuRects(
        startRect,
        carsRect,
        quitRect
    );

    drawButton(
        startRect,
        "START RACING",
        hoveredButton == BUTTON_START
    );

    drawButton(
        carsRect,
        "MY CARS",
        hoveredButton == BUTTON_CARS
    );

    drawButton(
        quitRect,
        "QUIT GAME",
        hoveredButton == BUTTON_QUIT
    );

    drawCenteredText2D(
        static_cast<float>(windowWidth) * 0.5f,
        45.0f,
        "Mouse: choose an option    Esc: quit",
        GLUT_BITMAP_HELVETICA_12,
        0.75f,
        0.80f,
        0.88f
    );

    endOverlay();
}

void drawPreviewPlatform()
{
    if (sharedQuadric == 0)
    {
        setMaterial(0.18f, 0.22f, 0.28f, 0.75f, 72.0f);

        drawScaledCube(
            0.0f,
            0.15f,
            0.0f,
            7.0f,
            0.30f,
            7.0f
        );

        return;
    }

    glPushMatrix();

    setMaterial(0.18f, 0.22f, 0.28f, 0.78f, 78.0f);

    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);

    gluCylinder(
        sharedQuadric,
        3.8f,
        3.8f,
        0.42f,
        48,
        2
    );

    glTranslatef(0.0f, 0.0f, 0.42f);

    setMaterial(0.30f, 0.34f, 0.42f, 0.86f, 88.0f);

    gluDisk(
        sharedQuadric,
        0.0f,
        3.8f,
        48,
        4
    );

    glPopMatrix();

    setMaterial(1.0f, 0.72f, 0.08f, 0.85f, 86.0f);

    glPushMatrix();

    glTranslatef(0.0f, 0.44f, 0.0f);
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);

    glutSolidTorus(
        0.07f,
        3.68f,
        10,
        48
    );

    glPopMatrix();
}

void drawCarSelection()
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    gluPerspective(
        45.0,
        static_cast<double>(windowWidth) /
            static_cast<double>(windowHeight),
        0.1,
        100.0
    );

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    gluLookAt(
        8.3, 5.5, 10.5,
        0.0, 1.15, 0.0,
        0.0, 1.0, 0.0
    );

    glEnable(GL_DEPTH_TEST);

    setupPreviewLights();
    drawPreviewPlatform();

    const Color previewRed = {
        0.84f,
        0.018f,
        0.025f
    };

    glPushMatrix();

    glTranslatef(0.0f, 0.46f, 0.0f);

    glRotatef(
        previewRotation,
        0.0f,
        1.0f,
        0.0f
    );

    drawCarByModel(
        previewCarModel,
        previewRed,
        previewWheelRotation
    );

    glPopMatrix();

    beginOverlay();

    drawCenteredText2D(
        static_cast<float>(windowWidth) * 0.5f,
        static_cast<float>(windowHeight) - 48.0f,
        "MY CARS",
        GLUT_BITMAP_TIMES_ROMAN_24,
        1.0f,
        0.84f,
        0.22f
    );

    drawCenteredText2D(
        static_cast<float>(windowWidth) * 0.5f,
        static_cast<float>(windowHeight) - 80.0f,
        carModelName(previewCarModel),
        GLUT_BITMAP_HELVETICA_18,
        1.0f,
        1.0f,
        1.0f
    );

    std::ostringstream modelText;
    modelText
        << "Model "
        << previewCarModel + 1
        << " of "
        << CAR_MODEL_COUNT;

    drawCenteredText2D(
        static_cast<float>(windowWidth) * 0.5f,
        static_cast<float>(windowHeight) - 106.0f,
        modelText.str(),
        GLUT_BITMAP_HELVETICA_12,
        0.74f,
        0.82f,
        0.94f
    );

    if (previewCarModel == selectedCarModel)
    {
        drawCenteredText2D(
            static_cast<float>(windowWidth) * 0.5f,
            static_cast<float>(windowHeight) - 131.0f,
            "CURRENTLY SELECTED",
            GLUT_BITMAP_HELVETICA_18,
            0.28f,
            1.0f,
            0.40f
        );
    }
    else
    {
        drawCenteredText2D(
            static_cast<float>(windowWidth) * 0.5f,
            static_cast<float>(windowHeight) - 131.0f,
            "Press Enter to select and start",
            GLUT_BITMAP_HELVETICA_12,
            0.92f,
            0.92f,
            0.92f
        );
    }

    Rect previousRect;
    Rect nextRect;
    Rect backRect;
    Rect selectRect;

    getSelectionRects(
        previousRect,
        nextRect,
        backRect,
        selectRect
    );

    drawButton(
        previousRect,
        "< PREV",
        hoveredButton == BUTTON_PREVIOUS
    );

    drawButton(
        nextRect,
        "NEXT >",
        hoveredButton == BUTTON_NEXT
    );

    drawButton(
        backRect,
        "BACK",
        hoveredButton == BUTTON_BACK
    );

    drawButton(
        selectRect,
        "START WITH THIS CAR",
        hoveredButton == BUTTON_SELECT
    );

    drawCenteredText2D(
        static_cast<float>(windowWidth) * 0.5f,
        92.0f,
        "Left/Right: change model    Enter: start    B: back",
        GLUT_BITMAP_HELVETICA_12,
        0.80f,
        0.85f,
        0.94f
    );

    endOverlay();
}

// -----------------------------------------------------------------------------
// Cockpit, HUD and game overlays
// -----------------------------------------------------------------------------

void drawDashboardAndWindshield()
{
    beginOverlay();

    const float width =
        static_cast<float>(windowWidth);

    const float height =
        static_cast<float>(windowHeight);

    const float dashboardHeight =
        height * 0.245f;

    // Textured dashboard.
    glEnable(GL_TEXTURE_2D);
    glBindTexture(
        GL_TEXTURE_2D,
        textures[TEX_DASHBOARD]
    );

    glColor3f(0.72f, 0.72f, 0.72f);

    glBegin(GL_QUADS);

    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(0.0f, 0.0f);

    glTexCoord2f(8.0f, 0.0f);
    glVertex2f(width, 0.0f);

    glTexCoord2f(8.0f, 3.0f);
    glVertex2f(width, dashboardHeight);

    glTexCoord2f(0.0f, 3.0f);
    glVertex2f(0.0f, dashboardHeight);

    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);

    // Windshield top frame.
    glColor3f(0.035f, 0.040f, 0.050f);

    glBegin(GL_QUADS);

    glVertex2f(0.0f, height - 52.0f);
    glVertex2f(width, height - 52.0f);
    glVertex2f(width, height);
    glVertex2f(0.0f, height);

    glEnd();

    // Left sloped windshield pillar.
    glBegin(GL_QUADS);

    glVertex2f(0.0f, dashboardHeight);
    glVertex2f(75.0f, dashboardHeight);
    glVertex2f(56.0f, height - 50.0f);
    glVertex2f(0.0f, height - 50.0f);

    glEnd();

    // Right sloped windshield pillar.
    glBegin(GL_QUADS);

    glVertex2f(width - 75.0f, dashboardHeight);
    glVertex2f(width, dashboardHeight);
    glVertex2f(width, height - 50.0f);
    glVertex2f(width - 56.0f, height - 50.0f);

    glEnd();

    // Dashboard upper trim.
    glColor3f(0.14f, 0.15f, 0.17f);

    glBegin(GL_QUADS);

    glVertex2f(0.0f, dashboardHeight - 8.0f);
    glVertex2f(width, dashboardHeight - 8.0f);
    glVertex2f(width, dashboardHeight + 12.0f);
    glVertex2f(0.0f, dashboardHeight + 12.0f);

    glEnd();

    // Steering wheel.
    const float steeringX = width * 0.36f;
    const float steeringY = dashboardHeight * 0.48f;
    const float steeringRadius =
        std::min(58.0f, height * 0.09f);

    glColor3f(0.06f, 0.065f, 0.075f);
    glLineWidth(9.0f);

    glBegin(GL_LINE_LOOP);

    for (int i = 0; i < 48; ++i)
    {
        const float angle =
            2.0f * PI * static_cast<float>(i) / 48.0f;

        glVertex2f(
            steeringX +
                std::cos(angle) * steeringRadius,
            steeringY +
                std::sin(angle) * steeringRadius
        );
    }

    glEnd();

    glLineWidth(5.0f);

    glBegin(GL_LINES);

    glVertex2f(
        steeringX - steeringRadius,
        steeringY
    );
    glVertex2f(
        steeringX + steeringRadius,
        steeringY
    );

    glVertex2f(
        steeringX,
        steeringY
    );
    glVertex2f(
        steeringX,
        steeringY - steeringRadius
    );

    glEnd();

    // Two dashboard gauges.
    for (int gauge = 0; gauge < 2; ++gauge)
    {
        const float gaugeX =
            width * 0.57f +
            static_cast<float>(gauge) * 82.0f;

        const float gaugeY =
            dashboardHeight * 0.50f;

        glColor3f(0.80f, 0.84f, 0.89f);
        glLineWidth(2.0f);

        glBegin(GL_LINE_LOOP);

        for (int i = 0; i < 32; ++i)
        {
            const float angle =
                2.0f * PI *
                static_cast<float>(i) / 32.0f;

            glVertex2f(
                gaugeX + std::cos(angle) * 29.0f,
                gaugeY + std::sin(angle) * 29.0f
            );
        }

        glEnd();

        glColor3f(0.95f, 0.18f, 0.10f);
        glLineWidth(3.0f);

        glBegin(GL_LINES);

        glVertex2f(gaugeX, gaugeY);

        glVertex2f(
            gaugeX + 18.0f,
            gaugeY + 12.0f
        );

        glEnd();
    }

    endOverlay();
}

void drawHUD()
{
    beginOverlay();

    glEnable(GL_BLEND);

    glBlendFunc(
        GL_SRC_ALPHA,
        GL_ONE_MINUS_SRC_ALPHA
    );

    const float panelX = 14.0f;
    const float panelY =
        static_cast<float>(windowHeight) - 255.0f;

    const float panelWidth = 355.0f;
    const float panelHeight = 238.0f;

    glColor4f(0.015f, 0.025f, 0.045f, 0.76f);

    glBegin(GL_QUADS);

    glVertex2f(panelX, panelY);
    glVertex2f(panelX + panelWidth, panelY);
    glVertex2f(
        panelX + panelWidth,
        panelY + panelHeight
    );
    glVertex2f(
        panelX,
        panelY + panelHeight
    );

    glEnd();

    glDisable(GL_BLEND);

    // Reusable physical coin model as the HUD icon.
    glPushMatrix();

    glTranslatef(
        panelX + 27.0f,
        panelY + panelHeight - 33.0f,
        0.0f
    );

    glScalef(11.0f, 11.0f, 11.0f);
    drawGoldenCoin(rewardCoinRotation);

    glPopMatrix();

    std::ostringstream line;

    line << "Coins: " << coinCount;

    drawText2D(
        panelX + 52.0f,
        panelY + panelHeight - 39.0f,
        line.str(),
        GLUT_BITMAP_HELVETICA_18,
        1.0f,
        0.82f,
        0.18f
    );

    line.str("");
    line.clear();
    line << "Vehicles overtaken: " << overtakeCount;

    drawText2D(
        panelX + 18.0f,
        panelY + panelHeight - 70.0f,
        line.str(),
        GLUT_BITMAP_HELVETICA_12,
        0.94f,
        0.94f,
        0.96f
    );

    line.str("");
    line.clear();
    line
        << "Speed: "
        << static_cast<int>(playerSpeed * 160.0f + 0.5f)
        << " km/h  ["
        << std::fixed
        << std::setprecision(2)
        << playerSpeed
        << "]";

    drawText2D(
        panelX + 18.0f,
        panelY + panelHeight - 94.0f,
        line.str(),
        GLUT_BITMAP_HELVETICA_12,
        0.94f,
        0.94f,
        0.96f
    );

    line.str("");
    line.clear();
    line
        << "Distance: "
        << std::fixed
        << std::setprecision(0)
        << distanceTravelled
        << " m";

    drawText2D(
        panelX + 18.0f,
        panelY + panelHeight - 118.0f,
        line.str(),
        GLUT_BITMAP_HELVETICA_12,
        0.94f,
        0.94f,
        0.96f
    );

    line.str("");
    line.clear();
    line << "Current lane: " << targetLane + 1;

    drawText2D(
        panelX + 18.0f,
        panelY + panelHeight - 142.0f,
        line.str(),
        GLUT_BITMAP_HELVETICA_12,
        0.94f,
        0.94f,
        0.96f
    );

    line.str("");
    line.clear();
    line << "Car: " << carModelName(selectedCarModel);

    drawText2D(
        panelX + 18.0f,
        panelY + panelHeight - 166.0f,
        line.str(),
        GLUT_BITMAP_HELVETICA_12,
        0.94f,
        0.94f,
        0.96f
    );

    line.str("");
    line.clear();

    line
        << "Camera: "
        << (
            firstPersonCamera
                ? "Cockpit"
                : "Third-person"
        );

    drawText2D(
        panelX + 18.0f,
        panelY + panelHeight - 190.0f,
        line.str(),
        GLUT_BITMAP_HELVETICA_12,
        0.94f,
        0.94f,
        0.96f
    );

    line.str("");
    line.clear();

    line
        << "Environment: "
        << (nightMode ? "Night" : "Day");

    if (gameState == PAUSED)
    {
        line << "    PAUSED";
    }

    drawText2D(
        panelX + 18.0f,
        panelY + panelHeight - 214.0f,
        line.str(),
        GLUT_BITMAP_HELVETICA_12,
        nightMode ? 0.70f : 0.78f,
        nightMode ? 0.82f : 0.92f,
        1.0f
    );

    if (rewardMessageTimer > 0.0f)
    {
        drawCenteredText2D(
            static_cast<float>(windowWidth) * 0.5f,
            static_cast<float>(windowHeight) - 105.0f,
            "+1 COIN",
            GLUT_BITMAP_TIMES_ROMAN_24,
            1.0f,
            0.78f,
            0.08f
        );
    }

    if (showControls)
    {
        glEnable(GL_BLEND);

        glBlendFunc(
            GL_SRC_ALPHA,
            GL_ONE_MINUS_SRC_ALPHA
        );

        glColor4f(0.02f, 0.03f, 0.05f, 0.73f);

        glBegin(GL_QUADS);

        glVertex2f(12.0f, 12.0f);
        glVertex2f(
            static_cast<float>(windowWidth) - 12.0f,
            12.0f
        );
        glVertex2f(
            static_cast<float>(windowWidth) - 12.0f,
            68.0f
        );
        glVertex2f(12.0f, 68.0f);

        glEnd();

        glDisable(GL_BLEND);

        drawCenteredText2D(
            static_cast<float>(windowWidth) * 0.5f,
            44.0f,
            "A/D or Left/Right: lane    W/S or Up/Down: speed    V: camera    P: pause",
            GLUT_BITMAP_HELVETICA_12,
            0.92f,
            0.94f,
            1.0f
        );

        drawCenteredText2D(
            static_cast<float>(windowWidth) * 0.5f,
            23.0f,
            "N: day/night    H: hide help    M or Esc: menu",
            GLUT_BITMAP_HELVETICA_12,
            0.78f,
            0.84f,
            0.94f
        );
    }

    endOverlay();
}

void drawPauseOverlay()
{
    beginOverlay();

    glEnable(GL_BLEND);

    glBlendFunc(
        GL_SRC_ALPHA,
        GL_ONE_MINUS_SRC_ALPHA
    );

    glColor4f(0.0f, 0.0f, 0.0f, 0.57f);

    glBegin(GL_QUADS);

    glVertex2f(0.0f, 0.0f);
    glVertex2f(
        static_cast<float>(windowWidth),
        0.0f
    );
    glVertex2f(
        static_cast<float>(windowWidth),
        static_cast<float>(windowHeight)
    );
    glVertex2f(
        0.0f,
        static_cast<float>(windowHeight)
    );

    glEnd();

    glDisable(GL_BLEND);

    drawCenteredText2D(
        static_cast<float>(windowWidth) * 0.5f,
        static_cast<float>(windowHeight) * 0.55f,
        "RACE PAUSED",
        GLUT_BITMAP_TIMES_ROMAN_24,
        1.0f,
        0.84f,
        0.20f
    );

    drawCenteredText2D(
        static_cast<float>(windowWidth) * 0.5f,
        static_cast<float>(windowHeight) * 0.55f - 38.0f,
        "Press P to resume",
        GLUT_BITMAP_HELVETICA_18,
        1.0f,
        1.0f,
        1.0f
    );

    endOverlay();
}

void drawGameOverOverlay()
{
    beginOverlay();

    glEnable(GL_BLEND);

    glBlendFunc(
        GL_SRC_ALPHA,
        GL_ONE_MINUS_SRC_ALPHA
    );

    const float panelWidth =
        std::min(
            540.0f,
            static_cast<float>(windowWidth) * 0.88f
        );

    const float panelHeight = 340.0f;

    const float panelX =
        (
            static_cast<float>(windowWidth) -
            panelWidth
        ) * 0.5f;

    const float panelY =
        (
            static_cast<float>(windowHeight) -
            panelHeight
        ) * 0.5f;

    glColor4f(0.035f, 0.015f, 0.018f, 0.90f);

    glBegin(GL_QUADS);

    glVertex2f(panelX, panelY);
    glVertex2f(panelX + panelWidth, panelY);
    glVertex2f(
        panelX + panelWidth,
        panelY + panelHeight
    );
    glVertex2f(
        panelX,
        panelY + panelHeight
    );

    glEnd();

    glDisable(GL_BLEND);

    drawCenteredText2D(
        static_cast<float>(windowWidth) * 0.5f,
        panelY + panelHeight - 65.0f,
        "CRASHED!",
        GLUT_BITMAP_TIMES_ROMAN_24,
        1.0f,
        0.12f,
        0.08f
    );

    std::ostringstream line;

    line << "Golden coins: " << coinCount;

    drawCenteredText2D(
        static_cast<float>(windowWidth) * 0.5f,
        panelY + panelHeight - 120.0f,
        line.str(),
        GLUT_BITMAP_HELVETICA_18,
        1.0f,
        0.80f,
        0.12f
    );

    line.str("");
    line.clear();
    line << "Vehicles overtaken: " << overtakeCount;

    drawCenteredText2D(
        static_cast<float>(windowWidth) * 0.5f,
        panelY + panelHeight - 158.0f,
        line.str(),
        GLUT_BITMAP_HELVETICA_18,
        1.0f,
        1.0f,
        1.0f
    );

    line.str("");
    line.clear();

    line
        << "Distance travelled: "
        << std::fixed
        << std::setprecision(0)
        << distanceTravelled
        << " m";

    drawCenteredText2D(
        static_cast<float>(windowWidth) * 0.5f,
        panelY + panelHeight - 196.0f,
        line.str(),
        GLUT_BITMAP_HELVETICA_18,
        1.0f,
        1.0f,
        1.0f
    );

    drawCenteredText2D(
        static_cast<float>(windowWidth) * 0.5f,
        panelY + 77.0f,
        "R - Restart Race",
        GLUT_BITMAP_HELVETICA_18,
        0.56f,
        1.0f,
        0.62f
    );

    drawCenteredText2D(
        static_cast<float>(windowWidth) * 0.5f,
        panelY + 43.0f,
        "M or Esc - Main Menu",
        GLUT_BITMAP_HELVETICA_18,
        0.78f,
        0.84f,
        1.0f
    );

    endOverlay();
}

// -----------------------------------------------------------------------------
// Game scene and cameras
// -----------------------------------------------------------------------------

void drawGameScene()
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    gluPerspective(
        62.0,
        static_cast<double>(windowWidth) /
            static_cast<double>(windowHeight),
        0.1,
        700.0
    );

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if (firstPersonCamera)
    {
        const float eyeHeight =
            cockpitEyeHeight(selectedCarModel);

        gluLookAt(
            playerX,
            eyeHeight,
            playerZ + 0.15f,

            playerX,
            eyeHeight - 0.07f,
            playerZ - 42.0f,

            0.0f,
            1.0f,
            0.0f
        );
    }
    else
    {
        gluLookAt(
            cameraFollowX,
            7.3f,
            playerZ + 15.0f,

            playerX,
            1.05f,
            playerZ - 25.0f,

            0.0f,
            1.0f,
            0.0f
        );
    }

    // Light positions are set after the viewing transform.
    setupLights();

    drawRoad();
    drawHillsAndCity();
    drawRoadsideEnvironment();
    drawTraffic();

    if (!firstPersonCamera)
    {
        drawPlayerCar();
    }

    drawRewardCoinInWorld();

    if (firstPersonCamera)
    {
        drawDashboardAndWindshield();
    }

    drawHUD();

    if (gameState == PAUSED)
    {
        drawPauseOverlay();
    }
    else if (gameState == GAME_OVER)
    {
        drawGameOverOverlay();
    }
}

// -----------------------------------------------------------------------------
// Traffic spawning
// -----------------------------------------------------------------------------

bool isSpawnPositionSafe(
    int lane,
    float candidateZ,
    int slotToIgnore
)
{
    const float minimumLaneGap =
        lane <= 1 ? 54.0f : 78.0f;

    int carsInHorizontalSlice = 0;

    for (int i = 0; i < MAX_TRAFFIC; ++i)
    {
        if (
            i == slotToIgnore ||
            !traffic[i].active
        )
        {
            continue;
        }

        const float separation =
            std::fabs(traffic[i].z - candidateZ);

        if (
            traffic[i].lane == lane &&
            separation < minimumLaneGap
        )
        {
            return false;
        }

        if (separation < 16.0f)
        {
            ++carsInHorizontalSlice;
        }
    }

    // A candidate is rejected if it would become the third
    // vehicle in one narrow cross-road slice. This prevents
    // impossible four-lane traffic walls.
    if (carsInHorizontalSlice >= 2)
    {
        return false;
    }

    return true;
}

bool findSafeSpawnPosition(
    int lane,
    int direction,
    int slotIndex,
    float &safeZ
)
{
    float minimumDistance;
    float maximumDistance;

    if (direction == SAME_DIRECTION)
    {
        minimumDistance = 85.0f;
        maximumDistance = 390.0f;
    }
    else
    {
        minimumDistance = 150.0f;
        maximumDistance = 520.0f;
    }

    for (int attempt = 0; attempt < 35; ++attempt)
    {
        const float candidateZ =
            playerZ -
            randomFloat(
                minimumDistance,
                maximumDistance
            );

        if (
            isSpawnPositionSafe(
                lane,
                candidateZ,
                slotIndex
            )
        )
        {
            safeZ = candidateZ;
            return true;
        }
    }

    return false;
}

bool spawnTrafficCar(int slotIndex, int direction)
{
    const int firstLane =
        direction == SAME_DIRECTION
            ? randomInt(0, 1)
            : randomInt(2, 3);

    const int secondLane =
        direction == SAME_DIRECTION
            ? (firstLane == 0 ? 1 : 0)
            : (firstLane == 2 ? 3 : 2);

    const int possibleLanes[2] = {
        firstLane,
        secondLane
    };

    float safeZ = 0.0f;
    int chosenLane = -1;

    for (int laneAttempt = 0; laneAttempt < 2; ++laneAttempt)
    {
        if (
            findSafeSpawnPosition(
                possibleLanes[laneAttempt],
                direction,
                slotIndex,
                safeZ
            )
        )
        {
            chosenLane =
                possibleLanes[laneAttempt];

            break;
        }
    }

    if (chosenLane < 0)
    {
        traffic[slotIndex].active = false;
        return false;
    }

    const float difficultyIncrease =
        std::min(
            0.05f,
            static_cast<float>(coinCount / 5) * 0.01f
        );

    TrafficCar &car = traffic[slotIndex];

    car.id = nextTrafficId++;
    car.active = true;
    car.lane = chosenLane;
    car.x = LANE_X[chosenLane];
    car.z = safeZ;
    car.direction = direction;

    if (direction == SAME_DIRECTION)
    {
        car.speed =
            randomFloat(0.30f, 0.50f) +
            difficultyIncrease;
    }
    else
    {
        car.speed =
            randomFloat(0.25f, 0.40f) +
            difficultyIncrease;
    }

    car.model =
        randomInt(0, CAR_MODEL_COUNT - 1);

    car.color =
        NPC_COLORS[
            randomInt(0, NPC_COLOR_COUNT - 1)
        ];

    car.coinAwarded = false;

    car.wasAheadOfPlayer =
        direction == SAME_DIRECTION;

    car.collidedWithPlayer = false;
    car.wheelRotation = randomFloat(0.0f, 359.0f);

    return true;
}

// -----------------------------------------------------------------------------
// Game reset and updates
// -----------------------------------------------------------------------------

void resetGame()
{
    playerX = LANE_X[1];
    playerZ = 0.0f;
    playerSpeed = START_PLAYER_SPEED;
    playerTilt = 0.0f;
    playerWheelRotation = 0.0f;

    targetLane = 1;
    cameraFollowX = playerX;

    firstPersonCamera = false;

    coinCount = 0;
    overtakeCount = 0;
    distanceTravelled = 0.0f;

    rewardMessageTimer = 0.0f;
    rewardCoinRotation = 0.0f;

    nextTrafficId = 1;
    respawnRetryTimer = 1.0f;

    for (int i = 0; i < MAX_TRAFFIC; ++i)
    {
        traffic[i].id = 0;
        traffic[i].active = false;
        traffic[i].lane = 0;
        traffic[i].x = 0.0f;
        traffic[i].z = 0.0f;
        traffic[i].speed = 0.0f;

        traffic[i].direction =
            i < SAME_TRAFFIC_COUNT
                ? SAME_DIRECTION
                : OPPOSITE_DIRECTION;

        traffic[i].model = SEDAN;
        traffic[i].color = NPC_COLORS[0];
        traffic[i].coinAwarded = false;
        traffic[i].wasAheadOfPlayer = false;
        traffic[i].collidedWithPlayer = false;
        traffic[i].wheelRotation = 0.0f;
    }

    for (int i = 0; i < SAME_TRAFFIC_COUNT; ++i)
    {
        spawnTrafficCar(i, SAME_DIRECTION);
    }

    for (
        int i = SAME_TRAFFIC_COUNT;
        i < MAX_TRAFFIC;
        ++i
    )
    {
        spawnTrafficCar(i, OPPOSITE_DIRECTION);
    }

    gameState = PLAYING;
    hoveredButton = BUTTON_NONE;

    previousTimerMilliseconds =
        glutGet(GLUT_ELAPSED_TIME);
}

void requestLaneChange(int laneDelta)
{
    targetLane += laneDelta;

    if (targetLane < 0)
    {
        targetLane = 0;
    }

    if (targetLane >= LANE_COUNT)
    {
        targetLane = LANE_COUNT - 1;
    }
}

void updatePlayer(float deltaTime)
{
    const float forwardDistance =
        playerSpeed *
        WORLD_SPEED_SCALE *
        deltaTime;

    playerZ -= forwardDistance;
    distanceTravelled += forwardDistance;

    // Wheel rotation is derived from actual distance travelled.
    playerWheelRotation -=
        (
            forwardDistance / 0.50f
        ) *
        180.0f /
        PI;

    if (
        playerWheelRotation < -360.0f ||
        playerWheelRotation > 360.0f
    )
    {
        playerWheelRotation =
            std::fmod(
                playerWheelRotation,
                360.0f
            );
    }

    // Smooth lane movement.
    const float targetX = LANE_X[targetLane];
    const float difference = targetX - playerX;
    const float maximumLateralMove =
        7.0f * deltaTime;

    if (
        std::fabs(difference) <=
        maximumLateralMove
    )
    {
        playerX = targetX;
    }
    else
    {
        playerX +=
            difference > 0.0f
                ? maximumLateralMove
                : -maximumLateralMove;
    }

    // Body tilt follows the lateral movement error.
    float desiredTilt = 0.0f;

    if (std::fabs(difference) > 0.02f)
    {
        desiredTilt =
            clampFloat(
                -difference * 2.3f,
                -8.0f,
                8.0f
            );
    }

    const float tiltBlend =
        std::min(
            1.0f,
            deltaTime * 8.5f
        );

    playerTilt +=
        (desiredTilt - playerTilt) *
        tiltBlend;

    const float cameraBlend =
        std::min(
            1.0f,
            deltaTime * 5.5f
        );

    cameraFollowX +=
        (playerX - cameraFollowX) *
        cameraBlend;
}

void updateTraffic(float deltaTime)
{
    for (int i = 0; i < MAX_TRAFFIC; ++i)
    {
        if (!traffic[i].active)
        {
            continue;
        }

        TrafficCar &car = traffic[i];

        float effectiveSpeed = car.speed;

        // Simple same-lane following prevents faster NPCs from
        // entering slower NPCs after a safe initial spawn.
        for (int j = 0; j < MAX_TRAFFIC; ++j)
        {
            if (
                i == j ||
                !traffic[j].active ||
                traffic[j].lane != car.lane ||
                traffic[j].direction != car.direction
            )
            {
                continue;
            }

            float gapToCarAhead = -1.0f;

            if (car.direction == SAME_DIRECTION)
            {
                if (traffic[j].z < car.z)
                {
                    gapToCarAhead =
                        car.z - traffic[j].z;
                }
            }
            else
            {
                if (traffic[j].z > car.z)
                {
                    gapToCarAhead =
                        traffic[j].z - car.z;
                }
            }

            if (
                gapToCarAhead > 0.0f &&
                gapToCarAhead < 31.0f &&
                effectiveSpeed > traffic[j].speed
            )
            {
                effectiveSpeed =
                    traffic[j].speed;
            }
        }

        const float distanceMoved =
            effectiveSpeed *
            WORLD_SPEED_SCALE *
            deltaTime;

        car.z +=
            static_cast<float>(car.direction) *
            distanceMoved;

        car.wheelRotation -=
            (
                distanceMoved / 0.52f
            ) *
            180.0f /
            PI;

        if (
            car.wheelRotation < -360.0f ||
            car.wheelRotation > 360.0f
        )
        {
            car.wheelRotation =
                std::fmod(
                    car.wheelRotation,
                    360.0f
                );
        }
    }
}

void checkCollisions()
{
    for (int i = 0; i < MAX_TRAFFIC; ++i)
    {
        if (!traffic[i].active)
        {
            continue;
        }

        const float xCollisionLimit =
            (
                carWidth(selectedCarModel) +
                carWidth(traffic[i].model)
            ) *
            0.42f;

        const float zCollisionLimit =
            (
                carLength(selectedCarModel) +
                carLength(traffic[i].model)
            ) *
            0.40f;

        const bool overlapsX =
            std::fabs(playerX - traffic[i].x) <
            xCollisionLimit;

        const bool overlapsZ =
            std::fabs(playerZ - traffic[i].z) <
            zCollisionLimit;

        if (overlapsX && overlapsZ)
        {
            traffic[i].collidedWithPlayer = true;
            gameState = GAME_OVER;
            return;
        }
    }
}

void checkOvertakes()
{
    for (int i = 0; i < MAX_TRAFFIC; ++i)
    {
        TrafficCar &car = traffic[i];

        if (
            !car.active ||
            car.direction != SAME_DIRECTION ||
            car.coinAwarded ||
            car.collidedWithPlayer
        )
        {
            continue;
        }

        const float relativeZ =
            car.z - playerZ;

        const float clearDistance =
            (
                carLength(selectedCarModel) +
                carLength(car.model)
            ) *
            0.5f +
            1.0f;

        // Negative relative Z means the NPC is ahead.
        if (relativeZ < -clearDistance)
        {
            car.wasAheadOfPlayer = true;
        }

        // Positive relative Z means the player is now ahead.
        if (
            car.wasAheadOfPlayer &&
            relativeZ > clearDistance
        )
        {
            car.coinAwarded = true;

            ++coinCount;
            ++overtakeCount;

            rewardMessageTimer = 1.55f;
            rewardCoinRotation = 0.0f;
        }
    }
}

void recycleTraffic()
{
    for (int i = 0; i < MAX_TRAFFIC; ++i)
    {
        if (!traffic[i].active)
        {
            continue;
        }

        const float relativeZ =
            traffic[i].z - playerZ;

        bool shouldRecycle = false;

        if (traffic[i].direction == SAME_DIRECTION)
        {
            shouldRecycle =
                relativeZ > 100.0f ||
                relativeZ < -560.0f;
        }
        else
        {
            shouldRecycle =
                relativeZ > 110.0f ||
                relativeZ < -620.0f;
        }

        if (shouldRecycle)
        {
            const int direction =
                traffic[i].direction;

            traffic[i].active = false;

            // A failed safe spawn leaves the pooled slot inactive.
            // It will be retried later without forcing an unsafe car.
            spawnTrafficCar(i, direction);
        }
    }
}

void updateAnimations(float deltaTime)
{
    rewardCoinRotation += 190.0f * deltaTime;

    if (rewardCoinRotation >= 360.0f)
    {
        rewardCoinRotation -= 360.0f;
    }

    if (rewardMessageTimer > 0.0f)
    {
        rewardMessageTimer -= deltaTime;

        if (rewardMessageTimer < 0.0f)
        {
            rewardMessageTimer = 0.0f;
        }
    }

    respawnRetryTimer -= deltaTime;

    if (respawnRetryTimer <= 0.0f)
    {
        respawnRetryTimer = 1.0f;

        for (int i = 0; i < MAX_TRAFFIC; ++i)
        {
            if (!traffic[i].active)
            {
                const int direction =
                    i < SAME_TRAFFIC_COUNT
                        ? SAME_DIRECTION
                        : OPPOSITE_DIRECTION;

                spawnTrafficCar(i, direction);
            }
        }
    }
}

// -----------------------------------------------------------------------------
// GLUT callbacks
// -----------------------------------------------------------------------------

void display()
{
    glClear(
        GL_COLOR_BUFFER_BIT |
        GL_DEPTH_BUFFER_BIT
    );

    drawSkyBackground();

    if (gameState == MAIN_MENU)
    {
        drawMainMenu();
    }
    else if (gameState == CAR_SELECTION)
    {
        drawCarSelection();
    }
    else
    {
        drawGameScene();
    }

    glutSwapBuffers();
}

void reshape(int width, int height)
{
    windowWidth = std::max(1, width);
    windowHeight = std::max(1, height);

    glViewport(
        0,
        0,
        windowWidth,
        windowHeight
    );

    updateHoveredButton();
}

void keyboard(unsigned char key, int x, int y)
{
    (void)x;
    (void)y;

    if (key == 27)
    {
        if (gameState == MAIN_MENU)
        {
            quitGame();
        }
        else
        {
            gameState = MAIN_MENU;
            hoveredButton = BUTTON_NONE;
        }

        glutPostRedisplay();
        return;
    }

    if (gameState == MAIN_MENU)
    {
        if (key == 13)
        {
            resetGame();
        }

        return;
    }

    if (gameState == CAR_SELECTION)
    {
        if (key == 'b' || key == 'B')
        {
            gameState = MAIN_MENU;
            hoveredButton = BUTTON_NONE;
        }
        else if (key == 13)
        {
            selectedCarModel = previewCarModel;
            resetGame();
        }

        glutPostRedisplay();
        return;
    }

    if (gameState == GAME_OVER)
    {
        if (key == 'r' || key == 'R')
        {
            resetGame();
        }
        else if (key == 'm' || key == 'M')
        {
            gameState = MAIN_MENU;
            hoveredButton = BUTTON_NONE;
        }

        glutPostRedisplay();
        return;
    }

    if (
        gameState == PLAYING ||
        gameState == PAUSED
    )
    {
        if (key == 'p' || key == 'P')
        {
            if (gameState == PLAYING)
            {
                gameState = PAUSED;
            }
            else
            {
                gameState = PLAYING;
            }

            previousTimerMilliseconds =
                glutGet(GLUT_ELAPSED_TIME);
        }
        else if (key == 'v' || key == 'V')
        {
            firstPersonCamera =
                !firstPersonCamera;
        }
        else if (key == 'n' || key == 'N')
        {
            nightMode = !nightMode;
        }
        else if (key == 'h' || key == 'H')
        {
            showControls = !showControls;
        }
        else if (key == 'm' || key == 'M')
        {
            gameState = MAIN_MENU;
            hoveredButton = BUTTON_NONE;
        }
    }

    if (gameState == PLAYING)
    {
        if (key == 'a' || key == 'A')
        {
            requestLaneChange(-1);
        }
        else if (key == 'd' || key == 'D')
        {
            requestLaneChange(1);
        }
        else if (key == 'w' || key == 'W')
        {
            playerSpeed =
                clampFloat(
                    playerSpeed + 0.05f,
                    MIN_PLAYER_SPEED,
                    MAX_PLAYER_SPEED
                );
        }
        else if (key == 's' || key == 'S')
        {
            playerSpeed =
                clampFloat(
                    playerSpeed - 0.05f,
                    MIN_PLAYER_SPEED,
                    MAX_PLAYER_SPEED
                );
        }
    }

    glutPostRedisplay();
}

void specialKeys(int key, int x, int y)
{
    (void)x;
    (void)y;

    if (gameState == CAR_SELECTION)
    {
        if (key == GLUT_KEY_LEFT)
        {
            previewCarModel =
                positiveModulo(
                    previewCarModel - 1,
                    CAR_MODEL_COUNT
                );
        }
        else if (key == GLUT_KEY_RIGHT)
        {
            previewCarModel =
                positiveModulo(
                    previewCarModel + 1,
                    CAR_MODEL_COUNT
                );
        }

        glutPostRedisplay();
        return;
    }

    if (gameState != PLAYING)
    {
        return;
    }

    if (key == GLUT_KEY_LEFT)
    {
        requestLaneChange(-1);
    }
    else if (key == GLUT_KEY_RIGHT)
    {
        requestLaneChange(1);
    }
    else if (key == GLUT_KEY_UP)
    {
        playerSpeed =
            clampFloat(
                playerSpeed + 0.05f,
                MIN_PLAYER_SPEED,
                MAX_PLAYER_SPEED
            );
    }
    else if (key == GLUT_KEY_DOWN)
    {
        playerSpeed =
            clampFloat(
                playerSpeed - 0.05f,
                MIN_PLAYER_SPEED,
                MAX_PLAYER_SPEED
            );
    }

    glutPostRedisplay();
}

void mouse(int button, int state, int x, int y)
{
    if (
        button != GLUT_LEFT_BUTTON ||
        state != GLUT_DOWN
    )
    {
        return;
    }

    const int convertedY =
        windowHeight - y;

    if (gameState == MAIN_MENU)
    {
        Rect startRect;
        Rect carsRect;
        Rect quitRect;

        getMainMenuRects(
            startRect,
            carsRect,
            quitRect
        );

        if (pointInsideRect(x, convertedY, startRect))
        {
            resetGame();
        }
        else if (pointInsideRect(x, convertedY, carsRect))
        {
            previewCarModel = selectedCarModel;
            gameState = CAR_SELECTION;
            hoveredButton = BUTTON_NONE;
        }
        else if (pointInsideRect(x, convertedY, quitRect))
        {
            quitGame();
        }
    }
    else if (gameState == CAR_SELECTION)
    {
        Rect previousRect;
        Rect nextRect;
        Rect backRect;
        Rect selectRect;

        getSelectionRects(
            previousRect,
            nextRect,
            backRect,
            selectRect
        );

        if (
            pointInsideRect(
                x,
                convertedY,
                previousRect
            )
        )
        {
            previewCarModel =
                positiveModulo(
                    previewCarModel - 1,
                    CAR_MODEL_COUNT
                );
        }
        else if (
            pointInsideRect(
                x,
                convertedY,
                nextRect
            )
        )
        {
            previewCarModel =
                positiveModulo(
                    previewCarModel + 1,
                    CAR_MODEL_COUNT
                );
        }
        else if (
            pointInsideRect(
                x,
                convertedY,
                backRect
            )
        )
        {
            gameState = MAIN_MENU;
            hoveredButton = BUTTON_NONE;
        }
        else if (
            pointInsideRect(
                x,
                convertedY,
                selectRect
            )
        )
        {
            selectedCarModel = previewCarModel;
            resetGame();
        }
    }

    updateHoveredButton();
    glutPostRedisplay();
}

void passiveMouseMotion(int x, int y)
{
    mouseX = x;

    // GLUT mouse Y starts at the top of the window.
    mouseY = windowHeight - y;

    updateHoveredButton();
    glutPostRedisplay();
}

void timer(int value)
{
    (void)value;

    const int currentMilliseconds =
        glutGet(GLUT_ELAPSED_TIME);

    float deltaTime =
        static_cast<float>(
            currentMilliseconds -
            previousTimerMilliseconds
        ) /
        1000.0f;

    previousTimerMilliseconds =
        currentMilliseconds;

    if (deltaTime < 0.0f)
    {
        deltaTime = 0.0f;
    }

    // Prevent a large physics jump after resizing or dragging.
    if (deltaTime > 0.05f)
    {
        deltaTime = 0.05f;
    }

    if (gameState == CAR_SELECTION)
    {
        previewRotation += 34.0f * deltaTime;
        previewWheelRotation -= 145.0f * deltaTime;

        if (previewRotation >= 360.0f)
        {
            previewRotation -= 360.0f;
        }

        if (previewWheelRotation <= -360.0f)
        {
            previewWheelRotation += 360.0f;
        }
    }

    if (gameState == PLAYING)
    {
        updatePlayer(deltaTime);
        updateTraffic(deltaTime);

        // Collision must be tested before an overtake can award a coin.
        checkCollisions();

        if (gameState == PLAYING)
        {
            checkOvertakes();
            recycleTraffic();
            updateAnimations(deltaTime);
        }
    }

    glutPostRedisplay();

    glutTimerFunc(
        TIMER_INTERVAL_MS,
        timer,
        0
    );
}

// -----------------------------------------------------------------------------
// Program entry
// -----------------------------------------------------------------------------

int main(int argc, char **argv)
{
    std::srand(
        static_cast<unsigned int>(
            std::time(0)
        )
    );

    glutInit(&argc, argv);

    glutInitDisplayMode(
        GLUT_DOUBLE |
        GLUT_RGB |
        GLUT_DEPTH
    );

    glutInitWindowSize(
        INITIAL_WINDOW_WIDTH,
        INITIAL_WINDOW_HEIGHT
    );

    glutInitWindowPosition(80, 50);

    glutCreateWindow(
        "Four-Lane Highway Overtake Racing"
    );

    initializeOpenGL();

    for (int i = 0; i < MAX_TRAFFIC; ++i)
    {
        traffic[i].active = false;
    }

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);
    glutMouseFunc(mouse);
    glutPassiveMotionFunc(passiveMouseMotion);

    previousTimerMilliseconds =
        glutGet(GLUT_ELAPSED_TIME);

    glutTimerFunc(
        TIMER_INTERVAL_MS,
        timer,
        0
    );

    glutMainLoop();

    cleanupGLResources();
    return 0;
}
