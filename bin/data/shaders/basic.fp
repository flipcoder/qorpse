#version 120

/*varying vec3 VertexPosition;*/
varying vec3 Position;
varying vec2 Wrap;
/*varying vec2 Normal;*/

uniform sampler2D Texture;
uniform vec4 Ambient = vec4(1.0, 1.0, 1.0, 1.0);
/*uniform int Flags = 0;*/

/*#define FLAG_FOG 0x1*/

// This color key stuff could be done on the CPU, and using a separate tex
/*uniform vec4 ColorKeyLow;*/
/*uniform vec4 ColorKeyHigh;*/
/*uniform vec4 ColorReplaceLow;*/
/*uniform vec4 ColorReplaceHigh;*/

#define M_PI 3.1415926535897932384626433832795
#define M_TAU (M_PI * 2.0)

bool floatcmp(float a, float b, float e)
{
    return abs(a-b) < e;
}

bool colorcmp(vec4 a, vec4 b, float t)
{
    return floatcmp(a.r,b.r,t) &&
        floatcmp(a.g,b.g,t) &&
        floatcmp(a.b,b.b,t);
}

vec4 grayscale(vec4 c)
{
    float v = (c.r + c.g + c.b) / 3.0;
    return vec4(v,v,v, c.a);
}

vec4 evil(vec4 c)
{
    if(colorcmp(c, vec4(1.0, 0.0, 0.0, 1.0), 0.1))
        return c;
    return grayscale(c);
}

float avg(vec3 c)
{
    return (c.r + c.g + c.b) / 3.0;
}

float saturate(float v)
{
    return clamp(v, 0.0, 1.0);
}

vec3 saturate(vec3 v)
{
    vec3 r;
    r.x = saturate(v.x);
    r.y = saturate(v.y);
    r.z = saturate(v.z);
    return r;
}

void main()
{
    vec4 color = texture2D(Texture, Wrap);
    float e = 0.1; // threshold
    if(floatcmp(color.r, 1.0, e) &&
        floatcmp(color.g, 0.0, e) &&
        floatcmp(color.b, 1.0, e))
    {
        discard;
    }
    if(floatcmp(color.a, 0.0, e)) {
        discard;
    }
    
    float dist = length(Position.xy);
    /*if((Flags & FLAG_FOG) != 0)*/
    /*{*/
        /*float fog = 1.0f - sqrt(dist * dist * dist) * 0.6f;*/
        /*gl_FragColor = vec4(saturate(vec3(color.xyz * Ambient.xyz * fog)), color.a * Ambient.a);*/
    /*}*/
    /*else*/
    /*{*/
        gl_FragColor = vec4(saturate(vec3(color.xyz * Ambient.xyz)), color.a * Ambient.a);
    /*}*/
    /*gl_FragColor = vec4(saturate(vec3(color * vec4(vec3(1.5),1.0) * fog)), 1.0);*/
    /*gl_FragColor = mix(color, evil(color), 1.0-fog) * LightBrightness;*/
}

