/**
 * ============================================================================
 *  UNIVERSIDAD NACIONAL AUTÓNOMA DE MÉXICO
 *  FACULTAD DE INGENIERÍA
 * ============================================================================
 *  Archivo         : Iluminacion.h
 *  Proyecto        : Juego de Slalom - Escenario Interactivo 3D
 *  Descripción     : Esquemas de iluminación por escena (Hub y Sala de
 *                    Trofeos). Cada esquema agrupa la luz direccional, las
 *                    luces puntuales, las luces focales y la niebla, y se
 *                    envía al shader de iluminación con EnviarEsquema().
 *
 *  Uso:
 *      EsquemaIluminacion esquema = EsquemaHub(tiempo, nivel, interior, base);
 *      EnviarEsquema(lightingShader, esquema);
 * ============================================================================
 */

#pragma once

// ============================================================================
//  1. INCLUSIONES
// ============================================================================
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Shader.h"

// ============================================================================
//  2. CONSTANTES (deben coincidir con las del fragment shader)
// ============================================================================
const int MAX_LUCES_PUNTUALES = 12;
const int MAX_LUCES_FOCALES   = 8;

// ---- Configuración del Hub (calibrar con la tecla P dentro del Hub) ----
const glm::vec3 HUB_LUCES_TECHO[5] = {
    glm::vec3(-14.1939f, 8.0f, -4.95983f),
    glm::vec3(-14.7388f, 8.0f, 13.3787f),
    glm::vec3(16.4873f, 8.0f, 12.5388f),
    glm::vec3(15.0537f, 8.0f, -12.044f),
    glm::vec3(  0.0f, 7.0f,   2.0f)
};
const glm::vec3 HUB_LUZ_ENTRADA(0.36843f, 8.0f, 18.9748f);   // Sobre la puerta de acceso
const glm::vec3 HUB_LUZ_SALIDA (1.20504f, 8.0f, -16.0f);   // Sobre la salida hacia la pista
const glm::vec3 HUB_VITRINA_CENTRO(21.0f, 5.0f, -0.623778f);   // Centro del panel de trofeos (calibrar con P)
const glm::vec3 HUB_VITRINA_NORMAL(0.0f, 0.0f, 1.0f);     // Hacia dónde mira el panel

// ---- Configuración de la Sala de Trofeos ----
const float SALA_ALTURA_FOCOS = 7.0f;                  // Altura de los focos sobre cada trofeo

// ============================================================================
//  3. TIPOS DE DATOS
// ============================================================================

/** @brief Luz puntual con atenuación por distancia. */
struct LuzPuntual
{
    glm::vec3 posicion;
    glm::vec3 ambiente, difusa, especular;
    float     constante, lineal, cuadratica;
};

/** @brief Luz focal (cono) con atenuación por distancia. Ángulos en grados. */
struct LuzFocal
{
    glm::vec3 posicion, direccion;
    glm::vec3 ambiente, difusa, especular;
    float     anguloInterno, anguloExterno;
    float     constante, lineal, cuadratica;
};

/** @brief Conjunto completo de iluminación y niebla de una escena. */
struct EsquemaIluminacion
{
    glm::vec3               dirDireccion;    // Dirección de la luz (de la fuente hacia la escena)
    glm::vec3               dirAmbiente;
    glm::vec3               dirDifusa;
    glm::vec3               dirEspecular;
    std::vector<LuzPuntual> puntuales;
    std::vector<LuzFocal>   focales;
    float                   densidadNiebla;
    glm::vec3               colorNiebla;

    EsquemaIluminacion()
        : dirDireccion(0.0f, -1.0f, 0.0f), dirAmbiente(0.2f), dirDifusa(0.5f),
          dirEspecular(0.3f), densidadNiebla(0.0f), colorNiebla(0.5f)
    {
    }
};

// ============================================================================
//  4. FÁBRICAS DE LUCES
// ============================================================================

/**
 * @brief Crea una luz puntual a partir de un color base.
 * @param posicion   Posición en el mundo.
 * @param color      Color base de la luz.
 * @param intensidad Multiplicador de brillo (0 = apagada).
 * @param lineal     Coeficiente lineal de atenuación (menor = más alcance).
 * @param cuadratica Coeficiente cuadrático de atenuación.
 */
