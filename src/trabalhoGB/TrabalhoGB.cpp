//Bibliotecas base 
#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

//inserção de biblioteca para salvamento e carregamento
#include <fstream>
#include <sstream>

using namespace std;

int count = 0;

// Dimensões da janela
const GLuint WIDTH = 1920, HEIGHT = 1080;

// Variáveis globais de controle da câmera (posição, direção e orientação)
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 20.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float yaw = -90.0f;
float pitch = 0.0f;
bool firstMouse = true;
float lastX = WIDTH / 2.0f;
float lastY = HEIGHT / 2.0f;
float fov = 45.0f;
bool modoEdicao = false;
bool cameraFast = false;

// Controle de tempo entre frames
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// IDs de shader e VAO
GLuint shaderID, VAO;
GLFWwindow *window;

struct Voxel
{
    glm::vec3 pos;
    float fatorEscala;
    bool visivel = true, selecionado = false;
    int corPos;
};

int selecaoX, selecaoY, selecaoZ;
const int TAM = 10;
Voxel grid[TAM][TAM][TAM];

// Lista de cores para os voxels (com comentários)
glm::vec4 colorList[] = {
    {0.5f, 0.5f, 0.5f, 0.5f}, // cinza     0   -- reservado para a interface
    {1.0f, 0.0f, 0.0f, 1.0f}, // vermelho  1
    {0.0f, 1.0f, 0.0f, 1.0f}, // verde     2
    {0.0f, 0.0f, 1.0f, 1.0f}, // azul      3
    {1.0f, 1.0f, 0.0f, 1.0f}, // amarelo   4
    {1.0f, 0.0f, 1.0f, 1.0f}, // magenta   5
    {0.0f, 1.0f, 1.0f, 1.0f}, // ciano     6
    {1.0f, 1.0f, 1.0f, 1.0f}, // branco    7
    {0.0f, 0.0f, 0.0f, 1.0f}, // preto     8  
};

// Código do Vertex Shader
const GLchar *vertexShaderSource = R"glsl(
    #version 450
    layout(location = 0) in vec3 position;
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 proj;
    void main() {
        gl_Position = proj * view * model * vec4(position, 1.0);
    }
)glsl";

// Código do Fragment Shader
const GLchar *fragmentShaderSource = R"glsl(
    #version 450
    uniform vec4 uColor;
    out vec4 color;
    void main() {
        color = uColor;
    }
)glsl";

// Atualiza o viewport ao redimensionar a janela
void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
}

// Callback para movimentação do mouse — controla rotação da câmera
void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
    // Condicional para travar a movimentação da camera com base no mouse se o botão direito não estiver pressionado, com base no resultado da booleana
    if (!modoEdicao)
        return;
        
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.05f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);

    glm::vec3 right = glm::normalize(glm::cross(cameraFront, glm::vec3(0.0, 1.0, 0.0)));
    cameraUp = glm::normalize(glm::cross(right, cameraFront));
}

// Callback de scroll — altera o FOV (zoom)
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    if (fov >= 1.0f && fov <= 120.0f)
        fov -= yoffset;
    if (fov <= 1.0f)
        fov = 1.0f;
    if (fov >= 120.0f)
        fov = 120.0f;
}

