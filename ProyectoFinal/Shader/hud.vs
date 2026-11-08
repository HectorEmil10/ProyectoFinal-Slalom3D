#version 330 core
/*
 * hud.vs
 * Vertex Shader del HUD 2D. Recibe posiciones en pixeles (origen arriba a la
 * izquierda) y las transforma con una proyeccion ortografica.
 */
layout (location = 0) in vec2 position;

uniform mat4 ortho;

void main()
{
    gl_Position = ortho * vec4(position, 0.0, 1.0);
}
