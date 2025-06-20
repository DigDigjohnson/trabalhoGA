### README ATUALIZADO PARA TRABALHOGB

## 👥 Integrantes do Grupo

- Rodrigo Osvaldo de Moraes Lehnen  
- Mariana Oliveira Flores

---

## 🚀 Como Executar

Este programa foi desenvolvido em **C++** utilizando **OpenGL 4.0**, **GLFW**, **GLAD** e **GLM**.

---

## Pré-requisitos

Para compilar e executar, certifique-se de que você possui:

- Uma IDE como o **VS Code**
- Um compilador C++
- As bibliotecas OpenGL, GLFW, GLAD e GLM instaladas
- CMake instalado e configurado

Para instalação do **CMake**, seguir o passo-a-passo do arquivo GettingStarted

---

## 💡 Sobre o Programa

O projeto é um editor de cenas voxelizadas em 3D, onde:

O usuário pode navegar livremente por um espaço 10×10×10 de voxels (cubos).

É possível inserir (tecla INSERT), remover (tecla DELETE) e pintar voxels (Teclas Q e E) usando o teclado. A inserção e remoção só ocorre dentro do espaço 10x10x10 previamente estabelecido.

Os dados da cena podem ser salvos e carregados de um arquivo de texto simples (save.txt).

As informações que servem de HUD para o usuário ficam no título da janela da aplicação e exibem informações úteis, como cor atual (de 0 a 9), voxel selecionado (posição X, Y, Z), visibilidade do Voxel e velocidade da câmera.

A movimentação da câmera é controlada pelo teclado e mouse (modo de edição com botão direito pressionado).

A renderização utiliza VAO e VBO com shaders personalizados.

---

## 📌 Observações

O código está no arquivo TrabalhoGB.cpp.

Não é necessário instalar bibliotecas externas além de GLFW, GLAD, GLM e OpenGL.

O salvamento é feito com a tecla HOME e o carregamento com a tecla END.

A HUD exibe informações em tempo real no título da janela.

O código foi testado em sistemas Windows apenas.