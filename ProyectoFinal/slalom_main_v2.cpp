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

// ---- Cámara tercera persona (suavizado) ----
const float TASA_YAW_CAMARA = 2.0f;   // giro de la cámara (menor = más lenta)
const float TASA_ALTURA_CAMARA = 3.0f;   // altura de la cámara sobre el terreno
const float TASA_OBJETIVO_CAMARA = 4.0f;   // altura del punto al que mira
const float TASA_FOV_CAMARA = 1.5f;   // ensanchamiento del FOV
const float FACTOR_FOV_DINAMICO = 0.5f;   // fracción del ensanchamiento original
const float FACTOR_VIBRACION_CAM = 0.20f;  // fracción de la vibración original
const float RETRASO_MENSAJES_META = 3.0f;   // segundos antes de mostrar resultados

float tiempoCel = 0.0f;   // segundos desde que cruzó la meta
float pesoCel = 0.0f;   // 0 = pose normal, 1 = pose de la animación
float camGiroCel = 0.0f;   // 0 = cámara detrás, 1 = cámara delante

bool  camSuaveInit = false;   // false = reinicializar al siguiente cuadro
float camYawSuave = 0.0f;
float camYSuave = 0.0f;
float camTargetYSuave = 0.0f;
float velNormCamSuave = 0.0f;
float fovMultSuave = 1.0f;
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

// ---- Articulaciones del esquiador ----
// Coordenadas ABSOLUTAS en el espacio de los .obj (Y arriba).
// Conversión desde Blender (Z arriba): (x, y, z)_Blender -> (x, z, -y)
// Pivote = punto donde la pieza se une a su padre.
const glm::vec3 PIV_CADERA(-0.093f, 2.09765f, 0.010548f);
const glm::vec3 PIV_CINTURA(-0.093006f, 2.0831f, 0.010679f);
const glm::vec3 PIV_PECHO(-0.093264f, 2.48759f, -0.004624f);
const glm::vec3 PIV_CUELLO(-0.094275f, 3.03322f, -0.04391f);
const glm::vec3 PIV_CABEZA(-0.079021f, 3.25954f, -0.003005f);

const glm::vec3 PIV_HOMBRO_IZQ(0.347077f, 2.95232f, -0.046983f);
const glm::vec3 PIV_CODO_IZQ(0.470338f, 2.44909f, -0.05014f);
const glm::vec3 PIV_MUNECA_IZQ(0.588524f, 1.89387f, -0.043184f);
const glm::vec3 PIV_HOMBRO_DER(-0.534423f, 2.95017f, -0.031312f);
const glm::vec3 PIV_CODO_DER(-0.66137f, 2.44762f, -0.00413f);
const glm::vec3 PIV_MUNECA_DER(-0.777016f, 1.89241f, 0.014972f);

const glm::vec3 PIV_MUSLO_IZQ(0.054624f, 2.00339f, -0.018259f);   // articulación de la cadera
const glm::vec3 PIV_RODILLA_IZQ(0.081512f, 1.09564f, -0.020852f);
const glm::vec3 PIV_TOBILLO_IZQ(0.08112f, 0.076162f, -0.011668f);
const glm::vec3 PIV_MUSLO_DER(-0.242427f, 2.0036f, -0.00772f);
const glm::vec3 PIV_RODILLA_DER(-0.270705f, 1.09588f, -0.008355f);
const glm::vec3 PIV_TOBILLO_DER(-0.271096f, 0.076409f, -0.011668f);

const glm::vec3 PIV_SKI_IZQ(0.10514f, -0.042328f, 0.26877f);
const glm::vec3 PIV_SKI_DER(-0.252572f, -0.039645f, 0.252299f);

const glm::vec3 PIV_POLE_IZQ(0.546009f, 1.82591f, -0.038445f);
const glm::vec3 PIV_POLE_DER(-0.73605f, 1.82721f, -0.039651f);

const glm::vec3 PIV_CASCO(-0.083644f, 3.49318f, 0.020631f);

// ---- Parámetros de la pose ---- COPIAR A REPO
const glm::vec3 PIV_SUELO(-0.093f, 0.0f, 0.0f);   // entre los esquís, a nivel del suelo (eje de inclinación lateral)
const float LONG_MUSLO = PIV_MUSLO_IZQ.y - PIV_RODILLA_IZQ.y;   // ~0.908
const float LONG_PANTORRILLA = PIV_RODILLA_IZQ.y - PIV_TOBILLO_IZQ.y;   // ~1.019

// ---- Recepcionista del Hub ----
const glm::vec3 RECEP_POS(-16.2307f, -3.0f, 16.0674f);   // x,z de la silla; y = altura del piso (ajustar)
const float RECEP_ROT_Y = 180.0f;    // 0 = mira hacia +Z (la puerta); 180 = hacia el escritorio. Prueba 0/90/180/270
const float RECEP_ESCALA = 2.5f;    // ajustar a ojo (hub: ojos a 5.0 u. de alto; si el modelo mide ~3.3 u., prueba ~1.6)
const float RECEP_RADIO_SALUDO = 9.0f;    // distancia (en XZ) a la que empieza a saludar
const float RECEP_DURACION_SALUDO = 2.8f;    // segundos del saludo completo

// Pivotes en coordenadas ABSOLUTAS del .obj (Blender (x,y,z) -> (x, z, -y)). ¡Placeholders!
const glm::vec3 PIV_RECEP_HOMBRO(0.331799f, 3.02107f, -0.025263f);
const glm::vec3 PIV_RECEP_CODO(0.473854f, 2.44983f, -0.049436f);

bool  recepSaludando = false;
bool  recepYaSaludo = false;   // evita que repita mientras sigas cerca
float recepTiempo = 0.0f;

// ---- Comisario de pista (señal de salida) ----
const float COMI_LATERAL = 8.0f;    // distancia al eje de la pista (+ derecha, - izquierda)
const float COMI_AVANCE = 3.0f;    // unidades por delante de la línea de salida
const float COMI_ESCALA = 1.0f;    // ajustar a ojo (el esquiador mide ~3.5 u.)
const float COMI_ROT_EXTRA = 0.0f;    // giro extra sobre la dirección de la pista (prueba 0/45/-45)

// Pivote en coordenadas ABSOLUTAS del .obj: Blender (x,y,z) -> (x, z, -y). ¡Placeholder!
const glm::vec3 PIV_COMI_HOMBRO(-0.53f, 2.95f, -0.03f);

const float COMI_ANG_REPOSO = 0.0f;     // brazo colgando
const float COMI_ANG_LISTO = -165.0f;  // brazo arriba ("¡listos!")
const float COMI_ANG_SENAL = -80.0f;   // brazo al frente, horizontal ("¡fuera!")
const float COMI_TIEMPO_SENAL = 0.8f;   // segundos que mantiene la señal

float comiAnguloBrazo = 0.0f;
glm::vec3 comiPos(0.0f);
float comiRotY = 0.0f;

// ---- Impulso con palos y frenada de brazos ----
const float UMBRAL_VEL_IMPULSO = 0.85f;  // fracción de velocidad máx. a la que termina el impulso
const float DURACION_IMPULSO_MAX = 3.0f;   // segundos máx. de impulso por pulsación de W
const float FREC_IMPULSO = 10.0f;  // rad/s (~1.6 empujes por segundo)
const float DURACION_EXT_FRENADA = 2.0f;   // segundos que los brazos permanecen abiertos al frenar

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


//xcc- tiempos limite para desbloquear trofeos
const float TIEMPO_TROFEO_FACIL = 18.0f;
const float TIEMPO_TROFEO_MEDIO = 15.0f;
const float TIEMPO_TROFEO_DIFICIL = 12.0f;

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
 * @brief Puerta de slalom detectada automáticamente en la malla.
 */
