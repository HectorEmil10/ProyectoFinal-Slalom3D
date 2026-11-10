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
 *        - Control manual del esquiador (avance, freno y giro lateral).
 *        - Colisiones con el terreno, límites laterales, árboles y rocas.
 *        - Lógica de carrera con cronómetro; la victoria se registra solo
 *          al cruzar la línea de meta. Mensajes en pantalla (HUD) con
 *          desvanecimiento gradual.
 *        - Tres niveles de dificultad/clima con transición suave.
 *        - Efecto de ventisca (nieve golpeando la cámara) activable.
 *        - Dos contrincantes controlados por IA con trayectoria aleatoria.
 *        - Paredes invisibles en el arco de meta.
 *
 *  Controles:
 *      W / Flecha arriba    : avanzar (3ª persona) / mover cámara (inspección)
 *      S / Flecha abajo     : frenar (3ª persona) / retroceder cámara
 *      A / D, Flechas izq/der: esquiar lateralmente / mover cámara
 *      Mouse                : orientar cámara de inspección
 *      C                    : alternar cámara (inspección / tercera persona)
 *      R                    : iniciar descenso        T : volver a la salida
 *      1 / 2 / 3 : nivel de dificultad y clima (solo en el Hub)
 *      V                    : activar / desactivar ventisca
 *      N / M                : aumentar / disminuir niebla manualmente
 *      F3                   : mostrar / ocultar colisionadores (depuración)
 *      P                    : imprimir coordenada de cámara
 *      F1 / F2              : fijar punto de SALIDA / META en la cámara actual
 *      ESC                  : salir
 * ============================================================================
 */

 // ============================================================================
 //  ETAPA 1. INCLUSIONES
 // ============================================================================

 // Biblioteca estándar
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
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
#include "Iluminacion.h"

// ============================================================================
//  ETAPA 2. CONSTANTES DE CONFIGURACIÓN
// ============================================================================

// ---- Ventana ----
const GLuint WIDTH = 1200, HEIGHT = 800;

// ---- Cámara ----
const float ANGULO_GIRO_ESQUIADOR = 18.0f;  // Inclinación del esquiador al desplazarse (grados)
const float FOV_BASE_GRADOS = 58.0f;  // Campo de visión base (grados)
const float FOV_MAXIMO_GRADOS = 100.0f; // Tope de seguridad del campo de visión

// ---- Pista ----
const float ANCHO_MEDIO_PISTA = 60.0f;   // Distancia máxima al eje de la pista (unidades)
const float SEPARACION_POSTES = 20.0f;   // Separación entre postes de límite a lo largo de la pista
const float ALTURA_POSTE = 6.0f;    // Altura visual de los postes de límite
const float PASO_MUESTREO_ALTURA = 2.0f;    // Paso del recorrido para seguir la superficie (unidades)
const float DESFASE_POSTE = 0.2f;    // Hundimiento de la base para ocultar huecos en pendiente

// ---- Puerta de Meta y Paredes Invisibles ----
const float ANCHO_PUERTA_META = 7.5f;  // Semiancho libre bajo el cartel de META (ajustable con F3)
const float GROSOR_MURO_META = 2.5f;  // Grosor de los muros invisibles laterales en la meta

// ---- Contrincantes (IA) ----
const int   NUM_CONTRINCANTES = 2;     // Dos rivales en pista
const int   NUM_NODOS_TRAYECTORIA = 16;    // Puntos de control del zigzag aleatorio
const float ANCHO_ZIGZAG_RIVAL = 28.0f; // Amplitud lateral máxima de la ruta de los rivales
// Factor de velocidad de los rivales según el nivel (Fácil, Medio, Difícil)
const float FACTOR_VEL_RIVAL_NIVEL[3] = { 0.80f, 0.90f, 0.98f };

// ---- Control del esquiador ----
const float ACELERACION_AVANCE = 1.2f;      // Rapidez para alcanzar la velocidad máxima (1/s)
const float FRICCION_LIBRE = 0.7f;      // Desaceleración al soltar el avance (1/s)
const float FRENADO = 4.0f;      // Desaceleración al frenar (1/s)
const float RADIO_ESQUIADOR = 1.1f;      // Radio de colisión del esquiador en el plano XZ

// ---- Obstáculos (detección automática de árboles y rocas) ----
const float MAX_ANCHO_OBSTACULO = 14.0f; // Objetos más anchos se ignoran (terreno, carteles)
const float MIN_ANCHO_OBSTACULO = 0.3f;  // Objetos más angostos se ignoran
const float MIN_ALTO_OBSTACULO = 0.4f;  // Objetos más bajos se ignoran
const float RADIO_MINIMO_OBSTACULO = 0.8f;  // Radio de colisión mínimo
const float FACTOR_RADIO_ARBOL = 0.20f; // Radio del tronco respecto al ancho del árbol
const float FACTOR_RADIO_ROCA = 0.45f; // Radio de la roca respecto a su ancho
const float FACTOR_VEL_CHOQUE = 0.30f; // Fracción de la velocidad máxima permitida en contacto
const float ZONA_LIBRE_SALIDA_META = 15.0f; // Radio alrededor de salida/meta sin colisiones

// ---- Ventisca ----
const int   MAX_COPOS = 450;    // Cantidad máxima de copos en pantalla
const float NIEBLA_EXTRA_VENTISCA = 0.0025f; // Densidad de niebla adicional con ventisca total

// ---- Mensajes en pantalla (segundos) ----
const float TIEMPO_MENSAJE_META = 5.0f;    // Tiempo que permanece el resultado visible
const float TIEMPO_DESVANECER = 2.0f;    // Duración del desvanecimiento posterior
const float TIEMPO_MENSAJE_CORTO = 1.5f;    // Mensajes breves (salida, cambio de nivel, límite)

