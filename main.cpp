
#define GL_GLEXT_PROTOTYPES
#include <GLFW/glfw3.h>
#include <GL/glext.h>
#include <iostream>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <string>
#include <vector>

const int WIDTH = 480;
const int HEIGHT = 720;

const float BIRD_X = 150;
const float GRAVITY = -900;
const float JUMP = 320;
const float PIPE_SPEED = 160;
const float PIPE_WIDTH = 75;
const float PIPE_GAP = 190;
const float GROUND = 80;

float birdY = 350;
float velocity = 0;

int score = 0;
int bestScore = 0;

bool started = false;
bool gameOver = false;
bool spacePressed = false;

struct Pipe {
    float x;
    float gapY;
    bool passed;
};

std::vector<Pipe> pipes;


struct Vertex {
    float x, y;
    float r, g, b;
};

std::vector<Vertex> vertices;
std::vector<unsigned int> indices;
GLuint shaderProgram = 0;
GLuint vao = 0;
GLuint vbo = 0;
GLuint ebo = 0;

void addTriangle(float x1, float y1, float x2, float y2,
                 float x3, float y3, float r, float g, float b) {
    unsigned int first = static_cast<unsigned int>(vertices.size());
    indices.push_back(first);
    indices.push_back(first + 1);
    indices.push_back(first + 2);
    vertices.push_back({x1, y1, r, g, b});
    vertices.push_back({x2, y2, r, g, b});
    vertices.push_back({x3, y3, r, g, b});
}

void drawRect(float x, float y, float w, float h,
              float r, float g, float b) {
    addTriangle(x, y, x + w, y, x + w, y + h, r, g, b);
    addTriangle(x, y, x + w, y + h, x, y + h, r, g, b);
}

void drawCircle(float x, float y, float radius,
                float r, float g, float b) {
    for (int i = 0; i < 40; i++) {
        float a1 = i * 2.0f * 3.14159f / 40;
        float a2 = (i + 1) * 2.0f * 3.14159f / 40;
        addTriangle(x, y,
                    x + std::cos(a1) * radius, y + std::sin(a1) * radius,
                    x + std::cos(a2) * radius, y + std::sin(a2) * radius,
                    r, g, b);
    }
}

GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        std::cerr << "Shader error: " << log << "\n";
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

bool initGraphics() {
    const char* vertexSource = R"(
        #version 330 core
        layout(location = 0) in vec2 aPos;
        layout(location = 1) in vec3 aColor;
        out vec3 vColor;
        void main() {
            vec2 pos = vec2(aPos.x / 480.0 * 2.0 - 1.0,
                            aPos.y / 720.0 * 2.0 - 1.0);
            gl_Position = vec4(pos, 0.0, 1.0);
            vColor = aColor;
        }
    )";
    const char* fragmentSource = R"(
        #version 330 core
        in vec3 vColor;
        out vec4 FragColor;
        void main() {
            FragColor = vec4(vColor, 1.0);
        }
    )";

    GLuint vs = compileShader(GL_VERTEX_SHADER, vertexSource);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
    if (!vs || !fs) {
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        return false;
    }

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vs);
    glAttachShader(shaderProgram, fs);
    glLinkProgram(shaderProgram);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint ok = GL_FALSE;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(shaderProgram, sizeof(log), nullptr, log);
        std::cerr << "Program error: " << log << "\n";
        return false;
    }

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    return true;
}

void renderGraphics() {
    glUseProgram(shaderProgram);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex),
                 vertices.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
                 indices.data(), GL_DYNAMIC_DRAW);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()),
                   GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
    vertices.clear();
    indices.clear();
}

void drawCloud(float x, float y) {
    drawCircle(x, y, 24, 1, 1, 1);
    drawCircle(x + 25, y + 12, 30, 1, 1, 1);
    drawCircle(x + 55, y, 25, 1, 1, 1);
    drawRect(x, y - 20, 55, 25, 1, 1, 1);
}

