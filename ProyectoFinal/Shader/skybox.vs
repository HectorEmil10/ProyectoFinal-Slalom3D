#version 330 core
/*
 * skybox.vs
 * Vertex Shader del cielo (skybox). Se elimina la traslacion de la matriz de
 * vista desde la aplicacion; aqui se fuerza w = depth para que el cielo
 * quede siempre en el plano lejano (z/w = 1.0).
 */
layout (location = 0) in vec3 position;

out vec3 TexCoords;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    TexCoords = position;
    vec4 pos = projection * view * vec4(position, 1.0);
    gl_Position = pos.xyww;
}