inline LuzPuntual CrearLuzPuntual(const glm::vec3& posicion, const glm::vec3& color, float intensidad,
                                  float lineal = 0.07f, float cuadratica = 0.017f)
{
    LuzPuntual l;
    l.posicion  = posicion;
    l.ambiente  = color * (0.08f * intensidad);
    l.difusa    = color * intensidad;
    l.especular = color * (0.8f * intensidad);
    l.constante = 1.0f;
    l.lineal    = lineal;
    l.cuadratica = cuadratica;
    return l;
}

/**
 * @brief Crea una luz focal (cono) a partir de un color base.
 * @param anguloInterno Semiángulo del cono a plena intensidad (grados).
 * @param anguloExterno Semiángulo donde la luz se extingue (grados).
 */
inline LuzFocal CrearFoco(const glm::vec3& posicion, const glm::vec3& direccion, const glm::vec3& color,
                          float intensidad, float anguloInterno, float anguloExterno,
                          float lineal = 0.045f, float cuadratica = 0.0075f)
{
    LuzFocal f;
    f.posicion  = posicion;
    f.direccion = glm::normalize(direccion);
    f.ambiente  = glm::vec3(0.0f);
    f.difusa    = color * intensidad;
    f.especular = color * (0.9f * intensidad);
    f.anguloInterno = anguloInterno;
    f.anguloExterno = anguloExterno;
    f.constante = 1.0f;
    f.lineal    = lineal;
    f.cuadratica = cuadratica;
    return f;
}

// ============================================================================
//  5. ESQUEMAS POR ESCENA
// ============================================================================

/**
 * @brief Iluminación del Hub. Mezcla el exterior (sol de atardecer) con el
 *        interior (luces cálidas de techo) según el factor de interior.
 *          - 5 luces cálidas de techo.
 *          - Luz cálida sobre la puerta de acceso.
 *          - Luz sobre la salida a la pista, con el color de la dificultad
 *            seleccionada (verde / ámbar / rojo) y pulso suave.
 *          - Luz direccional tenue y fría que simula luz de ventanas.
 * @param t        Tiempo en segundos (animaciones).
 * @param nivel    Dificultad seleccionada (0, 1, 2).
 * @param interior 0 = cámara afuera del Hub, 1 = adentro (suavizado).
 * @param exterior Esquema exterior (sol de atardecer) del cual se parte.
 */
inline EsquemaIluminacion EsquemaHub(float t, int nivel, float interior, const EsquemaIluminacion& exterior)
{
    EsquemaIluminacion e = exterior;
    nivel = std::max(0, std::min(2, nivel));

    // Luz direccional: del sol de atardecer a un resplandor frío de ventanas
    const glm::vec3 dirInterior = glm::normalize(glm::vec3(0.0f, -0.6f, -1.0f));
    e.dirDireccion = glm::normalize(glm::mix(exterior.dirDireccion, dirInterior, interior));
    e.dirAmbiente  = glm::mix(exterior.dirAmbiente,  glm::vec3(0.30f, 0.27f, 0.26f), interior);
    e.dirDifusa    = glm::mix(exterior.dirDifusa,    glm::vec3(0.18f, 0.22f, 0.30f), interior);
    e.dirEspecular = glm::mix(exterior.dirEspecular, glm::vec3(0.15f), interior);

    // Niebla: casi nula adentro, con tono cálido oscuro
    e.densidadNiebla = glm::mix(exterior.densidadNiebla, 0.0008f, interior);
    e.colorNiebla    = glm::mix(exterior.colorNiebla, glm::vec3(0.16f, 0.13f, 0.12f), interior);

    // Las luces posicionales solo existen dentro del Hub
    if (interior > 0.01f)
    {
        const glm::vec3 calida(1.0f, 0.82f, 0.58f);

        // Luces de techo
        for (int i = 0; i < 5; i++)
            e.puntuales.push_back(CrearLuzPuntual(HUB_LUCES_TECHO[i], calida, 0.9f * interior));

        // Puerta de acceso
        e.puntuales.push_back(CrearLuzPuntual(HUB_LUZ_ENTRADA, glm::vec3(1.0f, 0.9f, 0.75f), 0.7f * interior, 0.09f, 0.032f));

        // Salida a la pista: color según dificultad, con pulso (más brillo y más alcance)
        const glm::vec3 coloresNivel[3] = {
            glm::vec3(0.20f, 1.00f, 0.35f),   // Fácil: verde
            glm::vec3(1.00f, 0.70f, 0.15f),   // Medio: ámbar
            glm::vec3(1.00f, 0.20f, 0.15f)    // Difícil: rojo
        };
        float pulso = 0.80f + 0.20f * std::sin(t * 1.5f);
        e.puntuales.push_back(CrearLuzPuntual(HUB_LUZ_SALIDA, coloresNivel[nivel], 3.0f * pulso * interior, 0.045f, 0.0075f));

        // Vitrina de trofeos: dos focos dorados cruzados y un destello que barre el cristal
        const glm::vec3 oro(1.0f, 0.85f, 0.55f);
        glm::vec3 base = HUB_VITRINA_CENTRO + HUB_VITRINA_NORMAL * 5.0f;
        for (int lado = -1; lado <= 1; lado += 2)
        {
            glm::vec3 pos = base + glm::vec3(4.0f * lado, 3.5f, 0.0f);
            e.focales.push_back(CrearFoco(pos, HUB_VITRINA_CENTRO - pos, oro, 2.2f * interior, 16.0f, 30.0f));
        }

        float barrido = std::sin(t * 0.8f);
        e.puntuales.push_back(CrearLuzPuntual(HUB_VITRINA_CENTRO + HUB_VITRINA_NORMAL * 2.0f + glm::vec3(5.0f * barrido, 0.5f, 0.0f),
            glm::vec3(1.0f, 0.95f, 0.8f), 0.8f * interior, 0.14f, 0.07f));
    }

    return e;
}