void drawBackground() {
    drawRect(0, 0, WIDTH, HEIGHT,
             0.42f, 0.79f, 0.96f);

    drawCircle(390, 610, 48,
               1.0f, 0.92f, 0.52f);

    drawCloud(50, 580);
    drawCloud(270, 490);
    drawCloud(100, 380);

    drawCircle(80, 90, 130,
               0.39f, 0.75f, 0.43f);
    drawCircle(300, 90, 160,
               0.30f, 0.67f, 0.39f);
    drawCircle(480, 90, 120,
               0.39f, 0.75f, 0.43f);

    drawRect(0, 0, WIDTH, 65,
             0.86f, 0.69f, 0.43f);

    drawRect(0, 65, WIDTH, 15,
             0.35f, 0.78f, 0.30f);
}

void drawBird(float x, float y) {
    drawCircle(x, y, 23,
               0.55f, 0.34f, 0.12f);

    drawCircle(x, y + 2, 20,
               1.0f, 0.84f, 0.20f);

    drawCircle(x - 9, y - 5, 12,
               1.0f, 0.65f, 0.12f);
    drawCircle(x - 11, y - 2, 9,
               1.0f, 0.91f, 0.42f);

    drawCircle(x + 10, y + 9, 9, 1, 1, 1);
    drawCircle(x + 13, y + 9, 4,
               0.12f, 0.12f, 0.12f);

    drawRect(x + 17, y - 5, 17, 9,
             1.0f, 0.43f, 0.12f);
    drawRect(x + 17, y, 17, 5,
             1.0f, 0.66f, 0.15f);
}


void drawPipe(float x, float gapY) {
    float bottomHeight = gapY - PIPE_GAP / 2;
    float topY = gapY + PIPE_GAP / 2;

    drawRect(x, GROUND, PIPE_WIDTH,
             bottomHeight - GROUND,
             0.12f, 0.53f, 0.20f);

    drawRect(x + 7, GROUND, 17,
             bottomHeight - GROUND,
             0.38f, 0.86f, 0.32f);

    drawRect(x - 5, bottomHeight - 25,
             PIPE_WIDTH + 10, 25,
             0.20f, 0.68f, 0.24f);

    drawRect(x, topY, PIPE_WIDTH,
             HEIGHT - topY,
             0.12f, 0.53f, 0.20f);

    drawRect(x + 7, topY, 17,
             HEIGHT - topY,
             0.38f, 0.86f, 0.32f);

    drawRect(x - 5, topY,
             PIPE_WIDTH + 10, 25,
             0.20f, 0.68f, 0.24f);
}

float randomGap() {
    return 250 + rand() % 220;
}

void addPipe(float x) {
    pipes.push_back({x, randomGap(), false});
}


std::vector<std::string> getLetter(char c) {
    switch (c) {
        case 'A': return {"01110","10001","10001","11111","10001","10001","10001"};
        case 'B': return {"11110","10001","10001","11110","10001","10001","11110"};
        case 'C': return {"01111","10000","10000","10000","10000","10000","01111"};
        case 'D': return {"11110","10001","10001","10001","10001","10001","11110"};
        case 'E': return {"11111","10000","10000","11110","10000","10000","11111"};
        case 'G': return {"01111","10000","10000","10111","10001","10001","01111"};
        case 'M': return {"10001","11011","10101","10101","10001","10001","10001"};
        case 'O': return {"01110","10001","10001","10001","10001","10001","01110"};
        case 'P': return {"11110","10001","10001","11110","10000","10000","10000"};
        case 'R': return {"11110","10001","10001","11110","10100","10010","10001"};
        case 'S': return {"01111","10000","10000","01110","00001","00001","11110"};
        case 'T': return {"11111","00100","00100","00100","00100","00100","00100"};
        case 'V': return {"10001","10001","10001","10001","10001","01010","00100"};
        case '0': return {"01110","10001","10011","10101","11001","10001","01110"};
        case '1': return {"00100","01100","00100","00100","00100","00100","01110"};
        case '2': return {"01110","10001","00001","00010","00100","01000","11111"};
        case '3': return {"11110","00001","00001","01110","00001","00001","11110"};
        case '4': return {"00010","00110","01010","10010","11111","00010","00010"};
        case '5': return {"11111","10000","10000","11110","00001","00001","11110"};
        case '6': return {"01110","10000","10000","11110","10001","10001","01110"};
        case '7': return {"11111","00001","00010","00100","01000","01000","01000"};
        case '8': return {"01110","10001","10001","01110","10001","10001","01110"};
        case '9': return {"01110","10001","10001","01111","00001","00001","01110"};
        default: return {};
    }
}

