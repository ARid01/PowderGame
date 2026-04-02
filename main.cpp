#include "SFML/Graphics.hpp"
#include <iostream>

//World Class
class World {
public:
    //Behaviors and Block Count
    typedef void(*BlockBehaviorFunc)(World&, int, bool* updated);
    static constexpr int NUM_BLOCK_TYPES = 2;

    //Private stuff
private:
    int width, height, scale;
    char* prevWorld;
    char* rendWorld;
    bool paused;
    bool debug;
    sf::Font font;
    sf::Text txt;
    int fps;

    //velocity arrays
    float* prevVelX;
    float* prevVelY;
    float* rendVelX;
    float* rendVelY;

    const float density[NUM_BLOCK_TYPES] = {
        1.0f,   //sand
        0.5f    //water
    };
    const float gravity[NUM_BLOCK_TYPES] = {
        0.4f,
        0.4f
    };
    const sf::Color colors[NUM_BLOCK_TYPES] = { 
        sf::Color(255, 213, 0),
        sf::Color(0, 0, 255)
    };
    static BlockBehaviorFunc blockBehaviors[NUM_BLOCK_TYPES];

public:
    //Constructor
    World(int w, int h, int s) : width(w), height(h), scale(s) {
        int total = width * height;
        prevWorld = new char[total]();
        rendWorld = new char[total]();
        prevVelX = new float[total]();
        prevVelY = new float[total]();
        rendVelX = new float[total]();
        rendVelY = new float[total]();
        initArrs();
        paused = false;
        debug = true;
        initTxt();
    }

    //Initializations and Helpers 
    ~World() {
        delete[] prevWorld;
        delete[] rendWorld;
        delete[] prevVelX;
        delete[] prevVelY;
        delete[] rendVelX;
        delete[] rendVelY;
    }
    void initArrs() {
        for (int i = 0; i < width * height; i++) {
            prevWorld[i] = 0;
            rendWorld[i] = 0;
            prevVelX[i] = 0.0f;
            prevVelY[i] = 0.0f;
            rendVelX[i] = 0.0f;
            rendVelY[i] = 0.0f;
        }
    }
    void sendToPrev() {
        for (int i = 0; i < width * height; i++) {
            prevWorld[i] = rendWorld[i];
        }
    }
    void sendVelToPrev() {
        for (int i = 0; i < width * height; i++) {
            prevVelX[i] = rendVelX[i];
            prevVelY[i] = rendVelY[i];
        }
    }
    void clearPrev() {
        for (int i = 0; i < width * height; i++) {
            prevWorld[i] = 0;
        }
    }
    void clearRend() {
        for (int i = 0; i < width * height; i++) {
            rendWorld[i] = 0;
        }
    }
    void clearPrevVel() {
        for (int i = 0; i < width * height; i++) {
            prevVelX[i] = 0.0f;
            prevVelY[i] = 0.0f;
        }
    }
    void clearRendVel() {
        for (int i = 0; i < width * height; i++) {
            rendVelX[i] = 0.0f;
            rendVelY[i] = 0.0f;
        }
    }
    void initTxt() {
        if (!font.loadFromFile("Minecraft.ttf")) {
            std::cout << "Error loading font!\n";
        }
        txt.setFont(font);
        txt.setCharacterSize(16);
        txt.setOutlineThickness(1);
        txt.setOutlineColor(sf::Color::Black);
        txt.setFillColor(sf::Color::White);
    }

    //Draw rendWorld
    void draw(sf::RenderWindow& window) {
        sf::RectangleShape cell(sf::Vector2f(scale, scale));

        for (int i = 0; i < width * height; i++) {
            if (rendWorld[i] == 0) continue;

            int x = i % width;
            int y = i / width;
            cell.setFillColor(colors[rendWorld[i] - 1]);
            cell.setPosition(sf::Vector2f(x * scale, y * scale));
            window.draw(cell);
        }

        if (!debug) return;
        //Debug Info
        txt.setString(sf::String(paused ? "Paused\n" : "") + 
                      sf::String("FPS: " + std::to_string(fps) + "\n"));
        txt.setPosition(sf::Vector2f(5, 5));
        window.draw(txt);
    }