struct PuertaSlalom
{
    glm::vec2 posteIzq;   // Posición XZ del primer poste
    glm::vec2 posteDer;   // Posición XZ del segundo poste
    float     y;          // Altura del terreno en el centro (solo depuración)
    bool      cruzada;
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
//xcc- guarda el estado de los nueve trofeos
bool trofeosDesbloqueados[9] = {
    false, false, false, false, false,
    false, false, false, false
};

//xcc- guarda cuales trofeos todavia no han sido vistos en la sala
bool trofeosNuevos[9] = {
    false, false, false, false, false,
    false, false, false, false
};

//xcc- nombres que se muestran en el aviso de la sala
const char* NOMBRES_TROFEOS[9] = {
    "MAESTRO DEL SLALOM",
    "VELOCISTA FACIL",
    "VELOCISTA MEDIO",
    "VELOCISTA DIFICIL",
    "CAMPEON FACIL",
    "CAMPEON MEDIO",
    "CAMPEON DIFICIL",
    "CONTRA LA TORMENTA",
    "LEYENDA DEL SLALOM"
};

//xcc- controla cuanto tiempo aparece el resumen al entrar
double tiempoMensajeTrofeos = -1.0;

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


// ---- Variables temporales de prueba ----
float rotTorso = 0.0f;
float rotCadera = 0.0f;
float rotCabeza = 0.0f;
float rotCodos = 0.0f;
float rotRodillas = 0.0f;

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

// ---- Puertas de Slalom (Detección de paso) ----
std::vector<PuertaSlalom> puertas;
int puertasCruzadasCorrectamente = 0;
bool registrarPoste = false;          // Solicitud de la tecla G (la atiende main)
bool hayPostePendiente = false;       // true si ya se guardó el primer poste
glm::vec2 postePendiente(0.0f);       // Primer poste de la puerta en construcción
std::string lineasPuertas;            // Líneas acumuladas para pegar en CargarPuertasDefinidas()

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
// -------------------------------------COPIAR A REPO
float agachadoAnim = 0.2f;   // 0 = erguido, 1 = posición de huevo 
float frenadaAnim = 0.0f;   // 0 = libre, 1 = frenando 
float frenadaBrazosAnim = 0.0f;   // extensión de brazos al frenar (temporal)
float impulsoAnim = 0.0f;   // peso de la animación de impulso (0..1)
float fasePalos = -1.5708f; // fase del ciclo de empuje
float tiempoImpulso = 0.0f;
float tiempoFrenada = 0.0f;

// ============================================================================
//  Animación por keyframes: celebración del jugador
// ============================================================================
struct PoseBrazo { float adelante, abre, codo; };   // grados: adelante (+ al frente), abre (+ hacia afuera), codo (flexión)
struct PoseCelebracion
{
    float salto;      // elevación sobre el suelo (unidades del mundo)
    float giro;       // giro extra sobre el eje vertical (grados)
    float agachado;   // 0 = piernas estiradas, 1 = posición de huevo
    float tronco;     // inclinación del tronco (+ adelante)
    float roll;       // inclinación lateral (grados)
    float cabeza;     // cabeceo extra de la cabeza (+ hacia abajo)
    PoseBrazo izq, der;
};
struct KeyframeCel { float t; PoseCelebracion pose; };

const KeyframeCel KEYS_CELEBRACION[] =
{
    //   t      salto  giro   agach  tronco roll  cabeza   brazo izq {adel, abre, codo}   brazo der {adel, abre, codo}
    { 0.00f, { 0.00f,   0.0f, 0.25f, 10.0f,  0.0f,   0.0f, { 20.0f,  10.0f, 25.0f }, { 20.0f,  10.0f, 25.0f } } },  // reposo
    { 0.60f, { 0.00f,   0.0f, 0.90f, 28.0f,  0.0f,   0.0f, {-30.0f,  15.0f, 10.0f }, {-30.0f,  15.0f, 10.0f } } },  // agacharse
    { 1.20f, { 1.00f, 180.0f, 0.00f, -5.0f,  0.0f, -10.0f, {  0.0f, 150.0f,  0.0f }, {  0.0f, 150.0f,  0.0f } } },  // salto
    { 1.70f, { 0.00f, 360.0f, 0.75f, 15.0f,  0.0f,   0.0f, {  0.0f, 120.0f, 20.0f }, {  0.0f, 120.0f, 20.0f } } },  // aterriza
    { 2.20f, { 0.00f, 360.0f, 0.15f, -4.0f,  0.0f, -12.0f, {  0.0f, 150.0f,  0.0f }, {  0.0f, 150.0f,  0.0f } } },  // brazos en V  <- inicio del bucle
    { 2.70f, { 0.15f, 360.0f, 0.30f,  0.0f,  6.0f,  -8.0f, {  0.0f, 150.0f,  0.0f }, { 45.0f,  55.0f, 75.0f } } },  // puño derecho
    { 3.20f, { 0.30f, 360.0f, 0.10f, -4.0f,  0.0f, -12.0f, {  0.0f, 150.0f,  0.0f }, {  0.0f, 150.0f,  0.0f } } },  // saltito
    { 3.70f, { 0.15f, 360.0f, 0.30f,  0.0f, -6.0f,  -8.0f, { 45.0f,  55.0f, 75.0f }, {  0.0f, 150.0f,  0.0f } } },  // puño izquierdo
    { 4.20f, { 0.30f, 360.0f, 0.10f, -4.0f,  0.0f, -12.0f, {  0.0f, 150.0f,  0.0f }, {  0.0f, 150.0f,  0.0f } } },  // saltito
    { 4.70f, { 0.00f, 360.0f, 0.15f, -4.0f,  0.0f, -12.0f, {  0.0f, 150.0f,  0.0f }, {  0.0f, 150.0f,  0.0f } } },  // = keyframe de t=2.20 (cierra el bucle)
};
const int   NUM_KEYS_CEL = sizeof(KEYS_CELEBRACION) / sizeof(KEYS_CELEBRACION[0]);
const float CEL_LOOP_INICIO = 2.20f;   // el último tramo se repite entre estos dos tiempos
const float CEL_LOOP_FIN = 4.70f;



PoseCelebracion EvaluarCelebracion(float t)
{
    // Pasado el último keyframe, se repite el tramo [LOOP_INICIO, LOOP_FIN]
    if (t > CEL_LOOP_FIN)
        t = CEL_LOOP_INICIO + std::fmod(t - CEL_LOOP_INICIO, CEL_LOOP_FIN - CEL_LOOP_INICIO);

    // Segmento [i, i+1] que contiene a t
    int i = 0;
    while (i < NUM_KEYS_CEL - 2 && t > KEYS_CELEBRACION[i + 1].t) i++;
    const PoseCelebracion& A = KEYS_CELEBRACION[i].pose;
    const PoseCelebracion& B = KEYS_CELEBRACION[i + 1].pose;

    float k = (t - KEYS_CELEBRACION[i].t) / (KEYS_CELEBRACION[i + 1].t - KEYS_CELEBRACION[i].t);
    k = glm::clamp(k, 0.0f, 1.0f);
    k = k * k * (3.0f - 2.0f * k);   // suavizado de entrada/salida (smoothstep)

    auto L = [k](float a, float b) { return a + (b - a) * k; };
    auto LB = [&](const PoseBrazo& a, const PoseBrazo& b)
        { return PoseBrazo{ L(a.adelante, b.adelante), L(a.abre, b.abre), L(a.codo, b.codo) }; };

    PoseCelebracion r;
    r.salto = L(A.salto, B.salto);
    r.giro = L(A.giro, B.giro);
    r.agachado = L(A.agachado, B.agachado);
    r.tronco = L(A.tronco, B.tronco);
    r.roll = L(A.roll, B.roll);
    r.cabeza = L(A.cabeza, B.cabeza);
    r.izq = LB(A.izq, B.izq);
    r.der = LB(A.der, B.der);
    return r;
}

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
int indiceTrajeActual = 0; // 0 = Turquesa, 1 = Rojo, 2 = Negro
const char* NOMBRES_TRAJES[3] = { "TURQUESA", "ROJO", "NEGRO" };

// Texturas del traje que se muestra en el Hub (casillero). Mismo orden que opcionesTrajes
std::vector<GLuint> opcionesTrajesHub;
// Mallas del Hub sin textura en el .mtl (Material.001 / Material.002): son el traje del casillero
std::vector<size_t> mallasTrajeHub;
int trajeHubAplicado = -1;   // Traje que tienen puesto ahora las mallas del Hub (-1 = ninguno)

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
    { 'Y', { 0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04 } },
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

// Localiza en el Hub las mallas que el .mtl dejó sin textura (el traje del casillero).
// Debe llamarse UNA vez justo después de cargar el modelo, antes de asignarles textura.
void DetectarMallasTrajeHub(Model& hub)
{
    mallasTrajeHub.clear();
    for (size_t m = 0; m < hub.meshes.size(); m++)
    {
        if (hub.meshes[m].textures.empty())
        {
            mallasTrajeHub.push_back(m);
            std::cout << "[Hub] Malla sin textura (traje) #" << m
                << " con " << hub.meshes[m].vertices.size() << " vertices." << std::endl;
        }
    }
    trajeHubAplicado = -1;
}

// Pone en el Hub la textura del traje elegido (igual que el esquiador). Solo trabaja
// cuando el traje cambia, así que se puede llamar en cada cuadro sin costo.
void AplicarTrajeHub(Model& hub)
{
    if (opcionesTrajesHub.empty() || mallasTrajeHub.empty() || trajeHubAplicado == indiceTrajeActual)
        return;

    GLuint texID = opcionesTrajesHub[indiceTrajeActual % (int)opcionesTrajesHub.size()];
    for (size_t i = 0; i < mallasTrajeHub.size(); i++)
    {
        Mesh& malla = hub.meshes[mallasTrajeHub[i]];
        if (malla.textures.empty())
        {
            Texture t;
            t.id = texID;
            t.type = "texture_diffuse";
            malla.textures.push_back(t);
        }
        else
        {
            malla.textures[0].id = texID;
        }
    }
    trajeHubAplicado = indiceTrajeActual;
    std::cout << "[Hub] Traje aplicado al casillero: indice " << indiceTrajeActual
        << " (textura " << texID << ", " << mallasTrajeHub.size() << " malla(s))" << std::endl;
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

// Misma lógica de articulación que el esquiador: M = padre · T(pivote) · R · T(-pivote)
glm::mat4 ArticularPieza(const glm::mat4& padre, const glm::vec3& pivote, const glm::vec3& rot)
{
    glm::mat4 m = glm::translate(padre, pivote);
    m = glm::rotate(m, glm::radians(rot.z), glm::vec3(0.0f, 0.0f, 1.0f));
    m = glm::rotate(m, glm::radians(rot.y), glm::vec3(0.0f, 1.0f, 0.0f));
    m = glm::rotate(m, glm::radians(rot.x), glm::vec3(1.0f, 0.0f, 0.0f));
    return glm::translate(m, -pivote);
}

// Detecta cercanía y avanza el reloj del saludo
void ActualizarRecepcionista(float dt)
{
    if (escenaActual != ESCENA_HUB)
    {
        recepSaludando = false; recepYaSaludo = false; recepTiempo = 0.0f;
        return;
    }

    glm::vec3 p = camera.GetPosition();
    float dist = glm::length(glm::vec2(p.x - RECEP_POS.x, p.z - RECEP_POS.z));

    if (!recepSaludando && !recepYaSaludo && dist < RECEP_RADIO_SALUDO)
    {
        recepSaludando = true;
        recepYaSaludo = true;
        recepTiempo = 0.0f;
    }
    if (dist > RECEP_RADIO_SALUDO * 1.4f) recepYaSaludo = false;   // se rearma al alejarte

    if (recepSaludando)
    {
        recepTiempo += dt;
        if (recepTiempo >= RECEP_DURACION_SALUDO) { recepSaludando = false; recepTiempo = 0.0f; }
    }
}

// El brazo sigue al estado de la carrera: arriba en espera, baja de golpe al arrancar
void ActualizarComisario(float dt)
{
    if (escenaActual != ESCENA_PISTA) return;

    float objetivo, velocidad;   // velocidad en grados/s
    if (estadoCarrera == 0)
    {
        objetivo = COMI_ANG_LISTO; velocidad = 120.0f;
    }
    else if (estadoCarrera == 1 && (glfwGetTime() - tiempoInicioCarrera) < COMI_TIEMPO_SENAL)
    {
        objetivo = COMI_ANG_SENAL; velocidad = 600.0f;
    }   // golpe rápido
    else
    {
        objetivo = COMI_ANG_REPOSO; velocidad = 90.0f;
    }   // regresa lento

    float paso = velocidad * dt;
    comiAnguloBrazo += glm::clamp(objetivo - comiAnguloBrazo, -paso, paso);
}

// alzado: 0 = brazo en reposo, 1 = brazo levantado. oscil: vaivén del antebrazo (grados)
void CalcularSaludoRecep(float& alzado, float& oscil)
{
    alzado = 0.0f; oscil = 0.0f;
    if (!recepSaludando) return;

    const float tSube = 0.5f, tBaja = 0.5f;
    float t = recepTiempo, k;
    if (t < tSube)                                   k = t / tSube;
    else if (t > RECEP_DURACION_SALUDO - tBaja)      k = (RECEP_DURACION_SALUDO - t) / tBaja;
    else                                             k = 1.0f;
    k = glm::clamp(k, 0.0f, 1.0f);
    alzado = k * k * (3.0f - 2.0f * k);              // suavizado
    oscil = 22.0f * alzado * std::sin(t * 15.7f);   // ~2.5 saludos por segundo
}

//xcc- verifica si la camara esta cerca de la entrada a trofeos
bool CercaEntradaTrofeos()
{
    glm::vec3 posicionCamara = camera.GetPosition();

    return posicionCamara.x > 13.0f && posicionCamara.x < 29.0f && posicionCamara.z > -3.0f && posicionCamara.z < 5.0f;
}

//xcc- verifica si la camara esta cerca del interruptor de trofeos
bool CercaInterruptorTrofeos()
{
    glm::vec3 posicionCamara = camera.GetPosition();
    glm::vec2 posicionActual(posicionCamara.x, posicionCamara.z);
    glm::vec2 posicionInterruptor(2.10798f, 11.262f);

    float distancia = glm::length(posicionActual - posicionInterruptor);
    return distancia < 2.0f;
}

//xcc- verifica si la camara esta cerca de la vitrina de trofeos
bool CercaVitrinaTrofeos()
{
    glm::vec3 posicionCamara = camera.GetPosition();

    return posicionCamara.x > -5.7f && posicionCamara.x < 5.7f &&
        posicionCamara.z > -1.9f && posicionCamara.z < 1.5f;
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
void CargarPuertasDefinidas(Model& terreno);
void RegistrarPoste(Model& terreno);
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

// Lo coloca al lado de la salida, apoyado en el terreno
void PosicionarComisario(Model& terreno)
{
    glm::vec2 v(posMeta.x - posSalida.x, posMeta.z - posSalida.z);
    glm::vec2 dirP = (glm::length(v) > 0.001f) ? glm::normalize(v) : glm::vec2(1.0f, 0.0f);
    glm::vec2 dirD(dirP.y, -dirP.x);   // "derecha", igual que en InicializarContrincantes

    comiPos.x = posSalida.x + dirP.x * COMI_AVANCE + dirD.x * COMI_LATERAL;
    comiPos.z = posSalida.z + dirP.y * COMI_AVANCE + dirD.y * COMI_LATERAL;
    comiPos.y = ObtenerAlturaTerreno(terreno, comiPos.x, comiPos.z, posSalida.y);  // pies al piso
    comiRotY = glm::degrees(std::atan2(dirP.x, dirP.y)) + COMI_ROT_EXTRA;
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

    //xcc- carga los nueve trofeos desbloqueables
    Model TrofeoMaestro((char*)"Models/Trofeos/Maestro_del_slalom.obj");
    Model TrofeoVelocistaFacil((char*)"Models/Trofeos/Velocista_facil.obj");
    Model TrofeoVelocistaMedio((char*)"Models/Trofeos/Velocista_medio.obj");
    Model TrofeoVelocistaDificil((char*)"Models/Trofeos/Velocista_dificil.obj");
    Model TrofeoCampeonFacil((char*)"Models/Trofeos/Campeon_facil.obj");
    Model TrofeoCampeonMedio((char*)"Models/Trofeos/Campeon_medio.obj");
    Model TrofeoCampeonDificil((char*)"Models/Trofeos/Campeon_dificil.obj");
    Model TrofeoTormenta((char*)"Models/Trofeos/Contra_la_tormenta.obj");
    Model TrofeoLeyenda((char*)"Models/Trofeos/Leyenda_del_slalom.obj");

    // Cargar las opciones de trajes para el sistema de personalización
    GLuint texTrajeTurquesa = CargarTextura("images/traje_turquesa.png");
    GLuint texTrajeRojo = CargarTextura("images/traje_rojo.png");
    // Llenar el vector global
    opcionesTrajes.push_back(texTrajeTurquesa);
    opcionesTrajes.push_back(texTrajeRojo);

    // 3ra skin (negra) para que Jugador, Rival 1 y Rival 2 siempre tengan trajes distintos
    GLuint texTrajeNegro = CargarTextura("images/traje_negro.png");
    opcionesTrajes.push_back(texTrajeNegro);

    // Traje que se ve en el casillero del Hub (mismo orden: turquesa, rojo, negro)
    opcionesTrajesHub.push_back(CargarTextura("images/traje_turquesa_hub.png"));
    opcionesTrajesHub.push_back(CargarTextura("images/traje_rojo_hub.png"));
    opcionesTrajesHub.push_back(CargarTextura("images/traje_negro_hub.png"));
    DetectarMallasTrajeHub(HubCentral);

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

    Model SkiDerModel((char*)"Models/Skis/ski_der.obj");
    Model SkiIzqModel((char*)"Models/Skis/ski_izq.obj");

    Model PoleDerModel((char*)"Models/SkiPoles/pole_der.obj");
    Model PoleIzqModel((char*)"Models/SkiPoles/pole_izq.obj");

    Model CascoModel((char*)"Models/Casco/casco.obj");

    Model RecepCuerpoModel((char*)"Models/Recepcionista/recepcionista_cuerpo.obj");
    Model RecepBrazoModel((char*)"Models/Recepcionista/recepcionista_brazo_der.obj");
    Model RecepAntebrazoModel((char*)"Models/Recepcionista/recepcionista_antebrazo_der.obj");

    Model ComisarioCuerpoModel((char*)"Models/Comisario/comisario_cuerpo.obj");
    Model ComisarioBrazoModel((char*)"Models/Comisario/comisario_brazo.obj");

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

    CargarPuertasDefinidas(PistaCompleta);

    // Copos de nieve para la ventisca
    InicializarCopos();

    // Inicializar posiciones, skins y rutas aleatorias de los 2 contrincantes
    InicializarContrincantes(PistaCompleta);
    PosicionarComisario(PistaCompleta);

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

        if (registrarPoste)
        {
            RegistrarPoste(PistaCompleta);
            registrarPoste = false;
        }

        // Reinicio de contrincantes solicitado desde el teclado (E, R, T)
        if (reiniciarContrincantes)
        {
            InicializarContrincantes(PistaCompleta);
            PosicionarComisario(PistaCompleta);
            // Reasienta al jugador sobre la malla del terreno en la salida
            skierPos.y = ObtenerAlturaTerreno(PistaCompleta, skierPos.x, skierPos.z, posSalida.y) + offsetPiesEsquiador;
            reiniciarContrincantes = false;
        }

        // ---- 6.7.2 Actualización lógica ----
        DoMovement();
        ActualizarRecepcionista(deltaTime);
        ActualizarComisario(deltaTime);

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
            // Yaw objetivo (detrás del esquiador; delante durante la celebración)
            float angObjetivo = skierRotY + 180.0f * camGiroCel;
            float alturaPiso0 = ObtenerAlturaTerreno(PistaCompleta, skierPos.x, skierPos.z, skierPos.y);

            if (!camSuaveInit)
            {
                camYawSuave = angObjetivo;
                camYSuave = glm::max(skierPos.y + 6.0f, alturaPiso0 + 4.0f);
                camTargetYSuave = skierPos.y + 1.0f;
                velNormCamSuave = velNorm;
                fovMultSuave = 1.0f;
                camSuaveInit = true;
            }

            camYawSuave = Suavizar(camYawSuave, angObjetivo, TASA_YAW_CAMARA, deltaTime);
            velNormCamSuave = Suavizar(velNormCamSuave, velNorm, 3.0f, deltaTime);

            // XZ: pegado al esquiador (sin retraso), con yaw suavizado
            glm::vec3 camPos(
                skierPos.x - std::sin(glm::radians(camYawSuave)) * 12.0f,
                0.0f,
                skierPos.z - std::cos(glm::radians(camYawSuave)) * 12.0f);

            // Y: suavizada para absorber baches del terreno
            float alturaPisoCamara = ObtenerAlturaTerreno(PistaCompleta, camPos.x, camPos.z, skierPos.y);
            float yDeseada = glm::max(skierPos.y + 6.0f, alturaPisoCamara + 4.0f);
            camYSuave = Suavizar(camYSuave, yDeseada, TASA_ALTURA_CAMARA, deltaTime);
            camYSuave = glm::max(camYSuave, alturaPisoCamara + 2.5f);   // nunca bajo el terreno
            camPos.y = camYSuave;

            // Vibración reducida y de menor frecuencia
            if (estadoCarrera == 1)
            {
                float vib = ambienteActual.amplitudVibracion * velNormCamSuave * FACTOR_VIBRACION_CAM;
                camPos.x += vib * std::sin(currentFrame * 9.0f);
                camPos.y += vib * std::sin(currentFrame * 13.0f + 1.7f);
            }

            camTargetYSuave = Suavizar(camTargetYSuave, skierPos.y + 1.0f, TASA_OBJETIVO_CAMARA, deltaTime);
            glm::vec3 target(skierPos.x, camTargetYSuave, skierPos.z);
            view = glm::lookAt(camPos, target, glm::vec3(0.0f, 1.0f, 0.0f));
            currentViewPos = camPos;

            // FOV dinámico suavizado y más discreto
            float fovMultObj = 1.0f + (ambienteActual.factorFov - 1.0f) * FACTOR_FOV_DINAMICO * velNormCamSuave;
            fovMultSuave = Suavizar(fovMultSuave, fovMultObj, TASA_FOV_CAMARA, deltaTime);
            fov *= fovMultSuave;
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
            AplicarTrajeHub(HubCentral);   // El traje del casillero sigue al del personaje
            HubCentral.Draw(lightingShader);

            //xcc- dibuja el exterior del Hub
            HubExterior.Draw(lightingShader);

            // ---- Recepcionista (sentada) con saludo al acercarse ----
            {
                glm::mat4 baseR = glm::translate(glm::mat4(1.0f), RECEP_POS);
                baseR = glm::rotate(baseR, glm::radians(RECEP_ROT_Y), glm::vec3(0.0f, 1.0f, 0.0f));
                baseR = glm::scale(baseR, glm::vec3(RECEP_ESCALA));

                float alzado, oscil;
                CalcularSaludoRecep(alzado, oscil);

                // Cuerpo (nodo padre)
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(baseR));
                RecepCuerpoModel.Draw(lightingShader);

                // Brazo: sale un poco al frente y hacia afuera (brazo derecho = Z negativo)
                glm::mat4 mBrazoR = ArticularPieza(baseR, PIV_RECEP_HOMBRO,
                    glm::vec3(-20.0f * alzado, 0.0f, 35.0f * alzado));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(mBrazoR));
                RecepBrazoModel.Draw(lightingShader);

                // Antebrazo: el codo se flexiona hacia arriba y la mano se balancea
                glm::mat4 mAntebR = ArticularPieza(mBrazoR, PIV_RECEP_CODO,
                    glm::vec3(-150.0f * alzado, 0.0f, oscil));
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(mAntebR));
                RecepAntebrazoModel.Draw(lightingShader);

                // Restaurar la matriz identidad para lo que se dibuje después
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(glm::mat4(1.0f)));
            }
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


            //xcc- dibuja solamente los trofeos desbloqueados
            if (trofeosDesbloqueados[0]) TrofeoMaestro.Draw(lightingShader);
            if (trofeosDesbloqueados[1]) TrofeoVelocistaFacil.Draw(lightingShader);
            if (trofeosDesbloqueados[2]) TrofeoVelocistaMedio.Draw(lightingShader);
            if (trofeosDesbloqueados[3]) TrofeoVelocistaDificil.Draw(lightingShader);
            if (trofeosDesbloqueados[4]) TrofeoCampeonFacil.Draw(lightingShader);
            if (trofeosDesbloqueados[5]) TrofeoCampeonMedio.Draw(lightingShader);
            if (trofeosDesbloqueados[6]) TrofeoCampeonDificil.Draw(lightingShader);
            if (trofeosDesbloqueados[7]) TrofeoTormenta.Draw(lightingShader);
            if (trofeosDesbloqueados[8]) TrofeoLeyenda.Draw(lightingShader);
           
            //se dibuja despues por que si no podria quedar del vidrio con la transparencia 

            //xcc- dibuja la sala de trofeos
            SalaTrofeos.Draw(lightingShader);

            //xcc- dibuja la parte nueva de la sala
            SalaTrofeosNueva.Draw(lightingShader);


            //xcc- desactiva transparencia para los demas objetos
            glUniform1i(glGetUniformLocation(lightingShader.Program, "transparency"), 0);
            glDisable(GL_BLEND);  // <---- ¡ESTA LÍNEA-- borrala----ROMPE LA VENTISCA!
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

                // Puertas de slalom: amarillo = pendiente, blanco = ya cruzada
                for (size_t i = 0; i < puertas.size(); i++)
                {
                    const PuertaSlalom& pu = puertas[i];
                    glm::vec2 d = pu.posteDer - pu.posteIzq;
                    glm::vec2 m = (pu.posteIzq + pu.posteDer) * 0.5f;

                    glm::mat4 modelPuerta = glm::translate(glm::mat4(1.0f), glm::vec3(m.x, pu.y + 2.0f, m.y));
                    modelPuerta = glm::rotate(modelPuerta, std::atan2(d.x, d.y), glm::vec3(0.0f, 1.0f, 0.0f));
                    modelPuerta = glm::scale(modelPuerta, glm::vec3(0.3f, 4.0f, glm::length(d)));
                    glUniformMatrix4fv(colorModelLoc, 1, GL_FALSE, glm::value_ptr(modelPuerta));

                    glm::vec3 colorPuerta = pu.cruzada ? glm::vec3(1.0f, 1.0f, 1.0f) : glm::vec3(1.0f, 0.9f, 0.1f);
                    glUniform3fv(colorShader.uniformColor, 1, glm::value_ptr(colorPuerta));
                    glDrawArrays(GL_TRIANGLES, 0, 36);
                }

                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            }

            glBindVertexArray(0);

            // ---- Esquiadores articulados (jugador y contrincantes) ----
            lightingShader.Use();
            glUniform1i(glGetUniformLocation(lightingShader.Program, "transparency"), 0);
            glUniform2f(glGetUniformLocation(lightingShader.Program, "uvOffset"), 0.0f, 0.0f);

            // ---- Comisario de pista ----
            glUniform3f(glGetUniformLocation(lightingShader.Program, "material.diffuse"), 0.8f, 0.8f, 0.8f);

            glm::mat4 baseC = glm::translate(glm::mat4(1.0f), comiPos);
            baseC = glm::rotate(baseC, glm::radians(comiRotY), glm::vec3(0.0f, 1.0f, 0.0f));
            baseC = glm::scale(baseC, glm::vec3(COMI_ESCALA));

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(baseC));
            ComisarioCuerpoModel.Draw(lightingShader);

            glm::mat4 mBrazoC = ArticularPieza(baseC, PIV_COMI_HOMBRO, glm::vec3(comiAnguloBrazo, 0.0f, 0.0f));
            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(mBrazoC));
            ComisarioBrazoModel.Draw(lightingShader);

            // Lambda reutilizable para dibujar cualquier esquiador articulado (Jugador o Contrincantes)
            auto DibujarEsquiador = [&](const glm::vec3& posEsq, float rotYEsq, float incLat, float incTorso, float oscPiernas, float agachado, float frenada, float frenadaBrazos, float impulso, float fasePalos, float tCel, float wCel, GLuint texTraje, const glm::vec3& tinteDifuso)
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

                    // ===== POSE DEL ESQUIADOR =====
