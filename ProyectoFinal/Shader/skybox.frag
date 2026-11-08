#version 330 core
/*
 * skybox.frag
 * Fragment Shader del cielo. Muestrea un cubemap (paisaje montañoso) y le
 * aplica: tinte de atardecer, resplandor del sol y mezcla con la niebla.
 * Si el cubemap no pudo cargarse (hayTextura = false) genera un degradado
 * procedural de atardecer para que la escena nunca quede sin cielo.
 */
in vec3 TexCoords;
out vec4 color;

uniform samplerCube skybox;
uniform bool  hayTextura;
uniform vec3  sunDir;      // Direccion HACIA el sol (normalizada)
uniform vec3  sunColor;    // Color calido del sol
uniform vec3  fogColor;    // Color de la niebla actual
uniform float hazeFactor;  // 0 = sin bruma, 1 = cielo totalmente cubierto
uniform vec3  skyTint;     // Tinte multiplicativo del atardecer

void main()
{
    vec3 dir = normalize(TexCoords);
    vec3 sky;

    if (hayTextura)
    {
        // Se invierte X al muestrear para corregir el efecto espejo del cubemap
        sky = texture(skybox, vec3(-dir.x, dir.y, dir.z)).rgb * skyTint;
    }
    else
    {
        // Degradado procedural: horizonte naranja -> cenit azul/violeta
        vec3 horizonte = vec3(1.00, 0.55, 0.30);
        vec3 cenit     = vec3(0.16, 0.20, 0.42);
        float h = clamp(dir.y, 0.0, 1.0);
        sky = mix(horizonte, cenit, smoothstep(0.0, 0.75, h));
        // Bajo el horizonte se funde con la niebla
        sky = mix(sky, fogColor, 1.0 - smoothstep(-0.35, 0.0, dir.y));
    }

    // Resplandor del sol (nucleo + halo amplio)
    float s = max(dot(dir, normalize(sunDir)), 0.0);
    float intensidadSol = hayTextura ? 0.6 : 1.0;
    sky += sunColor * (pow(s, 256.0) * 1.5 + pow(s, 8.0) * 0.25) * intensidadSol;

    // Bruma: mas fuerte cerca del horizonte, total con niebla densa
    float haze = clamp(hazeFactor * (1.0 - 0.5 * clamp(dir.y, 0.0, 1.0)), 0.0, 1.0);
    sky = mix(sky, fogColor, haze);

    color = vec4(sky, 1.0);
}