    //Run behaviors and send to rendWorld, then update prevWorld for next iteration
    void update() {
        clearRend();
        clearRendVel();
        int total = width * height;
        bool* updated = new bool[total]();

        for (int i = 0; i < total; i++) {
            if (updated[i]) continue;
            char blockID = prevWorld[i];
            if (blockID != 0) {
                int behaviorInd = blockID - 1;
                if (behaviorInd >= 0 && behaviorInd < NUM_BLOCK_TYPES && !paused) {
                    blockBehaviors[behaviorInd](*this, i, updated);
                }
                else {
                    rendWorld[i] = prevWorld[i];
                    updated[i] = true;
                }
            }
        }

        sendToPrev();
        sendVelToPrev();
        delete[] updated;
    }

    //User placed blocks
    void setPrevBlock(int x, int y, char blockID) {
        int index = y * width + x;
        prevWorld[index] = blockID;
        prevVelX[index] = 0.0f;
        prevVelY[index] = 0.0f;
    }

    //Drawing Blocks
    void paintBlocks(sf::Vector2i mousePos, int brushSize, int oneIn, char blockID) {
        int x = mousePos.x / scale;
        int y = mousePos.y / scale;

        if (brushSize != 1) {
            for (int dx = -brushSize; dx <= brushSize; dx++) {
                for (int dy = -brushSize; dy <= brushSize; dy++) {
                    if (dx * dx + dy * dy > brushSize * brushSize ||
                        (x + dx) < 0 || (x + dx) >= width ||
                        (y + dy) < 0 || (y + dy) >= height)
                        continue;

                    if (rand() % oneIn == 0 && getPrevAt((x + dx) + (y + dy) * width) == 0) {
                        setPrevBlock(x + dx, y + dy, blockID);
                    }
                }
            }
        }
        else {
            setPrevBlock(x, y, blockID);
        }
    }

    //Erasing Functionality, or for explosions
    void clearInRadius(sf::Vector2i mousePos, int brushSize) {
        int x = mousePos.x / scale;
        int y = mousePos.y / scale;

        for (int dx = -brushSize; dx <= brushSize; dx++) {
            for (int dy = -brushSize; dy <= brushSize; dy++) {
                if (dx * dx + dy * dy > brushSize * brushSize ||
                    (x + dx) < 0 || (x + dx) >= width || 
                    (y + dy) < 0 || (y + dy) >= height)
                    continue;
                setPrevBlock(x + dx, y + dy, 0);
            }
        }
    }

    #pragma region Get and Set Functions
    //World Dimensions and Cell State
    int getWidth() const { return width; }
    int getHeight() const { return height; }
    char getPrevAt(int index) const { return prevWorld[index]; }
    void setRendAt(int index, char blockID) { rendWorld[index] = blockID; }
    char getRendAt(int index) const { return rendWorld[index]; }

    //Velocities
    float getPrevVelX(int index) const { return prevVelX[index]; }
    float getPrevVelY(int index) const { return prevVelY[index]; }
    void setRendVel(int index, float vx, float vy) { rendVelX[index] = vx; rendVelY[index] = vy; }

    //Density & Gravity
    float getDensity(char blockID) const {
        return (blockID == 0) ? 0.0f : density[blockID - 1];
    }
    float getGravity(char blockID) const {
        return (blockID == 0) ? 0.0f : gravity[blockID - 1];
    }
    bool isPaused() const { return paused; }
    void setPaused(bool pause) { paused = pause; }
    bool DebugOn() const { return debug; }
    void setDebug(bool isDebug) { debug = isDebug; }
    void setFPS(float frames) { fps = (int)frames; }
    #pragma endregion
};

#pragma region Behaviors
//Helper Function: Moves cell from i to target and transfers velocity.
void moveCell(World& world, int i, int target, bool* updated, float vx, float vy) {
    char targetID = world.getPrevAt(target);
    char currID = world.getPrevAt(i);
    world.setRendAt(target, currID);
    world.setRendVel(target, vx, vy);
    world.setRendAt(i, targetID);
    world.setRendVel(i, 0, 0);
    updated[target] = true;
    updated[i] = true;
}