// 
                    // ===== CELEBRACIÓN (keyframes): se mezcla con la pose normal según el peso wCel =====
                    const float w = wCel;
                    PoseCelebracion cel = {};
                    if (w > 0.001f)
                    {
                        cel = EvaluarCelebracion(tCel);
                        incLat = glm::mix(incLat, cel.roll, w);   // los parámetros son copias: se pueden modificar
                        agachado = glm::mix(agachado, cel.agachado, w);
                    }

                    const float giro = glm::clamp(incLat / ANGULO_GIRO_ESQUIADOR, -1.0f, 1.0f);  // -1 izq ... +1 der
                    const float rebote = oscPiernas / 12.0f;                                       // -1..1 (amortiguación)

                    // Tronco, brazos y cabeza
                    // Ciclo de impulso: 0 = brazos adelante (clavar palos), 1 = brazos atrás (empuje)
                    const float pushW = 0.5f + 0.5f * std::sin(fasePalos);
                    const float imp = impulso * (1.0f - frenada);   // frenar cancela el impulso
                    const float crouch = glm::clamp(agachado + 0.05f * rebote + 0.10f * imp * pushW, 0.0f, 1.0f);

                    // Piernas paralelas: muslo adelante 'aMuslo', pantorrilla adelante 'sPant',
                    // con el tobillo siempre bajo la cadera (los pies no se despegan ni se hunden)
                    const float aMuslo = glm::radians(5.0f + 50.0f * crouch);
                    const float sPant = std::asin(glm::clamp(LONG_MUSLO / LONG_PANTORRILLA * std::sin(aMuslo), -1.0f, 1.0f));
                    const float bajada = LONG_MUSLO * (1.0f - std::cos(aMuslo)) + LONG_PANTORRILLA * (1.0f - std::cos(sPant));
                    const float aDeg = glm::degrees(aMuslo);
                    const float sDeg = glm::degrees(sPant);

                    // Tronco, brazos y cabeza
                    float inclTronco = glm::mix(8.0f + 40.0f * crouch, -6.0f, frenada)
                        + 0.5f * incTorso + imp * (4.0f + 10.0f * pushW);   // el tronco acompaña el empuje

                    const float brazoBase = glm::mix(10.0f + 30.0f * crouch, 5.0f, frenadaBrazos);
                    const float brazoAdelante = glm::mix(brazoBase, 55.0f - 95.0f * pushW, imp);   // +55° adelante -> -40° atrás

                    const float flexBase = 15.0f + 55.0f * crouch * (1.0f - frenadaBrazos);
                    const float flexCodo = glm::mix(flexBase, 50.0f - 45.0f * pushW, imp);         // se extiende al empujar

                    const float abd = 8.0f + 45.0f * frenadaBrazos;   // apertura lateral (ahora temporal)

                    // Valores por brazo (en la celebración cada brazo puede ir distinto)
                    float brazoAdelIzq = brazoAdelante, brazoAdelDer = brazoAdelante;
                    float abdIzq = abd, abdDer = abd;
                    float codoIzq = flexCodo, codoDer = flexCodo;
                    if (w > 0.001f)
                    {
                        inclTronco = glm::mix(inclTronco, cel.tronco, w);
                        brazoAdelIzq = glm::mix(brazoAdelIzq, cel.izq.adelante, w);
                        abdIzq = glm::mix(abdIzq, cel.izq.abre, w);
                        codoIzq = glm::mix(codoIzq, cel.izq.codo, w);
                        brazoAdelDer = glm::mix(brazoAdelDer, cel.der.adelante, w);
                        abdDer = glm::mix(abdDer, cel.der.abre, w);
                        codoDer = glm::mix(codoDer, cel.der.codo, w);
                    }

                    // 1. RAÍZ GLOBAL: origen de los .obj a nivel del suelo (los pies)
                    glm::mat4 raiz = glm::translate(glm::mat4(1.0f), posEsq - glm::vec3(0.0f, offsetPiesEsquiador - w * cel.salto - 0.5, 0.0f));
                    raiz = glm::rotate(raiz, glm::radians(rotYEsq + w * cel.giro), glm::vec3(0.0f, 1.0f, 0.0f));

                    auto Articular = [](const glm::mat4& padre, const glm::vec3& pivote, const glm::vec3& rot)
                        {
                            glm::mat4 m = glm::translate(padre, pivote);
                            m = glm::rotate(m, glm::radians(rot.z), glm::vec3(0.0f, 0.0f, 1.0f));
                            m = glm::rotate(m, glm::radians(rot.y), glm::vec3(0.0f, 1.0f, 0.0f));
                            m = glm::rotate(m, glm::radians(rot.x), glm::vec3(1.0f, 0.0f, 0.0f));
                            return glm::translate(m, -pivote);
                        };
                    auto Pintar = [&](Model& pieza, const glm::mat4& m)
                        {
                            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(m));
                            pieza.Draw(lightingShader);
                        };
                    const glm::vec3 CERO(0.0f);

                    // 2. Inclinación lateral (pivota en los esquís) + bajada de la cadera al agacharse
                    glm::mat4 mBase = Articular(raiz, PIV_SUELO, glm::vec3(0.0f, 0.0f, incLat));
                    mBase = glm::translate(mBase, glm::vec3(0.0f, -bajada, 0.0f));

                    // 3. CADERAS (nodo padre del esqueleto)
                    glm::mat4 mCadera = Articular(mBase, PIV_CADERA, glm::vec3(rotCadera, 0.0f, 0.0f));
                    Pintar(CaderasModel, mCadera);

                    // 4. PIERNA IZQUIERDA (el pie se contragira para quedar plano; cuña al frenar)
                    glm::mat4 mMusloIzq = Articular(mCadera, PIV_MUSLO_IZQ, glm::vec3(-aDeg, 0.0f, 0.0f));
                    Pintar(PiernaSupIzqModel, mMusloIzq);
                    glm::mat4 mPantIzq = Articular(mMusloIzq, PIV_RODILLA_IZQ, glm::vec3(aDeg + sDeg + rotRodillas, 0.0f, 0.0f));
                    Pintar(PiernaInfIzqModel, mPantIzq);
                    glm::mat4 mPieIzq = Articular(mPantIzq, PIV_TOBILLO_IZQ, glm::vec3(-sDeg, -12.0f * frenada, 0.0f));
                    Pintar(PieIzqModel, mPieIzq);
                    glm::mat4 mSkiIzq = Articular(mPieIzq, PIV_SKI_IZQ, glm::vec3(rotRodillas, 0.0f, 0.0f));
                    Pintar(SkiIzqModel, mPieIzq);
                    

                    // 5. PIERNA DERECHA (misma pose: esquís paralelos)
                    glm::mat4 mMusloDer = Articular(mCadera, PIV_MUSLO_DER, glm::vec3(-aDeg, 0.0f, 0.0f));
                    Pintar(PiernaSupDerModel, mMusloDer);
                    glm::mat4 mPantDer = Articular(mMusloDer, PIV_RODILLA_DER, glm::vec3(aDeg + sDeg + rotRodillas, 0.0f, 0.0f));
                    Pintar(PiernaInfDerModel, mPantDer);
                    glm::mat4 mPieDer = Articular(mPantDer, PIV_TOBILLO_DER, glm::vec3(-sDeg, 12.0f * frenada, 0.0f));
                    Pintar(PieDerModel, mPieDer);
                    glm::mat4 mSkiDer = Articular(mPieDer, PIV_SKI_DER, glm::vec3(rotRodillas, 0.0f, 0.0f));
                    Pintar(SkiDerModel, mSkiDer);

                    // 6. TREN SUPERIOR: Cintura (inclinación) -> Pecho (contrarrotación) -> Cuello -> Cabeza
                    glm::mat4 mCintura = Articular(mCadera, PIV_CINTURA, glm::vec3(inclTronco, 0.0f, 0.0f));
                    Pintar(CinturaModel, mCintura);
                    glm::mat4 mPecho = Articular(mCintura, PIV_PECHO, glm::vec3(rotTorso, 8.0f * giro, 0.0f));
                    Pintar(PechoModel, mPecho);
                    glm::mat4 mCuello = Articular(mPecho, PIV_CUELLO, glm::vec3(-0.3f * inclTronco, 0.0f, 0.0f));
                    Pintar(CuelloModel, mCuello);
                    glm::mat4 mCabeza = Articular(mCuello, PIV_CABEZA, glm::vec3(-0.5f * inclTronco + rotCabeza + w * cel.cabeza, -18.0f * giro, 0.0f));
                    Pintar(CabezaModel, mCabeza);
                    glm::mat4 mCasco = Articular(mCabeza, PIV_CASCO, glm::vec3(0.0f, 0.0f, 0.0f));
                    Pintar(CascoModel, mCasco);

                    // 7. BRAZO IZQUIERDO (el brazo exterior a la curva se abre, el interior se cierra)
                    glm::mat4 mHombroIzq = Articular(mPecho, PIV_HOMBRO_IZQ, CERO);
                    Pintar(HombroIzqModel, mHombroIzq);
                    glm::mat4 mBrazoIzq = Articular(mHombroIzq, PIV_HOMBRO_IZQ, glm::vec3(-brazoAdelante - inclTronco, 0.0f, abd + 0.8f * incLat));
                    Pintar(BrazoIzqModel, mBrazoIzq);
                    glm::mat4 mAntebIzq = Articular(mBrazoIzq, PIV_CODO_IZQ, glm::vec3(-codoIzq + rotCodos, 0.0f, 0.0f));
                    Pintar(AntebrazoIzqModel, mAntebIzq);
                    glm::mat4 mManoIzq = Articular(mAntebIzq, PIV_MUNECA_IZQ, CERO);
                    Pintar(ManoIzqModel, mManoIzq);
                    glm::mat4 mPoleIzq = Articular(mManoIzq, PIV_POLE_IZQ, glm::vec3(rotRodillas, 0.0f, 0.0f));
                    Pintar(PoleIzqModel, mPoleIzq);

                    // 8. BRAZO DERECHO
                    glm::mat4 mHombroDer = Articular(mPecho, PIV_HOMBRO_DER, CERO);
                    Pintar(HombroDerModel, mHombroDer);
                    glm::mat4 mBrazoDer = Articular(mHombroDer, PIV_HOMBRO_DER, glm::vec3(-brazoAdelDer - inclTronco, 0.0f, -abdDer + 0.8f * incLat));
                    Pintar(BrazoDerModel, mBrazoDer);
                    glm::mat4 mAntebDer = Articular(mBrazoDer, PIV_CODO_DER, glm::vec3(-codoDer + rotCodos, 0.0f, 0.0f));
                    Pintar(AntebrazoDerModel, mAntebDer);
                    glm::mat4 mManoDer = Articular(mAntebDer, PIV_MUNECA_DER, CERO);
                    Pintar(ManoDerModel, mManoDer);
                    glm::mat4 mPoleDer = Articular(mManoDer, PIV_POLE_DER, glm::vec3(rotRodillas, 0.0f, 0.0f));
                    Pintar(PoleDerModel, mPoleDer);


                };

            // 1. Dibujar al Jugador
            AsignarSkinsContrincantes();
            // Jugador
            DibujarEsquiador(skierPos, skierRotY, inclinacionLateral, inclinacionTorso,
                oscilacionPiernas, agachadoAnim, frenadaAnim, frenadaBrazosAnim, impulsoAnim, fasePalos,
                tiempoCel, pesoCel,
                opcionesTrajes[indiceTrajeActual], glm::vec3(0.85f, 0.85f, 0.85f));

            // 2. Dibujar a los 2 Contrincantes con sus skins distintas
            glm::vec3 tintesRivales[2] = { glm::vec3(0.95f, 0.80f, 0.80f), glm::vec3(0.80f, 0.90f, 1.00f) };
            for (int r = 0; r < NUM_CONTRINCANTES; r++)
            {
                const Contrincante& c = contrincantes[r];
                float ratioRival = glm::clamp(c.velActual / NIVELES[nivelActual].velocidadDescenso, 0.0f, 1.0f);
                DibujarEsquiador(c.pos, c.rotY, c.inclinacionLat, c.inclinacionTorso,
                    c.oscilacionPiernas, 0.2f + 0.8f * ratioRival, 0.0f, 0.0f, 0.0f, 0.0f,
                    0.0f, 0.0f,
                    opcionesTrajes[c.indiceTraje], tintesRivales[r]);
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

            //xcc- muestra el regreso al Hub antes de iniciar o al terminar (con retraso)
            if (estadoCarrera == 0 || (estadoCarrera == 2 && glfwGetTime() >= tiempoMensajeMeta)) // <--- ACTUALIZAR CONDICIÓN
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

/** @brief Agrega una puerta definida por la posición XZ de sus dos postes. */
void AgregarPuerta(Model& terreno, float x1, float z1, float x2, float z2)
{
    PuertaSlalom p;
    p.posteIzq = glm::vec2(x1, z1);
    p.posteDer = glm::vec2(x2, z2);
    p.cruzada = false;
    glm::vec2 medio = (p.posteIzq + p.posteDer) * 0.5f;
    p.y = ObtenerAlturaTerreno(terreno, medio.x, medio.y, posSalida.y);
    puertas.push_back(p);
}

/** @brief Puertas fijas del circuito. Pega aquí las líneas que imprime la tecla G. */
void CargarPuertasDefinidas(Model& terreno)
{
    puertas.clear();
    puertasCruzadasCorrectamente = 0;
    AgregarPuerta(terreno, -192.74f, 31.93f, -192.07f, 21.20f);
    AgregarPuerta(terreno, -172.25f, 24.36f, -171.76f, 35.84f);
    AgregarPuerta(terreno, -142.10f, 35.62f, -142.49f, 46.41f);
    AgregarPuerta(terreno, -117.24f, 31.82f, -116.60f, 42.66f);
    AgregarPuerta(terreno, -89.31f, 8.13f, -83.95f, 16.34f);
    AgregarPuerta(terreno, -74.36f, -15.55f, -64.75f, -10.18f);
    AgregarPuerta(terreno, -25.88f, -35.61f, -23.47f, -24.96f);
    AgregarPuerta(terreno, -2.99f, -38.09f, -1.83f, -28.18f);
    AgregarPuerta(terreno, 27.59f, -34.96f, 29.65f, -24.69f);
    AgregarPuerta(terreno, 65.98f, -29.15f, 66.74f, -40.40f);
    AgregarPuerta(terreno, 102.91f, -25.20f, 103.73f, -13.94f);
    AgregarPuerta(terreno, 132.06f, -10.96f, 133.86f, -20.68f);
    AgregarPuerta(terreno, 161.08f, -12.79f, 165.20f, -2.68f);
    AgregarPuerta(terreno, 210.17f, 0.73f, 210.37f, 11.44f);
    
        

    // Ejemplo (reemplázalo con tus coordenadas):
        

    std::cout << "[Puertas] Puertas cargadas: " << puertas.size() << std::endl;
}


/**
 * @brief Con la tecla G se guarda la posición activa como poste: la del esquiador
 *        (3ª persona) o la de la cámara libre. Cada dos postes forman una puerta
 *        y se imprime la línea nueva y la lista acumulada.
 */
void RegistrarPoste(Model& terreno)
{
    glm::vec3 c = (modoCamara == 1) ? skierPos : camera.GetPosition();
    glm::vec2 punto(c.x, c.z);

    std::printf("[Puertas] Poste en x=%.2f  z=%.2f\n", punto.x, punto.y);

    if (!hayPostePendiente)
    {
        postePendiente = punto;
        hayPostePendiente = true;
        std::cout << "[Puertas] Poste 1 guardado. Ve al otro poste y pulsa G." << std::endl;
        return;
    }

    // Evita registrar los dos postes en el mismo lugar
    if (glm::distance(punto, postePendiente) < 1.5f)
    {
        std::cout << "[Puertas] El 2o poste esta pegado al 1o. Muevete al otro poste y pulsa G." << std::endl;
        return;
    }

    hayPostePendiente = false;
    AgregarPuerta(terreno, postePendiente.x, postePendiente.y, punto.x, punto.y);

    char linea[160];
    std::snprintf(linea, sizeof(linea), "    AgregarPuerta(terreno, %.2ff, %.2ff, %.2ff, %.2ff);\n",
        postePendiente.x, postePendiente.y, punto.x, punto.y);
    lineasPuertas += linea;

    std::printf("\n[Puertas] Linea nueva:\n%s", linea);
    std::printf("[Puertas] ===== TODAS LAS PUERTAS DE ESTA SESION =====\n%s[Puertas] ===== FIN =====\n\n",
        lineasPuertas.c_str());
}

/** @brief Indica si el segmento p1-p2 cruza al segmento q1-q2 (plano XZ). */
static bool SegmentosSeCruzan(const glm::vec2& p1, const glm::vec2& p2, const glm::vec2& q1, const glm::vec2& q2)
{
    glm::vec2 r = p2 - p1;
    glm::vec2 s = q2 - q1;
    float den = r.x * s.y - r.y * s.x;
    if (std::abs(den) < 1e-6f)
        return false;   // Paralelos

    glm::vec2 qp = q1 - p1;
    float t = (qp.x * s.y - qp.y * s.x) / den;
    float u = (qp.x * r.y - qp.y * r.x) / den;
    return t >= 0.0f && t <= 1.0f && u >= 0.0f && u <= 1.0f;
}

/**
 * @brief Cuenta una puerta cuando la trayectoria del esquiador en este cuadro
 *        atraviesa el segmento entre sus dos postes.
 * @param posAnterior Posición del esquiador al inicio del cuadro.
 */
void EvaluarPuertasSlalom(const glm::vec3& posAnterior)
{
    glm::vec2 a(posAnterior.x, posAnterior.z);
    glm::vec2 b(skierPos.x, skierPos.z);
    if (a == b)
        return;

    for (size_t i = 0; i < puertas.size(); i++)
    {
        if (puertas[i].cruzada)
            continue;

        if (SegmentosSeCruzan(a, b, puertas[i].posteIzq, puertas[i].posteDer))
        {
            puertas[i].cruzada = true;
            puertasCruzadasCorrectamente++;
            std::cout << ">>> Puerta " << i + 1 << " cruzada! (Total: "
                << puertasCruzadasCorrectamente << "/" << puertas.size() << ")" << std::endl;
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
    glm::vec3 posAnterior = skierPos;   // Para detectar el cruce de puertas

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

    // Evaluar paso por las puertas
    EvaluarPuertasSlalom(posAnterior);

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
        tiempoMensajeMeta = ahora + RETRASO_MENSAJES_META;   // <--- ACTUALIZAR (añadir + RETRASO)
        velActual = 0.0f;
        //xcc- desbloquea trofeo por tiempo en dificultad facil
        if (nivelActual == 0 && tiempoFinalCarrera <= TIEMPO_TROFEO_FACIL &&
            !trofeosDesbloqueados[1])
        {
            trofeosDesbloqueados[1] = true;
            trofeosNuevos[1] = true;
            std::cout << "Trofeo desbloqueado: Velocista Facil" << std::endl;
        }

        //xcc- desbloquea trofeo por tiempo en dificultad media
        if (nivelActual == 1 && tiempoFinalCarrera <= TIEMPO_TROFEO_MEDIO &&
            !trofeosDesbloqueados[2])
        {
            trofeosDesbloqueados[2] = true;
            trofeosNuevos[2] = true;
            std::cout << "Trofeo desbloqueado: Velocista Medio" << std::endl;
        }

        //xcc- desbloquea trofeo por tiempo en dificultad dificil
        if (nivelActual == 2 && tiempoFinalCarrera <= TIEMPO_TROFEO_DIFICIL &&
            !trofeosDesbloqueados[3])
        {
            trofeosDesbloqueados[3] = true;
            trofeosNuevos[3] = true;
            std::cout << "Trofeo desbloqueado: Velocista Dificil" << std::endl;
        }

        //xcc- conserva la posicion final calculada en la carrera
        posicionJugadorActual = CalcularPosicionJugador();

        std::cout << "\n==========================================" << std::endl;
        std::cout << "  META CRUZADA - POSICION: " << posicionJugadorActual << " LUGAR" << std::endl;
        std::cout << "  Tiempo del Jugador: " << tiempoFinalCarrera << " seg" << std::endl;
        std::cout << "==========================================\n" << std::endl;


        //xcc- desbloquea Maestro del Slalom al completar todas las puertas
        if (puertasCruzadasCorrectamente == (int)puertas.size() && !trofeosDesbloqueados[0])
        {
            trofeosDesbloqueados[0] = true;
            trofeosNuevos[0] = true;
            std::cout << "Trofeo desbloqueado: Maestro del Slalom" << std::endl;
        }

        //xcc- desbloquea Campeon segun la dificultad al quedar en primer lugar
        if (posicionJugadorActual == 1)
        {
            if (nivelActual == 0 && !trofeosDesbloqueados[4])
            {
                trofeosDesbloqueados[4] = true;
                trofeosNuevos[4] = true;
                std::cout << "Trofeo desbloqueado: Campeon Facil" << std::endl;
            }

            if (nivelActual == 1 && !trofeosDesbloqueados[5])
            {
                trofeosDesbloqueados[5] = true;
                trofeosNuevos[5] = true;
                std::cout << "Trofeo desbloqueado: Campeon Medio" << std::endl;
            }

            if (nivelActual == 2 && !trofeosDesbloqueados[6])
            {
                trofeosDesbloqueados[6] = true;
                trofeosNuevos[6] = true;
                std::cout << "Trofeo desbloqueado: Campeon Dificil" << std::endl;
            }
        }

        //xcc- desbloquea Contra la Tormenta al ganar con ventisca
        if (posicionJugadorActual == 1 && ventiscaActiva && !trofeosDesbloqueados[7])
        {
            trofeosDesbloqueados[7] = true;
            trofeosNuevos[7] = true;
            std::cout << "Trofeo desbloqueado: Contra la Tormenta" << std::endl;
        }

        //xcc- desbloquea Leyenda al conseguir los otros ocho trofeos
        if (trofeosDesbloqueados[0] && trofeosDesbloqueados[1] &&
            trofeosDesbloqueados[2] && trofeosDesbloqueados[3] &&
            trofeosDesbloqueados[4] && trofeosDesbloqueados[5] &&
            trofeosDesbloqueados[6] && trofeosDesbloqueados[7] &&
            !trofeosDesbloqueados[8])
        {
            trofeosDesbloqueados[8] = true;
            trofeosNuevos[8] = true;
            std::cout << "Trofeo desbloqueado: Leyenda del Slalom" << std::endl;
        }
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
    if (t < 0.0f)
        return 0.0f;            // <-- NUEVO: programado a futuro, aún no se muestra
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
 * @brief Dibuja un texto alineado a la derecha: mide el ancho real del texto y
 *        lo coloca a margenDer píxeles del borde derecho, así nunca se sale
 *        de la pantalla sin importar la longitud ni la resolución.
 */
static void DibujarTextoDerecha(Shader& hud, const std::string& texto, float margenDer, float y, float escala,
    const glm::vec4& color)
{
    float ancho = texto.size() * 6.0f * escala - escala;
    float x = (float)SCREEN_WIDTH - ancho - margenDer;
    if (x < 0.0f)
        x = 0.0f;
    DibujarTexto(hud, texto, x, y, escala, color, false);
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

        //xcc- indica como cambiar el traje
        DibujarTexto(hudShader, "CAMBIAR TRAJE: E", 20.0f * e, 100.0f * e, 3.0f * e, glm::vec4(1.0f, 1.0f, 1.0f, 0.9f), false);

        //xcc- muestra el traje actual
        DibujarTexto(hudShader, NOMBRES_TRAJES[indiceTrajeActual], 20.0f * e, 140.0f * e, 3.0f * e, glm::vec4(1.0f, 0.85f, 0.4f, 1.0f), false);
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
    //xcc- muestra como desbloquear los trofeos al acercarse a la vitrina
    if (escenaActual == ESCENA_TROFEOS && CercaVitrinaTrofeos())
    {
        DibujarTexto(hudShader, "COMO DESBLOQUEAR LOS TROFEOS",
            0.0f, SCREEN_HEIGHT - 145.0f * e, 3.0f * e,
            glm::vec4(1.0f, 0.85f, 0.3f, 0.95f), true);

        DibujarTexto(hudShader, "MAESTRO - CRUZA TODAS LAS PUERTAS",
            0.0f, SCREEN_HEIGHT - 185.0f * e, 2.2f * e,
            glm::vec4(1.0f, 1.0f, 1.0f, 0.9f), true);

        DibujarTexto(hudShader, "VELOCISTA - TERMINA ANTES DEL TIEMPO",
            0.0f, SCREEN_HEIGHT - 215.0f * e, 2.2f * e,
            glm::vec4(1.0f, 1.0f, 1.0f, 0.9f), true);

        DibujarTexto(hudShader, "CAMPEON - TERMINA EN PRIMER LUGAR",
            0.0f, SCREEN_HEIGHT - 245.0f * e, 2.2f * e,
            glm::vec4(1.0f, 1.0f, 1.0f, 0.9f), true);

        DibujarTexto(hudShader, "TORMENTA - GANA CON VENTISCA",
            0.0f, SCREEN_HEIGHT - 275.0f * e, 2.2f * e,
            glm::vec4(1.0f, 1.0f, 1.0f, 0.9f), true);

        DibujarTexto(hudShader, "LEYENDA - CONSIGUE LOS OTROS 8",
            0.0f, SCREEN_HEIGHT - 305.0f * e, 2.2f * e,
            glm::vec4(1.0f, 1.0f, 1.0f, 0.9f), true);
    }

    //xcc- muestra el resumen de trofeos al entrar a la sala
    if (escenaActual == ESCENA_TROFEOS)
    {
        float aTrofeos = AlphaMensaje(tiempoMensajeTrofeos, 4.0f, 1.0f);

        if (aTrofeos > 0.0f)
        {
            int totalDesbloqueados = 0;
            bool hayNuevos = false;

            //xcc- cuenta cuantos trofeos se han conseguido
            for (int i = 0; i < 9; i++)
            {
                if (trofeosDesbloqueados[i])
                    totalDesbloqueados++;

                if (trofeosNuevos[i])
                    hayNuevos = true;
            }

            float yAviso = SCREEN_HEIGHT - 120.0f * e;

            if (hayNuevos)
                DibujarTexto(hudShader, "NUEVOS TROFEOS", 0.0f, yAviso,
                    4.0f * e, glm::vec4(1.0f, 0.85f, 0.3f, aTrofeos), true);
            else
                DibujarTexto(hudShader, "SALA DE TROFEOS", 0.0f, yAviso,
                    4.0f * e, glm::vec4(1.0f, 0.85f, 0.3f, aTrofeos), true);

            char progreso[32];
            std::snprintf(progreso, sizeof(progreso),
                "PROGRESO: %d / 9", totalDesbloqueados);

            DibujarTexto(hudShader, progreso, 0.0f, yAviso - 45.0f * e,
                3.0f * e, glm::vec4(1.0f, 1.0f, 1.0f, aTrofeos), true);

            //xcc- escribe solamente los trofeos nuevos
            float yNombre = yAviso - 85.0f * e;

            for (int i = 0; i < 9; i++)
            {
                if (trofeosNuevos[i])
                {
                    DibujarTexto(hudShader, NOMBRES_TROFEOS[i],
                        0.0f, yNombre, 2.5f * e,
                        glm::vec4(0.45f, 1.0f, 0.55f, aTrofeos), true);

                    yNombre -= 32.0f * e;
                }
            }
        }
    }

    //xcc- muestra el control de luces solo cerca del interruptor
    if (escenaActual == ESCENA_TROFEOS && CercaInterruptorTrofeos())
    {
        if (lucesTrofeosCompletas)
            DibujarTexto(hudShader, "L - DEJAR SOLO PODIO", 0.0f, 70.0f * e, 2.5f * e,
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
        DibujarTextoDerecha(hudShader, buf, 30.0f * e, 20.0f * e, 3.5f * e, glm::vec4(1.0f, 0.85f, 0.25f, 1.0f));

        // Marcador de puertas cruzadas en vivo (verde), debajo de la posición
        std::snprintf(buf, sizeof(buf), "PUERTAS: %d / %d", puertasCruzadasCorrectamente, (int)puertas.size());
        DibujarTextoDerecha(hudShader, buf, 30.0f * e, 60.0f * e, 3.5f * e, glm::vec4(0.35f, 1.0f, 0.45f, 1.0f));
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

        // Total de puertas completadas en los resultados finales
        std::snprintf(buf, sizeof(buf), "PUERTAS COMPLETADAS: %d / %d", puertasCruzadasCorrectamente, (int)puertas.size());
        DibujarTexto(hudShader, buf, 0.0f, SCREEN_HEIGHT * 0.26f, 4.0f * e, glm::vec4(0.35f, 1.0f, 0.45f, aMeta), true);

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

        //xcc- velocidad de movimiento de la camara
        float velocidadCamara = deltaTime * 4.0f;

        //xcc- movimiento mas lento dentro de la sala de trofeos
        if (escenaActual == ESCENA_TROFEOS)
            velocidadCamara = deltaTime * 1.8f;   // Sala más pequeña y cámara más baja: sin esto se siente más rápida
        if (keys[GLFW_KEY_W] || keys[GLFW_KEY_UP])  camera.ProcessKeyboard(FORWARD, velocidadCamara);
        if (keys[GLFW_KEY_S] || keys[GLFW_KEY_DOWN])  camera.ProcessKeyboard(BACKWARD, velocidadCamara);
        if (keys[GLFW_KEY_A] || keys[GLFW_KEY_LEFT])  camera.ProcessKeyboard(LEFT, velocidadCamara);
        if (keys[GLFW_KEY_D] || keys[GLFW_KEY_RIGHT]) camera.ProcessKeyboard(RIGHT, velocidadCamara);

        //xcc- limita el movimiento de la camara dentro del Hub
        if (escenaActual == ESCENA_HUB)
        {
            glm::vec3 posicionCamara = camera.GetPosition();

            //xcc- mantiene la altura de la camara
            posicionCamara.y = 5.6f;

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
            posicionCamara.y = 2.2f;

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

    // Postura de esquiador
    float ratioVel = (ambienteActual.velocidadDescenso > 0.01f)
        ? glm::clamp(velActual / ambienteActual.velocidadDescenso, 0.0f, 1.0f) : 0.0f;
    bool teclaW = keys[GLFW_KEY_W] || keys[GLFW_KEY_UP];
    bool frenando = (estadoCarrera == 1) && (modoCamara == 1) &&
        (keys[GLFW_KEY_S] || keys[GLFW_KEY_DOWN]) && velActual > 0.5f;

    float objAgachado = frenando ? 0.30f : (0.20f + 0.80f * ratioVel);
    agachadoAnim = Suavizar(agachadoAnim, objAgachado, 3.0f, deltaTime);
    frenadaAnim = Suavizar(frenadaAnim, frenando ? 1.0f : 0.0f, 6.0f, deltaTime);

    // ---- Impulso con los palos: solo al arrancar (límite de velocidad y de tiempo) ----
    bool puedeImpulsar = (estadoCarrera == 1) && (modoCamara == 1) && teclaW && !frenando
        && ratioVel < UMBRAL_VEL_IMPULSO && tiempoImpulso < DURACION_IMPULSO_MAX;
    if (puedeImpulsar) tiempoImpulso += deltaTime;
    if (!teclaW)       tiempoImpulso = 0.0f;          // soltar W rearma el impulso

    impulsoAnim = Suavizar(impulsoAnim, puedeImpulsar ? 1.0f : 0.0f, 5.0f, deltaTime);
    if (impulsoAnim > 0.01f) fasePalos += deltaTime * FREC_IMPULSO;
    else                     fasePalos = -1.5708f;    // siempre arranca con brazos adelante

    // ---- Extensión de brazos al frenar: dura solo unos segundos ----
    if (frenando) tiempoFrenada += deltaTime; else tiempoFrenada = 0.0f;
    bool extenderBrazos = frenando && tiempoFrenada < DURACION_EXT_FRENADA;
    frenadaBrazosAnim = Suavizar(frenadaBrazosAnim, extenderBrazos ? 1.0f : 0.0f, 6.0f, deltaTime);

    // ---- Celebración (keyframes): arranca al cruzar la meta ----
    bool celebrando = (estadoCarrera == 2);
    if (celebrando) tiempoCel += deltaTime;
    pesoCel = Suavizar(pesoCel, celebrando ? 1.0f : 0.0f, 6.0f, deltaTime);
    if (!celebrando && pesoCel < 0.01f) tiempoCel = 0.0f;   // se reinicia solo tras desvanecerse
    camGiroCel = Suavizar(camGiroCel, celebrando ? 1.0f : 0.0f, 1.2f, deltaTime);

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
    if (key == GLFW_KEY_Z)
    {
        rotTorso += 1.0f;
    }
    if (key == GLFW_KEY_X)
    {
        rotCabeza += 1.0f;
    }
    if (key == GLFW_KEY_C)
    {
        rotCadera += 1.0f;
    }
    if (key == GLFW_KEY_V)
    {
        rotRodillas += 1.0f;
    }
    if (key == GLFW_KEY_B)
    {
        rotCodos += 1.0f;
    }

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
        camSuaveInit = false; // <--- AÑADIR

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

        //xcc- inicia el aviso de trofeos al entrar a la sala
        tiempoMensajeTrofeos = glfwGetTime();

        //xcc- al entrar solo quedan encendidas las luces del podio
        lucesTrofeosCompletas = false;


        std::cout << "Cambio de escena: Hub -> Trofeos" << std::endl;
    }

    //xcc- regresa al Hub desde pista o trofeos
    if (key == GLFW_KEY_H && action == GLFW_PRESS && (escenaActual == ESCENA_PISTA || escenaActual == ESCENA_TROFEOS))
    {

        //xcc- al salir de trofeos los premios nuevos ya fueron vistos
        if (escenaActual == ESCENA_TROFEOS)
        {
            for (int i = 0; i < 9; i++)
                trofeosNuevos[i] = false;
        }
    

        camSuaveInit = false; // <--- AÑADIR, reinicia la camara suave al cambiar de escena
        escenaActual = ESCENA_HUB;
        modoCamara = 0;
        camera.SetPosition(glm::vec3(0.0f, 3.0f, 30.0f));
        std::cout << "Cambio de escena: regreso al Hub" << std::endl;
    }

    //xcc- controla las luces solamente desde el interruptor
    if (key == GLFW_KEY_L && action == GLFW_PRESS &&
        escenaActual == ESCENA_TROFEOS && CercaInterruptorTrofeos())
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

    // Registro de postes de puertas: cámara libre (C) dentro de la pista
    if (key == GLFW_KEY_G && action == GLFW_PRESS && escenaActual == ESCENA_PISTA)
        registrarPoste = true;

    // Inicio de carrera

    //xcc- inicia la carrera solo desde la pista
    if (key == GLFW_KEY_R && action == GLFW_PRESS && escenaActual == ESCENA_PISTA)


    {
        camSuaveInit = false; // <--- AÑADIR

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

        puertasCruzadasCorrectamente = 0;
        for (size_t i = 0; i < puertas.size(); i++)
        {
            puertas[i].cruzada = false;
        }
    }

    // Regreso a la salida
    if (key == GLFW_KEY_T && action == GLFW_PRESS)
    {
        camSuaveInit = false; // <--- AÑADIR

        skierPos = posSalida;
        estadoCarrera = 0;
        velActual = 0.0f;
        tiempoActualCarrera = 0.0;
        tiempoMensajeMeta = -1.0;
        std::cout << ">>> Esquiador posicionado en la puerta de Salida (START)." << std::endl;
        reiniciarContrincantes = true;

        puertasCruzadasCorrectamente = 0;
        for (size_t i = 0; i < puertas.size(); i++)
        {
            puertas[i].cruzada = false;
        }
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
