#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;

int count = 0;

const GLuint WIDTH = 800, HEIGHT = 600;
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 20.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float yaw = -90.0f, pitch = 0.0f, lastX = WIDTH / 2.0f, lastY = HEIGHT / 2.0f, fov = 45.0f;
bool firstMouse = true;
float deltaTime = 0.0f, lastFrame = 0.0f;

GLuint shaderID, VAO;
GLFWwindow *window;

struct Voxel {
    glm::vec3 pos;
    float fatorEscala;
    bool visivel = true, selecionado = false;
    int corPos;
};

int selecaoX, selecaoY, selecaoZ;
const int TAM = 10;
Voxel grid[TAM][TAM][TAM];

glm::vec4 colorList[] = {
    {0.5f, 0.5f, 0.5f, 0.5f}, {1.0f, 0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f},
    {0.0f, 0.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 1.0f, 1.0f},
    {0.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f, 1.0f}
};

const GLchar *vertexShaderSource = R"glsl(
    #version 450
    layout(location = 0) in vec3 position;
    uniform mat4 model, view, proj;
    void main() {
        gl_Position = proj * view * model * vec4(position, 1.0);
    }
)glsl";

const GLchar *fragmentShaderSource = R"glsl(
    #version 450
    uniform vec4 uColor;
    out vec4 color;
    void main() {
        color = uColor;
    }
)glsl";

GLuint setupShader();
GLuint setupGeometry();
void framebuffer_size_callback(GLFWwindow *, int, int);
void mouse_callback(GLFWwindow *, double, double);
void scroll_callback(GLFWwindow *, double, double);
void key_callback(GLFWwindow *, int, int, int, int);
void processInput(GLFWwindow *);
void especificaVisualizacao();
void especificaProjecao();
void transformaObjeto(float, float, float, float, float, float, float, float, float);
void setColor(GLuint, glm::vec4);

int main() {
    glfwInit();
    window = glfwCreateWindow(WIDTH, HEIGHT, "Camera Cube", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    shaderID = setupShader();
    VAO = setupGeometry();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    selecaoX = 0; selecaoY = 0; selecaoZ = TAM - 1;
    for (int y = 0, yPos = -TAM / 2; y < TAM; y++, yPos++)
        for (int x = 0, xPos = -TAM / 2; x < TAM; x++, xPos++)
            for (int z = 0, zPos = -TAM / 2; z < TAM; z++, zPos++) {
                grid[y][x][z].pos = glm::vec3(xPos, yPos, zPos);
                grid[y][x][z].corPos = 0;
                grid[y][x][z].fatorEscala = 0.98f;
            }
    grid[selecaoY][selecaoX][selecaoZ].selecionado = true;

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderID);
        especificaVisualizacao();
        especificaProjecao();
        glBindVertexArray(VAO);

        for (int x = 0; x < TAM; x++)
            for (int y = 0; y < TAM; y++)
                for (int z = 0; z < TAM; z++) {
                    if(grid[y][x][z].selecionado)
                        setColor(shaderID, colorList[grid[y][x][z].corPos] + 0.3f);
                    else
                        setColor(shaderID, colorList[grid[y][x][z].corPos]);

                    if (grid[y][x][z].visivel || grid[y][x][z].selecionado) {
                        float fator = grid[y][x][z].fatorEscala;
                        transformaObjeto(grid[y][x][z].pos.x, grid[y][x][z].pos.y, grid[y][x][z].pos.z,
                                         0.0f, 0.0f, 0.0f, fator, fator, fator);
                        glDrawArrays(GL_TRIANGLES, 0, 36);
                    }
                }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glfwTerminate();
    return 0;
}

GLuint setupShader() {
    GLint success;
    GLchar infoLog[512];

    // Compilação do vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cout << "Erro ao compilar Vertex Shader:\n" << infoLog << std::endl;
    }

    // Compilação do fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        std::cout << "Erro ao compilar Fragment Shader:\n" << infoLog << std::endl;
    }

    // Linkagem do programa de shader
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cout << "Erro ao linkar Shader Program:\n" << infoLog << std::endl;
    }

    // Libera os shaders individuais após linkar
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