void behaviorSand(World& world, int i, bool* updated) {
    int w = world.getWidth(), total = world.getWidth() * world.getHeight();
    char cellID = world.getPrevAt(i);
    float vx = world.getPrevVelX(i);
    float vy = world.getPrevVelY(i) + world.getGravity(cellID);

    if (vy >= 1.0f) {
        int downIndex = i + w;
        if (downIndex < total) {
            char below = world.getPrevAt(downIndex);

            if (below == 0 || (world.getDensity(below) < world.getDensity(cellID))) {
                moveCell(world, i, downIndex, updated, vx, vy);
                return;
            }

            int diagLeft = downIndex - 1;
            int diagRight = downIndex + 1;
            bool canLeft = (i % w != 0 && diagLeft < total&& world.getPrevAt(diagLeft) == 0 && world.getRendAt(diagLeft) == 0);
            bool canRight = ((i + 1) % w != 0 && diagRight < total&& world.getPrevAt(diagRight) == 0 && world.getRendAt(diagRight) == 0);
            if (canLeft && canRight) {
                int target = (rand() % 2 == 0) ? diagLeft : diagRight;
                moveCell(world, i, target, updated, vx, vy);
                return;
            }
            else if (canLeft) {
                moveCell(world, i, diagLeft, updated, vx, vy);
                return;
            }
            else if (canRight) {
                moveCell(world, i, diagRight, updated, vx, vy);
                return;
            }
        }
    }

    world.setRendAt(i, cellID);
    world.setRendVel(i, vx, vy);
    updated[i] = true;
}

void behaviorWater(World& world, int i, bool* updated) {
    int w = world.getWidth(), total = world.getWidth() * world.getHeight();
    char cellID = world.getPrevAt(i);
    float vx = world.getPrevVelX(i);
    float vy = world.getPrevVelY(i) + world.getGravity(cellID);

    if (vy >= 1.0f) {
        int downIndex = i + w;
        if (downIndex < total) {
            char below = world.getPrevAt(downIndex);

            if (below == 0 || (world.getDensity(below) < world.getDensity(cellID))) {
                moveCell(world, i, downIndex, updated, vx, vy);
                return;
            }
        }


        // Check diagonals if direct downward move is not possible.
        int diagLeft = i + w - 1;
        int diagRight = i + w + 1;
        bool canLeft = (i % w != 0 && diagLeft < total&& world.getPrevAt(diagLeft) == 0 && world.getRendAt(diagLeft) == 0);
        bool canRight = ((i + 1) % w != 0 && diagRight < total&& world.getPrevAt(diagRight) == 0 && world.getRendAt(diagRight) == 0);
        if (canLeft && canRight) {
            int target = (rand() % 2 == 0) ? diagLeft : diagRight;
            moveCell(world, i, target, updated, vx, vy);
            return;
        }
        else if (canLeft) {
            moveCell(world, i, diagLeft, updated, vx, vy);
            return;
        }
        else if (canRight) {
            moveCell(world, i, diagRight, updated, vx, vy);
            return;
        }
    }

    // If vertical and diagonal moves fail, try horizontal movement.
    bool canMoveLeft = (i % w != 0 && world.getPrevAt(i - 1) == 0 && world.getRendAt(i - 1) == 0);
    bool canMoveRight = ((i + 1) % w != 0 && world.getPrevAt(i + 1) == 0 && world.getRendAt(i + 1) == 0);
    if (canMoveLeft && canMoveRight) {
        int target = (rand() % 2 == 0) ? i - 1 : i + 1;
        moveCell(world, i, target, updated, vx, vy);
        return;
    }
    else if (canMoveLeft) {
        moveCell(world, i, i - 1, updated, vx, vy);
        return;
    }
    else if (canMoveRight) {
        moveCell(world, i, i + 1, updated, vx, vy);
        return;
    }

    // Otherwise, remain in place.
    world.setRendAt(i, cellID);
    world.setRendVel(i, vx, vy);
    updated[i] = true;
}