/**
 * @brief Iluminación de la Sala de Trofeos: ambiente oscuro, un foco dorado
 *        sobre cada trofeo, dos luces de relleno tenues y un destello que
 *        orbita lentamente para dar brillo a los metales.
 * @param t       Tiempo en segundos.
 * @param trofeos Posición base de cada trofeo (máximo MAX_LUCES_FOCALES).
 */
inline EsquemaIluminacion EsquemaTrofeos(float t, const std::vector<glm::vec3>& trofeos)
{
    EsquemaIluminacion e;

    // Ambiente casi a oscuras
    e.dirDireccion = glm::normalize(glm::vec3(0.0f, -1.0f, -0.2f));
    e.dirAmbiente  = glm::vec3(0.07f, 0.06f, 0.08f);
    e.dirDifusa    = glm::vec3(0.06f, 0.06f, 0.10f);
    e.dirEspecular = glm::vec3(0.05f);
    e.densidadNiebla = 0.0f;
    e.colorNiebla    = glm::vec3(0.02f, 0.02f, 0.03f);

    // Centro de la sala
    glm::vec3 centro(0.0f);
    for (size_t i = 0; i < trofeos.size(); i++)
        centro += trofeos[i];
    if (!trofeos.empty())
        centro /= (float)trofeos.size();

    // Un foco dorado sobre cada trofeo
    const glm::vec3 dorado(1.0f, 0.85f, 0.55f);
    for (size_t i = 0; i < trofeos.size() && (int)i < MAX_LUCES_FOCALES; i++)
        e.focales.push_back(CrearFoco(trofeos[i] + glm::vec3(0.0f, SALA_ALTURA_FOCOS, 0.0f),
                                      glm::vec3(0.0f, -1.0f, 0.0f), dorado, 1.6f, 14.0f, 22.0f));

    // Relleno tenue
    const glm::vec3 tenue(1.0f, 0.78f, 0.55f);
    e.puntuales.push_back(CrearLuzPuntual(centro + glm::vec3(-10.0f, 4.0f, 0.0f), tenue, 0.35f));
    e.puntuales.push_back(CrearLuzPuntual(centro + glm::vec3( 10.0f, 4.0f, 0.0f), tenue, 0.35f));

    // Destello orbital
    glm::vec3 orbita = centro + glm::vec3(std::cos(t * 0.5f) * 8.0f, 3.5f, std::sin(t * 0.5f) * 8.0f);
    e.puntuales.push_back(CrearLuzPuntual(orbita, glm::vec3(1.0f, 0.9f, 0.6f), 0.6f, 0.14f, 0.07f));

    return e;
}

