 #include <iostream>
 #include <string>
 #include <assert.h>
 #include <cmath>
 //#include "hitbox/hitbox.h"
 
 using namespace std;
 
 // GLAD
 #include <glad/glad.h>
 
 // GLFW
 #include <GLFW/glfw3.h>
 
 // GLM
 #include <glm/glm.hpp>
 #include <glm/gtc/matrix_transform.hpp>
 #include <glm/gtc/type_ptr.hpp>
 
 using namespace glm;
 
 // STB_IMAGE
 #define STB_IMAGE_IMPLEMENTATION
 #include <stb_image.h>
 
 struct Sprite 
 {
     GLuint VAO;
     GLuint texID;
     vec3 pos;
     vec3 dimensions;
     float angle;
     float vel;
     float gravity;
     int count;
     int nAnimations, nFrames;
     int iFrame, iAnimation;
     float ds, dt;
     float space;
 };
 
 // Protótipo da função de callback de teclado
 void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);
 
 // Protótipos das funções
 int setupShader();
 int setupSprite(int nAnimations, int nFrames, float &ds, float &dt);
 int loadTexture(string filePath);
 void drawSprite(GLuint shaderID, Sprite spr);
 
 // Dimensões da janela (pode ser alterado em tempo de execução)
 const GLuint WIDTH = 800, HEIGHT = 600;
 
 // Código fonte do Vertex Shader (em GLSL): ainda hardcoded
 const GLchar *vertexShaderSource = R"(
  #version 400
  layout (location = 0) in vec2 position;
  layout (location = 1) in vec2 texc;
  
  uniform mat4 projection;
  uniform mat4 model;
  out vec2 tex_coord;
  void main()
  {
     tex_coord = vec2(texc.s,1.0-texc.t);
     gl_Position = projection * model * vec4(position, 0.0, 1.0);
  }
  )";
 
 // Código fonte do Fragment Shader (em GLSL): ainda hardcoded
 const GLchar *fragmentShaderSource = R"(
  #version 400
 in vec2 tex_coord;
 out vec4 color;
 uniform sampler2D tex_buff;
 uniform vec2 offset_tex;
 void main()
 {
      color = texture(tex_buff,tex_coord + offset_tex);
 }
 )";
 
 bool keys[1024];
 float FPS = 12.0;
 float lastTime = 0.0;
 
 bool checkCollision(const Sprite& a, const Sprite& b)
{
    float ax = a.pos.x - a.dimensions.x / 2.0f;
    float ay = a.pos.y - a.dimensions.y / 2.0f;
    float bx = b.pos.x - b.dimensions.x / 2.0f;
    float by = b.pos.y - b.dimensions.y / 2.0f;

    return (
        ax < bx + b.dimensions.x &&
        ax + a.dimensions.x > bx &&
        ay < by + b.dimensions.y &&
        ay + a.dimensions.y > by
    );
}
 
 // Função MAIN
 int main()
 {
    bool gameover = false;
    bool showGameOver = false;
    
     // Inicialização da GLFW
     glfwInit();
 
     // Ativa a suavização de serrilhado (MSAA) com 8 amostras por pixel
     glfwWindowHint(GLFW_SAMPLES, 8);
 
     for(int i=0; i<1024;i++) { keys[i] = false; }
 
     // Criação da janela GLFW
     GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Trabalho GA - Rodrigo & Mariana", nullptr, nullptr);
     if (!window)
     {
         std::cerr << "Falha ao criar a janela GLFW" << std::endl;
         glfwTerminate();
         return -1;
     }
     glfwMakeContextCurrent(window);
 
     // Fazendo o registro da função de callback para a janela GLFW
     glfwSetKeyCallback(window, key_callback);
 
     // GLAD: carrega todos os ponteiros d funções da OpenGL
     if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
     {
         std::cerr << "Falha ao inicializar GLAD" << std::endl;
         return -1;
     }
 
     // Obtendo as informações de versão
     const GLubyte *renderer = glGetString(GL_RENDERER); /* get renderer string */
     const GLubyte *version = glGetString(GL_VERSION);	/* version as a string */
     cout << "Renderer: " << renderer << endl;
     cout << "OpenGL version supported " << version << endl;
 
     // Definindo as dimensões da viewport com as mesmas dimensões da janela da aplicação
     int width, height;
     glfwGetFramebufferSize(window, &width, &height);
     glViewport(0, 0, width, height);
 
     // Compilando e buildando o programa de shader
     GLuint shaderID = setupShader();
 
     Sprite background, flappy, pipeDown, pipeUp, gameOverScreen;
 
     // Gerando um buffer simples, com a geometria de um triângulo
     background.texID = loadTexture("../assets/tex/1.png");
     background.VAO = setupSprite(1,1,background.ds,background.dt);
     background.pos = vec3(400,300,0);
     background.dimensions = vec3(800, 600, 1);
     background.angle = 0.0;
 
     // Carregando uma textura
     flappy.texID = loadTexture("../assets/sprites/flappy.png");
     flappy.VAO = setupSprite(1,3,flappy.ds,flappy.dt);
     flappy.pos = vec3(400,300,0);
     flappy.dimensions = vec3(34, 24, 1);
     //flappy.vel = 0.2;
     flappy.nAnimations = 12;
     flappy.nFrames = 2;
     flappy.angle = 0.0;
     flappy.iAnimation = 6;
     flappy.iFrame = 0;
     flappy.gravity = 0.1;

     pipeDown.texID = loadTexture("../assets/sprites/pipe.png");
     pipeDown.VAO = setupSprite(1, 1, pipeDown.ds, pipeDown.dt);
     pipeDown.dimensions = vec3(52, 320, 1);


     pipeUp.texID = loadTexture("../assets/sprites/pipe2.png");
     pipeUp.VAO = setupSprite(1, 1, pipeUp.ds, pipeUp.dt);
     pipeUp.dimensions = vec3(52, 320, 1);

     gameOverScreen.texID = loadTexture("../assets/sprites/gameover.jpg");
     gameOverScreen.VAO = setupSprite(1, 1, gameOverScreen.ds, gameOverScreen.dt);
     gameOverScreen.pos = vec3(400,300,0.2);
     gameOverScreen.dimensions = vec3(800, 600, 1);
     gameOverScreen.angle = 0.0;

     //Pedaço do código para armazenar os pares de canos e calcular um espaço entre eles
     const int N_PIPES = 10; // quantidade de pares visíveis na tela
     Sprite pipeUps[N_PIPES];
     Sprite pipeDowns[N_PIPES];
     float pipeGap = 25.0; // espaço vertical entre os canos
     float pipeSpacing = 100.0; // espaço horizontal entre os pares
     float pipeSpeed = 1.7;

     for (int i = 0; i < N_PIPES; ++i) 
      {
         float x = 800 + i * pipeSpacing;
         float offsetY = rand() % 200 - 100; // entre -100 e 100

         // Cano de baixo
         pipeDowns[i].texID = pipeDown.texID;
         pipeDowns[i].VAO = pipeDown.VAO;
         pipeDowns[i].dimensions = pipeDown.dimensions;
         pipeDowns[i].angle = 0.0;
         pipeDowns[i].pos = vec3(x, offsetY - pipeGap / 2.0 - pipeDowns[i].dimensions.y + 300.0, 0.1);

         // Cano de cima (virado)
         pipeUps[i].texID = pipeUp.texID;
         pipeUps[i].VAO = pipeUp.VAO;
         pipeUps[i].dimensions = pipeUp.dimensions;
         pipeUps[i].angle = 0.0;
         pipeUps[i].pos = vec3(x, offsetY + pipeGap / 2.0 + pipeUps[i].dimensions.y + 300.0, 0.1);
     }
 
     glUseProgram(shaderID); // Reseta o estado do shader para evitar problemas futuros
 
     double prev_s = glfwGetTime();	// Define o "tempo anterior" inicial.
     double title_countdown_s = 0.1; // Intervalo para atualizar o título da janela com o FPS.
 
     float colorValue = 0.0;
 
     // Ativando o primeiro buffer de textura do OpenGL
     glActiveTexture(GL_TEXTURE0);
 
     // Criando a variável uniform pra mandar a textura pro shader
     glUniform1i(glGetUniformLocation(shaderID, "tex_buff"), 0);
 
     // Criação da matriz de projeção paralela ortográfica
     mat4 projection = mat4(1); // matriz identidade
     projection = ortho(0.0, 800.0, 0.0, 600.0, -1.0, 1.0);
     // Envio para o shader
     glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, value_ptr(projection));
 
     //Habilitando transparência/função de mistura
     glEnable(GL_BLEND);
     glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
 
     //Habilitando teste de profundidade
     glEnable(GL_DEPTH_TEST);
     glDepthFunc(GL_ALWAYS);
 
     // Loop da aplicação - "game loop"
     while (!glfwWindowShouldClose(window))
{
    glfwPollEvents(); // SEMPRE deve ocorrer!

    // Limpa o buffer de cor
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Atualização normal do jogo (movimento, física etc.)
    if (!gameover)
    {
        // Flappy physics
        if (keys[GLFW_KEY_SPACE])
        {
            flappy.gravity = -0.3;
            flappy.pos.y -= flappy.gravity;
        }
        else
        {
            flappy.gravity = 0.1;
            flappy.pos.y -= flappy.gravity;
        }

        // Animação do flappy
        float now = glfwGetTime();
        float deltaTime = now - lastTime;
        if (deltaTime >= 1 / FPS)
        {
            flappy.iFrame = (flappy.iFrame + 1) % flappy.nFrames;
            lastTime = now;
        }

        // Movimento dos canos
        for (int i = 0; i < N_PIPES; ++i)
        {
            pipeDowns[i].pos.x -= pipeSpeed * deltaTime;
            pipeUps[i].pos.x -= pipeSpeed * deltaTime;

            if (pipeDowns[i].pos.x + pipeDowns[i].dimensions.x < 0)
            {
                float x = 800 + pipeSpacing;
                float offsetY = rand() % 200 - 100;

                pipeDowns[i].pos.x = x;
                pipeDowns[i].pos = vec3(x, offsetY - pipeGap / 2.0 - pipeDowns[i].dimensions.y + 300.0, 0.1);

                pipeUps[i].pos.x = x;
                pipeUps[i].pos = vec3(x, offsetY + pipeGap / 2.0 + pipeUps[i].dimensions.y + 300.0, 0.1);
            }

            // Colisão
            if (checkCollision(flappy, pipeUps[i]) || checkCollision(flappy, pipeDowns[i]))
            {
                gameover = true;
                showGameOver = true;
            }
            //Checa fora da tela
            if (flappy.pos.y > 900 || flappy.pos.y < -100)
            {
                gameover = true;
                showGameOver = true;
            }

        }
    }
    else
    {
        // Se o jogo acabou, espera o jogador apertar espaço para reiniciar
        if (keys[GLFW_KEY_ENTER])
        {
            gameover = false;
            showGameOver = false;

            // Reset do flappy
            flappy.pos = vec3(400, 300, 0);
            flappy.gravity = 0.1;
            flappy.iFrame = 0;

            // Reset dos canos
            for (int i = 0; i < N_PIPES; ++i)
            {
                float x = 800 + i * pipeSpacing;
                float offsetY = rand() % 200 - 100;

                pipeDowns[i].pos = vec3(x, offsetY - pipeGap / 2.0 - pipeDowns[i].dimensions.y + 300.0, 0.1);
                pipeUps[i].pos = vec3(x, offsetY + pipeGap / 2.0 + pipeUps[i].dimensions.y + 300.0, 0.1);
            }
        }
    }

    // Renderização
    glUniform2f(glGetUniformLocation(shaderID, "offset_tex"), 0.0f, 0.0f);
    drawSprite(shaderID, background);

    float offsetS = flappy.iFrame * flappy.ds;
    float offsetT = flappy.iAnimation * flappy.dt;
    glUniform2f(glGetUniformLocation(shaderID, "offset_tex"), offsetS, offsetT);
    drawSprite(shaderID, flappy);

    for (int i = 0; i < N_PIPES; ++i)
    {
        glUniform2f(glGetUniformLocation(shaderID, "offset_tex"), 0.0f, 0.0f);
        drawSprite(shaderID, pipeDowns[i]);
        drawSprite(shaderID, pipeUps[i]);
    }

    if (showGameOver)
    {
        glUniform2f(glGetUniformLocation(shaderID, "offset_tex"), 0.0f, 0.0f);
        drawSprite(shaderID, gameOverScreen);
    }

    glfwSwapBuffers(window);
}
     // Finaliza a execução da GLFW, limpando os recursos alocados por ela
     glfwTerminate();
     return 0;
 }

 void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
 {
     if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
         glfwSetWindowShouldClose(window, GL_TRUE);
 
     if (action == GLFW_PRESS)
     {
         keys[key] = true;
     }
     else if (action == GLFW_RELEASE)
     {
         keys[key] = false;
     }
 }

 int setupShader()
 {
     // Vertex shader
     GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
     glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
     glCompileShader(vertexShader);
     // Checando erros de compilação (exibição via log no terminal)
     GLint success;
     GLchar infoLog[512];
     glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
     if (!success)
     {
         glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
         std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n"
                   << infoLog << std::endl;
     }
     // Fragment shader
     GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
     glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
     glCompileShader(fragmentShader);
     // Checando erros de compilação (exibição via log no terminal)
     glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
     if (!success)
     {
         glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
         std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n"
                   << infoLog << std::endl;
     }
     // Linkando os shaders e criando o identificador do programa de shader
     GLuint shaderProgram = glCreateProgram();
     glAttachShader(shaderProgram, vertexShader);
     glAttachShader(shaderProgram, fragmentShader);
     glLinkProgram(shaderProgram);
     // Checando por erros de linkagem
     glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
     if (!success)
     {
         glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
         std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n"
                   << infoLog << std::endl;
     }
     glDeleteShader(vertexShader);
     glDeleteShader(fragmentShader);
     return shaderProgram;
 }

 int setupSprite(int nAnimations, int nFrames, float &ds, float &dt)
 {
     ds = 1.0 / (float) nFrames;
     dt = 1.0 / (float) nAnimations;
     GLfloat vertices[] = {
         // x   y   s     t
         -0.5, 0.5, 0.0, dt,
         -0.5,-0.5, 0.0,0.0,
          0.5, 0.5, ds, dt,
         -0.5,-0.5, 0.0,0.0,
          0.5,-0.5, ds, 0.0,
          0.5, 0.5, ds,	dt,
     };
 
     GLuint VBO, VAO;
     // Geração do identificador do VBO
     glGenBuffers(1, &VBO);
     // Faz a conexão (vincula) do buffer como um buffer de array
     glBindBuffer(GL_ARRAY_BUFFER, VBO);
     // Envia os dados do array de floats para o buffer da OpenGl
     glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
     // Geração do identificador do VAO (Vertex Array Object)
     glGenVertexArrays(1, &VAO);
     // Vincula (bind) o VAO primeiro, e em seguida  conecta e seta o(s) buffer(s) de vértices
     glBindVertexArray(VAO);
     // Ponteiro pro atributo 0 - Posição - coordenadas x, y
     glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid *)0);
     glEnableVertexAttribArray(0);
     // Ponteiro pro atributo 2 - Coordenada de textura - coordenadas s,t
     glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid *)(2 * sizeof(GLfloat)));
     glEnableVertexAttribArray(1);
     // Observe que isso é permitido, a chamada para glVertexAttribPointer registrou o VBO como o objeto de buffer de vértice
     glBindBuffer(GL_ARRAY_BUFFER, 0);
     // Desvincula o VAO (é uma boa prática desvincular qualquer buffer ou array para evitar bugs medonhos)
     glBindVertexArray(0);
     return VAO;
 }
 
 void drawSprite(GLuint shaderID, Sprite spr)
 {
     // Neste código, usamos o mesmo buffer de geometria para todos os sprites
     glBindVertexArray(spr.VAO);      
     // Desenhar o sprite 1
     glBindTexture(GL_TEXTURE_2D, spr.texID);
     // Criação da matriz de transformações do objeto
     mat4 model = mat4(1);
     // Move origem para o centro do sprite (rotação acontece no centro)
     model = translate(model, spr.pos);
     // Escala o sprite
     model = scale(model, spr.dimensions);
     // Aplica rotação em torno do centro
     model = translate(model, vec3(0.5f * spr.dimensions.x, 0.5f * spr.dimensions.y, 0.0f));
     model = rotate(model, radians(spr.angle), vec3(0.0, 0.0, 1.0));
     model = translate(model, vec3(-0.5f * spr.dimensions.x, -0.5f * spr.dimensions.y, 0.0f));
     glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(model));
     glDrawArrays(GL_TRIANGLES, 0, 6);
 }
 
 int loadTexture(string filePath)
 {
     GLuint texID;
     // Gera o identificador da textura na memória
     glGenTextures(1, &texID);
     glBindTexture(GL_TEXTURE_2D, texID); 
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); 
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

     int width, height, nrChannels;
 
     unsigned char *data = stbi_load(filePath.c_str(), &width, &height, &nrChannels, 0);
 
     if (data)
     {
         if (nrChannels == 3)
         {
             glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
         }
         else
         {
             glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
         }
         glGenerateMipmap(GL_TEXTURE_2D);
     }
     else
     {
         std::cout << "Failed to load texture" << std::endl;
     }
     stbi_image_free(data); 
     glBindTexture(GL_TEXTURE_2D, 0);
     return texID;
 }
 