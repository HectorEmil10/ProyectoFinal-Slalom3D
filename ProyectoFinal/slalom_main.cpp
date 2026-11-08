/**
 * ============================================================================
 *  UNIVERSIDAD NACIONAL AUTÓNOMA DE MÉXICO
 *  FACULTAD DE INGENIERÍA
 * ============================================================================
 *  Proyecto        : Juego de Slalom - Escenario Interactivo 3D
 *  Asignatura      : ______________________________________
 *  Grupo / Semestre: ______________________________________
 *  Profesor        : ______________________________________
 *  Fecha de creación: 07/11/2026
 *
 *  Integrantes del equipo:
 *      1. ______________________________________  No. de cuenta: __________
 *      2. ______________________________________  No. de cuenta: __________
 *      3. ______________________________________  No. de cuenta: __________
 *      4. ______________________________________  No. de cuenta: __________
 *
 *  Descripción:
 *      Aplicación gráfica en OpenGL 3.3 (GLFW + GLEW + GLM + SOIL2) que
 *      representa una pista de slalom de montaña. Incluye:
 *        - Terreno con pendiente, pinos, rocas y carteles de salida/meta.
 *        - Iluminación Blinn-Phong: luz direccional (atardecer cálido) y
 *          luces posicionales.
 *        - Niebla (fog) de intensidad variable.
 *        - Skybox (cubemap) con paisaje montañoso alrededor de la escena.
 *        - Cámara de inspección (libre) y cámara en tercera persona.
 *        - Lógica de carrera con cronómetro, límites laterales de pista y
 *          mensajes en pantalla (HUD) con desvanecimiento gradual.
 *        - Tres niveles de dificultad/clima con transición suave.
 *
 *  Controles:
 *      W/A/S/D o flechas : mover cámara de inspección / esquiar lateralmente
 *      Mouse             : orientar cámara de inspección
 *      C                 : alternar cámara (inspección / tercera persona)
 *      R                 : iniciar descenso        T : volver a la salida
 *      1 / 2 / 3         : nivel de dificultad y clima
 *      N / M             : aumentar / disminuir niebla manualmente
 *      P                 : imprimir coordenada de cámara
 *      F1 / F2           : fijar punto de SALIDA / META en la cámara actual
 *      ESC               : salir
 * ============================================================================
 */

// ============================================================================
//  ETAPA 1. INCLUSIONES
// ============================================================================

// Biblioteca estándar
#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <cstdio>
#include <cctype>

// GLEW y GLFW
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// GLM (matemáticas)
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Carga de texturas y utilerías del laboratorio
#include "SOIL2/SOIL2.h"
#include "Shader.h"
#include "Camera.h"
#include "Model.h"

// ============================================================================
//  ETAPA 2. CONSTANTES DE CONFIGURACIÓN
// ============================================================================

// ---- Ventana ----
const GLuint WIDTH = 1200, HEIGHT = 800;

// ---- Pista ----
const float ANCHO_MEDIO_PISTA    = 22.0f;  // Distancia máxima al eje de la pista (unidades)
const float SEPARACION_POSTES    = 20.0f;  // Separación entre postes de límite a lo largo de la pista
const float ALTURA_POSTE         = 6.0f;   // Altura visual de los postes de límite

// ---- Mensajes en pantalla (segundos) ----
const float TIEMPO_MENSAJE_META  = 5.0f;   // Tiempo que permanece el resultado visible
const float TIEMPO_DESVANECER    = 2.0f;   // Duración del desvanecimiento posterior
const float TIEMPO_MENSAJE_CORTO = 1.5f;   // Mensajes breves (salida, cambio de nivel, límite)

// ---- Cielo ----
// Orden de caras del cubemap: +X, -X, +Y, -Y, +Z, -Z
const char* CARAS_SKYBOX[6] = {
    "Textures/skybox/tile-r02-c02.png",
    "Textures/skybox/tile-r02-c03.png",
    "Textures/skybox/tile-r01-c02.png",
    "Textures/skybox/tile-r02-c01.png",
    "Textures/skybox/tile-r01-c01.png",
    "Textures/skybox/tile-r01-c03.png"
};

// ============================================================================
//  ETAPA 3. TIPOS DE DATOS
// ============================================================================

/**
 * @brief Conjunto de parámetros que definen la dificultad y el ambiente.
 *        Se interpolan suavemente al cambiar de nivel.
 */
struct ParametrosAmbiente
{
    float     velocidadDescenso;   // Velocidad objetivo de descenso
    float     velocidadLateral;    // Velocidad de desplazamiento lateral
    float     densidadNiebla;      // Densidad de la niebla exponencial
    float     intensidadSol;       // Intensidad de la luz direccional
    float     factorFov;           // Ensanchamiento del FOV a máxima velocidad
    float     amplitudVibracion;   // Vibración de cámara a máxima velocidad
    glm::vec3 colorNiebla;         // Color de la niebla / bruma
    glm::vec3 colorSol;            // Color de la luz direccional
    glm::vec3 colorAmbiente;       // Color de la luz ambiental
};

/**
 * @brief Glifo de 5x7 píxeles para el texto del HUD (una fila por byte,
 *        bit 4 = columna izquierda).
 */
struct Glifo
{
    char          caracter;
    unsigned char filas[7];
};

// ============================================================================
//  ETAPA 4. VARIABLES GLOBALES
// ============================================================================

// ---- Ventana ----
int SCREEN_WIDTH, SCREEN_HEIGHT;

//XCC -Saber en que zona estamos 
enum EscenaActual
{
    ESCENA_HUB,
    ESCENA_PISTA,
    ESCENA_TROFEOS
};

EscenaActual escenaActual = ESCENA_HUB;


// ---- Cámaras ----
int modoCamara = 0;                                   // 0 = inspección, 1 = tercera persona
//Camera camera(glm::vec3(-255.0f, 52.0f, 36.0f));  

//xcc- camara inicial frente al Hub
Camera camera(glm::vec3(0.0f, 3.0f, 30.0f));

GLfloat lastX = WIDTH / 2.0f;
GLfloat lastY = HEIGHT / 2.0f;
bool keys[1024];
bool firstMouse = true;

// ---- Tiempo ----
GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;

// ---- Carrera y esquiador ----
glm::vec3 posSalida(-244.717f, 46.3459f, 35.9673f);   // Bajo el cartel START
glm::vec3 posMeta(250.214f, -112.315f, 7.09446f);     // Bajo el cartel META
glm::vec3 skierPos = posSalida;                       // Nodo raíz del esquiador
float skierRotY = 93.34f;                             // Orientación (grados)
float offsetPiesEsquiador = 1.5f;                     // Mitad de la altura del indicador
float velActual = 0.0f;                               // Velocidad instantánea de descenso