// ---- Cielo ----
// Orden de caras del cubemap: +X, -X, +Y, -Y, +Z, -Z
const char* CARAS_SKYBOX[6] = {
    "Textures/skybox/tile-r02-c02.png", //top
    "Textures/skybox/tile-r01-c03.png", // front b
    "Textures/skybox/tile-r02-c01.png", // back
    "Textures/skybox/tile-r01-c01.png", //down b
    "Textures/skybox/tile-r01-c02.png", //der
    "Textures/skybox/tile-r02-c03.png"  //izq b
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
    float     velocidadDescenso;   // Velocidad máxima de descenso
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
 * @brief Obstáculo físico (árbol o roca) modelado como cilindro vertical.
 */
struct Obstaculo
{
    glm::vec3 centro;   // Centro en XZ (y sin uso)
    float     radio;    // Radio de colisión en el plano XZ
    float     yMin;     // Base del objeto
    float     yMax;     // Parte superior del objeto
    bool      esArbol;  // true = árbol, false = roca
};

/**
 * @brief Copo de nieve en coordenadas de pantalla normalizadas.
 */
struct Copo
{
    float u;      // Posición horizontal [0, 1)
    float v;      // Posición vertical [0, 1)
    float z;      // Profundidad [0, 1): 0 = lejano, 1 = cercano
    float fase;   // Fase de turbulencia
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

/**
 * @brief Estado completo de un esquiador contrincante controlado por IA.
 */
struct Contrincante
{
    std::string        nombre;
    glm::vec3          pos;
    float              rotY;
    float              velActual;
    float              factorPersonal;     // Pequeña variación entre Rival 1 y Rival 2
    float              inclinacionLat;
    float              inclinacionTorso;
    float              oscilacionPiernas;
    float              tiempoAnim;
    int                indiceTraje;        // Skin (siempre distinta a la del jugador)
    float              avanceActual;       // Distancia recorrida sobre el eje Salida -> Meta
    bool               terminoCarrera;
    double             tiempoFinal;
    std::vector<float> nodosLaterales;     // Desfases laterales aleatorios de su trayectoria
};

/**
 * @brief Registro auxiliar para ordenar el podio final (1°, 2° y 3° lugar).
 */
struct ResultadoCompetidor
{
    std::string nombre;
    bool        termino;
    double      tiempo;
    float       avance;
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

//xcc- camara inicial frente al Hub
Camera camera(glm::vec3(0.0f, 3.0f, 30.0f));

// Posición base de cada pedestal de trofeos (calibrar con la tecla P dentro de la sala)
std::vector<glm::vec3> posicionesTrofeos = {
    glm::vec3(-9.0f, 0.0f, -6.0f), glm::vec3(-3.0f, 0.0f, -6.0f),
    glm::vec3(3.0f, 0.0f, -6.0f), glm::vec3(9.0f, 0.0f, -6.0f)
};

//xcc- guarda cuales trofeos se han desbloqueado
bool trofeosDesbloqueados[4] = { false, false, false, false};

//xcc- controla las luces generales de la sala de trofeos
bool lucesTrofeosCompletas = false;


float factorInteriorHub = 0.0f;   // 0 = afuera del Hub, 1 = adentro (suavizado)


// ---- Cámaras ----
int modoCamara = 0;                                   // 0 = inspección, 1 = tercera persona
//Camera camera(glm::vec3(-255.0f, 52.0f, 36.0f));  





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
double tiempoFinalCarrera = 0.0;

// ---- Contrincantes y Posiciones ----
Contrincante contrincantes[NUM_CONTRINCANTES];
int posicionJugadorActual = 1; // 1 = 1er lugar, 2 = 2do lugar, 3 = 3er lugar
bool reiniciarContrincantes = false;   // Se activa desde el teclado; main la atiende


// ---- Límites laterales de pista ----
std::vector<glm::vec3> postesLimite;                  // Posición base de cada poste
bool reconstruirLimites = false;                      // Se activa al mover salida/meta (F1/F2)

// ---- Obstáculos ----
std::vector<Obstaculo> obstaculos;                    // Árboles y rocas detectados en el modelo
bool mostrarColisionadores = false;                   // Depuración (F3)

// ---- Ventisca ----
std::vector<Copo> copos;                              // Copos de nieve
bool  ventiscaActiva = false;                         // Estado solicitado por el usuario (V)
float intensidadVentisca = 0.0f;                      // Intensidad suavizada [0, 1]
float vientoVentisca = 0.0f;                          // Velocidad del viento (fracción de pantalla / s)

// ---- Mensajes del HUD (instante de inicio; negativo = inactivo) ----
double tiempoMensajeSalida = -1.0;
double tiempoMensajeMeta = -1.0;
double tiempoAvisoLimite = -1.0;
double tiempoMensajeNivel = -1.0;
double tiempoAvisoChoque = -1.0;
double tiempoAyudaControles = -1.0;
double tiempoMensajeVentisca = -1.0;

// ---- Variables de animación ----
float tiempoAnim = 0.0f;
float oscilacionPiernas = 0.0f;
float inclinacionLateral = 0.0f;
float inclinacionTorso = 0.0f;

// ---- Dificultad / clima ----
int nivelActual = 0;
const char* NOMBRES_NIVEL[3] = { "NIVEL 1 - FACIL", "NIVEL 2 - MEDIO", "NIVEL 3 - DIFICIL" };
//xcc- guarda la opcion de equipamiento elegida en el Hub
int equipamientoSeleccionado = 0;

const char* NOMBRES_EQUIPAMIENTO[3] = {
    "OPCION 1",
    "OPCION 2",
    "OPCION 3"
};

// Vector para almacenar las texturas disponibles
std::vector<GLuint> opcionesTrajes;
int indiceTrajeActual = 0; // 0 = Turquesa, 1 = Rojo

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
// Cubo unitario (indicador del esquiador, postes de límite y colisionadores)
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
    { '/', { 0x01, 0x02, 0x02, 0x04, 0x08, 0x08, 0x10 } },
    { 'A', { 0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 } },
    { 'B', { 0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E } },
    { 'C', { 0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E } },
    { 'D', { 0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E } },
    { 'E', { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F } },
    { 'F', { 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10 } },
    { 'G', { 0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F } },
    { 'H', { 0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 } },
    { 'I', { 0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E } },
    { 'J', { 0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0C } },
    { 'L', { 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F } },
    { 'M', { 0x11, 0x1B, 0x15, 0x15, 0x11, 0x11, 0x11 } },
    { 'N', { 0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11 } },
    { 'O', { 0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E } },
    { 'P', { 0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10 } },
    { 'Q', { 0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D } },
    { 'R', { 0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11 } },
    { 'S', { 0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E } },
    { 'T', { 0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04 } },
    { 'U', { 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E } },
    { 'V', { 0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04 } },
    { 'Z', { 0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F } }
};


// Crea una 3ra textura deportiva en memoria si no existe un 3er archivo PNG
GLuint CrearTexturaTrajeProcedural()
{
    const int W = 64, H = 64;
    std::vector<unsigned char> pixels(W * H * 3);
    for (int y = 0; y < H; y++)
    {
        for (int x = 0; x < W; x++)
        {
            bool franja = ((x / 8 + y / 8) % 2 == 0);
            int idx = (y * W + x) * 3;
            // Combinación Dorado / Azul Marino Deportivo
            pixels[idx + 0] = franja ? 245 : 25;
            pixels[idx + 1] = franja ? 190 : 55;
            pixels[idx + 2] = franja ? 30 : 130;
        }
    }
    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, W, H, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    return texID;
}

// Garantiza que los 2 contrincantes tengan siempre skins distintas a la del jugador y entre sí
void AsignarSkinsContrincantes()
{
    int totalSkins = (int)opcionesTrajes.size();
    if (totalSkins < 3)
        return;

    contrincantes[0].indiceTraje = (indiceTrajeActual + 1) % totalSkins;
    contrincantes[1].indiceTraje = (indiceTrajeActual + 2) % totalSkins;
}

// Sobreescribe la textura activa en todas las mallas de una parte del cuerpo (para que Model::Draw use el traje indicado)
void ForzarTexturaEnModelo(Model& modelo, GLuint texID)
{
    for (size_t m = 0; m < modelo.meshes.size(); m++)
    {
        if (modelo.meshes[m].textures.empty())
        {
            Texture t;
            t.id = texID;
            t.type = "texture_diffuse";
            modelo.meshes[m].textures.push_back(t);
        }
        else
        {
            modelo.meshes[m].textures[0].id = texID;
        }
    }
}

// Prototipos de funciones de contrincantes
void  GenerarTrayectoriaContrincante(Contrincante& c, int indiceRival);
float EvaluarLateralTrayectoria(const Contrincante& c, float avance, float distTotalXZ);
void  InicializarContrincantes(Model& terreno);
void  ActualizarContrincantes(Model& terreno);
int   CalcularPosicionJugador();


// ============================================================================
//  ETAPA 5. PROTOTIPOS DE FUNCIONES
// ============================================================================

// ---- Eventos de entrada ----
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseCallback(GLFWwindow* window, double xPos, double yPos);

//xcc- verifica la distancia al panel de configuracion
bool CercaPanelConfiguracion()
{
    glm::vec3 posicionCamara = camera.GetPosition();
    glm::vec2 posicionActual(posicionCamara.x, posicionCamara.z);
    glm::vec2 posicionPanel(-13.1333f, -8.00144f);
    float distancia = glm::length(posicionActual - posicionPanel);
    return distancia < 5.0f;
}

//xcc- verifica si la camara esta cerca de la salida a pista
bool CercaSalidaPista()
{
    glm::vec3 posicionCamara = camera.GetPosition();

    return posicionCamara.z < -7.0f && posicionCamara.x > -7.0f && posicionCamara.x < 7.0f;
}

//xcc- verifica si la camara esta cerca de la entrada a trofeos
bool CercaEntradaTrofeos()
{
    glm::vec3 posicionCamara = camera.GetPosition();

    return posicionCamara.x > 13.0f && posicionCamara.x < 29.0f && posicionCamara.z > -3.0f && posicionCamara.z < 5.0f;
}

//xcc- verifica colisiones con los muebles del Hub
bool ColisionMueblesHub(glm::vec3 posicion)
{
    //xcc-  mesa de la maqueta
    if (posicion.x > -5.0f && posicion.x < 8.1f &&
        posicion.z > -4.6f && posicion.z < 3.9f)
    {
        return true;
    }

    //xcc- colision con la parte de atras de la maqueta
    if (posicion.x > -21.5f && posicion.x < -20.5f &&
        posicion.z > 11.0f && posicion.z < 15.0f)
    {
        return true;
    }

    //xcc- colision con la mesa del escritorio
    if (posicion.x > -20.44f && posicion.x < -9.90f &&
        posicion.z > 11.54f && posicion.z < 14.93f)
    {
        return true;
    }


    //xcc- colision con silla 1
    if (posicion.x > -17.8f && posicion.x < -16.1f &&
        posicion.z > 15.6f && posicion.z < 17.3f)
    {
        return true;
    }

    //xcc- colision con silla 2
    if (posicion.x > -13.3f && posicion.x < -11.6f &&
        posicion.z > 8.9f && posicion.z < 10.6f)
    {
        return true;
    }

    //xcc- colision con silla 3
    if (posicion.x > -17.4f && posicion.x < -15.6f &&
        posicion.z > 9.4f && posicion.z < 11.2f)
    {
        return true;
    }



    //xcc- colision con mueble de la pared izquierda
    if (posicion.x > -20.2f && posicion.x < -18.2f &&
        posicion.z > 1.8f && posicion.z < 3.6f)
    {
        return true;
    }

    //xcc- mueble derecho 1
    if (posicion.x > 6.0f && posicion.x < 9.5f &&
        posicion.z > 9.0f && posicion.z < 12.5f)
    {
        return true;
    }

    //xcc- mueble derecho 2
    if (posicion.x > 11.0f && posicion.x < 14.0f &&
        posicion.z > 9.0f && posicion.z < 12.5f)
    {
        return true;
    }

    //xcc- mueble derecho 3
    if (posicion.x > 14.5f && posicion.x < 17.5f &&
        posicion.z > 8.5f && posicion.z < 12.0f)
    {
        return true;
    }

    //xcc- muebles pegados a la pared izquierda
    if (posicion.x > -20.5f && posicion.x < -17.0f &&
        posicion.z > -16.0f && posicion.z < -12.5f)
    {
        return true;
    }

    if (posicion.x > -20.5f && posicion.x < -17.0f &&
        posicion.z > -7.5f && posicion.z < -4.5f)
    {
        return true;
    }

    if (posicion.x > -20.5f && posicion.x < -17.0f &&
        posicion.z > -3.0f && posicion.z < 0.0f)
    {
        return true;
    }

    if (posicion.x > -20.5f && posicion.x < -17.0f &&
        posicion.z > 4.5f && posicion.z < 8.0f)
    {
        return true;
    }

    //xcc- muebles pegados a las paredes
    if (posicion.x > 17.0f && posicion.x < 20.5f &&
        posicion.z > -15.0f && posicion.z < -11.5f)
    {
        return true;
    }

    if (posicion.x > 7.5f && posicion.x < 10.0f &&
        posicion.z > -15.0f && posicion.z < -11.5f)
    {
        return true;
    }

    if (posicion.x > 5.5f && posicion.x < 8.0f &&
        posicion.z > -15.0f && posicion.z < -11.5f)
    {
        return true;
    }

    if (posicion.x > -6.0f && posicion.x < -3.5f &&
        posicion.z > -16.0f && posicion.z < -13.0f)
    {
        return true;
    }


    //xcc- colision con banca horizontal 1
    if (posicion.x > -16.0f && posicion.x < -12.5f &&
        posicion.z > 3.0f && posicion.z < 4.1f)
    {
        return true;
    }

    //xcc- colision con banca horizontal 2
    if (posicion.x > -17.3f && posicion.x < -12.5f &&
        posicion.z > -1.0f && posicion.z < 0.2f)
    {
        return true;
    }

    //xcc- colision con banca horizontal 3
    if (posicion.x > -17.3f && posicion.x < -12.6f &&
        posicion.z > -4.9f && posicion.z < -3.8f)
    {
        return true;
    }


    return false;
}

//xcc- verifica los limites del exterior del Hub
bool ColisionExteriorHub(glm::vec3 posicion)
{
    float linea;

    //xcc- limite derecho
    if (posicion.z > 21.0f && posicion.z < 40.6f)
    {
        linea = -0.0171f * posicion.z + 46.013f;
        if (posicion.x > linea - 1.0f && posicion.x < linea + 1.0f)
            return true;
    }

    //xcc- limite superior derecho
    if (posicion.x > 24.27f && posicion.x < 45.57f)
    {
        linea = -0.0148f * posicion.x + 41.475f;
        if (posicion.z > linea - 1.0f && posicion.z < linea + 1.0f)
            return true;
    }

    //xcc- limite diagonal derecho
    if (posicion.x > 12.58f && posicion.x < 24.26f)
    {
        linea = -2.2274f * posicion.x + 95.734f;
        if (posicion.z > linea - 1.0f && posicion.z < linea + 1.0f)
            return true;
    }

    //xcc- limite superior
    if (posicion.x > -9.78f && posicion.x < 12.43f)
    {
        linea = 0.0138f * posicion.x + 68.184f;
        if (posicion.z > linea - 1.0f && posicion.z < linea + 1.0f)
            return true;
    }

    //xcc- limite diagonal izquierdo
    if (posicion.x > -21.57f && posicion.x < -10.53f)
    {
        linea = 2.4600f * posicion.x + 93.940f;
        if (posicion.z > linea - 1.0f && posicion.z < linea + 1.0f)
            return true;
    }

    //xcc- limite superior izquierdo
    if (posicion.x > -41.38f && posicion.x < -22.16f)
    {
        linea = 0.0068f * posicion.x + 40.814f;
        if (posicion.z > linea - 1.0f && posicion.z < linea + 1.0f)
            return true;
    }

    //xcc- limite izquierdo
    if (posicion.z > 21.02f && posicion.z < 38.86f)
    {
        linea = 0.1248f * posicion.z - 46.219f;
        if (posicion.x > linea - 1.0f && posicion.x < linea + 1.0f)
            return true;
    }

    //xcc- colision con la cerca superior
    if (posicion.x > -16.94f && posicion.x < 17.23f)
    {
        linea = 0.0630f * posicion.x + 69.144f;

        if (posicion.z > linea - 1.0f && posicion.z < linea + 1.0f)
            return true;
    }

    //xcc- colision con roca o arbol exterior 1
    if (posicion.x > -6.7f && posicion.x < -3.7f &&
        posicion.z > 50.1f && posicion.z < 53.1f) return true;

    //xcc- colision con roca o arbol exterior 2
    if (posicion.x > 8.2f && posicion.x < 11.2f &&
        posicion.z > 40.3f && posicion.z < 43.3f) return true;

    //xcc- colision con roca o arbol exterior 3
    if (posicion.x > 11.7f && posicion.x < 14.7f &&
        posicion.z > 37.5f && posicion.z < 40.5f) return true;

    //xcc- colision con roca o arbol exterior 4
    if (posicion.x > 13.1f && posicion.x < 16.1f &&
        posicion.z > 40.3f && posicion.z < 43.3f) return true;

    //xcc- colision con roca o arbol exterior 5
    if (posicion.x > 18.0f && posicion.x < 21.0f &&
        posicion.z > 39.6f && posicion.z < 42.6f) return true;

    //xcc- colision con roca o arbol exterior 6
    if (posicion.x > 22.7f && posicion.x < 25.7f &&
        posicion.z > 35.9f && posicion.z < 38.9f) return true;

    //xcc- colision con roca o arbol exterior 7
    if (posicion.x > 28.1f && posicion.x < 31.1f &&
        posicion.z > 33.3f && posicion.z < 36.3f) return true;

    //xcc- colision con roca o arbol exterior 8
    if (posicion.x > 32.4f && posicion.x < 35.4f &&
        posicion.z > 28.7f && posicion.z < 31.7f) return true;

    //xcc- colision con roca o arbol exterior 9
    if (posicion.x > 17.9f && posicion.x < 20.9f &&
        posicion.z > 38.8f && posicion.z < 41.8f) return true;

    //xcc- colision con roca o arbol exterior 10
    if (posicion.x > 6.9f && posicion.x < 9.9f &&
        posicion.z > 39.3f && posicion.z < 42.3f) return true;

    //xcc- colision con roca o arbol exterior 11
    if (posicion.x > 5.3f && posicion.x < 8.3f &&
        posicion.z > 36.5f && posicion.z < 39.5f) return true;

    //xcc- colision con roca o arbol exterior 12
    if (posicion.x > -8.2f && posicion.x < -5.2f &&
        posicion.z > 39.5f && posicion.z < 42.5f) return true;

    //xcc- colision con roca o arbol exterior 13
    if (posicion.x > -13.2f && posicion.x < -10.2f &&
        posicion.z > 39.0f && posicion.z < 42.0f) return true;

    //xcc- colision con roca o arbol exterior 14
    if (posicion.x > -14.4f && posicion.x < -11.4f &&
        posicion.z > 32.7f && posicion.z < 35.7f) return true;

    //xcc- colision con roca o arbol exterior 15
    if (posicion.x > -22.7f && posicion.x < -19.7f &&
        posicion.z > 32.9f && posicion.z < 35.9f) return true;

    //xcc- colision con roca o arbol exterior 16
    if (posicion.x > -31.6f && posicion.x < -28.6f &&
        posicion.z > 29.7f && posicion.z < 32.7f) return true;

    return false;
}

//xcc- verifica colisiones dentro de la sala de trofeos
bool ColisionMueblesTrofeos(glm::vec3 posicion)
{

    //xcc- colision con banca central
    if (posicion.x > -0.7f && posicion.x < 0.9f &&
        posicion.z > 3.0f && posicion.z < 3.9f)
    {
        return true;
    }

    //xcc- colision con banca derecha
    if (posicion.x > 3.6f && posicion.x < 5.4f &&
        posicion.z > 2.2f && posicion.z < 3.1f)
    {
        return true;
    }

    //xcc- colision con banca izquierda
    if (posicion.x > -5.7f && posicion.x < -3.8f &&
        posicion.z > 2.4f && posicion.z < 3.5f)
    {
        return true;
    }

    //xcc- colision con el podio
    if (posicion.x > -2.65f && posicion.x < 2.77f &&
        posicion.z > 0.37f && posicion.z < 1.39f)
    {
        return true;
    }

    //xcc- colision con las vitrinas
    if (posicion.x > -5.59f && posicion.x < 5.70f &&
        posicion.z > -3.60f && posicion.z < -1.95f)
    {
        return true;
    }


    //xcc- colision con planta izquierda
    if (posicion.x > -4.1f && posicion.x < -3.2f &&
        posicion.z > -0.55f && posicion.z < 0.35f)
    {
        return true;
    }

    //xcc- colision con planta derecha
    if (posicion.x > 3.3f && posicion.x < 4.2f &&
        posicion.z > -0.65f && posicion.z < 0.25f)
    {
        return true;
    }

    //xcc- colision con repisa de la parte nueva
    if (posicion.x > 3.75f && posicion.x < 5.62f &&
        posicion.z > 11.20f && posicion.z < 11.68f)
    {
        return true;
    }

    //xcc- colision con silla o mesa nueva 1
    if (posicion.x > -5.62f && posicion.x < -4.57f &&
        posicion.z > 9.61f && posicion.z < 10.61f) return true;

    //xcc- colision con silla o mesa nueva 2
    if (posicion.x > -4.70f && posicion.x < -3.35f &&
        posicion.z > 8.82f && posicion.z < 9.39f) return true;

    //xcc- colision con silla o mesa nueva 3
    if (posicion.x > -3.40f && posicion.x < -2.09f &&
        posicion.z > 9.85f && posicion.z < 10.77f) return true;

    //xcc- colision con silla o mesa nueva 4
    if (posicion.x > 4.97f && posicion.x < 5.70f &&
        posicion.z > 4.86f && posicion.z < 6.11f) return true;

    //xcc- colision con planta nueva izquierda
    if (posicion.x > -5.7f && posicion.x < -4.8f &&
        posicion.z > 10.7f && posicion.z < 11.65f)
    {
        return true;
    }

    //xcc- colision con planta nueva derecha
    if (posicion.x > 2.6f && posicion.x < 3.5f &&
        posicion.z > 10.8f && posicion.z < 11.65f)
    {
        return true;
    }

    //xcc- colision con mesa nueva derecha
    if (posicion.x > 5.05f && posicion.x < 5.70f &&
        posicion.z > 4.85f && posicion.z < 6.21f)
    {
        return true;
    }

    return false;
}

void DoMovement();

// ---- Lógica del juego ----
void  ActualizarCarrera(Model& terreno);
void  ActualizarAmbiente(float dt);
float ObtenerAlturaTerreno(Model& terreno, float x, float z, float yActual);
void  ConstruirLimitesPista(Model& terreno);
void  ConstruirColisionadores(Model& modelo);
bool  ResolverColisiones(const glm::vec2& dirPista);
float Suavizar(float actual, float objetivo, float tasa, float dt);
//xcc- verifica si la camara esta cerca del panel
bool CercaPanelConfiguracion();
//xcc- verifica si la camara esta cerca de la salida
bool CercaSalidaPista();
//xcc- verifica colisiones con los muebles del Hub
bool ColisionMueblesHub(glm::vec3 posicion);
//xcc- colisiones de los muebles de trofeos
bool ColisionMueblesTrofeos(glm::vec3 posicion);
//xcc- verifica si esta cerca de la entrada a trofeos
bool CercaEntradaTrofeos();


/**
 * @brief Carga una textura 2D con SOIL2 (el mismo cargador que usa Model.h),
 *        detectando automáticamente si es RGB o RGBA.
 * @param rutaArchivo Ruta de la imagen.
 * @return Identificador de la textura.
 */
GLuint CargarTextura(const char* rutaArchivo)
{
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Configuración de wrapping y filtros
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int textureWidth, textureHeight, nrChannels;
    unsigned char* image = SOIL_load_image(rutaArchivo, &textureWidth, &textureHeight, &nrChannels, SOIL_LOAD_AUTO);

    if (image)
    {
        GLenum format = GL_RGB;
        if (nrChannels == 4)
            format = GL_RGBA;
        else if (nrChannels == 1)
            format = GL_RED;

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);   // Anchos no múltiplo de 4
        glTexImage2D(GL_TEXTURE_2D, 0, format, textureWidth, textureHeight, 0, format, GL_UNSIGNED_BYTE, image);
        glGenerateMipmap(GL_TEXTURE_2D);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
        SOIL_free_image_data(image);
    }
    else
    {
        std::cout << "Error: No se pudo cargar la textura en la ruta: " << rutaArchivo << std::endl;
    }

    return textureID;
}


// ---- Ventisca ----
void InicializarCopos();
void ActualizarVentisca(float dt, float velNorm);

// ---- Recursos gráficos ----
GLuint CargarSkybox(const char* caras[6]);

// ---- HUD ----
float AlphaMensaje(double inicio, float sostener, float desvanecer);
void  DibujarHUD(Shader& hudShader);

//xcc- HUD para las opciones del Hub
void DibujarHUDHub(Shader& hudShader);

//xcc- aviso para regresar al Hub
void DibujarAvisoVolverHub(Shader& hudShader);

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





    // ------------------------------------------------------------------------
    //  6.3 Carga de shaders
    // ------------------------------------------------------------------------
    Shader lightingShader("Shader/lighting.vs", "Shader/lighting.frag");  // Escena con Blinn-Phong y niebla
    Shader colorShader("Shader/core.vs", "Shader/core.frag");             // Color plano (indicador, postes, colisionadores)
    Shader skyShader("Shader/skybox.vs", "Shader/skybox.frag");           // Cielo
    Shader hudShader("Shader/hud.vs", "Shader/hud.frag");                 // Interfaz 2D

    // ------------------------------------------------------------------------
    //  6.4 Configuración de geometría (VAO/VBO)
    // ------------------------------------------------------------------------

    // 6.4.1 Cubo: indicador del esquiador, postes de límite y colisionadores
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
    //xcc- carga los elementos exteriores del Hub
    Model HubExterior((char*)"Models/Hub/Exterior.obj");
    //xcc- carga la sala de trofeos
    Model SalaTrofeos((char*)"Models/Trofeos/Sala_Trofeos.obj");
    //xcc- carga la parte nueva de la sala de trofeos
    Model SalaTrofeosNueva((char*)"Models/Trofeos/Sala_Trofeos_parte_nueva.obj");

    // Cargar las opciones de trajes para el sistema de personalización
    GLuint texTrajeTurquesa = CargarTextura("images/traje_turquesa.png");
    GLuint texTrajeRojo = CargarTextura("images/traje_rojo.png");
    // Llenar el vector global
    opcionesTrajes.push_back(texTrajeTurquesa);
    opcionesTrajes.push_back(texTrajeRojo);

    // 3ra skin para que Jugador, Rival 1 y Rival 2 siempre tengan trajes distintos
    GLuint texTrajeDorado = CrearTexturaTrajeProcedural();
    opcionesTrajes.push_back(texTrajeDorado);

    Model PechoModel((char*)"Models/Esquiador/pecho.obj");
    Model CaderasModel((char*)"Models/Esquiador/caderas.obj");
    Model CinturaModel((char*)"Models/Esquiador/cintura.obj");
    Model CuelloModel((char*)"Models/Esquiador/cuello.obj");
    Model CabezaModel((char*)"Models/Esquiador/cabeza.obj");

    Model HombroDerModel((char*)"Models/Esquiador/hombro_der.obj");
    Model HombroIzqModel((char*)"Models/Esquiador/hombro_izq.obj");
    Model BrazoDerModel((char*)"Models/Esquiador/brazo_der.obj");
    Model BrazoIzqModel((char*)"Models/Esquiador/brazo_izq.obj");
    Model AntebrazoDerModel((char*)"Models/Esquiador/antebrazo_der.obj");
    Model AntebrazoIzqModel((char*)"Models/Esquiador/antebrazo_izq.obj");
    Model ManoDerModel((char*)"Models/Esquiador/mano_der.obj");
    Model ManoIzqModel((char*)"Models/Esquiador/mano_izq.obj");

    Model PiernaSupDerModel((char*)"Models/Esquiador/pierna_sup_der.obj");
    Model PiernaSupIzqModel((char*)"Models/Esquiador/pierna_sup_izq.obj");
    Model PiernaInfDerModel((char*)"Models/Esquiador/pierna_inf_der.obj");
    Model PiernaInfIzqModel((char*)"Models/Esquiador/pierna_inf_izq.obj");
    Model PieDerModel((char*)"Models/Esquiador/pie_der.obj");
    Model PieIzqModel((char*)"Models/Esquiador/pie_izq.obj");

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

    // Colisionadores de árboles y rocas (detección automática desde la malla)
    ConstruirColisionadores(PistaCompleta);

    // Copos de nieve para la ventisca
    InicializarCopos();

    // Inicializar posiciones, skins y rutas aleatorias de los 2 contrincantes
    InicializarContrincantes(PistaCompleta);

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

        // Reinicio de contrincantes solicitado desde el teclado (E, R, T)
        if (reiniciarContrincantes)
        {
            InicializarContrincantes(PistaCompleta);
            // Reasienta al jugador sobre la malla del terreno en la salida
            skierPos.y = ObtenerAlturaTerreno(PistaCompleta, skierPos.x, skierPos.z, posSalida.y) + offsetPiesEsquiador;
            reiniciarContrincantes = false;
        }

        // ---- 6.7.2 Actualización lógica ----
        DoMovement();

        //xcc- actualiza la logica de carrera solo en la pista
        if (escenaActual == ESCENA_PISTA)
        {
            ActualizarAmbiente(deltaTime);
            ActualizarCarrera(PistaCompleta);

            if (reconstruirLimites)
            {
                ConstruirLimitesPista(PistaCompleta);
                reconstruirLimites = false;
            }
        }

        // Velocidad normalizada (0..1): modula FOV, vibración de cámara y viento
        float velNorm = glm::clamp(velActual / ambienteActual.velocidadDescenso, 0.0f, 1.0f);
        ActualizarVentisca(deltaTime, velNorm);

        // ---- 6.7.3 Limpieza de buffers ----
        glClearColor(ambienteActual.colorNiebla.r, ambienteActual.colorNiebla.g, ambienteActual.colorNiebla.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ---- 6.7.4 Matrices de vista y proyección ----
        glm::mat4 view;
        glm::vec3 currentViewPos = camera.GetPosition();
        float fov = glm::radians(FOV_BASE_GRADOS);

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

        fov = glm::clamp(fov, glm::radians(20.0f), glm::radians(FOV_MAXIMO_GRADOS));
        glm::mat4 projection = glm::perspective(fov, (GLfloat)SCREEN_WIDTH / (GLfloat)SCREEN_HEIGHT, 0.1f, 2000.0f);

        // ---- 6.7.5 Uniformes de iluminación y niebla ----
        lightingShader.Use();

        glUniform3f(glGetUniformLocation(lightingShader.Program, "viewPos"), currentViewPos.x, currentViewPos.y, currentViewPos.z);
        glUniform3f(glGetUniformLocation(lightingShader.Program, "material.ambient"), 0.5f, 0.5f, 0.5f);
        glUniform3f(glGetUniformLocation(lightingShader.Program, "material.diffuse"), 0.8f, 0.8f, 0.8f);
        glUniform3f(glGetUniformLocation(lightingShader.Program, "material.specular"), 0.6f, 0.6f, 0.6f);
        glUniform1f(glGetUniformLocation(lightingShader.Program, "material.shininess"), 32.0f);

        // Esquema base: sol de atardecer (pista y exterior del Hub)
        EsquemaIluminacion esquema;
        esquema.dirDireccion = direccionSol;
        esquema.dirAmbiente = ambienteActual.colorAmbiente * (0.6f + 0.4f * ambienteActual.intensidadSol);
        esquema.dirDifusa = ambienteActual.colorSol * ambienteActual.intensidadSol;
        esquema.dirEspecular = ambienteActual.colorSol * 0.5f;
        esquema.colorNiebla = ambienteActual.colorNiebla;
        esquema.densidadNiebla = glm::clamp(ambienteActual.densidadNiebla + ajusteNieblaManual
            + intensidadVentisca * NIEBLA_EXTRA_VENTISCA, 0.0f, 0.02f);

        // Esquemas interiores
        if (escenaActual == ESCENA_HUB)
        {
            glm::vec3 p = camera.GetPosition();
            //xcc- mantiene la iluminacion interior hasta las paredes del Hub
            bool dentroHub = (p.z < 19.0f && std::abs(p.x) <= 21.0f); // Mismos límites que DoMovement
            factorInteriorHub = Suavizar(factorInteriorHub, dentroHub ? 1.0f : 0.0f, 4.0f, deltaTime);
            esquema = EsquemaHub(currentFrame, nivelActual, factorInteriorHub, esquema);
        }
        else if (escenaActual == ESCENA_TROFEOS)
        {
            esquema = EsquemaTrofeos(currentFrame, posicionesTrofeos);
        }

        EnviarEsquema(lightingShader, esquema);
        float densidadNiebla = esquema.densidadNiebla;   // La usa el skybox más abajo

        GLint modelLoc = glGetUniformLocation(lightingShader.Program, "model");
        GLint viewLoc = glGetUniformLocation(lightingShader.Program, "view");
        GLint projLoc = glGetUniformLocation(lightingShader.Program, "projection");
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

            //xcc- dibuja el exterior del Hub
            HubExterior.Draw(lightingShader);
        }
        else if (escenaActual == ESCENA_PISTA)//xcc
        {
            PistaCompleta.Draw(lightingShader);
        }
        else if (escenaActual == ESCENA_TROFEOS)
        {
            //xcc- aumenta la luz ambiental dentro de trofeos
            glUniform3f(
                glGetUniformLocation(lightingShader.Program, "dirLight.ambient"),
                0.42f, 0.38f, 0.32f
            );

            //xcc- ilumina un poco mas los materiales
            glUniform3f(
                glGetUniformLocation(lightingShader.Program, "dirLight.diffuse"),
                0.75f, 0.70f, 0.65f
            );


            //xcc- usa 2 luces del podio o todas las luces de la sala
            if (lucesTrofeosCompletas)
                glUniform1i(glGetUniformLocation(lightingShader.Program, "numPointLights"), 6);
            else
                glUniform1i(glGetUniformLocation(lightingShader.Program, "numPointLights"), 2);

            //xcc- luz izquierda
            glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[0].position"), -3.0f, 3.5f, 0.0f);
            glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[0].ambient"), 0.15f, 0.12f, 0.08f);
            glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[0].diffuse"), 0.85f, 0.65f, 0.45f);
            glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[0].specular"), 1.0f, 0.85f, 0.65f);
            glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[0].constant"), 1.0f);
            glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[0].linear"), 0.09f);
            glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[0].quadratic"), 0.032f);

            //xcc- luz derecha
            glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[1].position"), 3.0f, 3.5f, 0.0f);
            glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[1].ambient"), 0.15f, 0.12f, 0.08f);
            glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[1].diffuse"), 0.85f, 0.65f, 0.45f);
            glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[1].specular"), 1.0f, 0.85f, 0.65f);
            glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[1].constant"), 1.0f);
            glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[1].linear"), 0.09f);
            glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[1].quadratic"), 0.032f);

            //xcc- luces generales que se encienden con L
            if (lucesTrofeosCompletas)
            {
                //xcc- luces de la parte media
                glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[2].position"), -3.0f, 3.5f, 6.0f);
                glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[2].ambient"), 0.15f, 0.12f, 0.08f);
                glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[2].diffuse"), 0.85f, 0.65f, 0.45f);
                glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[2].specular"), 1.0f, 0.85f, 0.65f);
                glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[2].constant"), 1.0f);
                glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[2].linear"), 0.09f);
                glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[2].quadratic"), 0.032f);

                glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[3].position"), 3.0f, 3.5f, 6.0f);
                glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[3].ambient"), 0.15f, 0.12f, 0.08f);
                glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[3].diffuse"), 0.85f, 0.65f, 0.45f);
                glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[3].specular"), 1.0f, 0.85f, 0.65f);
                glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[3].constant"), 1.0f);
                glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[3].linear"), 0.09f);
                glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[3].quadratic"), 0.032f);

                //xcc- luces de la parte nueva
                glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[4].position"), -3.0f, 3.5f, 10.0f);
                glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[4].ambient"), 0.15f, 0.12f, 0.08f);
                glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[4].diffuse"), 0.85f, 0.65f, 0.45f);
                glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[4].specular"), 1.0f, 0.85f, 0.65f);
                glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[4].constant"), 1.0f);
                glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[4].linear"), 0.09f);
                glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[4].quadratic"), 0.032f);

                glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[5].position"), 3.0f, 3.5f, 10.0f);
                glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[5].ambient"), 0.15f, 0.12f, 0.08f);
                glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[5].diffuse"), 0.85f, 0.65f, 0.45f);
                glUniform3f(glGetUniformLocation(lightingShader.Program, "pointLights[5].specular"), 1.0f, 0.85f, 0.65f);
                glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[5].constant"), 1.0f);
                glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[5].linear"), 0.09f);
                glUniform1f(glGetUniformLocation(lightingShader.Program, "pointLights[5].quadratic"), 0.032f);
            }

            //xcc- activa transparencia en la sala de trofeos
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glUniform1i(glGetUniformLocation(lightingShader.Program, "transparency"), 1);

            //xcc- dibuja la sala de trofeos
            SalaTrofeos.Draw(lightingShader);

            //xcc- dibuja la parte nueva de la sala
            SalaTrofeosNueva.Draw(lightingShader);

            //xcc- desactiva transparencia para los demas objetos
            glUniform1i(glGetUniformLocation(lightingShader.Program, "transparency"), 0);
            glDisable(GL_BLEND);
        }


// ---- 6.7.7 Elementos de color plano: postes, colisionadores y esquiador ----

//xcc- elementos visibles solo en la pista
        if (escenaActual == ESCENA_PISTA)
        {
            colorShader.Use();

            glUniformMatrix4fv(glGetUniformLocation(colorShader.Program, "view"), 1, GL_FALSE, glm::value_ptr(view));

            glUniformMatrix4fv(glGetUniformLocation(colorShader.Program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

            GLint colorModelLoc = glGetUniformLocation(colorShader.Program, "model");

            glBindVertexArray(VAO);

            // Postes de límite
            for (size_t i = 0; i < postesLimite.size(); i++)
            {
                glm::mat4 modelPoste = glm::translate(glm::mat4(1.0f), postesLimite[i] + glm::vec3(0.0f, ALTURA_POSTE * 0.5f - DESFASE_POSTE, 0.0f));

                modelPoste = glm::scale(modelPoste, glm::vec3(0.6f, ALTURA_POSTE, 0.6f));

                glUniformMatrix4fv(colorModelLoc, 1, GL_FALSE, glm::value_ptr(modelPoste));

                glm::vec3 colorPoste = ((i / 2) % 2 == 0) ? glm::vec3(0.85f, 0.10f, 0.10f) : glm::vec3(0.95f, 0.95f, 0.95f);

                glUniform3fv(colorShader.uniformColor, 1, glm::value_ptr(colorPoste));

                glDrawArrays(GL_TRIANGLES, 0, 36);
            }

            // Colisionadores de la pista
            if (mostrarColisionadores)
            {
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

                for (size_t i = 0; i < obstaculos.size(); i++)
                {
                    const Obstaculo& o = obstaculos[i];

                    float alto = o.yMax - o.yMin;

                    glm::mat4 modelCol = glm::translate(glm::mat4(1.0f), glm::vec3(o.centro.x, o.yMin + alto * 0.5f, o.centro.z));

                    modelCol = glm::scale(
                        modelCol,
                        glm::vec3(
                            o.radio * 2.0f,
                            alto,
                            o.radio * 2.0f
                        )
                    );

                    glUniformMatrix4fv(
                        colorModelLoc,
                        1,
                        GL_FALSE,
                        glm::value_ptr(modelCol)
                    );

                    glm::vec3 colorCol =
                        o.esArbol
                        ? glm::vec3(0.1f, 1.0f, 0.2f)
                        : glm::vec3(1.0f, 0.2f, 1.0f);

                    glUniform3fv(
                        colorShader.uniformColor,
                        1,
                        glm::value_ptr(colorCol)
                    );

                    glDrawArrays(GL_TRIANGLES, 0, 36);
                }

                // Dibujar las 2 paredes invisibles a los lados del arco de META (en color cian)
                glm::vec2 vecPistaXZ(posMeta.x - posSalida.x, posMeta.z - posSalida.z);
                float distTotalDbg = glm::length(vecPistaXZ);
                if (distTotalDbg > 0.001f)
                {
                    glm::vec2 dirP = glm::normalize(vecPistaXZ);
                    glm::vec2 dirD(dirP.y, -dirP.x);
                    float anguloMeta = glm::degrees(std::atan2(dirP.x, dirP.y));
                    float anchoMuroLat = ANCHO_MEDIO_PISTA - ANCHO_PUERTA_META;
                    float centroLat = ANCHO_PUERTA_META + anchoMuroLat * 0.5f;

                    for (int lado = -1; lado <= 1; lado += 2)
                    {
                        // El muro ocupa el tramo [distTotal - GROSOR, distTotal] sobre el eje de la pista
                        glm::vec3 posMuro(
                            posMeta.x + dirD.x * (centroLat * lado) - dirP.x * (GROSOR_MURO_META * 0.5f),
                            posMeta.y + 4.0f,
                            posMeta.z + dirD.y * (centroLat * lado) - dirP.y * (GROSOR_MURO_META * 0.5f)
                        );
                        glm::mat4 modelMuro = glm::translate(glm::mat4(1.0f), posMuro);
                        modelMuro = glm::rotate(modelMuro, glm::radians(anguloMeta), glm::vec3(0.0f, 1.0f, 0.0f));
                        modelMuro = glm::scale(modelMuro, glm::vec3(anchoMuroLat, 10.0f, GROSOR_MURO_META));
                        glUniformMatrix4fv(colorModelLoc, 1, GL_FALSE, glm::value_ptr(modelMuro));
                        glm::vec3 colorCian(0.0f, 0.9f, 1.0f);
                        glUniform3fv(colorShader.uniformColor, 1, glm::value_ptr(colorCian));
                        glDrawArrays(GL_TRIANGLES, 0, 36);
                    }
                }

                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            }

            glBindVertexArray(0);

            // ---- Esquiadores articulados (jugador y contrincantes) ----
            lightingShader.Use();
            glUniform1i(glGetUniformLocation(lightingShader.Program, "transparency"), 0);
            glUniform2f(glGetUniformLocation(lightingShader.Program, "uvOffset"), 0.0f, 0.0f);

            // Lambda reutilizable para dibujar cualquier esquiador articulado (Jugador o Contrincantes)
            auto DibujarEsquiador = [&](const glm::vec3& posEsq, float rotYEsq, float incLat, float incTorso, float oscPiernas, GLuint texTraje, const glm::vec3& tinteDifuso)
                {
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, texTraje);
                    glUniform1i(glGetUniformLocation(lightingShader.Program, "texture_diffuse"), 0);
                    glUniform1i(glGetUniformLocation(lightingShader.Program, "texture_diffuse1"), 0);
                    glUniform3f(glGetUniformLocation(lightingShader.Program, "material.diffuse"), tinteDifuso.r, tinteDifuso.g, tinteDifuso.b);

                    ForzarTexturaEnModelo(CaderasModel, texTraje);
                    ForzarTexturaEnModelo(PiernaSupIzqModel, texTraje);
                    ForzarTexturaEnModelo(PiernaInfIzqModel, texTraje);
                    ForzarTexturaEnModelo(PieIzqModel, texTraje);
                    ForzarTexturaEnModelo(PiernaSupDerModel, texTraje);
                    ForzarTexturaEnModelo(PiernaInfDerModel, texTraje);
                    ForzarTexturaEnModelo(PieDerModel, texTraje);
                    ForzarTexturaEnModelo(CinturaModel, texTraje);
                    ForzarTexturaEnModelo(PechoModel, texTraje);
                    ForzarTexturaEnModelo(CuelloModel, texTraje);
                    ForzarTexturaEnModelo(CabezaModel, texTraje);
                    ForzarTexturaEnModelo(HombroIzqModel, texTraje);
                    ForzarTexturaEnModelo(BrazoIzqModel, texTraje);
                    ForzarTexturaEnModelo(AntebrazoIzqModel, texTraje);
                    ForzarTexturaEnModelo(ManoIzqModel, texTraje);
                    ForzarTexturaEnModelo(HombroDerModel, texTraje);
                    ForzarTexturaEnModelo(BrazoDerModel, texTraje);
                    ForzarTexturaEnModelo(AntebrazoDerModel, texTraje);
                    ForzarTexturaEnModelo(ManoDerModel, texTraje);

                    glm::mat4 modelSkier = glm::mat4(1.0f);
                    modelSkier = glm::translate(modelSkier, posEsq);
                    modelSkier = glm::rotate(modelSkier, glm::radians(rotYEsq), glm::vec3(0.0f, 1.0f, 0.0f));

                    // --- TREN INFERIOR ---
                    glm::mat4 modelCaderas = glm::translate(modelSkier, glm::vec3(0.0f, -0.9f, 0.0f));
                    modelCaderas = glm::rotate(modelCaderas, glm::radians(incLat), glm::vec3(0.0f, 0.0f, 1.0f));
                    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelCaderas));
                    CaderasModel.Draw(lightingShader);

                    glm::mat4 modelPiernaSupIzq = glm::translate(modelCaderas, glm::vec3(-0.35f, -0.04f, 0.0f));
                    modelPiernaSupIzq = glm::rotate(modelPiernaSupIzq, glm::radians(oscPiernas), glm::vec3(1.0f, 0.0f, 0.0f));
                    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelPiernaSupIzq));
                    PiernaSupIzqModel.Draw(lightingShader);

                    glm::mat4 modelPiernaInfIzq = glm::translate(modelPiernaSupIzq, glm::vec3(0.0f, -0.04f, 0.0f));
                    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelPiernaInfIzq));
                    PiernaInfIzqModel.Draw(lightingShader);

                    glm::mat4 modelPieIzq = glm::translate(modelPiernaInfIzq, glm::vec3(0.0f, -0.04f, 0.0f));
                    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelPieIzq));
                    PieIzqModel.Draw(lightingShader);

                    glm::mat4 modelPiernaSupDer = glm::translate(modelCaderas, glm::vec3(0.35f, -0.04f, 0.0f));
                    modelPiernaSupDer = glm::rotate(modelPiernaSupDer, glm::radians(-oscPiernas), glm::vec3(1.0f, 0.0f, 0.0f));
                    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelPiernaSupDer));
                    PiernaSupDerModel.Draw(lightingShader);

                    glm::mat4 modelPiernaInfDer = glm::translate(modelPiernaSupDer, glm::vec3(0.0f, -0.04f, 0.0f));
                    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelPiernaInfDer));
                    PiernaInfDerModel.Draw(lightingShader);

                    glm::mat4 modelPieDer = glm::translate(modelPiernaInfDer, glm::vec3(0.0f, -0.04f, 0.0f));
                    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelPieDer));
                    PieDerModel.Draw(lightingShader);

                    // --- TREN SUPERIOR ---
                    glm::mat4 modelCintura = modelCaderas;
                    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelCintura));
                    CinturaModel.Draw(lightingShader);

                    glm::mat4 modelPecho = glm::translate(modelCintura, glm::vec3(0.0f, 0.04f, 0.0f));
                    modelPecho = glm::rotate(modelPecho, glm::radians(incTorso), glm::vec3(1.0f, 0.0f, 0.0f));
                    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelPecho));
                    PechoModel.Draw(lightingShader);

                    glm::mat4 modelCuello = glm::translate(modelPecho, glm::vec3(0.0f, 0.04f, 0.0f));
                    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelCuello));
                    CuelloModel.Draw(lightingShader);

                    glm::mat4 modelCabeza = glm::translate(modelCuello, glm::vec3(0.0f, 0.1f, 0.0f));
                    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelCabeza));
                    CabezaModel.Draw(lightingShader);

                    // --- BRAZOS Y MANOS ---
                    glm::mat4 modelHombroIzq = glm::translate(modelPecho, glm::vec3(-0.08f, -0.1f, 0.0f));
                    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelHombroIzq));
                    HombroIzqModel.Draw(lightingShader);

                    glm::mat4 modelBrazoIzq = modelHombroIzq;
                    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelBrazoIzq));
                    BrazoIzqModel.Draw(lightingShader);

                    glm::mat4 modelAntebrazoIzq = glm::translate(modelBrazoIzq, glm::vec3(0.0f, -0.009f, 0.0f));
                    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelAntebrazoIzq));
                    AntebrazoIzqModel.Draw(lightingShader);

                    glm::mat4 modelManoIzq = glm::translate(modelAntebrazoIzq, glm::vec3(0.0f, 0.01f, 0.0f));
                    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelManoIzq));
                    ManoIzqModel.Draw(lightingShader);

                    glm::mat4 modelHombroDer = glm::translate(modelPecho, glm::vec3(0.08f, -0.1f, 0.0f));
                    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelHombroDer));
                    HombroDerModel.Draw(lightingShader);

                    glm::mat4 modelBrazoDer = modelHombroDer;
                    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelBrazoDer));
                    BrazoDerModel.Draw(lightingShader);

                    glm::mat4 modelAntebrazoDer = glm::translate(modelBrazoDer, glm::vec3(0.0f, -0.009f, 0.0f));
                    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelAntebrazoDer));
                    AntebrazoDerModel.Draw(lightingShader);

                    glm::mat4 modelManoDer = glm::translate(modelAntebrazoDer, glm::vec3(0.0f, 0.01f, 0.0f));
                    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelManoDer));
                    ManoDerModel.Draw(lightingShader);
                };

            // 1. Dibujar al Jugador
            AsignarSkinsContrincantes();
            DibujarEsquiador(skierPos, skierRotY, inclinacionLateral, inclinacionTorso,
                oscilacionPiernas, opcionesTrajes[indiceTrajeActual], glm::vec3(0.85f, 0.85f, 0.85f));

            // 2. Dibujar a los 2 Contrincantes con sus skins distintas
            glm::vec3 tintesRivales[2] = { glm::vec3(0.95f, 0.80f, 0.80f), glm::vec3(0.80f, 0.90f, 1.00f) };
            for (int r = 0; r < NUM_CONTRINCANTES; r++)
            {
                const Contrincante& c = contrincantes[r];
                DibujarEsquiador(c.pos, c.rotY, c.inclinacionLat, c.inclinacionTorso,
                    c.oscilacionPiernas, opcionesTrajes[c.indiceTraje], tintesRivales[r]);
            }
            glUniform3f(glGetUniformLocation(lightingShader.Program, "material.diffuse"), 0.8f, 0.8f, 0.8f);
        }   // Fin del bloque exclusivo de la pista


        //xcc- terreno amplio alrededor del Hub
        if (escenaActual == ESCENA_HUB || escenaActual == ESCENA_TROFEOS)
        {
            colorShader.Use();
            glUniformMatrix4fv(glGetUniformLocation(colorShader.Program, "view"), 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(glGetUniformLocation(colorShader.Program, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
            GLint colorModelLoc = glGetUniformLocation(colorShader.Program, "model");
            glBindVertexArray(VAO);
            glm::mat4 sueloHub = glm::mat4(1.0f);

            //xcc- nieve debajo del Hub
            sueloHub = glm::translate(sueloHub, glm::vec3(0.0f, -0.11f, 0.0f));

            sueloHub = glm::scale(sueloHub, glm::vec3(500.0f, 0.10f, 500.0f));

            glUniformMatrix4fv(colorModelLoc, 1, GL_FALSE, glm::value_ptr(sueloHub));

            glm::vec3 colorNieve(0.90f, 0.94f, 1.0f);

            glUniform3fv(colorShader.uniformColor, 1, glm::value_ptr(colorNieve));

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

        // ---- 6.7.9 Interfaz 2D (ventisca + HUD) ----

        //xcc- muestra el HUD correspondiente a cada escena
        if (escenaActual == ESCENA_HUB)
        {
            DibujarHUDHub(hudShader);
        }
        else if (escenaActual == ESCENA_PISTA)
        {
            DibujarHUD(hudShader);

            //xcc- muestra el regreso al Hub antes de iniciar o al terminar
            if (estadoCarrera == 0 || estadoCarrera == 2)
                DibujarAvisoVolverHub(hudShader);
        }
        else if (escenaActual == ESCENA_TROFEOS)
        {
            //xcc- muestra como regresar al Hub
            DibujarAvisoVolverHub(hudShader);
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
//  7.3 Colisión con el terreno, límites de pista y obstáculos
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
 *        Se recorre la pista en pasos y la altura hallada en cada paso es la
 *        referencia del siguiente (mismo principio que el esquiador).
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
    glm::vec2 origen(posSalida.x, posSalida.z);

    // Altura de referencia por lado: se actualiza en cada muestra
    float yRef[2] = { posSalida.y, posSalida.y };
    float proximoPoste = 0.0f;

    for (float d = 0.0f; d <= distTotalXZ; d += PASO_MUESTREO_ALTURA)
    {
        for (int lado = 0; lado < 2; lado++)
        {
            float signo = (lado == 0) ? -1.0f : 1.0f;
            glm::vec2 p = origen + dirPista * d + dirDerecha * (ANCHO_MEDIO_PISTA * signo);
            yRef[lado] = ObtenerAlturaTerreno(terreno, p.x, p.y, yRef[lado]);

            if (d >= proximoPoste)
                postesLimite.push_back(glm::vec3(p.x, yRef[lado], p.y));
        }
        if (d >= proximoPoste)
            proximoPoste += SEPARACION_POSTES;
    }
}

/**
 * @brief Busca la raíz de un conjunto (estructura unión-búsqueda con
 *        compresión de caminos).
 */
static int BuscarRaiz(std::vector<int>& padre, int a)
{
    while (padre[a] != a)
    {
        padre[a] = padre[padre[a]];
        a = padre[a];
    }
    return a;
}

/** @brief Une los conjuntos que contienen a los elementos a y b. */
static void UnirConjuntos(std::vector<int>& padre, int a, int b)
{
    int ra = BuscarRaiz(padre, a);
    int rb = BuscarRaiz(padre, b);
    if (ra != rb)
        padre[rb] = ra;
}

/**
 * @brief Detecta automáticamente árboles y rocas dentro del modelo de la
 *        pista y los convierte en colisionadores cilíndricos.
 *
 *        Procedimiento:
 *          1. Se sueldan los vértices duplicados por posición.
 *          2. Se agrupan los triángulos conectados (cada objeto aislado
 *             forma una componente).
 *          3. De cada componente se obtiene su caja envolvente. Las
 *             componentes de tamaño razonable (ni terreno, ni carteles)
 *             se consideran obstáculos.
 *          4. Si el objeto es más alto que ancho se trata como árbol (el
 *             colisionador es el tronco); en caso contrario, como roca.
 *
 *        Los umbrales se ajustan con las constantes de la Etapa 2 y se
 *        verifican visualmente con la tecla F3.
 * @param modelo Modelo que contiene la pista y su vegetación.
 */
void ConstruirColisionadores(Model& modelo)
{
    obstaculos.clear();
    int arboles = 0, rocas = 0;

    for (size_t m = 0; m < modelo.meshes.size(); m++)
    {
        const std::vector<Vertex>& verts = modelo.meshes[m].vertices;
        const std::vector<GLuint>& inds = modelo.meshes[m].indices;
        if (verts.empty())
            continue;

        // 1. Soldadura de vértices por posición (cuadrícula de 0.02 unidades)
        std::unordered_map<unsigned long long, int> soldadura;
        std::vector<int> idSoldado(verts.size());
        int cantidad = 0;

        for (size_t v = 0; v < verts.size(); v++)
        {
            long long ix = (long long)std::floor(verts[v].Position.x / 0.02f);
            long long iy = (long long)std::floor(verts[v].Position.y / 0.02f);
            long long iz = (long long)std::floor(verts[v].Position.z / 0.02f);
            unsigned long long clave = ((unsigned long long)(ix & 0x1FFFFFLL) << 42)
                | ((unsigned long long)(iy & 0x1FFFFFLL) << 21)
                | (unsigned long long)(iz & 0x1FFFFFLL);

            std::unordered_map<unsigned long long, int>::iterator it = soldadura.find(clave);
            if (it == soldadura.end())
            {
                soldadura[clave] = cantidad;
                idSoldado[v] = cantidad;
                cantidad++;
            }
            else
            {
                idSoldado[v] = it->second;
            }
        }

        // 2. Componentes conexas
        std::vector<int> padre(cantidad);
        for (int i = 0; i < cantidad; i++)
            padre[i] = i;

        for (size_t i = 0; i + 2 < inds.size(); i += 3)
        {
            UnirConjuntos(padre, idSoldado[inds[i]], idSoldado[inds[i + 1]]);
            UnirConjuntos(padre, idSoldado[inds[i + 1]], idSoldado[inds[i + 2]]);
        }

        // 3. Caja envolvente por componente
        std::vector<glm::vec3> minimo(cantidad, glm::vec3(1e9f));
        std::vector<glm::vec3> maximo(cantidad, glm::vec3(-1e9f));
        for (size_t v = 0; v < verts.size(); v++)
        {
            int r = BuscarRaiz(padre, idSoldado[v]);
            minimo[r] = glm::min(minimo[r], verts[v].Position);
            maximo[r] = glm::max(maximo[r], verts[v].Position);
        }

        // 4. Filtrado y clasificación
        for (int r = 0; r < cantidad; r++)
        {
            if (padre[r] != r)
                continue;   // Solo raíces

            glm::vec3 tam = maximo[r] - minimo[r];
            float ancho = glm::max(tam.x, tam.z);
            float alto = tam.y;

            if (ancho > MAX_ANCHO_OBSTACULO || ancho < MIN_ANCHO_OBSTACULO || alto < MIN_ALTO_OBSTACULO)
                continue;

            Obstaculo o;
            o.centro = glm::vec3((minimo[r].x + maximo[r].x) * 0.5f, 0.0f, (minimo[r].z + maximo[r].z) * 0.5f);
            o.yMin = minimo[r].y;
            o.yMax = maximo[r].y;
            o.esArbol = (alto > ancho * 1.2f);
            o.radio = glm::max(RADIO_MINIMO_OBSTACULO, ancho * (o.esArbol ? FACTOR_RADIO_ARBOL : FACTOR_RADIO_ROCA));
            obstaculos.push_back(o);

            if (o.esArbol) arboles++; else rocas++;
        }
    }

    std::cout << "[Colisiones] Obstaculos detectados: " << obstaculos.size()
        << " (" << arboles << " arboles, " << rocas << " rocas). F3 para visualizarlos." << std::endl;
}

/**
 * @brief Resuelve las colisiones del esquiador con árboles y rocas. Si el
 *        esquiador penetra un colisionador se le empuja hacia afuera
 *        (deslizamiento alrededor del obstáculo).
 *        Se ignoran los obstáculos cercanos a salida y meta (carteles).
 * @param dirPista Dirección de la pista en XZ (para el caso degenerado).
 * @return true si hubo contacto con algún obstáculo en este cuadro.
 */
bool ResolverColisiones(const glm::vec2& dirPista)
{
    bool hubo = false;
    glm::vec2 salidaXZ(posSalida.x, posSalida.z);
    glm::vec2 metaXZ(posMeta.x, posMeta.z);
    float yPies = skierPos.y - offsetPiesEsquiador;
    float yCabeza = skierPos.y + offsetPiesEsquiador;

    for (size_t i = 0; i < obstaculos.size(); i++)
    {
        const Obstaculo& o = obstaculos[i];
        glm::vec2 c(o.centro.x, o.centro.z);

        // Zonas de salida y meta libres
        if (glm::distance(c, salidaXZ) < ZONA_LIBRE_SALIDA_META || glm::distance(c, metaXZ) < ZONA_LIBRE_SALIDA_META)
            continue;

        // Rango vertical
        if (yCabeza < o.yMin - 1.0f || yPies > o.yMax)
            continue;

        float dx = skierPos.x - c.x;
        float dz = skierPos.z - c.y;
        float distMinima = o.radio + RADIO_ESQUIADOR;
        float d2 = dx * dx + dz * dz;

        if (d2 < distMinima * distMinima)
        {
            float d = std::sqrt(d2);
            glm::vec2 normal = (d > 0.0001f) ? glm::vec2(dx / d, dz / d) : -dirPista;
            skierPos.x = c.x + normal.x * distMinima;
            skierPos.z = c.y + normal.y * distMinima;
            hubo = true;
        }
    }

    return hubo;
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
    ambienteActual.velocidadLateral += (obj.velocidadLateral - ambienteActual.velocidadLateral) * k;
    ambienteActual.densidadNiebla += (obj.densidadNiebla - ambienteActual.densidadNiebla) * k;
    ambienteActual.intensidadSol += (obj.intensidadSol - ambienteActual.intensidadSol) * k;
    ambienteActual.factorFov += (obj.factorFov - ambienteActual.factorFov) * k;
    ambienteActual.amplitudVibracion += (obj.amplitudVibracion - ambienteActual.amplitudVibracion) * k;
    ambienteActual.colorNiebla = glm::mix(ambienteActual.colorNiebla, obj.colorNiebla, k);
    ambienteActual.colorSol = glm::mix(ambienteActual.colorSol, obj.colorSol, k);
    ambienteActual.colorAmbiente = glm::mix(ambienteActual.colorAmbiente, obj.colorAmbiente, k);
}

/**
 * @brief Genera una trayectoria en zigzag semi-aleatoria desde la Salida
 *        hasta el centro del arco de Meta para un contrincante.
 */
void GenerarTrayectoriaContrincante(Contrincante& c, int indiceRival)
{
    c.nodosLaterales.resize(NUM_NODOS_TRAYECTORIA);

    // En la salida se colocan a la izquierda (-6 m) y derecha (+6 m) del jugador
    float ladoInicial = (indiceRival == 0) ? -6.0f : 6.0f;
    c.nodosLaterales[0] = ladoInicial;

    int signo = (indiceRival == 0) ? -1 : 1;
    for (int i = 1; i < NUM_NODOS_TRAYECTORIA - 2; i++)
    {
        float amplitud = 8.0f + (float)(std::rand() % (int)ANCHO_ZIGZAG_RIVAL);
        c.nodosLaterales[i] = signo * amplitud;
        signo = -signo; // Alterna izquierda y derecha como en slalom real
    }

    // Los últimos 2 nodos embudan obligatoriamente dentro de la puerta de META
    c.nodosLaterales[NUM_NODOS_TRAYECTORIA - 2] = (indiceRival == 0) ? -3.0f : 3.0f;
    c.nodosLaterales[NUM_NODOS_TRAYECTORIA - 1] = (indiceRival == 0) ? -2.0f : 2.0f;
}

/**
 * @brief Evalúa el desplazamiento lateral suave de la trayectoria del
 *        contrincante según su avance longitudinal.
 */
float EvaluarLateralTrayectoria(const Contrincante& c, float avance, float distTotalXZ)
{
    if (c.nodosLaterales.empty() || distTotalXZ <= 0.001f)
        return 0.0f;

    float u = glm::clamp(avance / distTotalXZ, 0.0f, 1.0f);
    float posNodo = u * (float)(c.nodosLaterales.size() - 1);
    int idx0 = (int)std::floor(posNodo);
    int idx1 = glm::min(idx0 + 1, (int)c.nodosLaterales.size() - 1);
    float f = posNodo - (float)idx0;

    // Interpolación cúbica suave (Smoothstep) para giros naturales
    float fSuave = f * f * (3.0f - 2.0f * f);
    return glm::mix(c.nodosLaterales[idx0], c.nodosLaterales[idx1], fSuave);
}

/**
 * @brief Coloca a los dos contrincantes en la línea de salida, les asigna
 *        skins diferentes a la del jugador y traza su camino hacia la meta.
 */
void InicializarContrincantes(Model& terreno)
{
    std::srand((unsigned int)(glfwGetTime() * 1000.0) + 777);

    glm::vec2 vecTotalXZ(posMeta.x - posSalida.x, posMeta.z - posSalida.z);
    glm::vec2 dirPista = (glm::length(vecTotalXZ) > 0.001f) ? glm::normalize(vecTotalXZ) : glm::vec2(1.0f, 0.0f);
    glm::vec2 dirDerecha(dirPista.y, -dirPista.x);
    float anguloBase = glm::degrees(std::atan2(dirPista.x, dirPista.y));

    AsignarSkinsContrincantes();

    for (int r = 0; r < NUM_CONTRINCANTES; r++)
    {
        Contrincante& c = contrincantes[r];
        c.nombre = (r == 0) ? "RIVAL 1" : "RIVAL 2";
        c.factorPersonal = (r == 0) ? 0.97f : 1.03f; // Variación entre ambos rivales
        GenerarTrayectoriaContrincante(c, r);

        float latInicial = c.nodosLaterales[0];
        c.pos.x = posSalida.x + dirDerecha.x * latInicial;
        c.pos.z = posSalida.z + dirDerecha.y * latInicial;
        c.pos.y = ObtenerAlturaTerreno(terreno, c.pos.x, c.pos.z, posSalida.y) + offsetPiesEsquiador;

        c.rotY = anguloBase;
        c.velActual = 0.0f;
        c.inclinacionLat = 0.0f;
        c.inclinacionTorso = 0.0f;
        c.oscilacionPiernas = 0.0f;
        c.tiempoAnim = 0.0f;
        c.avanceActual = 0.0f;
        c.terminoCarrera = false;
        c.tiempoFinal = 0.0;
    }
    posicionJugadorActual = 1;
}

/**
 * @brief Actualiza el avance, seguimiento de trayectoria, colisiones con
 *        obstáculos y altura sobre el terreno de los dos contrincantes.
 */
void ActualizarContrincantes(Model& terreno)
{
    if (estadoCarrera == 0)
        return;

    glm::vec2 vecTotalXZ(posMeta.x - posSalida.x, posMeta.z - posSalida.z);
    float distTotalXZ = glm::length(vecTotalXZ);
    if (distTotalXZ <= 0.001f)
        return;

    glm::vec2 dirPista = glm::normalize(vecTotalXZ);
    glm::vec2 dirDerecha(dirPista.y, -dirPista.x);
    float anguloBase = glm::degrees(std::atan2(dirPista.x, dirPista.y));
    double ahora = glfwGetTime();

    for (int r = 0; r < NUM_CONTRINCANTES; r++)
    {
        Contrincante& c = contrincantes[r];
        if (c.terminoCarrera)
        {
            c.velActual = Suavizar(c.velActual, 0.0f, FRENADO, deltaTime);
            c.oscilacionPiernas = 0.0f;
            c.inclinacionLat = Suavizar(c.inclinacionLat, 0.0f, 8.0f, deltaTime);
            c.inclinacionTorso = Suavizar(c.inclinacionTorso, 0.0f, 8.0f, deltaTime);
            continue;
        }

        // Velocidad objetivo según el nivel de dificultad (Fácil, Medio, Difícil)
        float pulsoRitmo = 1.0f + 0.04f * std::sin((float)ahora * 2.2f + r * 3.14f);
        float velObjetivo = NIVELES[nivelActual].velocidadDescenso
            * FACTOR_VEL_RIVAL_NIVEL[nivelActual]
            * c.factorPersonal * pulsoRitmo;

        c.velActual = Suavizar(c.velActual, velObjetivo, ACELERACION_AVANCE * 1.15f, deltaTime);

        // Avance longitudinal sobre la pendiente
        c.avanceActual += c.velActual * deltaTime;

        // Seguimiento de la trayectoria semi-aleatoria en zigzag
        float latObjetivo = EvaluarLateralTrayectoria(c, c.avanceActual + 8.0f, distTotalXZ);
        glm::vec2 relActual(c.pos.x - posSalida.x, c.pos.z - posSalida.z);
        float latActual = glm::dot(relActual, dirDerecha);

        float diffLat = latObjetivo - latActual;
        float pasoMaxLat = NIVELES[nivelActual].velocidadLateral * 0.85f * deltaTime;
        float pasoLat = glm::clamp(diffLat * 3.0f * deltaTime, -pasoMaxLat, pasoMaxLat);
        float nuevaLat = glm::clamp(latActual + pasoLat, -ANCHO_MEDIO_PISTA + 2.0f, ANCHO_MEDIO_PISTA - 2.0f);

        // Actualizar posición en XZ
        c.pos.x = posSalida.x + dirPista.x * c.avanceActual + dirDerecha.x * nuevaLat;
        c.pos.z = posSalida.z + dirPista.y * c.avanceActual + dirDerecha.y * nuevaLat;

        // Evasión de obstáculos (árboles y rocas)
        glm::vec2 salidaXZ(posSalida.x, posSalida.z);
        glm::vec2 metaXZ(posMeta.x, posMeta.z);
        for (size_t i = 0; i < obstaculos.size(); i++)
        {
            const Obstaculo& o = obstaculos[i];
            glm::vec2 centroObs(o.centro.x, o.centro.z);
            if (glm::distance(centroObs, salidaXZ) < ZONA_LIBRE_SALIDA_META ||
                glm::distance(centroObs, metaXZ) < ZONA_LIBRE_SALIDA_META)
                continue;

            float dx = c.pos.x - centroObs.x;
            float dz = c.pos.z - centroObs.y;
            float distMin = o.radio + RADIO_ESQUIADOR;
            float d2 = dx * dx + dz * dz;
            if (d2 < distMin * distMin && d2 > 0.0001f)
            {
                float d = std::sqrt(d2);
                c.pos.x = centroObs.x + (dx / d) * distMin;
                c.pos.z = centroObs.y + (dz / d) * distMin;
                c.velActual *= 0.85f;
            }
        }

        // Mantener al contrincante pegado exactamente a la malla del terreno
        float ySuelo = ObtenerAlturaTerreno(terreno, c.pos.x, c.pos.z, c.pos.y - offsetPiesEsquiador);
        c.pos.y = ySuelo + offsetPiesEsquiador;

        // Orientación y animaciones articuladas del contrincante
        float giroVisual = glm::clamp(diffLat * 1.8f, -ANGULO_GIRO_ESQUIADOR, ANGULO_GIRO_ESQUIADOR);
        c.rotY = Suavizar(c.rotY, anguloBase + giroVisual, 10.0f, deltaTime);
        c.inclinacionLat = Suavizar(c.inclinacionLat, -giroVisual, 10.0f, deltaTime);
        c.inclinacionTorso = Suavizar(c.inclinacionTorso, 8.0f, 8.0f, deltaTime);

        c.tiempoAnim += deltaTime * (c.velActual * 0.2f);
        c.oscilacionPiernas = std::sin(c.tiempoAnim * 15.0f) * (12.0f * (c.velActual / NIVELES[nivelActual].velocidadDescenso));

        // Verificar si el contrincante cruzó la línea de META
        if (c.avanceActual >= distTotalXZ)
        {
            c.avanceActual = distTotalXZ;
            c.terminoCarrera = true;
            c.tiempoFinal = ahora - tiempoInicioCarrera;
            std::cout << ">>> [" << c.nombre << "] cruzo la META en: " << c.tiempoFinal << " seg" << std::endl;
        }
    }
}

/**
 * @brief Calcula la posición del jugador (1, 2 o 3) en tiempo real o al final.
 */
int CalcularPosicionJugador()
{
    glm::vec2 vecTotalXZ(posMeta.x - posSalida.x, posMeta.z - posSalida.z);
    glm::vec2 dirPista = (glm::length(vecTotalXZ) > 0.001f) ? glm::normalize(vecTotalXZ) : glm::vec2(1.0f, 0.0f);
    float avanceJugador = glm::dot(glm::vec2(skierPos.x - posSalida.x, skierPos.z - posSalida.z), dirPista);

    int lugar = 1;
    for (int r = 0; r < NUM_CONTRINCANTES; r++)
    {
        const Contrincante& c = contrincantes[r];
        if (estadoCarrera == 2)
        {
            // El jugador ya cruzó la meta: solo está delante quien haya terminado con menor tiempo
            if (c.terminoCarrera && c.tiempoFinal < tiempoFinalCarrera)
                lugar++;
        }
        else
        {
            // En carrera: está delante quien ya terminó o lleva mayor avance longitudinal
            if (c.terminoCarrera || c.avanceActual > avanceJugador)
                lugar++;
        }
    }
    return lugar;
}

/**
 * @brief Actualiza la carrera del jugador, a los contrincantes y aplica las
 *        paredes invisibles en el arco de META.
 */
void ActualizarCarrera(Model& terreno)
{
    // Si el jugador ya cruzó la meta, los contrincantes siguen bajando hasta terminar
    if (estadoCarrera == 2)
    {
        ActualizarContrincantes(terreno);
        return;
    }

    if (estadoCarrera != 1)
        return;

    double ahora = glfwGetTime();
    tiempoActualCarrera = ahora - tiempoInicioCarrera;

    // Actualizar contrincantes de IA
    ActualizarContrincantes(terreno);

    glm::vec2 vecTotalXZ(posMeta.x - posSalida.x, posMeta.z - posSalida.z);
    float distTotalXZ = glm::length(vecTotalXZ);
    if (distTotalXZ <= 0.001f)
        return;

    glm::vec2 dirPista = glm::normalize(vecTotalXZ);
    glm::vec2 dirDerecha(dirPista.y, -dirPista.x);
    float anguloBase = glm::degrees(atan2(dirPista.x, dirPista.y));

    bool avanzar = (keys[GLFW_KEY_W] || keys[GLFW_KEY_UP]) && modoCamara == 1;
    bool frenar = (keys[GLFW_KEY_S] || keys[GLFW_KEY_DOWN]) && modoCamara == 1;

    float objetivo = 0.0f;
    float tasa = FRICCION_LIBRE;
    if (frenar)
    {
        tasa = FRENADO;
    }
    else if (avanzar)
    {
        objetivo = ambienteActual.velocidadDescenso;
        tasa = ACELERACION_AVANCE;
    }
    velActual = Suavizar(velActual, objetivo, tasa, deltaTime);
    if (velActual < 0.05f && objetivo == 0.0f)
        velActual = 0.0f;

    // Avance previo antes de mover este fotograma (para las paredes invisibles de la meta)
    float avancePrevio = glm::dot(glm::vec2(skierPos.x - posSalida.x, skierPos.z - posSalida.z), dirPista);

    // Avance en XZ sobre el eje de la pista
    skierPos.x += dirPista.x * velActual * deltaTime;
    skierPos.z += dirPista.y * velActual * deltaTime;

    // Slalom lateral (A/D o flechas)
    if (modoCamara == 1)
    {
        float anguloObjetivo = anguloBase;

        if (keys[GLFW_KEY_D] || keys[GLFW_KEY_RIGHT])
        {
            skierPos.x -= dirDerecha.x * ambienteActual.velocidadLateral * deltaTime;
            skierPos.z -= dirDerecha.y * ambienteActual.velocidadLateral * deltaTime;
            anguloObjetivo = anguloBase - ANGULO_GIRO_ESQUIADOR;
        }
        else if (keys[GLFW_KEY_A] || keys[GLFW_KEY_LEFT])
        {
            skierPos.x += dirDerecha.x * ambienteActual.velocidadLateral * deltaTime;
            skierPos.z += dirDerecha.y * ambienteActual.velocidadLateral * deltaTime;
            anguloObjetivo = anguloBase + ANGULO_GIRO_ESQUIADOR;
        }

        skierRotY = Suavizar(skierRotY, anguloObjetivo, 10.0f, deltaTime);
    }

    // Colisiones con árboles y rocas
    if (ResolverColisiones(dirPista))
    {
        velActual = glm::min(velActual, ambienteActual.velocidadDescenso * FACTOR_VEL_CHOQUE);
        tiempoAvisoChoque = ahora;
    }

    // Proyección longitudinal (avance) y lateral respecto al eje Salida -> Meta
    glm::vec2 relativo(skierPos.x - posSalida.x, skierPos.z - posSalida.z);
    float lateral = glm::dot(relativo, dirDerecha);
    float avance = glm::dot(relativo, dirPista);

    // Límites laterales generales de la pista
    if (std::abs(lateral) > ANCHO_MEDIO_PISTA)
    {
        float limite = (lateral > 0.0f) ? ANCHO_MEDIO_PISTA : -ANCHO_MEDIO_PISTA;
        float exceso = lateral - limite;
        skierPos.x -= dirDerecha.x * exceso;
        skierPos.z -= dirDerecha.y * exceso;
        lateral = limite;
        tiempoAvisoLimite = ahora;
    }

    // No retroceder más allá de la línea de salida
    if (avance < 0.0f)
    {
        skierPos.x -= dirPista.x * avance;
        skierPos.z -= dirPista.y * avance;
        avance = 0.0f;
    }

    // ---- PAREDES INVISIBLES EN EL ARCO DE META ----
    // 1. Si intenta cruzar la línea de meta por los costados (fuera del marco ANCHO_PUERTA_META), lo bloquea el muro invisible
    float umbralMuroMeta = distTotalXZ - GROSOR_MURO_META;
    if (avance >= umbralMuroMeta)
    {
        if (std::abs(lateral) > ANCHO_PUERTA_META)
        {
            if (avancePrevio <= umbralMuroMeta + 0.5f)
            {
                // Choca de frente contra la pared invisible lateral de la meta
                float excesoAvance = avance - umbralMuroMeta;
                skierPos.x -= dirPista.x * excesoAvance;
                skierPos.z -= dirPista.y * excesoAvance;
                avance = umbralMuroMeta;
                velActual *= 0.25f;
                tiempoAvisoLimite = ahora;
            }
            else
            {
                // Si ya está dentro del umbral de la puerta, impide atravesar los postes laterales del marco
                float limiteMarco = (lateral > 0.0f) ? ANCHO_PUERTA_META : -ANCHO_PUERTA_META;
                float excesoMarco = lateral - limiteMarco;
                skierPos.x -= dirDerecha.x * excesoMarco;
                skierPos.z -= dirDerecha.y * excesoMarco;
                lateral = limiteMarco;
            }
        }
    }

    // Altura exacta leyendo el triángulo de la malla bajo el esquiador
    float ySuelo = ObtenerAlturaTerreno(terreno, skierPos.x, skierPos.z, skierPos.y - offsetPiesEsquiador);
    skierPos.y = ySuelo + offsetPiesEsquiador;

    // Actualizar posición en vivo (1/3, 2/3, 3/3)
    posicionJugadorActual = CalcularPosicionJugador();

    // ---- META: solo se gana al cruzar dentro del marco de la puerta ----
    if (avance >= distTotalXZ && std::abs(lateral) <= ANCHO_PUERTA_META)
    {
        estadoCarrera = 2;
        tiempoFinalCarrera = tiempoActualCarrera;
        tiempoMensajeMeta = ahora;
        velActual = 0.0f;
        //xcc- desbloquea trofeos al terminar la carrera
        trofeosDesbloqueados[0] = true;

        if (nivelActual >= 1)
            trofeosDesbloqueados[1] = true;

        if (nivelActual >= 2)
            trofeosDesbloqueados[2] = true;

        //xcc- conserva la posicion final calculada en la carrera
        posicionJugadorActual = CalcularPosicionJugador();

        std::cout << "\n==========================================" << std::endl;
        std::cout << "  META CRUZADA - POSICION: " << posicionJugadorActual << " LUGAR" << std::endl;
        std::cout << "  Tiempo del Jugador: " << tiempoFinalCarrera << " seg" << std::endl;
        std::cout << "==========================================\n" << std::endl;
    }
}

// ----------------------------------------------------------------------------
//  7.5 Ventisca
// ----------------------------------------------------------------------------

/**
 * @brief Distribuye los copos de nieve de forma aleatoria en pantalla.
 */
void InicializarCopos()
{
    std::srand(12345);
    copos.resize(MAX_COPOS);
    for (size_t i = 0; i < copos.size(); i++)
    {
        copos[i].u = (float)(std::rand() % 1000) / 1000.0f;
        copos[i].v = (float)(std::rand() % 1000) / 1000.0f;
        copos[i].z = (float)(std::rand() % 1000) / 1000.0f;
        copos[i].fase = (float)(std::rand() % 6283) / 1000.0f;
    }
}

/**
 * @brief Actualiza la intensidad de la ventisca y el movimiento de los copos.
 *        El viento crece con las ráfagas y con la velocidad del esquiador.
 * @param dt      Tiempo transcurrido desde el cuadro anterior (s).
 * @param velNorm Velocidad normalizada del esquiador [0, 1].
 */
void ActualizarVentisca(float dt, float velNorm)
{
    intensidadVentisca = Suavizar(intensidadVentisca, ventiscaActiva ? 1.0f : 0.0f, 1.5f, dt);
    if (intensidadVentisca < 0.01f)
        return;

    float t = (float)glfwGetTime();
    float rafaga = 0.5f + 0.5f * std::sin(t * 0.7f) * std::sin(t * 0.31f + 1.0f);   // [0, 1]
    vientoVentisca = 0.20f + 0.25f * rafaga + 0.30f * velNorm;

    for (size_t i = 0; i < copos.size(); i++)
    {
        Copo& c = copos[i];
        float paralaje = 0.4f + 0.9f * c.z;                 // Los cercanos se mueven más rápido

        c.u -= vientoVentisca * paralaje * dt;
        c.v += (0.10f + 0.10f * c.z) * dt + 0.03f * std::sin(t * 2.0f + c.fase) * dt;

        if (c.u < 0.0f) c.u += 1.0f;
        if (c.u >= 1.0f) c.u -= 1.0f;
        if (c.v >= 1.0f) c.v -= 1.0f;
        if (c.v < 0.0f) c.v += 1.0f;
    }
}

// ----------------------------------------------------------------------------
//  7.6 Interfaz 2D (HUD)
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

/**
 * @brief Agrega un copo con estela (paralelogramo inclinado). La cabeza está
 *        en (x, y) y la estela se extiende hacia arriba a la derecha, pues el
 *        copo viaja hacia abajo a la izquierda.
 */
static void AnadirCopo(std::vector<GLfloat>& b, float x, float y, float largo, float alto)
{
    float dy = -largo * 0.35f;
    GLfloat v[12] = { x, y,
                      x + largo, y + dy,
                      x + largo, y + dy + alto,
                      x + largo, y + dy + alto,
                      x, y + alto,
                      x, y };
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
 * @brief Dibuja la ventisca sobre la pantalla: un velo blanquecino y tres
 *        capas de copos (lejanos, medios y cercanos) con estela proporcional
 *        a la velocidad del viento.
 * @param hud Shader del HUD (ya activo y con la proyección ortográfica).
 */
static void DibujarVentisca(Shader& hud)
{
    if (intensidadVentisca < 0.01f)
        return;

    float e = SCREEN_HEIGHT / 800.0f;
    std::vector<GLfloat> b;

    // Velo blanquecino que reduce el contraste
    AnadirRect(b, 0.0f, 0.0f, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT);
    EnviarBufferHud(hud, b, glm::vec4(0.92f, 0.95f, 1.0f, 0.10f * intensidadVentisca));

    // Cantidad de copos visibles según la intensidad
    int activos = (int)(copos.size() * intensidadVentisca);

    // Tres capas de profundidad
    for (int capa = 0; capa < 3; capa++)
    {
        b.clear();
        float zMin = capa / 3.0f;
        float zMax = (capa + 1) / 3.0f;
        float tam = (1.5f + 2.0f * capa) * e;
        float largo = tam * (2.0f + vientoVentisca * 12.0f);

        for (int i = 0; i < activos; i++)
        {
            const Copo& c = copos[i];
            if (c.z < zMin || c.z >= zMax)
                continue;
            AnadirCopo(b, c.u * SCREEN_WIDTH, c.v * SCREEN_HEIGHT, largo, tam);
        }

        EnviarBufferHud(hud, b, glm::vec4(1.0f, 1.0f, 1.0f, 0.35f + 0.25f * capa));
    }
}

/**
 * @brief Dibuja toda la interfaz: ventisca, nivel, cronómetro, avisos de
 *        salida, meta, límite, choque e indicaciones. Se renderiza sobre la
 *        escena sin prueba de profundidad.
 * @param hudShader Shader del HUD.
 */

 //xcc- muestra la dificultad seleccionada en el Hub
void DibujarHUDHub(Shader& hudShader)
{
    glDisable(GL_DEPTH_TEST);

    //xcc- activa transparencia para los textos del Hub
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    hudShader.Use();

    glm::mat4 ortho = glm::ortho(0.0f, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT, 0.0f);

    glUniformMatrix4fv(glGetUniformLocation(hudShader.Program, "ortho"), 1, GL_FALSE, glm::value_ptr(ortho));

    float e = SCREEN_HEIGHT / 800.0f;

    if (CercaPanelConfiguracion())
    {
        //xcc- instrucciones de seleccion
        DibujarTexto(hudShader, "SELECCIONA DIFICULTAD: 1 2 3", 20.0f * e, 20.0f * e, 3.0f * e, glm::vec4(1.0f, 1.0f, 1.0f, 0.9f), false);

        //xcc- muestra la opcion actual
        DibujarTexto(hudShader, NOMBRES_NIVEL[nivelActual], 20.0f * e, 60.0f * e, 3.0f * e, glm::vec4(1.0f, 0.85f, 0.4f, 1.0f), false);

        //xcc- muestra un aviso al cambiar la dificultad
        float aNivel = AlphaMensaje(tiempoMensajeNivel, 0.8f, TIEMPO_MENSAJE_CORTO);
        if (aNivel > 0.0f)
            DibujarTexto(hudShader, NOMBRES_NIVEL[nivelActual], 0.0f, SCREEN_HEIGHT * 0.42f, 6.0f * e, glm::vec4(1.0f, 0.75f, 0.35f, aNivel), true);

        //xcc- muestra las opciones de equipamiento
        DibujarTexto(hudShader, "EQUIPAMIENTO: 4 5 6", 20.0f * e, 100.0f * e, 3.0f * e, glm::vec4(1.0f, 1.0f, 1.0f, 0.9f), false);

        //xcc- muestra el equipamiento actual
        DibujarTexto(hudShader, NOMBRES_EQUIPAMIENTO[equipamientoSeleccionado], 20.0f * e, 140.0f * e, 3.0f * e, glm::vec4(1.0f, 0.85f, 0.4f, 1.0f), false);
    }

    //xcc- muestra aviso cerca de la salida a pista
    if (CercaSalidaPista())
    {
        DibujarTexto(hudShader, "PRESIONA E PARA IR A PISTA", 0.0f, SCREEN_HEIGHT - 70.0f * e, 3.0f * e, glm::vec4(0.0, 0.0, 0.0, 1.0f), true);
    }

    //xcc- muestra aviso cerca de la entrada a trofeos
    if (CercaEntradaTrofeos())
    {
        DibujarTexto(hudShader, "PRESIONA E PARA IR A TROFEOS",
            0.0f, SCREEN_HEIGHT - 70.0f * e, 3.0f * e,
            glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), true);
    }

    //xcc- termina las transparencias del HUD del Hub
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

//xcc- muestra el aviso para regresar al Hub
void DibujarAvisoVolverHub(Shader& hudShader)
{
    glDisable(GL_DEPTH_TEST);

    //xcc- activa transparencia para los avisos
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    hudShader.Use();

    glm::mat4 ortho = glm::ortho(0.0f, (float)SCREEN_WIDTH, 0.0f, (float)SCREEN_HEIGHT);
    glUniformMatrix4fv(glGetUniformLocation(hudShader.Program, "projection"), 1, GL_FALSE, glm::value_ptr(ortho));

    float e = SCREEN_HEIGHT / 800.0f;

    DibujarTexto(hudShader, "PRESIONA H PARA VOLVER AL HUB", 0.0f, 35.0f * e, 2.5f * e,
        glm::vec4(1.0f, 1.0f, 1.0f, 0.9f), true);

    //xcc- muestra el control de las luces en trofeos
    if (escenaActual == ESCENA_TROFEOS)
    {
        if (lucesTrofeosCompletas)
            DibujarTexto(hudShader, "L - SOLO PODIO", 0.0f, 70.0f * e, 2.5f * e,
                glm::vec4(1.0f, 1.0f, 1.0f, 0.9f), true);
        else
            DibujarTexto(hudShader, "L - ENCENDER LUCES", 0.0f, 70.0f * e, 2.5f * e,
                glm::vec4(1.0f, 1.0f, 1.0f, 0.9f), true);
    }

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

void DibujarHUD(Shader& hudShader)
{
    glDisable(GL_DEPTH_TEST);

    //xcc- activa transparencias para HUD y ventisca
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    hudShader.Use();

    glm::mat4 ortho = glm::ortho(0.0f, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT, 0.0f);
    glUniformMatrix4fv(glGetUniformLocation(hudShader.Program, "ortho"), 1, GL_FALSE, glm::value_ptr(ortho));

    float e = SCREEN_HEIGHT / 800.0f;     // Factor de escala según la resolución
    double ahora = glfwGetTime();
    char buf[64];

    // Ventisca (debajo del texto)
    DibujarVentisca(hudShader);

    // Nivel actual (esquina superior izquierda)
    DibujarTexto(hudShader, NOMBRES_NIVEL[nivelActual], 20.0f * e, 20.0f * e, 3.0f * e, glm::vec4(1.0f, 0.95f, 0.85f, 0.9f), false);

    // Cronómetro durante el descenso
    if (estadoCarrera == 1)
    {
        std::snprintf(buf, sizeof(buf), "TIEMPO: %.2f", tiempoActualCarrera);
        DibujarTexto(hudShader, buf, 0.0f, 20.0f * e, 4.0f * e, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f), true);

        std::snprintf(buf, sizeof(buf), "POSICION: %d / 3", posicionJugadorActual);
        DibujarTexto(hudShader, buf, SCREEN_WIDTH - 260.0f * e, 20.0f * e, 3.5f * e, glm::vec4(1.0f, 0.85f, 0.25f, 1.0f), false);
    }

    // Indicaciones en la salida
    if (estadoCarrera == 0)
    {
        float pulso = 0.65f + 0.35f * (float)std::sin(ahora * 3.0);
        DibujarTexto(hudShader, "PRESIONA R PARA INICIAR", 0.0f, SCREEN_HEIGHT - 70.0f * e, 3.5f * e, glm::vec4(1.0f, 1.0f, 1.0f, pulso), true);
        DibujarTexto(hudShader, "V - VENTISCA", 0.0f, SCREEN_HEIGHT - 120.0f * e, 3.0f * e, glm::vec4(1.0f, 0.95f, 0.85f, 0.85f), true);
    }

    // Ayuda de controles al iniciar la carrera
    float aAyuda = AlphaMensaje(tiempoAyudaControles, 3.0f, 1.5f);
    if (aAyuda > 0.0f)
        DibujarTexto(hudShader, "ARRIBA AVANZA - ABAJO FRENA", 0.0f, SCREEN_HEIGHT * 0.82f, 3.5f * e, glm::vec4(1.0f, 1.0f, 1.0f, aAyuda), true);

    // Mensaje de SALIDA
    float aSalida = AlphaMensaje(tiempoMensajeSalida, 0.8f, TIEMPO_MENSAJE_CORTO);
    if (aSalida > 0.0f)
        DibujarTexto(hudShader, "SALIDA!", 0.0f, SCREEN_HEIGHT * 0.30f, 10.0f * e, glm::vec4(0.35f, 1.0f, 0.45f, aSalida), true);

    // Mensaje de ventisca
    float aVentisca = AlphaMensaje(tiempoMensajeVentisca, 0.8f, TIEMPO_MENSAJE_CORTO);
    if (aVentisca > 0.0f)
        DibujarTexto(hudShader, ventiscaActiva ? "VENTISCA ACTIVADA" : "VENTISCA DESACTIVADA", 0.0f, SCREEN_HEIGHT * 0.54f, 5.0f * e, glm::vec4(0.8f, 0.9f, 1.0f, aVentisca), true);


    // Aviso de choque con un obstáculo
    float aChoque = AlphaMensaje(tiempoAvisoChoque, 0.1f, 0.7f);
    if (aChoque > 0.0f && estadoCarrera == 1)
        DibujarTexto(hudShader, "CHOQUE!", 0.0f, SCREEN_HEIGHT * 0.62f, 5.0f * e, glm::vec4(1.0f, 0.55f, 0.1f, aChoque), true);

    // Aviso de límite de pista
    float aLimite = AlphaMensaje(tiempoAvisoLimite, 0.1f, 0.6f);
    if (aLimite > 0.0f && estadoCarrera == 1)
        DibujarTexto(hudShader, "LIMITE DE PISTA", 0.0f, SCREEN_HEIGHT * 0.75f, 4.0f * e, glm::vec4(1.0f, 0.25f, 0.2f, aLimite), true);

    // Resultado en META con tabla de posiciones de los 3 esquiadores
    float aMeta = AlphaMensaje(tiempoMensajeMeta, TIEMPO_MENSAJE_META + 3.0f, TIEMPO_DESVANECER);
    if (aMeta > 0.0f)
    {
        int miLugar = CalcularPosicionJugador();
        std::snprintf(buf, sizeof(buf), "META! - %d LUGAR!", miLugar);
        DibujarTexto(hudShader, buf, 0.0f, SCREEN_HEIGHT * 0.18f, 9.0f * e, glm::vec4(1.0f, 0.85f, 0.2f, aMeta), true);

        // Armar y ordenar la tabla de los 3 competidores
        glm::vec2 vecTotalXZ(posMeta.x - posSalida.x, posMeta.z - posSalida.z);
        float distTotalXZ = glm::max(glm::length(vecTotalXZ), 1.0f);

        std::vector<ResultadoCompetidor> tabla(3);
        tabla[0] = { "JUGADOR", true, tiempoFinalCarrera, distTotalXZ };
        for (int r = 0; r < NUM_CONTRINCANTES; r++)
        {
            double tEst = contrincantes[r].terminoCarrera
                ? contrincantes[r].tiempoFinal
                : tiempoActualCarrera + (distTotalXZ - contrincantes[r].avanceActual) / glm::max(contrincantes[r].velActual, 10.0f);
            tabla[r + 1] = { contrincantes[r].nombre, contrincantes[r].terminoCarrera, tEst, contrincantes[r].avanceActual };
        }

        // Ordenar del menor tiempo al mayor tiempo
        for (size_t i = 0; i < tabla.size(); i++)
            for (size_t j = i + 1; j < tabla.size(); j++)
                if (tabla[j].tiempo < tabla[i].tiempo)
                    std::swap(tabla[i], tabla[j]);

        // Dibujar 1°, 2° y 3° lugar en pantalla
        for (int i = 0; i < 3; i++)
        {
            std::snprintf(buf, sizeof(buf), "%d LUGAR: %s - %.2f SEG", i + 1, tabla[i].nombre.c_str(), tabla[i].tiempo);
            glm::vec4 colFila = (tabla[i].nombre == "JUGADOR")
                ? glm::vec4(0.35f, 1.0f, 0.45f, aMeta)
                : glm::vec4(1.0f, 1.0f, 1.0f, aMeta);
            DibujarTexto(hudShader, buf, 0.0f, SCREEN_HEIGHT * (0.36f + i * 0.09f), 4.2f * e, colFila, true);
        }

        DibujarTexto(hudShader, "T: VOLVER A LA SALIDA", 0.0f, SCREEN_HEIGHT * 0.68f, 3.0f * e, glm::vec4(0.9f, 0.9f, 0.9f, aMeta), true);
    }

    //xcc- termina las transparencias del HUD
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

// ----------------------------------------------------------------------------
//  7.7 Entrada: teclado y mouse
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
            if (posicionCamara.z < 20.0f)
            {
                posicionCamara.x = glm::clamp(posicionCamara.x, -21.0f, 21.0f);
                posicionCamara.z = glm::max(posicionCamara.z, -16.0f);
            }

            //xcc- bloquea la pared frontal dejando libre la puerta
            if (posicionCamara.z > 18.0f && posicionCamara.z < 21.0f)
            {
                if (posicionCamara.x < -3.3f || posicionCamara.x > 3.3f)
                {
                    posicionCamara = posicionAnterior;
                    posicionCamara.y = 5.0f;
                }
            }

            //xcc- corrige las colisiones del Hub por cada eje
            glm::vec3 pruebaX(posicionCamara.x, 5.0f, posicionAnterior.z);

            if (ColisionMueblesHub(pruebaX) || ColisionExteriorHub(pruebaX))
                posicionCamara.x = posicionAnterior.x;

            glm::vec3 pruebaZ(posicionCamara.x, 5.0f, posicionCamara.z);

            if (ColisionMueblesHub(pruebaZ) || ColisionExteriorHub(pruebaZ))
                posicionCamara.z = posicionAnterior.z;

            camera.SetPosition(posicionCamara);
        }

        //xcc- limita el movimiento dentro de la sala de trofeos
        if (escenaActual == ESCENA_TROFEOS)
        {
            glm::vec3 posicionCamara = camera.GetPosition();

            //xcc- mantiene la altura de la camara
            posicionCamara.y = 1.7f;

            //xcc- limita las paredes laterales de trofeos
            posicionCamara.x = glm::clamp(posicionCamara.x, -5.7f, 5.7f);

            //xcc- limita las paredes del fondo y de la parte nueva
            posicionCamara.z = glm::clamp(posicionCamara.z, -3.6f, 11.65f);

            //xcc- corrige las colisiones de trofeos por cada eje
            glm::vec3 pruebaX(posicionCamara.x, 1.7f, posicionAnterior.z);
            if (ColisionMueblesTrofeos(pruebaX))
                posicionCamara.x = posicionAnterior.x;

            glm::vec3 pruebaZ(posicionCamara.x, 1.7f, posicionCamara.z);
            if (ColisionMueblesTrofeos(pruebaZ))
                posicionCamara.z = posicionAnterior.z;

            camera.SetPosition(posicionCamara);
        }


    }

    // --- CÁLCULO DE ANIMACIONES DEL ESQUIADOR ---
    if (estadoCarrera == 1) {
        tiempoAnim += deltaTime * (velActual * 0.2f);
        oscilacionPiernas = std::sin(tiempoAnim * 15.0f) * (12.0f * (velActual / ambienteActual.velocidadDescenso));
    }
    else {
        oscilacionPiernas = 0.0f;
    }

    float objetivoInclinacionLat = 0.0f;
    if (keys[GLFW_KEY_D] || keys[GLFW_KEY_RIGHT]) objetivoInclinacionLat = 18.0f;
    if (keys[GLFW_KEY_A] || keys[GLFW_KEY_LEFT])  objetivoInclinacionLat = -18.0f;
    inclinacionLateral = Suavizar(inclinacionLateral, objetivoInclinacionLat, 10.0f, deltaTime);

    float objetivoInclinacionTorso = 0.0f;
    if (keys[GLFW_KEY_W] || keys[GLFW_KEY_UP])     objetivoInclinacionTorso = 8.0f;
    if (keys[GLFW_KEY_S] || keys[GLFW_KEY_DOWN])  objetivoInclinacionTorso = -10.0f;
    inclinacionTorso = Suavizar(inclinacionTorso, objetivoInclinacionTorso, 8.0f, deltaTime);
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

    // Cambio de traje: solo en el Hub y cerca del panel de configuración
    if (key == GLFW_KEY_E && action == GLFW_PRESS && escenaActual == ESCENA_HUB && CercaPanelConfiguracion())
    {
        indiceTrajeActual = (indiceTrajeActual + 1) % (int)opcionesTrajes.size();
        std::cout << ">>> ¡Cambiando de atuendo! Índice actual: " << indiceTrajeActual << std::endl;
    }

    //xcc- cambia del Hub a la pista cerca de la salida donde dice "pista"
    if (key == GLFW_KEY_E && action == GLFW_PRESS && escenaActual == ESCENA_HUB && CercaSalidaPista())
    {
        escenaActual = ESCENA_PISTA;
        modoCamara = 1;

        skierPos = posSalida;
        estadoCarrera = 0;
        velActual = 0.0f;

        // La pista arranca con la dificultad elegida en el Hub (sin transición)
        ambienteActual = NIVELES[nivelActual];

        std::cout << "Cambio de escena: Hub -> Pista" << std::endl;

        reiniciarContrincantes = true;
    }

    //xcc- cambia del Hub a la sala de trofeos
    if (key == GLFW_KEY_E && action == GLFW_PRESS && escenaActual == ESCENA_HUB && CercaEntradaTrofeos())
    {
        escenaActual = ESCENA_TROFEOS;
        modoCamara = 0;
        camera.SetPosition(glm::vec3(0.0f, 3.0f, 10.0f));

        //xcc- al entrar solo quedan encendidas las luces del podio
        lucesTrofeosCompletas = false;

        std::cout << "Cambio de escena: Hub -> Trofeos" << std::endl;
    }

    //xcc- regresa al Hub desde pista o trofeos
    if (key == GLFW_KEY_H && action == GLFW_PRESS &&
        (escenaActual == ESCENA_PISTA || escenaActual == ESCENA_TROFEOS))
    {
        escenaActual = ESCENA_HUB;
        modoCamara = 0;
        camera.SetPosition(glm::vec3(0.0f, 3.0f, 30.0f));
        std::cout << "Cambio de escena: regreso al Hub" << std::endl;
    }

    //xcc- enciende o apaga las luces generales de trofeos
    if (key == GLFW_KEY_L && action == GLFW_PRESS && escenaActual == ESCENA_TROFEOS)
    {
        lucesTrofeosCompletas = !lucesTrofeosCompletas;

        std::cout << "Luces de trofeos: "
            << (lucesTrofeosCompletas ? "encendidas" : "solo podio")
            << std::endl;
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

    // Visualización de colisionadores (depuración)
    if (key == GLFW_KEY_F3 && action == GLFW_PRESS)
    {
        mostrarColisionadores = !mostrarColisionadores;
        std::cout << "Colisionadores: " << (mostrarColisionadores ? "visibles" : "ocultos") << std::endl;
    }

    // Ventisca
    if (key == GLFW_KEY_V && action == GLFW_PRESS)
    {
        ventiscaActiva = !ventiscaActiva;
        tiempoMensajeVentisca = glfwGetTime();
        std::cout << "Ventisca: " << (ventiscaActiva ? "activada" : "desactivada") << std::endl;
    }

    // Inicio de carrera

    //xcc- inicia la carrera solo desde la pista
    if (key == GLFW_KEY_R && action == GLFW_PRESS && escenaActual == ESCENA_PISTA)


    {
        skierPos = posSalida;
        estadoCarrera = 1;
        modoCamara = 1;
        velActual = 0.0f;
        tiempoInicioCarrera = glfwGetTime();
        tiempoMensajeSalida = glfwGetTime();
        tiempoAyudaControles = glfwGetTime();
        tiempoMensajeMeta = -1.0;
        std::cout << ">>> INICIA EL DESCENSO DE SLALOM" << std::endl;
        reiniciarContrincantes = true;
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
        reiniciarContrincantes = true;
    }

    // Niveles de dificultad y clima (la transición se interpola en ActualizarAmbiente)
   //xcc- selecciona dificultad y clima desde el Hub
    if (action == GLFW_PRESS && key >= GLFW_KEY_1 && key <= GLFW_KEY_3 && escenaActual == ESCENA_HUB)
    {
        //xcc- permite seleccionar solo cerca del panel
        if (escenaActual == ESCENA_HUB && CercaPanelConfiguracion())
        {
            nivelActual = key - GLFW_KEY_1;
            ajusteNieblaManual = 0.0f;
            tiempoMensajeNivel = glfwGetTime();

            std::cout << "Seleccion en Hub: " << NOMBRES_NIVEL[nivelActual] << std::endl;
        }

    }

    //xcc- selecciona equipamiento desde el panel del Hub
    if (action == GLFW_PRESS && key >= GLFW_KEY_4 && key <= GLFW_KEY_6 && escenaActual == ESCENA_HUB && CercaPanelConfiguracion())
    {
        equipamientoSeleccionado = key - GLFW_KEY_4;

        std::cout << "Equipamiento seleccionado: " << NOMBRES_EQUIPAMIENTO[equipamientoSeleccionado] << std::endl; //solamente funcionará cuando este cerca del panel.
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