// ============================================================================
//  6. ENVÍO AL SHADER
// ============================================================================

/**
 * @brief Envía un esquema completo al shader de iluminación: luz direccional,
 *        luces puntuales, luces focales y niebla.
 * @param shader Shader de iluminación (ya activo con Use()).
 * @param e      Esquema a enviar.
 */
inline void EnviarEsquema(Shader& shader, const EsquemaIluminacion& e)
{
    GLuint p = shader.Program;

    // Luz direccional
    glUniform3fv(glGetUniformLocation(p, "dirLight.direction"), 1, glm::value_ptr(e.dirDireccion));
    glUniform3fv(glGetUniformLocation(p, "dirLight.ambient"),   1, glm::value_ptr(e.dirAmbiente));
    glUniform3fv(glGetUniformLocation(p, "dirLight.diffuse"),   1, glm::value_ptr(e.dirDifusa));
    glUniform3fv(glGetUniformLocation(p, "dirLight.specular"),  1, glm::value_ptr(e.dirEspecular));

    // Niebla
    glUniform1f(glGetUniformLocation(p, "fogDensity"), e.densidadNiebla);
    glUniform3fv(glGetUniformLocation(p, "fogColor"), 1, glm::value_ptr(e.colorNiebla));

    // Luces puntuales
    int nP = (int)std::min<size_t>(e.puntuales.size(), (size_t)MAX_LUCES_PUNTUALES);
    glUniform1i(glGetUniformLocation(p, "numPointLights"), nP);
    for (int i = 0; i < nP; i++)
    {
        const LuzPuntual& l = e.puntuales[i];
        std::string b = "pointLights[" + std::to_string(i) + "].";
        glUniform3fv(glGetUniformLocation(p, (b + "position").c_str()),  1, glm::value_ptr(l.posicion));
        glUniform3fv(glGetUniformLocation(p, (b + "ambient").c_str()),   1, glm::value_ptr(l.ambiente));
        glUniform3fv(glGetUniformLocation(p, (b + "diffuse").c_str()),   1, glm::value_ptr(l.difusa));
        glUniform3fv(glGetUniformLocation(p, (b + "specular").c_str()),  1, glm::value_ptr(l.especular));
        glUniform1f(glGetUniformLocation(p, (b + "constant").c_str()),   l.constante);
        glUniform1f(glGetUniformLocation(p, (b + "linear").c_str()),     l.lineal);
        glUniform1f(glGetUniformLocation(p, (b + "quadratic").c_str()),  l.cuadratica);
    }

    // Luces focales
    int nF = (int)std::min<size_t>(e.focales.size(), (size_t)MAX_LUCES_FOCALES);
    glUniform1i(glGetUniformLocation(p, "numSpotLights"), nF);
    for (int i = 0; i < nF; i++)
    {
        const LuzFocal& f = e.focales[i];
        std::string b = "spotLights[" + std::to_string(i) + "].";
        glUniform3fv(glGetUniformLocation(p, (b + "position").c_str()),   1, glm::value_ptr(f.posicion));
        glUniform3fv(glGetUniformLocation(p, (b + "direction").c_str()),  1, glm::value_ptr(f.direccion));
        glUniform3fv(glGetUniformLocation(p, (b + "ambient").c_str()),    1, glm::value_ptr(f.ambiente));
        glUniform3fv(glGetUniformLocation(p, (b + "diffuse").c_str()),    1, glm::value_ptr(f.difusa));
        glUniform3fv(glGetUniformLocation(p, (b + "specular").c_str()),   1, glm::value_ptr(f.especular));
        glUniform1f(glGetUniformLocation(p, (b + "cutOff").c_str()),      glm::cos(glm::radians(f.anguloInterno)));
        glUniform1f(glGetUniformLocation(p, (b + "outerCutOff").c_str()), glm::cos(glm::radians(f.anguloExterno)));
        glUniform1f(glGetUniformLocation(p, (b + "constant").c_str()),    f.constante);
        glUniform1f(glGetUniformLocation(p, (b + "linear").c_str()),      f.lineal);
        glUniform1f(glGetUniformLocation(p, (b + "quadratic").c_str()),   f.cuadratica);
    }
}