World::BlockBehaviorFunc World::blockBehaviors[World::NUM_BLOCK_TYPES] = { behaviorSand, behaviorWater };

#pragma endregion

//Main Function
int main()
{
    #pragma region Settings
    //General
    int width = 160;
    int height = 100;
    int scale = 5;
    float DELAY = 0.025f;
    int oneIn = 4;

    //Selection
    int currBlock = 1;
    int minBlock = 1;
    int maxBlock = 2;
    int currBrushSize = 5;
    int minBrushSize = 1;
    int maxBrushSize = 10;

    //FPS Counting
    float frameCount = 0;
    float frameUpdateDelay = 0.016f;
    #pragma endregion

    //Setup Time-Keeping and World/Window
    sf::Clock clock;
    sf::Time lastUpdatedSim = clock.getElapsedTime();
    sf::Time lastUpdatedFPS = clock.getElapsedTime();
    World world(width, height, scale);
    sf::RenderWindow window(sf::VideoMode(width * scale, height * scale), "PowderGame");

    //Main loop
    while (window.isOpen())
    {
        //Get events
        sf::Event event;
        while (window.pollEvent(event))
        {
            //Hitting the 'X' on the window
            if (event.type == sf::Event::Closed)
                window.close();

            //Mouse Scroll Events
            if (event.type == sf::Event::MouseWheelScrolled) {
                if (event.mouseWheelScroll.wheel == sf::Mouse::VerticalWheel) {

                    if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) {
                        //Change brush size
                        currBrushSize += event.mouseWheelScroll.delta;

                        if (currBrushSize < minBrushSize) {
                            currBrushSize = minBrushSize;
                        }
                        if (currBrushSize > maxBrushSize) {
                            currBrushSize = maxBrushSize;
                        }
                        std::cout << "Brush Size: " << currBrushSize << std::endl;
                    }
                    else {
                        //Change blocks
                        currBlock += event.mouseWheelScroll.delta;

                        //Check bounds
                        if (currBlock < minBlock) {
                            currBlock = maxBlock;
                        }
                        if (currBlock > maxBlock) {
                            currBlock = minBlock;
                        }
                        std::cout << "Block ID: " << currBlock << std::endl;
                    }
                }
            }

            //Keyboard Events
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::C) {
                    world.clearPrev();
                }
                if (event.key.code == sf::Keyboard::P) {
                    world.setPaused(!world.isPaused());
                }
                if (event.key.code == sf::Keyboard::Escape) {
                    window.close();
                }
                if (event.key.code == sf::Keyboard::Space) {
                    DELAY = 0.05f;
                }
                if (event.key.code == sf::Keyboard::D) {
                    world.setDebug(!world.DebugOn());
                }
            }
            if (event.type == sf::Event::KeyReleased) {
                if (event.key.code == sf::Keyboard::Space) {
                    DELAY = 0.025f;
                }
            }
        }

        //Draw and Erase
        if (sf::Mouse::isButtonPressed(sf::Mouse::Button(0))) {
            world.paintBlocks(sf::Mouse::getPosition(window), currBrushSize, oneIn, currBlock);
        }
        else if (sf::Mouse::isButtonPressed(sf::Mouse::Button(1))) {
            world.clearInRadius(sf::Mouse::getPosition(window), currBrushSize);
        }

        //Control Simulation Updates
        if ((clock.getElapsedTime() - lastUpdatedSim).asSeconds() > DELAY) {
            lastUpdatedSim = clock.getElapsedTime();
            world.update();
        }

        //Update FPS Counter
        if ((clock.getElapsedTime() - lastUpdatedFPS).asSeconds() > frameUpdateDelay) {
            lastUpdatedFPS = clock.getElapsedTime();
            float fps = frameCount / frameUpdateDelay;
            frameCount = 0;
            world.setFPS(fps);
        }

        //Draw Frame
        window.clear();
        world.draw(window);
        window.display();
        frameCount++;
    }

    return 0;
}