GLuint setupGeometry() {
    float vertices[] = {
        // frente
        -0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,

         0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f,

        // costas
        -0.5f, -0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,

         0.5f,  0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,

        // esquerda
        -0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,

        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,

        // direita
         0.5f,  0.5f,  0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,

         0.5f, -0.5f, -0.5f,
         0.5f,  0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,

        // topo
        -0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,

         0.5f,  0.5f,  0.5f,
         0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,

        // base
        -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f, -0.5f,  0.5f,

         0.5f, -0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f,
        -0.5f, -0.5f, -0.5f,
    };

    GLuint VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return VAO;
}

void processInput(GLFWwindow* window) {
    float cameraSpeed = 5.0f * deltaTime; // velocidade baseada no tempo entre frames

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // Invertido porque y cresce de baixo para cima

    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.05f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw   += xoffset;
    pitch += yoffset;

    // Limita o pitch
    if(pitch > 89.0f)
        pitch = 89.0f;
    if(pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);

    // Atualiza o cameraUp com base no novo front
    glm::vec3 right = glm::normalize(glm::cross(cameraFront, glm::vec3(0.0f, 1.0f, 0.0f)));
    cameraUp = glm::normalize(glm::cross(right, cameraFront));
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    fov -= (float)yoffset;

    if (fov < 1.0f)
        fov = 1.0f;
    if (fov > 120.0f)
        fov = 120.0f;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS) return;

    // Alterna visibilidade do voxel selecionado
    if (key == GLFW_KEY_DELETE) {
        grid[selecaoY][selecaoX][selecaoZ].visivel = false;
    }
    if (key == GLFW_KEY_V) {
        grid[selecaoY][selecaoX][selecaoZ].visivel = true;
    }

    // Muda seleção
    grid[selecaoY][selecaoX][selecaoZ].selecionado = false;

    if (key == GLFW_KEY_RIGHT && selecaoX + 1 < TAM) selecaoX++;
    if (key == GLFW_KEY_LEFT  && selecaoX - 1 >= 0)  selecaoX--;
    if (key == GLFW_KEY_UP    && selecaoY + 1 < TAM) selecaoY++;
    if (key == GLFW_KEY_DOWN  && selecaoY - 1 >= 0)  selecaoY--;
    if (key == GLFW_KEY_PAGE_UP   && selecaoZ + 1 < TAM) selecaoZ++;
    if (key == GLFW_KEY_PAGE_DOWN && selecaoZ - 1 >= 0)  selecaoZ--;

    grid[selecaoY][selecaoX][selecaoZ].selecionado = true;

    // Muda cor (entre 0 e 8)
    if (key == GLFW_KEY_C) {
        int& cor = grid[selecaoY][selecaoX][selecaoZ].corPos;
        cor = (cor + 1) % 9;
    }
}

void especificaVisualizacao() {
    // Cria a matriz de visualização com posição da câmera e direção
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

    // Envia a matriz para o shader (uniform 'view')
    GLuint loc = glGetUniformLocation(shaderID, "view");
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(view));
}

void especificaProjecao() {
    // Define a matriz de projeção perspectiva (fov, aspecto, near, far)
    glm::mat4 proj = glm::perspective(glm::radians(fov), (float)WIDTH / HEIGHT, 0.1f, 100.0f);

    // Envia a matriz para o shader (uniform 'proj')
    GLuint loc = glGetUniformLocation(shaderID, "proj");
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(proj));
}

void transformaObjeto(float xpos, float ypos, float zpos,
                      float xrot, float yrot, float zrot,
                      float sx, float sy, float sz) {
    // Começa com a matriz identidade
    glm::mat4 model = glm::mat4(1.0f);

    // Translada para a posição do voxel
    model = glm::translate(model, glm::vec3(xpos, ypos, zpos));

    // Aplica rotações (em ordem X, Y, Z)
    model = glm::rotate(model, glm::radians(xrot), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(yrot), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(zrot), glm::vec3(0.0f, 0.0f, 1.0f));

    // Aplica escala
    model = glm::scale(model, glm::vec3(sx, sy, sz));

    // Envia a matriz 'model' para o shader
    GLuint loc = glGetUniformLocation(shaderID, "model");
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(model));
}

void setColor(GLuint shaderID, glm::vec4 cor) {
    GLint loc = glGetUniformLocation(shaderID, "uColor");
    glUniform4f(loc, cor.r, cor.g, cor.b, cor.a);
}