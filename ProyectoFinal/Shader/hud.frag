#version 330 core
/*
 * hud.frag
 * Fragment Shader del HUD 2D. Color plano con canal alfa (permite el
 * desvanecimiento gradual de los mensajes).
 */
out vec4 FragColor;

uniform vec4 color;

void main()
{
    FragColor = color;
}