//função para gerenciar a movimentação da câmera APENAS enquanto o botão direito estiver sendo pressionado
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_RIGHT)
    {
        if (action == GLFW_PRESS)
        {
            modoEdicao = true;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            firstMouse = true; // Precisei repetir este cara aqui pois a camera enlouquecia como se fosse a primeira inicialização kkkkk
        }
        else if (action == GLFW_RELEASE)
        {
            modoEdicao = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
}

// Função para salvar os voxels no arquivo texto
// Para melhor compreensão, o formato de salvamento será realizado salvando 5 informações: posição X, posição Y, posição Z, Visibilidade e a cor.
void salvamento(const char* filename)
{
    ofstream arquivo(filename);
    if (!arquivo.is_open())
    {
        cout << "Erro ao abrir arquivo para salvar.\n";
        return;
    }
    for (int y = 0; y < TAM; y++)
    {
        for (int x = 0; x < TAM; x++)
        {
            for (int z = 0; z < TAM; z++)
            {
                Voxel& v = grid[y][x][z];
                arquivo << x << " " << y << " " << z << " "
                        << (v.visivel ? 1 : 0) << " "
                        << v.corPos << "\n";
            }
        }
    }
    arquivo.close();
    cout << "Voxels salvos em " << filename << "\n";
}

// Função para carregar os voxels do arquivo texto
void carregamento(const char* filename)
{
    ifstream arquivo(filename);
    if (!arquivo.is_open())
    {
        cout << "Erro ao abrir arquivo para carregar ou não há arquivo salvo.\n";
        return;
    }

    string linha;
    while (getline(arquivo, linha))
    {
        istringstream iss(linha);
        int x, y, z, vis, cor;

        //Primeir realiza a leitura de valores e verifica se está correto ou não
        if (!(iss >> x >> y >> z >> vis >> cor))
        {
            cout << "Linha mal formatada: " << linha << "\n";
            continue;
        }

        // Agora faz a verificação com valores já lidos
        if (x >= 0 && x < TAM && y >= 0 && y < TAM && z >= 0 && z < TAM)
        {
            grid[y][x][z].visivel = (vis == 1);
            grid[y][x][z].corPos = cor;
        }
    }

    arquivo.close();
    cout << "Voxels carregados de " << filename << "\n";
}


void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{

    // troca a visibilidade de um voxel selecionado
    if (key == GLFW_KEY_DELETE && action == GLFW_PRESS)
    {
        grid[selecaoY][selecaoX][selecaoZ].visivel = false;
    }
    if (key == GLFW_KEY_INSERT && action == GLFW_PRESS)
    {
        grid[selecaoY][selecaoX][selecaoZ].visivel = true;
    }

    // Troca seleção do voxel na grid
    // Para facilitar entendimento: WASD movimentam no eixo X e Y. Page UP e Page DOWN irão movimentar no eixo Z
    bool mudouSelecao = false;

    if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS)
    {
        if (selecaoX + 1 < TAM)
        {
            grid[selecaoY][selecaoX][selecaoZ].selecionado = false;
            selecaoX++;
            mudouSelecao = true;
            grid[selecaoY][selecaoX][selecaoZ].selecionado = true;
        }
    }
    if (key == GLFW_KEY_LEFT && action == GLFW_PRESS)
    {
        if (selecaoX - 1 >= 0)
        {
            grid[selecaoY][selecaoX][selecaoZ].selecionado = false;
            selecaoX--;
            mudouSelecao = true;
            grid[selecaoY][selecaoX][selecaoZ].selecionado = true;
        }
    }
    if (key == GLFW_KEY_UP && action == GLFW_PRESS)
    {
        if (selecaoY + 1 < TAM)
        {
            grid[selecaoY][selecaoX][selecaoZ].selecionado = false;
            selecaoY++;
            mudouSelecao = true;
            grid[selecaoY][selecaoX][selecaoZ].selecionado = true;
        }
    }
    if (key == GLFW_KEY_DOWN && action == GLFW_PRESS)
    {
        if (selecaoY - 1 >= 0)
        {
            grid[selecaoY][selecaoX][selecaoZ].selecionado = false;
            selecaoY--;
            mudouSelecao = true;
            grid[selecaoY][selecaoX][selecaoZ].selecionado = true;
        }
    }
    if (key == GLFW_KEY_PAGE_DOWN && action == GLFW_PRESS)
    {
        if (selecaoZ + 1 < TAM)
        {
            grid[selecaoY][selecaoX][selecaoZ].selecionado = false;
            selecaoZ++;
            mudouSelecao = true;
            grid[selecaoY][selecaoX][selecaoZ].selecionado = true;
        }
    }
    if (key == GLFW_KEY_PAGE_UP && action == GLFW_PRESS)
    {
        if (selecaoZ - 1 >= 0)
        {
            grid[selecaoY][selecaoX][selecaoZ].selecionado = false;
            selecaoZ--;
            mudouSelecao = true;
            grid[selecaoY][selecaoX][selecaoZ].selecionado = true;
        }
    }

    // muda a cor do voxel para frente com o E e para trás com o Q
    if (key == GLFW_KEY_E && action == GLFW_PRESS)
    {
        int corAtual = grid[selecaoY][selecaoX][selecaoZ].corPos;
        if (corAtual < 8)
        {
            corAtual++;
        }
        else
        {
            corAtual = 0;
        }
        grid[selecaoY][selecaoX][selecaoZ].corPos = corAtual;
    }
    if (key == GLFW_KEY_Q && action == GLFW_PRESS)
    {
        int corAtual = grid[selecaoY][selecaoX][selecaoZ].corPos;
        if (corAtual > 0)
        {
            corAtual--;
        }
        else
        {
            corAtual = 8;
        }
        grid[selecaoY][selecaoX][selecaoZ].corPos = corAtual;
    }

    //Salvamento e carregamento de voxels
    if (key == GLFW_KEY_HOME && action == GLFW_PRESS)
    {
        salvamento("save.txt");
    }

if (key == GLFW_KEY_END && action == GLFW_PRESS)
    {
        carregamento("save.txt");
    }

    
}

// Processa as teclas pressionadas para movimentar a câmera no espaço 3D
void processInput(GLFWwindow *window)
{
    float baseSpeed = 5.0f;
    float shiftBoost = 3.0f;
    float cameraSpeed = baseSpeed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
    {
    cameraSpeed *= shiftBoost;
    cameraFast = true;
    }
    else
    {
        cameraFast = false;
    }
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

// Define a matriz de visualização usando a posição e direção da câmera
void especificaVisualizacao()
{
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    GLuint loc = glGetUniformLocation(shaderID, "view");
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(view));
}