// Estados: 0 = en espera en Salida, 1 = descendiendo, 2 = meta cruzada
int    estadoCarrera = 0;
double tiempoInicioCarrera = 0.0;
double tiempoActualCarrera = 0.0;
double tiempoFinalCarrera  = 0.0;

// ---- Límites laterales de pista ----
std::vector<glm::vec3> postesLimite;                  // Posición base de cada poste
bool reconstruirLimites = false;                      // Se activa al mover salida/meta (F1/F2)

// ---- Mensajes del HUD (instante de inicio; negativo = inactivo) ----
double tiempoMensajeSalida = -1.0;
double tiempoMensajeMeta   = -1.0;
double tiempoAvisoLimite   = -1.0;
double tiempoMensajeNivel  = -1.0;

// ---- Dificultad / clima ----
int nivelActual = 0;
const char* NOMBRES_NIVEL[3] = { "NIVEL 1 - FACIL", "NIVEL 2 - MEDIO", "NIVEL 3 - DIFICIL" };

//                                   vel   lat   niebla   sol   fov    vib   colorNiebla               colorSol                colorAmbiente
const ParametrosAmbiente NIVELES[3] = {
    { 40.0f, 26.0f, 0.0006f, 1.00f, 1.00f, 0.00f, glm::vec3(0.96f, 0.70f, 0.52f), glm::vec3(1.00f, 0.60f, 0.30f), glm::vec3(0.40f, 0.30f, 0.38f) },
    { 50.0f, 30.0f, 0.0035f, 0.75f, 1.08f, 0.05f, glm::vec3(0.82f, 0.64f, 0.62f), glm::vec3(0.95f, 0.55f, 0.35f), glm::vec3(0.34f, 0.28f, 0.36f) },
    { 64.0f, 34.0f, 0.0090f, 0.45f, 1.16f, 0.12f, glm::vec3(0.60f, 0.55f, 0.60f), glm::vec3(0.85f, 0.50f, 0.38f), glm::vec3(0.28f, 0.24f, 0.32f) }
};
ParametrosAmbiente ambienteActual = NIVELES[0];       // Valores en uso (interpolados)
float ajusteNieblaManual = 0.0f;                      // Desfase aplicado con las teclas N/M

// ---- Iluminación ----
// Dirección de la luz (del sol hacia la escena). Sol bajo = atardecer.
glm::vec3 direccionSol = glm::normalize(glm::vec3(-0.80f, -0.22f, -0.35f));

glm::vec3 pointLightPositions[] = {
    glm::vec3(-5.0f, 3.0f, -10.0f),
    glm::vec3(5.0f, 3.0f, -10.0f),
    glm::vec3(-4.0f, 3.0f, -25.0f),
    glm::vec3(4.0f, 3.0f, -25.0f)
};

// ---- Geometría auxiliar ----
// Cubo unitario (indicador del esquiador y postes de límite)
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

// Cubo del skybox (solo posiciones, 36 vértices)
float verticesSkybox[] = {
    -1.0f,  1.0f, -1.0f,  -1.0f, -1.0f, -1.0f,   1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,   1.0f,  1.0f, -1.0f,  -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,  -1.0f, -1.0f, -1.0f,  -1.0f,  1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,  -1.0f,  1.0f,  1.0f,  -1.0f, -1.0f,  1.0f,

     1.0f, -1.0f, -1.0f,   1.0f, -1.0f,  1.0f,   1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,   1.0f,  1.0f, -1.0f,   1.0f, -1.0f, -1.0f,

    -1.0f, -1.0f,  1.0f,  -1.0f,  1.0f,  1.0f,   1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,   1.0f, -1.0f,  1.0f,  -1.0f, -1.0f,  1.0f,

    -1.0f,  1.0f, -1.0f,   1.0f,  1.0f, -1.0f,   1.0f,  1.0f,  1.0f,
     1.0f,  1.0f,  1.0f,  -1.0f,  1.0f,  1.0f,  -1.0f,  1.0f, -1.0f,

    -1.0f, -1.0f, -1.0f,  -1.0f, -1.0f,  1.0f,   1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,  -1.0f, -1.0f,  1.0f,   1.0f, -1.0f,  1.0f
};

// ---- Recursos del HUD ----
GLuint hudVAO = 0, hudVBO = 0;

// Fuente de mapa de bits 5x7 (solo los caracteres usados en el juego)
static const Glifo GLIFOS[] = {
    { ' ', { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } },
    { '0', { 0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E } },
    { '1', { 0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E } },
    { '2', { 0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F } },
    { '3', { 0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E } },
    { '4', { 0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02 } },
    { '5', { 0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E } },
    { '6', { 0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E } },
    { '7', { 0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08 } },
    { '8', { 0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E } },
    { '9', { 0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C } },
    { '.', { 0x00, 0x00, 0x00, 0x00, 0x00, 0x06, 0x06 } },
    { ':', { 0x00, 0x06, 0x06, 0x00, 0x06, 0x06, 0x00 } },
    { '-', { 0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00 } },
    { '!', { 0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x04 } },
    { 'A', { 0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 } },
    { 'C', { 0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E } },
    { 'D', { 0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E } },
    { 'E', { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F } },
    { 'F', { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10 } },
    { 'G', { 0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F } },
    { 'I', { 0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E } },
    { 'L', { 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F } },
    { 'M', { 0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11 } },
    { 'N', { 0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11 } },
    { 'O', { 0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E } },
    { 'P', { 0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10 } },
    { 'R', { 0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11 } },
    { 'S', { 0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E } },
    { 'T', { 0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04 } },
    { 'U', { 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E } },
    { 'V', { 0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04 } }
};

// ============================================================================
//  ETAPA 5. PROTOTIPOS DE FUNCIONES
// ============================================================================

// ---- Eventos de entrada ----
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseCallback(GLFWwindow* window, double xPos, double yPos);
void DoMovement();

// ---- Lógica del juego ----
void  ActualizarCarrera(Model& terreno);
void  ActualizarAmbiente(float dt);
float ObtenerAlturaTerreno(Model& terreno, float x, float z, float yActual);
void  ConstruirLimitesPista(Model& terreno);
float Suavizar(float actual, float objetivo, float tasa, float dt);

// ---- Recursos gráficos ----
GLuint CargarSkybox(const char* caras[6]);

// ---- HUD ----
float AlphaMensaje(double inicio, float sostener, float desvanecer);
void  DibujarHUD(Shader& hudShader);