void drawText(std::string text, float centerX,
              float y, float size,
              float r, float g, float b) {

    float letterWidth = 6 * size;
    float startX = centerX - text.length() * letterWidth / 2;

    for (char c : text) {
        auto letter = getLetter(c);

        for (int row = 0; row < (int)letter.size(); row++) {
            for (int col = 0; col < 5; col++) {
                if (letter[row][col] == '1') {
                    drawRect(
                        startX + col * size,
                        y + (6 - row) * size,
                        size, size, r, g, b
                    );
                }
            }
        }

        startX += letterWidth;
    }
}


void drawGameOver() {
    drawRect(47, 248, 386, 238,
             0.15f, 0.36f, 0.40f);

    drawRect(55, 258, 370, 220,
             0.98f, 0.91f, 0.73f);

    drawRect(55, 450, 370, 28,
             0.95f, 0.42f, 0.28f);

    drawText("GAME OVER", 240, 393, 5,
             0.72f, 0.23f, 0.15f);

    drawText("SCORE", 240, 348, 3,
             0.35f, 0.34f, 0.28f);

    drawText(std::to_string(score), 240, 312, 4,
             0.17f, 0.55f, 0.30f);

    drawText("SPACE TO RESTART", 240, 278, 2.4f,
             0.35f, 0.34f, 0.28f);
}


void resetGame() {
    birdY = 350;
    velocity = 0;
    score = 0;

    started = false;
    gameOver = false;

    pipes.clear();

    addPipe(580);
    addPipe(850);
    addPipe(1120);
}

void updateGame(float dt) {
    if (!started || gameOver) return;

    velocity += GRAVITY * dt;
    birdY += velocity * dt;

    if (birdY - 20 <= GROUND ||
        birdY + 20 >= HEIGHT) {
        gameOver = true;
    }

    for (Pipe &p : pipes) {
        p.x -= PIPE_SPEED * dt;

        if (!p.passed &&
            p.x + PIPE_WIDTH < BIRD_X) {

            p.passed = true;
            score++;

            if (score > bestScore)
                bestScore = score;
        }

        bool horizontal =
            BIRD_X + 18 > p.x - 5 &&
            BIRD_X - 18 < p.x + PIPE_WIDTH + 5;

        bool vertical =
            birdY - 18 < p.gapY - PIPE_GAP / 2 ||
            birdY + 18 > p.gapY + PIPE_GAP / 2;

        if (horizontal && vertical) {
            gameOver = true;
        }
    }

    if (!pipes.empty() &&
        pipes.front().x < -PIPE_WIDTH - 10) {
        pipes.erase(pipes.begin());
    }

    if (!pipes.empty() &&
        pipes.back().x < WIDTH - 190) {
        addPipe(pipes.back().x + 270);
    }
}


int main() {
    srand(static_cast<unsigned>(time(nullptr)));

    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(
        WIDTH, HEIGHT, "Flappy Bird",
        nullptr, nullptr
    );

    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!initGraphics()) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }
    glViewport(0, 0, WIDTH, HEIGHT);

    resetGame();

    double lastTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        double now = glfwGetTime();
        float dt = float(now - lastTime);
        lastTime = now;

        if (dt > 0.05f) dt = 0.05f;

        glfwPollEvents();

        bool space =
            glfwGetKey(window, GLFW_KEY_SPACE)
            == GLFW_PRESS;

        if (space && !spacePressed) {
            if (gameOver) {
                resetGame();
            } else {
                started = true;
                velocity = JUMP;
            }
        }

        spacePressed = space;

        updateGame(dt);

        glClear(GL_COLOR_BUFFER_BIT);
        vertices.clear();
        indices.clear();

        drawBackground();

        for (const Pipe &p : pipes) {
            drawPipe(p.x, p.gapY);
        }

        drawBird(BIRD_X, birdY);

        if (started && !gameOver) {
            drawText(std::to_string(score),
                     240, 625, 7,
                     1, 1, 1);
        }

        if (gameOver) {
            drawGameOver();
        }

        std::string title =
            "Flappy Bird | Score: " +
            std::to_string(score) +
            " | Best: " +
            std::to_string(bestScore);

        glfwSetWindowTitle(window, title.c_str());

        renderGraphics();
        glfwSwapBuffers(window);
    }

    glDeleteBuffers(1, &ebo);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(shaderProgram);
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}