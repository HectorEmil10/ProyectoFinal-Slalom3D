#include <iostream>
#include <string>
#include <vector>
#include <cmath>

// GLEW y GLFW
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// GLM Mathematics
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Librerías de carga de texturas y utilerías del laboratorio
#include "SOIL2/SOIL2.h"
#include "Shader.h"
#include "Camera.h"
#include "Model.h"

// Prototipos de funciones de eventos
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseCallback(GLFWwindow* window, double xPos, double yPos);
void DoMovement();
void ActualizarCarrera(Model& terreno);
float ObtenerAlturaTerreno(Model& terreno, float x, float z, float yActual);

// Dimensiones de la ventana
const GLuint WIDTH = 1200, HEIGHT = 800;
int SCREEN_WIDTH, SCREEN_HEIGHT;

// ==================== CÁMARAS (Tarjetas HEP) ====================
int modoCamara = 0;
Camera camera(glm::vec3(-255.0f, 52.0f, 36.0f));
GLfloat lastX = WIDTH / 2.0f;
GLfloat lastY = HEIGHT / 2.0f;

bool keys[1024];
bool firstMouse = true;

// ==================== LÓGICA DE CARRERA Y ESQUIADOR (Tarjetas HEP) ====================
glm::vec3 posSalida(-244.717f, 46.3459f, 35.9673f); // Bajo el cartel START
glm::vec3 posMeta(250.214f, -112.315f, 7.09446f);   // Bajo el cartel META

// Nodo raíz del esquiador
glm::vec3 skierPos = posSalida;
float skierRotY = 93.34f;       // Mirando de Salida hacia Meta
float velDescenso = 45.0f;
float velGiroLateral = 28.0f;
float offsetPiesEsquiador = 1.5f; // Mitad de la altura de la caja para que apoye sobre la nieve

// Estados de la carrera: 0 = En espera en Salida, 1 = Descendiendo, 2 = Meta cruzada
int estadoCarrera = 0;
double tiempoInicioCarrera = 0.0;
double tiempoActualCarrera = 0.0;
double tiempoFinalCarrera = 0.0;

// ==================== ILUMINACIÓN Y CLIMA (Tarjetas HEP) ====================
int estadoClima = 0;
float intensidadSol = 0.85f;
float densidadNiebla = 0.0006f;
glm::vec3 colorNiebla(0.82f, 0.88f, 0.95f);

glm::vec3 pointLightPositions[] = {
    glm::vec3(-5.0f, 3.0f, -10.0f),
    glm::vec3(5.0f, 3.0f, -10.0f),
    glm::vec3(-4.0f, 3.0f, -25.0f),
    glm::vec3(4.0f, 3.0f, -25.0f)
};

// Cubo para visualizar el nodo raíz del esquiador
float verticesCubo[] = {
    -0.5f, -0.5f,  0.5f,   0.5f, -0.5f,  0.5f,   0.5f,  0.5f,  0.5f,
     0.5f,  0.5f,  0.5f,  -0.5f,  0.5f,  0.5f,  -0.5f, -0.5f,  0.5f,
    -0.5f, -0.5f, -0.5f,   0.5f, -0.5f, -0.5f,   0.5f,  0.5f, -0.5f,
     0.5f,  0.5f, -0.5f,  -0.5f,  0.5f, -0.5f,  -0.5f, -0.5f, -0.5f,
     0.5f, -0.5f,  0.5f,   0.5f, -0.5f, -0.5f,   0.5f,  0.5f, -0.5f,
     0.5f,  0.5f, -0.5f,   0.5f,  0.5f,  0.5f,   0.5f, -0.5f,  0.5f,
    -0.5f,  0.5f,  0.5f,  -0.5f,  0.5f, -0.5f,  -0.5f, -0.5f, -0.5f,
    -0.5f, -0.5f, -0.5f,  -0.5f, -0.5f,  0.5f,  -0.5f,  0.5f,  0.5f,
    -0.5f, -0.5f, -0.5f,   0.5f, -0.5f, -0.5f,   0.5f, -0.5f,  0.5f,
     0.5f, -0.5f,  0.5f,  -0.5f, -0.5f,  0.5f,  -0.5f, -0.5f, -0.5f,
    -0.5f,  0.5f, -0.5f,   0.5f,  0.5f, -0.5f,   0.5f,  0.5f,  0.5f,
     0.5f,  0.5f,  0.5f,  -0.5f,  0.5f,  0.5f,  -0.5f,  0.5f, -0.5f
};

GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Escenario Interactivo 3D de Slalom - Proyecto Final", nullptr, nullptr);
    if (nullptr == window)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return EXIT_FAILURE;
    }

    glfwMakeContextCurrent(window);
    glfwGetFramebufferSize(window, &SCREEN_WIDTH, &SCREEN_HEIGHT);

    glfwSetKeyCallback(window, KeyCallback);
    glfwSetCursorPosCallback(window, MouseCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glewExperimental = GL_TRUE;
    if (GLEW_OK != glewInit())
    {
        std::cout << "Failed to initialize GLEW" << std::endl;
        return EXIT_FAILURE;
    }

    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Configurar VAO/VBO para el indicador del esquiador
    GLuint VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verticesCubo), verticesCubo, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // ==================== CARGA DE SHADERS Y MODELOS ====================
    Shader lightingShader("Shader/lighting.vs", "Shader/lighting.frag");
    // Usamos core.vs y core.frag (de Práctica 4) para pintar el indicador de color naranja y que contraste con la nieve
    Shader colorShader("Shader/core.vs", "Shader/core.frag");

    Model PistaCompleta((char*)"Models/Entorno/pista.obj");

    // Ajustar posición inicial del esquiador exactamente sobre la malla en la salida
    skierPos.y = ObtenerAlturaTerreno(PistaCompleta, skierPos.x, skierPos.z, posSalida.y) + offsetPiesEsquiador;

    glm::mat4 projection = glm::perspective(camera.GetZoom(), (GLfloat)SCREEN_WIDTH / (GLfloat)SCREEN_HEIGHT, 0.1f, 2000.0f);

    // ==================== GAME LOOP ====================
    while (!glfwWindowShouldClose(window))
    {
        GLfloat currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glfwPollEvents();
        DoMovement();
        ActualizarCarrera(PistaCompleta);

        glClearColor(colorNiebla.r, colorNiebla.g, colorNiebla.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        lightingShader.Use();

        // ==================== MATRIZ DE VISTA (CÁMARA) ====================
        glm::mat4 view;
        glm::vec3 currentViewPos = camera.GetPosition();

        if (modoCamara == 0)
        {
            view = camera.GetViewMatrix();
        }
        else
        {
            // Calculamos la posición trasera de la cámara y verificamos su propia altura contra el terreno
            // para garantizar que la cámara TAMPOCO atraviese la montaña jamás
            glm::vec3 offset(-sin(glm::radians(skierRotY)) * 12.0f, 6.0f, -cos(glm::radians(skierRotY)) * 12.0f);
            glm::vec3 camPos = skierPos + offset;
            float alturaPisoCamara = ObtenerAlturaTerreno(PistaCompleta, camPos.x, camPos.z, camPos.y);
            if (camPos.y < alturaPisoCamara + 4.0f)
            {
                camPos.y = alturaPisoCamara + 4.0f; // Mantiene la cámara mínimo 4 unidades sobre la nieve
            }

            glm::vec3 target = skierPos + glm::vec3(0.0f, 1.0f, 0.0f);
            view = glm::lookAt(camPos, target, glm::vec3(0.0f, 1.0f, 0.0f));
            currentViewPos = camPos;
        }

        GLint viewPosLoc = glGetUniformLocation(lightingShader.Program, "viewPos");
        glUniform3f(viewPosLoc, currentViewPos.x, currentViewPos.y, currentViewPos.z);
        glUniform3f(glGetUniformLocation(lightingShader.Program, "material.ambient"), 0.5f, 0.5f, 0.5f);
        glUniform3f(glGetUniformLocation(lightingShader.Program, "material.diffuse"), 0.8f, 0.8f, 0.8f);
        glUniform3f(glGetUniformLocation(lightingShader.Program, "material.specular"), 0.6f, 0.6f, 0.6f);
        glUniform1f(glGetUniformLocation(lightingShader.Program, "material.shininess"), 32.0f);

        glUniform1f(glGetUniformLocation(lightingShader.Program, "fogDensity"), densidadNiebla);
        glUniform3f(glGetUniformLocation(lightingShader.Program, "fogColor"), colorNiebla.r, colorNiebla.g, colorNiebla.b);

        // 1. Luz Direccional
        glUniform3f(glGetUniformLocation(lightingShader.Program, "dirLight.direction"), -0.3f, -1.0f, -0.4f);
        glUniform3f(glGetUniformLocation(lightingShader.Program, "dirLight.ambient"), intensidadSol * 0.6f, intensidadSol * 0.6f, intensidadSol * 0.65f);
        glUniform3f(glGetUniformLocation(lightingShader.Program, "dirLight.diffuse"), intensidadSol, intensidadSol, intensidadSol);
        glUniform3f(glGetUniformLocation(lightingShader.Program, "dirLight.specular"), 0.5f, 0.5f, 0.5f);

        // 2. Luces Posicionales
        for (int i = 0; i < 4; i++)
        {
            std::string num = std::to_string(i);
            glUniform3f(glGetUniformLocation(lightingShader.Program, ("pointLights[" + num + "].position").c_str()), pointLightPositions[i].x, pointLightPositions[i].y, pointLightPositions[i].z);
            glUniform3f(glGetUniformLocation(lightingShader.Program, ("pointLights[" + num + "].ambient").c_str()), 0.15f, 0.15f, 0.12f);
            glUniform3f(glGetUniformLocation(lightingShader.Program, ("pointLights[" + num + "].diffuse").c_str()), 0.9f, 0.85f, 0.7f);
            glUniform3f(glGetUniformLocation(lightingShader.Program, ("pointLights[" + num + "].specular").c_str()), 1.0f, 1.0f, 1.0f);
            glUniform1f(glGetUniformLocation(lightingShader.Program, ("pointLights[" + num + "].constant").c_str()), 1.0f);
            glUniform1f(glGetUniformLocation(lightingShader.Program, ("pointLights[" + num + "].linear").c_str()), 0.09f);
            glUniform1f(glGetUniformLocation(lightingShader.Program, ("pointLights[" + num + "].quadratic").c_str()), 0.032f);
        }

        // 3. SpotLight
        glUniform3f(glGetUniformLocation(lightingShader.Program, "spotLight.ambient"), 0.0f, 0.0f, 0.0f);
        glUniform3f(glGetUniformLocation(lightingShader.Program, "spotLight.diffuse"), 0.0f, 0.0f, 0.0f);
        glUniform3f(glGetUniformLocation(lightingShader.Program, "spotLight.specular"), 0.0f, 0.0f, 0.0f);
        glUniform1f(glGetUniformLocation(lightingShader.Program, "spotLight.constant"), 1.0f);
        glUniform1f(glGetUniformLocation(lightingShader.Program, "spotLight.linear"), 0.09f);
        glUniform1f(glGetUniformLocation(lightingShader.Program, "spotLight.quadratic"), 0.032f);
        glUniform1f(glGetUniformLocation(lightingShader.Program, "spotLight.cutOff"), glm::cos(glm::radians(12.5f)));
        glUniform1f(glGetUniformLocation(lightingShader.Program, "spotLight.outerCutOff"), glm::cos(glm::radians(17.5f)));

        GLint modelLoc = glGetUniformLocation(lightingShader.Program, "model");
        GLint viewLoc = glGetUniformLocation(lightingShader.Program, "view");
        GLint projLoc = glGetUniformLocation(lightingShader.Program, "projection");

        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // ==================== DIBUJADO DEL ESCENARIO ====================
        glm::mat4 model = glm::mat4(1.0f);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(glGetUniformLocation(lightingShader.Program, "transparency"), 0);
        glUniform2f(glGetUniformLocation(lightingShader.Program, "uvOffset"), 0.0f, 0.0f);
        PistaCompleta.Draw(lightingShader);

        // ==================== NODO RAÍZ DEL ESQUIADOR (Jerarquía Padre) ====================
        glm::mat4 modelTemp = glm::mat4(1.0f);
        modelTemp = glm::translate(modelTemp, skierPos);
        modelTemp = glm::rotate(modelTemp, glm::radians(skierRotY), glm::vec3(0.0f, 1.0f, 0.0f));

        // Indicador en color NARANJA INTENSO usando core.vs / core.frag para que destaque sobre la nieve
        colorShader.Use();
        glUniformMatrix4fv(glGetUniformLocation(colorShader.Program, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(colorShader.Program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glm::mat4 modelIndicador = glm::scale(modelTemp, glm::vec3(1.5f, 3.0f, 1.5f));
        glUniformMatrix4fv(glGetUniformLocation(colorShader.Program, "model"), 1, GL_FALSE, glm::value_ptr(modelIndicador));
        glm::vec3 colorNaranja(1.0f, 0.35f, 0.0f); // Naranja deportivo
        glUniform3fv(colorShader.uniformColor, 1, glm::value_ptr(colorNaranja));
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);

        glfwSwapBuffers(window);
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glfwTerminate();
    return 0;
}

// ==================== COLISIÓN EXACTA CONTRA LOS TRIÁNGULOS DE LA MALLA ====================
// Evalúa en qué triángulo (X, Z) se encuentra el esquiador e interpola su altura Y exacta con coordenadas baricéntricas
float ObtenerAlturaTerreno(Model& terreno, float x, float z, float yActual)
{
    float alturaEncontrada = yActual;
    float menorDiferencia = 100000.0f;
    bool colisionDetectada = false;

    for (size_t m = 0; m < terreno.meshes.size(); m++)
    {
        const std::vector<Vertex>& verts = terreno.meshes[m].vertices;
        const std::vector<GLuint>& inds = terreno.meshes[m].indices;

        for (size_t i = 0; i + 2 < inds.size(); i += 3)
        {
            glm::vec3 p1 = verts[inds[i]].Position;
            glm::vec3 p2 = verts[inds[i + 1]].Position;
            glm::vec3 p3 = verts[inds[i + 2]].Position;

            // Filtro rápido por Bounding Box 2D del triángulo en XZ
            float minX = glm::min(p1.x, glm::min(p2.x, p3.x));
            float maxX = glm::max(p1.x, glm::max(p2.x, p3.x));
            float minZ = glm::min(p1.z, glm::min(p2.z, p3.z));
            float maxZ = glm::max(p1.z, glm::max(p2.z, p3.z));

            if (x < minX || x > maxX || z < minZ || z > maxZ)
                continue;

            // Cálculo de coordenadas baricéntricas en el plano XZ
            float det = (p2.z - p3.z) * (p1.x - p3.x) + (p3.x - p2.x) * (p1.z - p3.z);
            if (std::abs(det) < 0.00001f)
                continue; // Ignorar triángulos totalmente verticales

            float l1 = ((p2.z - p3.z) * (x - p3.x) + (p3.x - p2.x) * (z - p3.z)) / det;
            float l2 = ((p3.z - p1.z) * (x - p3.x) + (p1.x - p3.x) * (z - p3.z)) / det;
            float l3 = 1.0f - l1 - l2;

            // Si el punto (x, z) cae dentro del triángulo
            if (l1 >= -0.001f && l2 >= -0.001f && l3 >= -0.001f)
            {
                float yTriangulo = l1 * p1.y + l2 * p2.y + l3 * p3.y;

                // Elegimos la superficie del suelo más cercana a nuestra altura actual
                // (evita que el esquiador se suba al techo del cartel de salida o a la copa de un pino)
                float dif = std::abs(yTriangulo - yActual);
                if (dif < menorDiferencia && yTriangulo <= yActual + 8.0f)
                {
                    menorDiferencia = dif;
                    alturaEncontrada = yTriangulo;
                    colisionDetectada = true;
                }
            }
        }
    }

    return colisionDetectada ? alturaEncontrada : yActual;
}

// Lógica de descenso siguiendo la geometría real de la malla y cronómetro (Tarjetas HEP)
void ActualizarCarrera(Model& terreno)
{
    if (estadoCarrera == 1) // Carrera en curso
    {
        tiempoActualCarrera = glfwGetTime() - tiempoInicioCarrera;

        glm::vec2 vecTotalXZ(posMeta.x - posSalida.x, posMeta.z - posSalida.z);
        float distTotalXZ = glm::length(vecTotalXZ);

        if (distTotalXZ > 0.001f)
        {
            glm::vec2 dirPista = glm::normalize(vecTotalXZ);
            glm::vec2 dirDerecha(dirPista.y, -dirPista.x);

            float anguloBase = glm::degrees(atan2(dirPista.x, dirPista.y));

            // Avance en XZ
            skierPos.x += dirPista.x * velDescenso * deltaTime;
            skierPos.z += dirPista.y * velDescenso * deltaTime;

            // Slalom lateral (A/D o Flechas Izquierda/Derecha)
            if (modoCamara == 1)
            {
                if (keys[GLFW_KEY_A] || keys[GLFW_KEY_LEFT])
                {
                    skierPos.x -= dirDerecha.x * velGiroLateral * deltaTime;
                    skierPos.z -= dirDerecha.y * velGiroLateral * deltaTime;
                    skierRotY = anguloBase + 18.0f;
                }
                else if (keys[GLFW_KEY_D] || keys[GLFW_KEY_RIGHT])
                {
                    skierPos.x += dirDerecha.x * velGiroLateral * deltaTime;
                    skierPos.z += dirDerecha.y * velGiroLateral * deltaTime;
                    skierRotY = anguloBase - 18.0f;
                }
                else
                {
                    skierRotY = anguloBase;
                }
            }

            // Altura exacta leyendo el triángulo de la malla bajo los pies del esquiador
            float ySuelo = ObtenerAlturaTerreno(terreno, skierPos.x, skierPos.z, skierPos.y - offsetPiesEsquiador);
            skierPos.y = ySuelo + offsetPiesEsquiador;

            // Verificar llegada a la META
            glm::vec2 vecActualXZ(skierPos.x - posSalida.x, skierPos.z - posSalida.z);
            float progreso = glm::dot(vecActualXZ, dirPista) / distTotalXZ;

            if (progreso >= 0.99f || glm::distance(glm::vec2(skierPos.x, skierPos.z), glm::vec2(posMeta.x, posMeta.z)) < 6.0f)
            {
                estadoCarrera = 2;
                tiempoFinalCarrera = tiempoActualCarrera;
                std::cout << "\n==========================================" << std::endl;
                std::cout << "         ¡META CRUZADA!                   " << std::endl;
                std::cout << "   Tiempo Final del Descenso: " << tiempoFinalCarrera << " seg" << std::endl;
                std::cout << "==========================================\n" << std::endl;
            }
        }
    }
}

void DoMovement()
{
    if (modoCamara == 0)
    {
        float velocidadCamara = deltaTime * 12.0f;
        if (keys[GLFW_KEY_W] || keys[GLFW_KEY_UP])    camera.ProcessKeyboard(FORWARD, velocidadCamara);
        if (keys[GLFW_KEY_S] || keys[GLFW_KEY_DOWN])  camera.ProcessKeyboard(BACKWARD, velocidadCamara);
        if (keys[GLFW_KEY_A] || keys[GLFW_KEY_LEFT])  camera.ProcessKeyboard(LEFT, velocidadCamara);
        if (keys[GLFW_KEY_D] || keys[GLFW_KEY_RIGHT]) camera.ProcessKeyboard(RIGHT, velocidadCamara);
    }

    if (keys[GLFW_KEY_N])
    {
        densidadNiebla += 0.00005f;
        if (densidadNiebla > 0.02f) densidadNiebla = 0.02f;
    }
    if (keys[GLFW_KEY_M])
    {
        densidadNiebla -= 0.00005f;
        if (densidadNiebla < 0.0f) densidadNiebla = 0.0f;
    }
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (key >= 0 && key < 1024)
    {
        if (action == GLFW_PRESS)        keys[key] = true;
        else if (action == GLFW_RELEASE) keys[key] = false;
    }

    if (key == GLFW_KEY_C && action == GLFW_PRESS)
    {
        modoCamara = (modoCamara == 0) ? 1 : 0;
        std::cout << "Modo de Camara: " << (modoCamara == 0 ? "Inspeccion (Hub)" : "3ra Persona (Pista)") << std::endl;
    }

    if (key == GLFW_KEY_P && action == GLFW_PRESS)
    {
        std::cout << "Coordenada actual -> glm::vec3(" << camera.GetPosition().x
            << "f, " << camera.GetPosition().y
            << "f, " << camera.GetPosition().z << "f);" << std::endl;
    }

    if (key == GLFW_KEY_F1 && action == GLFW_PRESS)
    {
        posSalida = camera.GetPosition();
        skierPos = posSalida;
        std::cout << ">>> [F1] Punto de SALIDA actualizado." << std::endl;
    }

    if (key == GLFW_KEY_F2 && action == GLFW_PRESS)
    {
        posMeta = camera.GetPosition();
        std::cout << ">>> [F2] Punto de META actualizado." << std::endl;
    }

    if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        skierPos = posSalida;
        estadoCarrera = 1;
        modoCamara = 1;
        tiempoInicioCarrera = glfwGetTime();
        std::cout << ">>> ¡INICIA EL DESCENSO DE SLALOM! Cronometro corriendo..." << std::endl;
    }

    if (key == GLFW_KEY_T && action == GLFW_PRESS)
    {
        skierPos = posSalida;
        estadoCarrera = 0;
        tiempoActualCarrera = 0.0;
        std::cout << ">>> Esquiador posicionado en la puerta de Salida (START)." << std::endl;
    }

    if (key == GLFW_KEY_1 && action == GLFW_PRESS)
    {
        estadoClima = 0;
        densidadNiebla = 0.0006f;
        intensidadSol = 0.90f;
        colorNiebla = glm::vec3(0.82f, 0.88f, 0.95f);
        std::cout << "Clima: Despejado" << std::endl;
    }
    if (key == GLFW_KEY_2 && action == GLFW_PRESS)
    {
        estadoClima = 1;
        densidadNiebla = 0.0035f;
        intensidadSol = 0.65f;
        colorNiebla = glm::vec3(0.75f, 0.80f, 0.85f);
        std::cout << "Clima: Nevada Moderada" << std::endl;
    }
    if (key == GLFW_KEY_3 && action == GLFW_PRESS)
    {
        estadoClima = 2;
        densidadNiebla = 0.0090f;
        intensidadSol = 0.40f;
        colorNiebla = glm::vec3(0.65f, 0.70f, 0.75f);
        std::cout << "Clima: Niebla Densa" << std::endl;
    }
}

void MouseCallback(GLFWwindow* window, double xPos, double yPos)
{
    if (firstMouse)
    {
        lastX = xPos;
        lastY = yPos;
        firstMouse = false;
    }

    GLfloat xOffset = xPos - lastX;
    GLfloat yOffset = lastY - yPos;

    lastX = xPos;
    lastY = yPos;

    if (modoCamara == 0)
    {
        camera.ProcessMouseMovement(xOffset, yOffset);
    }
}