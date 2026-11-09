#version 330 core
/*
 * lighting.frag (versión con arreglos de luces)
 * Iluminación Blinn-Phong con una luz direccional, hasta 8 luces puntuales,
 * hasta 8 luces focales y niebla exponencial. La cantidad de luces activas
 * se indica desde la aplicación con numPointLights y numSpotLights.
 */

struct Material
{
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float shininess;
};

struct DirLight
{
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct PointLight
{
    vec3 position;
    float constant;
    float linear;
    float quadratic;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

struct SpotLight
{
    vec3 position;
    vec3 direction;
    float cutOff;
    float outerCutOff;
    float constant;
    float linear;
    float quadratic;
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

#define MAX_POINT_LIGHTS 12
#define MAX_SPOT_LIGHTS  8

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

out vec4 color;

uniform vec3 viewPos;
uniform DirLight dirLight;
uniform int numPointLights;
uniform int numSpotLights;
uniform PointLight pointLights[MAX_POINT_LIGHTS];
uniform SpotLight spotLights[MAX_SPOT_LIGHTS];
uniform Material material;
uniform sampler2D texture_diffuse;
uniform int transparency;

// Uniforms para el efecto de niebla (Fog) y clima
uniform float fogDensity;
uniform vec3 fogColor;

// Prototipos de funciones
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir);

void main()
{
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);

    // 1. Luz direccional principal
    vec3 result = CalcDirLight(dirLight, norm, viewDir);

    // 2. Luces posicionales activas
    for (int i = 0; i < numPointLights; i++)
    {
        result += CalcPointLight(pointLights[i], norm, FragPos, viewDir);
    }

    // 3. Luces focales activas
    for (int i = 0; i < numSpotLights; i++)
    {
        result += CalcSpotLight(spotLights[i], norm, FragPos, viewDir);
    }

    // Muestreo de textura UV
    vec4 texColor = texture(texture_diffuse, TexCoords);
    if (transparency == 1 && texColor.a < 0.1)
        discard;

    vec3 litColor = result * vec3(texColor);

    // 4. Efecto de Niebla (Fog) exponencial por distancia a la cámara
    float distanceToCam = length(viewPos - FragPos);
    float fogFactor = exp(-pow(distanceToCam * fogDensity, 2.0));
    fogFactor = clamp(fogFactor, 0.0, 1.0);

    vec3 finalColor = mix(fogColor, litColor, fogFactor);
    color = vec4(finalColor, texColor.a);
}

// Cálculo de Luz Direccional con Blinn-Phong
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir)
{
    vec3 lightDir = normalize(-light.direction);

    // Difusa
    float diff = max(dot(normal, lightDir), 0.0);

    // Especular (Blinn-Phong: vector intermedio entre lightDir y viewDir)
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);

    vec3 ambient = light.ambient * vec3(texture(texture_diffuse, TexCoords));
    vec3 diffuse = light.diffuse * diff * vec3(texture(texture_diffuse, TexCoords));
    vec3 specular = light.specular * spec * material.specular;

    return (ambient + diffuse + specular);
}

// Cálculo de Luces Puntuales con Blinn-Phong
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);

    // Difusa
    float diff = max(dot(normal, lightDir), 0.0);

    // Especular (Blinn-Phong)
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);

    // Atenuación por distancia
    float distance = length(light.position - fragPos);
    float attenuation = 1.0f / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    vec3 ambient = light.ambient * vec3(texture(texture_diffuse, TexCoords));
    vec3 diffuse = light.diffuse * diff * vec3(texture(texture_diffuse, TexCoords));
    vec3 specular = light.specular * spec * material.specular;

    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;

    return (ambient + diffuse + specular);
}

// Cálculo de Luz Focal (SpotLight) con Blinn-Phong
vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir)
{
    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);

    // Especular (Blinn-Phong)
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), material.shininess);

    float distance = length(light.position - fragPos);
    float attenuation = 1.0f / (light.constant + light.linear * distance + light.quadratic * (distance * distance));

    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.cutOff - light.outerCutOff;
    float intensity = clamp((theta - light.outerCutOff) / epsilon, 0.0, 1.0);

    vec3 ambient = light.ambient * vec3(texture(texture_diffuse, TexCoords));
    vec3 diffuse = light.diffuse * diff * vec3(texture(texture_diffuse, TexCoords));
    vec3 specular = light.specular * spec * material.specular;

    ambient *= attenuation * intensity;
    diffuse *= attenuation * intensity;
    specular *= attenuation * intensity;

    return (ambient + diffuse + specular);
}