// Define a matriz de projeção perspectiva com base no FOV
void especificaProjecao()
{
    glm::mat4 proj = glm::perspective(glm::radians(fov), (float)WIDTH / HEIGHT, 0.1f, 100.0f);
    GLuint loc = glGetUniformLocation(shaderID, "proj");
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(proj));
}

// Aplica as transformações de translação, rotação e escala no objeto
void transformaObjeto(float xpos, float ypos, float zpos,
                      float xrot, float yrot, float zrot,
                      float sx, float sy, float sz)
{
    glm::mat4 transform = glm::mat4(1.0f); // matriz identidade

    transform = glm::translate(transform, glm::vec3(xpos, ypos, zpos));
    transform = glm::rotate(transform, glm::radians(xrot), glm::vec3(1, 0, 0));
    transform = glm::rotate(transform, glm::radians(yrot), glm::vec3(0, 1, 0));
    transform = glm::rotate(transform, glm::radians(zrot), glm::vec3(0, 0, 1));
    transform = glm::scale(transform, glm::vec3(sx, sy, sz));

    GLuint loc = glGetUniformLocation(shaderID, "model");
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(transform));
}

// Compila shaders e cria o programa de shader
GLuint setupShader()
{
    GLint success;
    GLchar infoLog[512];

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        cout << "Vertex Shader error:\n"
             << infoLog << endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        cout << "Fragment Shader error:\n"
             << infoLog << endl;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        cout << "Shader Program Linking error:\n"
             << infoLog << endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

// Cria o VAO com os vértices do cubo 3D
GLuint setupGeometry()
{
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

    GLuint VBO, vao;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &VBO);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return vao;
}

void setColor(GLuint shaderID, glm::vec4 cor)
{
    GLint loc = glGetUniformLocation(shaderID, "uColor");
    glUniform4f(loc, cor.r, cor.g, cor.b, cor.a);
}

// Função principal da aplicação
int main()
{
    glfwInit();
    window = glfwCreateWindow(WIDTH, HEIGHT, "Trabalho Rodrigo & Mariana", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    shaderID = setupShader();
    VAO = setupGeometry();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float xPos, yPos, zPos;

    selecaoX = 0;
    selecaoY = 0;
    selecaoZ = TAM - 1;

    for (int y = 0, yPos = -TAM / 2; y < TAM; y++, yPos += 1.0f)
    {
        for (int x = 0, xPos = -TAM / 2; x < TAM; x++, xPos += 1.0f)
        {
            for (int z = 0, zPos = -TAM / 2; z < TAM; z++, zPos += 1.0f)
            {
                grid[y][x][z].pos = glm::vec3(xPos, yPos, zPos);
                grid[y][x][z].corPos = 0;
                grid[y][x][z].fatorEscala = 0.98f;
                grid[y][x][z].visivel = true;
                grid[y][x][z].selecionado = false;
            }
        }
    }

    grid[selecaoY][selecaoX][selecaoZ].selecionado = true;

    while (!glfwWindowShouldClose(window))
    {
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

        // Renderiza a grade tridimensional de voxels
        for (int x = 0; x < TAM; x++)
        {
            for (int y = 0; y < TAM; y++)
            {
                for (int z = 0; z < TAM; z++)
                {
                    if(grid[y][x][z].selecionado)
                    {
                        setColor(shaderID, colorList[grid[y][x][z].corPos] + glm::vec4(0.3f));
                    }
                    else
                    {
                        setColor(shaderID, colorList[grid[y][x][z].corPos]);
                    }

                    if (grid[y][x][z].visivel || grid[y][x][z].selecionado)
                    {
                        float fatorEscala = grid[y][x][z].fatorEscala;
                        transformaObjeto(grid[y][x][z].pos.x, grid[y][x][z].pos.y, grid[y][x][z].pos.z,
                                         0.0f, 0.0f, 0.0f,
                                         fatorEscala, fatorEscala, fatorEscala);
                        glDrawArrays(GL_TRIANGLES, 0, 36);
                    }
                }
            }
        }

        // Atualiza título da janela como HUD temporária
        stringstream hudInfo;
        hudInfo << "Voxel selecionado: (" << selecaoX << ", " << selecaoY << ", " << selecaoZ << ") ";
        hudInfo << "| Cor: " << grid[selecaoY][selecaoX][selecaoZ].corPos << " ";
        hudInfo << "| Visível: " << (grid[selecaoY][selecaoX][selecaoZ].visivel ? "Sim" : "Não") << " ";
        hudInfo << "| Mover Câmera: " << (modoEdicao ? "Ativo" : "Inativo") << " ";
        hudInfo << "| Velocidade da Câmera: " << (cameraFast ? "Rápida" : "Normal");

        glfwSwapBuffers(window);
        glfwPollEvents();
        glfwSetWindowTitle(window, hudInfo.str().c_str());
    }

    glDeleteVertexArrays(1, &VAO);
    glfwTerminate();
    return 0;
}