// ============================================================================
//  ETAPA 6. PROGRAMA PRINCIPAL
// ============================================================================
int main()
{
    // ------------------------------------------------------------------------
    //  6.1 Inicialización de GLFW y creación de la ventana
    // ------------------------------------------------------------------------
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

    // Registro de callbacks de entrada
    glfwSetKeyCallback(window, KeyCallback);
    glfwSetCursorPosCallback(window, MouseCallback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // ------------------------------------------------------------------------
    //  6.2 Inicialización de GLEW y estado global de OpenGL
    // ------------------------------------------------------------------------
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

    // ------------------------------------------------------------------------
    //  6.3 Carga de shaders
    // ------------------------------------------------------------------------
    Shader lightingShader("Shader/lighting.vs", "Shader/lighting.frag");  // Escena con Blinn-Phong y niebla
    Shader colorShader("Shader/core.vs", "Shader/core.frag");             // Color plano (indicador y postes)
    Shader skyShader("Shader/skybox.vs", "Shader/skybox.frag");           // Cielo
    Shader hudShader("Shader/hud.vs", "Shader/hud.frag");                 // Interfaz 2D

    // ------------------------------------------------------------------------
    //  6.4 Configuración de geometría (VAO/VBO)
    // ------------------------------------------------------------------------

    // 6.4.1 Cubo: indicador del esquiador y postes de límite
    GLuint VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verticesCubo), verticesCubo, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // 6.4.2 Skybox
    GLuint skyVAO, skyVBO;
    glGenVertexArrays(1, &skyVAO);
    glGenBuffers(1, &skyVBO);
    glBindVertexArray(skyVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verticesSkybox), verticesSkybox, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // 6.4.3 HUD (buffer dinámico de posiciones 2D)
    glGenVertexArrays(1, &hudVAO);
    glGenBuffers(1, &hudVBO);
    glBindVertexArray(hudVAO);
    glBindBuffer(GL_ARRAY_BUFFER, hudVBO);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // ------------------------------------------------------------------------
    //  6.5 Carga de modelos y texturas
    // ------------------------------------------------------------------------
    Model PistaCompleta((char*)"Models/Entorno/pista.obj");
    Model HubCentral((char*)"Models/Hub/Hub_central.obj"); //xcc

    GLuint texturaSkybox = CargarSkybox(CARAS_SKYBOX);
    bool hayTexturaSkybox = (texturaSkybox != 0);
    if (!hayTexturaSkybox)
        std::cout << "[Skybox] No se pudo cargar el cubemap; se usara el cielo procedural." << std::endl;

    // ------------------------------------------------------------------------
    //  6.6 Preparación de la escena
    // ------------------------------------------------------------------------

    // Esquiador apoyado sobre la malla en la salida
    skierPos.y = ObtenerAlturaTerreno(PistaCompleta, skierPos.x, skierPos.z, posSalida.y) + offsetPiesEsquiador;

    // Postes que delimitan lateralmente la pista
    ConstruirLimitesPista(PistaCompleta);

    // ------------------------------------------------------------------------
    //  6.7 CICLO PRINCIPAL (GAME LOOP)
    // ------------------------------------------------------------------------
    while (!glfwWindowShouldClose(window))
    {
        // ---- 6.7.1 Tiempo y eventos ----
        GLfloat currentFrame = (GLfloat)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glfwPollEvents();

        // ---- 6.7.2 Actualización lógica ----
        DoMovement();
        ActualizarAmbiente(deltaTime);
        ActualizarCarrera(PistaCompleta);

        if (reconstruirLimites)
        {
            ConstruirLimitesPista(PistaCompleta);
            reconstruirLimites = false;
        }

        // Velocidad normalizada (0..1): modula FOV y vibración de cámara
        float velNorm = glm::clamp(velActual / ambienteActual.velocidadDescenso, 0.0f, 1.0f);

        // ---- 6.7.3 Limpieza de buffers ----
        glClearColor(ambienteActual.colorNiebla.r, ambienteActual.colorNiebla.g, ambienteActual.colorNiebla.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ---- 6.7.4 Matrices de vista y proyección ----
        glm::mat4 view;
        glm::vec3 currentViewPos = camera.GetPosition();
        float fov = camera.GetZoom();

        if (modoCamara == 0)
        {
            view = camera.GetViewMatrix();
        }
        else
        {
            // Posición trasera de la cámara, sin atravesar el terreno
            glm::vec3 offset(-sin(glm::radians(skierRotY)) * 12.0f, 6.0f, -cos(glm::radians(skierRotY)) * 12.0f);
            glm::vec3 camPos = skierPos + offset;
            float alturaPisoCamara = ObtenerAlturaTerreno(PistaCompleta, camPos.x, camPos.z, camPos.y);
            if (camPos.y < alturaPisoCamara + 4.0f)
                camPos.y = alturaPisoCamara + 4.0f;

            // Vibración de cámara: sensación de velocidad según la dificultad
            if (estadoCarrera == 1)
            {
                float vib = ambienteActual.amplitudVibracion * velNorm;
                camPos.x += vib * std::sin(currentFrame * 41.0f);
                camPos.y += vib * std::sin(currentFrame * 53.0f + 1.7f);
            }

            glm::vec3 target = skierPos + glm::vec3(0.0f, 1.0f, 0.0f);
            view = glm::lookAt(camPos, target, glm::vec3(0.0f, 1.0f, 0.0f));
            currentViewPos = camPos;

            // FOV dinámico: se ensancha al aumentar la velocidad
            fov *= 1.0f + (ambienteActual.factorFov - 1.0f) * velNorm;
        }

        glm::mat4 projection = glm::perspective(fov, (GLfloat)SCREEN_WIDTH / (GLfloat)SCREEN_HEIGHT, 0.1f, 2000.0f);

        // ---- 6.7.5 Uniformes de iluminación y niebla ----
        lightingShader.Use();

        glUniform3f(glGetUniformLocation(lightingShader.Program, "viewPos"), currentViewPos.x, currentViewPos.y, currentViewPos.z);
        glUniform3f(glGetUniformLocation(lightingShader.Program, "material.ambient"), 0.5f, 0.5f, 0.5f);
        glUniform3f(glGetUniformLocation(lightingShader.Program, "material.diffuse"), 0.8f, 0.8f, 0.8f);
        glUniform3f(glGetUniformLocation(lightingShader.Program, "material.specular"), 0.6f, 0.6f, 0.6f);
        glUniform1f(glGetUniformLocation(lightingShader.Program, "material.shininess"), 32.0f);

        // Niebla (nivel de dificultad + ajuste manual N/M)
        float densidadNiebla = glm::clamp(ambienteActual.densidadNiebla + ajusteNieblaManual, 0.0f, 0.02f);
        glUniform1f(glGetUniformLocation(lightingShader.Program, "fogDensity"), densidadNiebla);
        glUniform3f(glGetUniformLocation(lightingShader.Program, "fogColor"),
                    ambienteActual.colorNiebla.r, ambienteActual.colorNiebla.g, ambienteActual.colorNiebla.b);

        // Luz direccional: sol de atardecer (baja, cálida, ambiente fría)
        glm::vec3 solAmbiente = ambienteActual.colorAmbiente * (0.6f + 0.4f * ambienteActual.intensidadSol);
        glm::vec3 solDifuso   = ambienteActual.colorSol * ambienteActual.intensidadSol;
        glm::vec3 solEspecular = ambienteActual.colorSol * 0.5f;
        glUniform3f(glGetUniformLocation(lightingShader.Program, "dirLight.direction"), direccionSol.x, direccionSol.y, direccionSol.z);
        glUniform3f(glGetUniformLocation(lightingShader.Program, "dirLight.ambient"),  solAmbiente.r, solAmbiente.g, solAmbiente.b);
        glUniform3f(glGetUniformLocation(lightingShader.Program, "dirLight.diffuse"),  solDifuso.r, solDifuso.g, solDifuso.b);
        glUniform3f(glGetUniformLocation(lightingShader.Program, "dirLight.specular"), solEspecular.r, solEspecular.g, solEspecular.b);

        // Luces posicionales (Hub y Sala de Trofeos)
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

        // SpotLight (apagada)
        glUniform3f(glGetUniformLocation(lightingShader.Program, "spotLight.ambient"), 0.0f, 0.0f, 0.0f);
        glUniform3f(glGetUniformLocation(lightingShader.Program, "spotLight.diffuse"), 0.0f, 0.0f, 0.0f);
        glUniform3f(glGetUniformLocation(lightingShader.Program, "spotLight.specular"), 0.0f, 0.0f, 0.0f);
        glUniform1f(glGetUniformLocation(lightingShader.Program, "spotLight.constant"), 1.0f);
        glUniform1f(glGetUniformLocation(lightingShader.Program, "spotLight.linear"), 0.09f);
        glUniform1f(glGetUniformLocation(lightingShader.Program, "spotLight.quadratic"), 0.032f);
        glUniform1f(glGetUniformLocation(lightingShader.Program, "spotLight.cutOff"), glm::cos(glm::radians(12.5f)));
        glUniform1f(glGetUniformLocation(lightingShader.Program, "spotLight.outerCutOff"), glm::cos(glm::radians(17.5f)));

        GLint modelLoc = glGetUniformLocation(lightingShader.Program, "model");
        GLint viewLoc  = glGetUniformLocation(lightingShader.Program, "view");
        GLint projLoc  = glGetUniformLocation(lightingShader.Program, "projection");
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // ---- 6.7.6 Dibujado del escenario ----
        glm::mat4 model = glm::mat4(1.0f);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(glGetUniformLocation(lightingShader.Program, "transparency"), 0);
        glUniform2f(glGetUniformLocation(lightingShader.Program, "uvOffset"), 0.0f, 0.0f);
       /* PistaCompleta.Draw(lightingShader); */
        if (escenaActual == ESCENA_HUB)
        {
            HubCentral.Draw(lightingShader);
        }
        else if (escenaActual == ESCENA_PISTA)
        {
            PistaCompleta.Draw(lightingShader);
        }//xcc prueba 


        // ---- 6.7.7 Elementos de color plano: postes de límite y esquiador ----

        //xcc- elementos visibles solo en la pista
        if (escenaActual == ESCENA_PISTA)
        {

            colorShader.Use();
            glUniformMatrix4fv(glGetUniformLocation(colorShader.Program, "view"), 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(glGetUniformLocation(colorShader.Program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
            GLint colorModelLoc = glGetUniformLocation(colorShader.Program, "model");

            glBindVertexArray(VAO);

            // Postes de límite (alternan rojo / blanco, estilo puertas de slalom)
            for (size_t i = 0; i < postesLimite.size(); i++)
            {
                glm::mat4 modelPoste = glm::translate(glm::mat4(1.0f), postesLimite[i] + glm::vec3(0.0f, ALTURA_POSTE * 0.5f, 0.0f));
                modelPoste = glm::scale(modelPoste, glm::vec3(0.6f, ALTURA_POSTE, 0.6f));
                glUniformMatrix4fv(colorModelLoc, 1, GL_FALSE, glm::value_ptr(modelPoste));
                glm::vec3 colorPoste = ((i / 2) % 2 == 0) ? glm::vec3(0.85f, 0.10f, 0.10f) : glm::vec3(0.95f, 0.95f, 0.95f);
                glUniform3fv(colorShader.uniformColor, 1, glm::value_ptr(colorPoste));
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }

            // Nodo raíz del esquiador (indicador naranja, hasta recibir el modelo final)
            glm::mat4 modelTemp = glm::mat4(1.0f);
            modelTemp = glm::translate(modelTemp, skierPos);
            modelTemp = glm::rotate(modelTemp, glm::radians(skierRotY), glm::vec3(0.0f, 1.0f, 0.0f));
            glm::mat4 modelIndicador = glm::scale(modelTemp, glm::vec3(1.5f, 3.0f, 1.5f));
            glUniformMatrix4fv(colorModelLoc, 1, GL_FALSE, glm::value_ptr(modelIndicador));
            glm::vec3 colorNaranja(1.0f, 0.35f, 0.0f);
            glUniform3fv(colorShader.uniformColor, 1, glm::value_ptr(colorNaranja));
            glDrawArrays(GL_TRIANGLES, 0, 36);

            glBindVertexArray(0);

        }

        //xcc- terreno amplio alrededor del Hub
        if (escenaActual == ESCENA_HUB)
        {
            colorShader.Use();

            glUniformMatrix4fv(
                glGetUniformLocation(colorShader.Program, "view"), 1, GL_FALSE, glm::value_ptr(view)
            );

            glUniformMatrix4fv(
                glGetUniformLocation(colorShader.Program, "projection"), 1, GL_FALSE, glm::value_ptr(projection)
            );

            GLint colorModelLoc = glGetUniformLocation(colorShader.Program, "model");

            glBindVertexArray(VAO);

            glm::mat4 sueloHub = glm::mat4(1.0f);

            //xcc- nieve justo debajo del piso del Hub
            sueloHub = glm::translate(sueloHub, glm::vec3(0.0f, -0.11f, 0.0f));
            sueloHub = glm::scale(sueloHub, glm::vec3(500.0f, 0.10f, 500.0f));

            glUniformMatrix4fv(
                colorModelLoc,
                1,
                GL_FALSE,
                glm::value_ptr(sueloHub)
            );

            //xcc- color de nieve
            glm::vec3 colorNieve(0.90f, 0.94f, 1.0f);
            glUniform3fv( colorShader.uniformColor, 1, glm::value_ptr(colorNieve));

            glDrawArrays(GL_TRIANGLES, 0, 36);

            glBindVertexArray(0);
        }


        // ---- 6.7.8 Skybox (se dibuja al final con prueba de profundidad LEQUAL) ----
        glDepthFunc(GL_LEQUAL);
        skyShader.Use();
        glm::mat4 viewSky = glm::mat4(glm::mat3(view));   // Sin traslación
        glUniformMatrix4fv(glGetUniformLocation(skyShader.Program, "view"), 1, GL_FALSE, glm::value_ptr(viewSky));
        glUniformMatrix4fv(glGetUniformLocation(skyShader.Program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

        glm::vec3 haciaSol = -direccionSol;
        glm::vec3 tinteCielo = glm::mix(glm::vec3(1.0f), glm::vec3(1.0f, 0.82f, 0.72f), 0.8f) * (0.55f + 0.45f * ambienteActual.intensidadSol);
        float hazeFactor = glm::clamp(densidadNiebla * 90.0f, 0.0f, 0.95f);

        glUniform3f(glGetUniformLocation(skyShader.Program, "sunDir"), haciaSol.x, haciaSol.y, haciaSol.z);
        glUniform3f(glGetUniformLocation(skyShader.Program, "sunColor"), ambienteActual.colorSol.r, ambienteActual.colorSol.g, ambienteActual.colorSol.b);
        glUniform3f(glGetUniformLocation(skyShader.Program, "fogColor"), ambienteActual.colorNiebla.r, ambienteActual.colorNiebla.g, ambienteActual.colorNiebla.b);
        glUniform3f(glGetUniformLocation(skyShader.Program, "skyTint"), tinteCielo.r, tinteCielo.g, tinteCielo.b);
        glUniform1f(glGetUniformLocation(skyShader.Program, "hazeFactor"), hazeFactor);
        glUniform1i(glGetUniformLocation(skyShader.Program, "hayTextura"), hayTexturaSkybox ? 1 : 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, texturaSkybox);
        glUniform1i(glGetUniformLocation(skyShader.Program, "skybox"), 0);

        glBindVertexArray(skyVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS);

        // ---- 6.7.9 Interfaz 2D (HUD) ----
     /*   DibujarHUD(hudShader);*/




        //xcc- HUD visible solo en la pista
        if (escenaActual == ESCENA_PISTA)
        {
            DibujarHUD(hudShader);
        }

        // ---- 6.7.10 Intercambio de buffers ----
        glfwSwapBuffers(window);
    }

    // ------------------------------------------------------------------------
    //  6.8 Liberación de recursos
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &skyVAO);
    glDeleteBuffers(1, &skyVBO);
    glDeleteVertexArrays(1, &hudVAO);
    glDeleteBuffers(1, &hudVBO);
    if (hayTexturaSkybox)
        glDeleteTextures(1, &texturaSkybox);
    glfwTerminate();
    return 0;
}

// ============================================================================
//  ETAPA 7. IMPLEMENTACIÓN DE FUNCIONES
// ============================================================================

// ----------------------------------------------------------------------------
//  7.1 Utilidades matemáticas
// ----------------------------------------------------------------------------

/**
 * @brief Aproxima un valor hacia un objetivo con decaimiento exponencial
 *        (independiente de la tasa de cuadros).
 * @param actual   Valor actual.
 * @param objetivo Valor al que se desea llegar.
 * @param tasa     Rapidez de la aproximación (1/s).
 * @param dt       Tiempo transcurrido desde el cuadro anterior (s).
 * @return Nuevo valor interpolado.
 */
float Suavizar(float actual, float objetivo, float tasa, float dt)
{
    return actual + (objetivo - actual) * (1.0f - std::exp(-tasa * dt));
}

// ----------------------------------------------------------------------------
//  7.2 Recursos gráficos
// ----------------------------------------------------------------------------

/**
 * @brief Carga un cubemap a partir de seis imágenes.
 * @param caras Rutas en el orden +X, -X, +Y, -Y, +Z, -Z.
 * @return Identificador de la textura, o 0 si alguna cara no pudo cargarse.
 */
GLuint CargarSkybox(const char* caras[6])
{
    GLuint textura;
    glGenTextures(1, &textura);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textura);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);   // Imágenes RGB con ancho no múltiplo de 4

    for (int i = 0; i < 6; i++)
    {
        int ancho, alto;
        unsigned char* imagen = SOIL_load_image(caras[i], &ancho, &alto, 0, SOIL_LOAD_RGB);
        if (!imagen)
        {
            std::cout << "[Skybox] Error al cargar: " << caras[i] << std::endl;
            glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
            glDeleteTextures(1, &textura);
            return 0;
        }
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, ancho, alto, 0, GL_RGB, GL_UNSIGNED_BYTE, imagen);
        SOIL_free_image_data(imagen);
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    return textura;
}

// ----------------------------------------------------------------------------
//  7.3 Colisión con el terreno y límites de pista
// ----------------------------------------------------------------------------

/**
 * @brief Obtiene la altura exacta del terreno bajo el punto (x, z).
 *        Localiza el triángulo de la malla que contiene el punto e
 *        interpola Y con coordenadas baricéntricas.
 * @param terreno Modelo que contiene la malla de la pista.
 * @param x,z     Coordenadas horizontales a evaluar.
 * @param yActual Altura de referencia; se elige la superficie más cercana a ella.
 * @return Altura del terreno, o yActual si no hay colisión.
 */
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

            // Filtro rápido: caja envolvente 2D del triángulo en XZ
            float minX = glm::min(p1.x, glm::min(p2.x, p3.x));
            float maxX = glm::max(p1.x, glm::max(p2.x, p3.x));
            float minZ = glm::min(p1.z, glm::min(p2.z, p3.z));
            float maxZ = glm::max(p1.z, glm::max(p2.z, p3.z));

            if (x < minX || x > maxX || z < minZ || z > maxZ)
                continue;

            // Coordenadas baricéntricas en el plano XZ
            float det = (p2.z - p3.z) * (p1.x - p3.x) + (p3.x - p2.x) * (p1.z - p3.z);
            if (std::abs(det) < 0.00001f)
                continue; // Triángulo vertical

            float l1 = ((p2.z - p3.z) * (x - p3.x) + (p3.x - p2.x) * (z - p3.z)) / det;
            float l2 = ((p3.z - p1.z) * (x - p3.x) + (p1.x - p3.x) * (z - p3.z)) / det;
            float l3 = 1.0f - l1 - l2;

            if (l1 >= -0.001f && l2 >= -0.001f && l3 >= -0.001f)
            {
                float yTriangulo = l1 * p1.y + l2 * p2.y + l3 * p3.y;

                // Superficie más cercana a la altura actual (ignora techos de carteles y copas de pinos)
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

/**
 * @brief Genera los postes que delimitan lateralmente la pista, a ambos
 *        lados del eje Salida-Meta, apoyados sobre la malla del terreno.
 *        Se ejecuta una sola vez (y al reubicar salida/meta) porque
 *        ObtenerAlturaTerreno recorre toda la malla.
 * @param terreno Modelo de la pista.
 */
void ConstruirLimitesPista(Model& terreno)
{
    postesLimite.clear();

    glm::vec2 vecTotalXZ(posMeta.x - posSalida.x, posMeta.z - posSalida.z);
    float distTotalXZ = glm::length(vecTotalXZ);
    if (distTotalXZ < 0.001f)
        return;

    glm::vec2 dirPista = glm::normalize(vecTotalXZ);
    glm::vec2 dirDerecha(dirPista.y, -dirPista.x);
    int cantidad = (int)(distTotalXZ / SEPARACION_POSTES);

    for (int i = 0; i <= cantidad; i++)
    {
        float t = (float)i / (float)(cantidad > 0 ? cantidad : 1);
        glm::vec2 centro = glm::vec2(posSalida.x, posSalida.z) + dirPista * (t * distTotalXZ);
        float yLinea = glm::mix(posSalida.y, posMeta.y, t);

        for (int lado = -1; lado <= 1; lado += 2)
        {
            glm::vec2 p = centro + dirDerecha * (ANCHO_MEDIO_PISTA * (float)lado);
            float y = ObtenerAlturaTerreno(terreno, p.x, p.y, yLinea);
            postesLimite.push_back(glm::vec3(p.x, y, p.y));
        }
    }
}

// ----------------------------------------------------------------------------
//  7.4 Lógica del juego
// ----------------------------------------------------------------------------

/**
 * @brief Interpola los parámetros de ambiente y dificultad hacia los del
 *        nivel seleccionado para que los cambios se perciban de forma gradual.
 * @param dt Tiempo transcurrido desde el cuadro anterior (s).
 */
void ActualizarAmbiente(float dt)
{
    const ParametrosAmbiente& obj = NIVELES[nivelActual];
    const float tasa = 2.0f;
    float k = 1.0f - std::exp(-tasa * dt);

    ambienteActual.velocidadDescenso += (obj.velocidadDescenso - ambienteActual.velocidadDescenso) * k;
    ambienteActual.velocidadLateral  += (obj.velocidadLateral  - ambienteActual.velocidadLateral)  * k;
    ambienteActual.densidadNiebla    += (obj.densidadNiebla    - ambienteActual.densidadNiebla)    * k;
    ambienteActual.intensidadSol     += (obj.intensidadSol     - ambienteActual.intensidadSol)     * k;
    ambienteActual.factorFov         += (obj.factorFov         - ambienteActual.factorFov)         * k;
    ambienteActual.amplitudVibracion += (obj.amplitudVibracion - ambienteActual.amplitudVibracion) * k;
    ambienteActual.colorNiebla  = glm::mix(ambienteActual.colorNiebla,  obj.colorNiebla,  k);
    ambienteActual.colorSol     = glm::mix(ambienteActual.colorSol,     obj.colorSol,     k);
    ambienteActual.colorAmbiente = glm::mix(ambienteActual.colorAmbiente, obj.colorAmbiente, k);
}

/**
 * @brief Actualiza la carrera: avance siguiendo el eje Salida-Meta,
 *        desplazamiento lateral, límites de pista, altura sobre la malla,
 *        cronómetro y detección de meta.
 * @param terreno Modelo de la pista (para ajustar la altura del esquiador).
 */
void ActualizarCarrera(Model& terreno)
{
    if (estadoCarrera != 1)
        return;

    double ahora = glfwGetTime();
    tiempoActualCarrera = ahora - tiempoInicioCarrera;

    glm::vec2 vecTotalXZ(posMeta.x - posSalida.x, posMeta.z - posSalida.z);
    float distTotalXZ = glm::length(vecTotalXZ);
    if (distTotalXZ <= 0.001f)
        return;

    glm::vec2 dirPista = glm::normalize(vecTotalXZ);
    glm::vec2 dirDerecha(dirPista.y, -dirPista.x);
    float anguloBase = glm::degrees(atan2(dirPista.x, dirPista.y));

    // Aceleración progresiva hacia la velocidad del nivel actual
    velActual = Suavizar(velActual, ambienteActual.velocidadDescenso, 1.2f, deltaTime);

    // Avance en XZ
    skierPos.x += dirPista.x * velActual * deltaTime;
    skierPos.z += dirPista.y * velActual * deltaTime;

    // Slalom lateral (A/D o flechas)
    if (modoCamara == 1)
    {
        if (keys[GLFW_KEY_A] || keys[GLFW_KEY_LEFT])
        {
            skierPos.x -= dirDerecha.x * ambienteActual.velocidadLateral * deltaTime;
            skierPos.z -= dirDerecha.y * ambienteActual.velocidadLateral * deltaTime;
            skierRotY = anguloBase + 18.0f;
        }
        else if (keys[GLFW_KEY_D] || keys[GLFW_KEY_RIGHT])
        {
            skierPos.x += dirDerecha.x * ambienteActual.velocidadLateral * deltaTime;
            skierPos.z += dirDerecha.y * ambienteActual.velocidadLateral * deltaTime;
            skierRotY = anguloBase - 18.0f;
        }
        else
        {
            skierRotY = anguloBase;
        }
    }

    // Límites laterales: se proyecta la posición sobre el eje lateral de la pista
    glm::vec2 relativo(skierPos.x - posSalida.x, skierPos.z - posSalida.z);
    float lateral = glm::dot(relativo, dirDerecha);
    if (std::abs(lateral) > ANCHO_MEDIO_PISTA)
    {
        float limite = (lateral > 0.0f) ? ANCHO_MEDIO_PISTA : -ANCHO_MEDIO_PISTA;
        float exceso = lateral - limite;
        skierPos.x -= dirDerecha.x * exceso;
        skierPos.z -= dirDerecha.y * exceso;
        tiempoAvisoLimite = ahora;
    }

    // Altura exacta leyendo el triángulo de la malla bajo el esquiador
    float ySuelo = ObtenerAlturaTerreno(terreno, skierPos.x, skierPos.z, skierPos.y - offsetPiesEsquiador);
    skierPos.y = ySuelo + offsetPiesEsquiador;

    // Verificación de llegada a la META
    glm::vec2 vecActualXZ(skierPos.x - posSalida.x, skierPos.z - posSalida.z);
    float progreso = glm::dot(vecActualXZ, dirPista) / distTotalXZ;

    if (progreso >= 0.99f || glm::distance(glm::vec2(skierPos.x, skierPos.z), glm::vec2(posMeta.x, posMeta.z)) < 6.0f)
    {
        estadoCarrera = 2;
        tiempoFinalCarrera = tiempoActualCarrera;
        tiempoMensajeMeta = ahora;
        velActual = 0.0f;
        std::cout << "META CRUZADA - Tiempo final: " << tiempoFinalCarrera << " seg" << std::endl;
    }
}

// ----------------------------------------------------------------------------
//  7.5 Interfaz 2D (HUD)
// ----------------------------------------------------------------------------

/**
 * @brief Calcula la opacidad de un mensaje temporal: 1.0 durante el tiempo
 *        de permanencia y descenso lineal a 0.0 durante el desvanecimiento.
 * @param inicio     Instante de aparición (s); negativo = inactivo.
 * @param sostener   Duración a opacidad completa (s).
 * @param desvanecer Duración del desvanecimiento (s).
 * @return Opacidad en [0, 1].
 */
float AlphaMensaje(double inicio, float sostener, float desvanecer)
{
    if (inicio < 0.0)
        return 0.0f;
    float t = (float)(glfwGetTime() - inicio);
    if (t <= sostener)
        return 1.0f;
    if (t >= sostener + desvanecer)
        return 0.0f;
    return 1.0f - (t - sostener) / desvanecer;
}

/** @brief Busca el glifo de un carácter (devuelve nullptr si no existe). */
static const unsigned char* ObtenerGlifo(char c)
{
    c = (char)std::toupper((unsigned char)c);
    for (size_t i = 0; i < sizeof(GLIFOS) / sizeof(GLIFOS[0]); i++)
        if (GLIFOS[i].caracter == c)
            return GLIFOS[i].filas;
    return nullptr;
}

/** @brief Agrega un rectángulo (2 triángulos) al buffer de vértices del HUD. */
static void AnadirRect(std::vector<GLfloat>& b, float x, float y, float w, float h)
{
    GLfloat v[12] = { x, y,  x + w, y,  x + w, y + h,  x + w, y + h,  x, y + h,  x, y };
    b.insert(b.end(), v, v + 12);
}

/** @brief Genera la geometría de un texto (un rectángulo por píxel encendido). */
static void ConstruirTexto(std::vector<GLfloat>& b, const std::string& texto, float x, float y, float e)
{
    for (size_t k = 0; k < texto.size(); k++)
    {
        const unsigned char* g = ObtenerGlifo(texto[k]);
        if (g)
        {
            for (int fila = 0; fila < 7; fila++)
                for (int col = 0; col < 5; col++)
                    if (g[fila] & (1 << (4 - col)))
                        AnadirRect(b, x + col * e, y + fila * e, e, e);
        }
        x += 6.0f * e;   // 5 columnas + 1 de separación
    }
}

/** @brief Envía el buffer al GPU y lo dibuja con el color indicado. */
static void EnviarBufferHud(Shader& hud, const std::vector<GLfloat>& b, const glm::vec4& color)
{
    if (b.empty() || color.a <= 0.0f)
        return;
    glUniform4fv(glGetUniformLocation(hud.Program, "color"), 1, glm::value_ptr(color));
    glBindVertexArray(hudVAO);
    glBindBuffer(GL_ARRAY_BUFFER, hudVBO);
    glBufferData(GL_ARRAY_BUFFER, b.size() * sizeof(GLfloat), b.data(), GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(b.size() / 2));
    glBindVertexArray(0);
}

/**
 * @brief Dibuja un texto con sombra. Si centrado es verdadero, x se ignora y
 *        el texto se centra horizontalmente.
 * @param escala Tamaño de cada píxel del glifo (en píxeles de pantalla).
 */
static void DibujarTexto(Shader& hud, const std::string& texto, float x, float y, float escala,
                         const glm::vec4& color, bool centrado)
{
    if (centrado)
    {
        float ancho = texto.size() * 6.0f * escala - escala;
        x = (SCREEN_WIDTH - ancho) * 0.5f;
    }

    std::vector<GLfloat> buffer;
    ConstruirTexto(buffer, texto, x + escala * 0.8f, y + escala * 0.8f, escala);   // Sombra
    EnviarBufferHud(hud, buffer, glm::vec4(0.0f, 0.0f, 0.0f, color.a * 0.6f));

    buffer.clear();
    ConstruirTexto(buffer, texto, x, y, escala);                                    // Texto
    EnviarBufferHud(hud, buffer, color);
}

/**
 * @brief Dibuja toda la interfaz: nivel, cronómetro, avisos de salida, meta,
 *        límite de pista e indicaciones. Se renderiza sobre la escena sin
 *        prueba de profundidad.
 * @param hudShader Shader del HUD.
 */
void DibujarHUD(Shader& hudShader)
{
    glDisable(GL_DEPTH_TEST);
    hudShader.Use();

    glm::mat4 ortho = glm::ortho(0.0f, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT, 0.0f);
    glUniformMatrix4fv(glGetUniformLocation(hudShader.Program, "ortho"), 1, GL_FALSE, glm::value_ptr(ortho));

    float e = SCREEN_HEIGHT / 800.0f;     // Factor de escala según la resolución
    double ahora = glfwGetTime();
    char buf[64];

    // Nivel actual (esquina superior izquierda)
    DibujarTexto(hudShader, NOMBRES_NIVEL[nivelActual], 20.0f * e, 20.0f * e, 3.0f * e, glm::vec4(1.0f, 0.95f, 0.85f, 0.9f), false);

    // Cronómetro durante el descenso
    if (estadoCarrera == 1)
    {
        std::snprintf(buf, sizeof(buf), "TIEMPO: %.2f", tiempoActualCarrera);
        DibujarTexto(hudShader, buf, 0.0f, 20.0f * e, 4.0f * e, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), true);
    }

    // Indicación en la salida
    if (estadoCarrera == 0)
    {
        float pulso = 0.65f + 0.35f * (float)std::sin(ahora * 3.0);
        DibujarTexto(hudShader, "PRESIONA R PARA INICIAR", 0.0f, SCREEN_HEIGHT - 70.0f * e, 3.5f * e, glm::vec4(1.0f, 1.0f, 1.0f, pulso), true);
    }

    // Mensaje de SALIDA
    float aSalida = AlphaMensaje(tiempoMensajeSalida, 0.8f, TIEMPO_MENSAJE_CORTO);
    if (aSalida > 0.0f)
        DibujarTexto(hudShader, "SALIDA!", 0.0f, SCREEN_HEIGHT * 0.30f, 10.0f * e, glm::vec4(0.35f, 1.0f, 0.45f, aSalida), true);

    // Mensaje de cambio de nivel
    float aNivel = AlphaMensaje(tiempoMensajeNivel, 0.8f, TIEMPO_MENSAJE_CORTO);
    if (aNivel > 0.0f)
        DibujarTexto(hudShader, NOMBRES_NIVEL[nivelActual], 0.0f, SCREEN_HEIGHT * 0.42f, 6.0f * e, glm::vec4(1.0f, 0.75f, 0.35f, aNivel), true);

    // Aviso de límite de pista
    float aLimite = AlphaMensaje(tiempoAvisoLimite, 0.1f, 0.6f);
    if (aLimite > 0.0f && estadoCarrera == 1)
        DibujarTexto(hudShader, "LIMITE DE PISTA", 0.0f, SCREEN_HEIGHT * 0.75f, 4.0f * e, glm::vec4(1.0f, 0.25f, 0.2f, aLimite), true);

    // Resultado en META: permanece 5 s y se desvanece gradualmente
    float aMeta = AlphaMensaje(tiempoMensajeMeta, TIEMPO_MENSAJE_META, TIEMPO_DESVANECER);
    if (aMeta > 0.0f)
    {
        DibujarTexto(hudShader, "META!", 0.0f, SCREEN_HEIGHT * 0.25f, 12.0f * e, glm::vec4(1.0f, 0.85f, 0.2f, aMeta), true);
        std::snprintf(buf, sizeof(buf), "TIEMPO FINAL: %.2f SEG", tiempoFinalCarrera);
        DibujarTexto(hudShader, buf, 0.0f, SCREEN_HEIGHT * 0.45f, 6.0f * e, glm::vec4(1.0f, 1.0f, 1.0f, aMeta), true);
        DibujarTexto(hudShader, "T: VOLVER A LA SALIDA", 0.0f, SCREEN_HEIGHT * 0.58f, 3.0f * e, glm::vec4(0.9f, 0.9f, 0.9f, aMeta), true);
    }

    glEnable(GL_DEPTH_TEST);
}

// ----------------------------------------------------------------------------
//  7.6 Entrada: teclado y mouse
// ----------------------------------------------------------------------------

/**
 * @brief Procesa las teclas mantenidas: movimiento de la cámara de
 *        inspección y ajuste manual de la niebla.
 */
void DoMovement()
{
    if (modoCamara == 0)
    {

        //xcc- guarda la posicion antes de mover la camara
        glm::vec3 posicionAnterior = camera.GetPosition();

        float velocidadCamara = deltaTime * 12.0f;
        if (keys[GLFW_KEY_W] || keys[GLFW_KEY_UP])    camera.ProcessKeyboard(FORWARD, velocidadCamara);
        if (keys[GLFW_KEY_S] || keys[GLFW_KEY_DOWN])  camera.ProcessKeyboard(BACKWARD, velocidadCamara);
        if (keys[GLFW_KEY_A] || keys[GLFW_KEY_LEFT])  camera.ProcessKeyboard(LEFT, velocidadCamara);
        if (keys[GLFW_KEY_D] || keys[GLFW_KEY_RIGHT]) camera.ProcessKeyboard(RIGHT, velocidadCamara);

        //xcc- limita el movimiento de la camara dentro del Hub
        if (escenaActual == ESCENA_HUB)
        {
            glm::vec3 posicionCamara = camera.GetPosition();

            //xcc- mantiene la altura de la camara
            posicionCamara.y = 5.0f;

            //xcc- limita las paredes cuando la camara entra al Hub
            if (posicionCamara.z < 20.0f) // no te limita mientras te acercas a la puerta; los límites empiezan cuando ya estás entrando.
            {
                posicionCamara.x = glm::clamp(posicionCamara.x, -21.0f, 21.0f);
                posicionCamara.z = glm::max(posicionCamara.z, -16.0f); //pone limites a donde llega la camara dentro del hub
            }

            //xcc- bloquea la pared frontal dejando libre la puerta
            if (posicionCamara.z > 18.0f && posicionCamara.z < 21.0f)
            {
                if (posicionCamara.x < -3.3f || posicionCamara.x > 3.3f)
                {
                    posicionCamara = posicionAnterior;
                    posicionCamara.y = 3.0f;
                }
            }

            camera.SetPosition(posicionCamara);
        }
    }

    // Niebla manual: se suma al valor del nivel (limitada en el render)
    if (keys[GLFW_KEY_N])
        ajusteNieblaManual = glm::min(ajusteNieblaManual + 0.00005f, 0.02f);
    if (keys[GLFW_KEY_M])
        ajusteNieblaManual = glm::max(ajusteNieblaManual - 0.00005f, -0.02f);
}

/**
 * @brief Callback de teclado (eventos de pulsación y liberación).
 */
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (key >= 0 && key < 1024)
    {
        if (action == GLFW_PRESS)        keys[key] = true;
        else if (action == GLFW_RELEASE) keys[key] = false;
    }

    // Cambio de cámara
    if (key == GLFW_KEY_C && action == GLFW_PRESS)
    {
        modoCamara = (modoCamara == 0) ? 1 : 0;
        std::cout << "Modo de Camara: " << (modoCamara == 0 ? "Inspeccion (Hub)" : "3ra Persona (Pista)") << std::endl;
    }

    // Utilerías de posicionamiento
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
        reconstruirLimites = true;
        std::cout << ">>> [F1] Punto de SALIDA actualizado." << std::endl;
    }

    if (key == GLFW_KEY_F2 && action == GLFW_PRESS)
    {
        posMeta = camera.GetPosition();
        reconstruirLimites = true;
        std::cout << ">>> [F2] Punto de META actualizado." << std::endl;
    }

    // Inicio de carrera
    if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        skierPos = posSalida;
        estadoCarrera = 1;
        modoCamara = 1;
        velActual = 0.0f;
        tiempoInicioCarrera = glfwGetTime();
        tiempoMensajeSalida = glfwGetTime();
        tiempoMensajeMeta = -1.0;
        std::cout << ">>> INICIA EL DESCENSO DE SLALOM" << std::endl;
    }

    // Regreso a la salida
    if (key == GLFW_KEY_T && action == GLFW_PRESS)
    {
        skierPos = posSalida;
        estadoCarrera = 0;
        velActual = 0.0f;
        tiempoActualCarrera = 0.0;
        tiempoMensajeMeta = -1.0;
        std::cout << ">>> Esquiador posicionado en la puerta de Salida (START)." << std::endl;
    }

    // Niveles de dificultad y clima (la transición se interpola en ActualizarAmbiente)
    if (action == GLFW_PRESS && key >= GLFW_KEY_1 && key <= GLFW_KEY_3)
    {
        nivelActual = key - GLFW_KEY_1;
        ajusteNieblaManual = 0.0f;
        tiempoMensajeNivel = glfwGetTime();
        std::cout << "Dificultad/Clima: " << NOMBRES_NIVEL[nivelActual] << std::endl;
    }
}

/**
 * @brief Callback de mouse: orienta la cámara de inspección.
 */
void MouseCallback(GLFWwindow* window, double xPos, double yPos)
{
    if (firstMouse)
    {
        lastX = (GLfloat)xPos;
        lastY = (GLfloat)yPos;
        firstMouse = false;
    }

    GLfloat xOffset = (GLfloat)xPos - lastX;
    GLfloat yOffset = lastY - (GLfloat)yPos;

    lastX = (GLfloat)xPos;
    lastY = (GLfloat)yPos;

    if (modoCamara == 0)
        camera.ProcessMouseMovement(xOffset, yOffset);
}
