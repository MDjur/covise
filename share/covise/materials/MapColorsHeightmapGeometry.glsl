#version 330 compatibility
#extension GL_ARB_texture_rectangle : enable

layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

uniform sampler2DRect heightTex;
uniform sampler2DRect dataTex;
uniform bool dataValid;
uniform vec2 size;
uniform vec2 dist;
uniform vec4 origin;

uniform vec2 off[] = vec2[4](vec2(0, 0), vec2(0, 1), vec2(1, 0), vec2(1, 1));

out vec3 N;
out vec3 V;
out float data;

float getHeight(vec2 xy)
{
    return texture2DRect(heightTex, xy + vec2(.5, .5)).r;
}

float getData(vec2 xy)
{
    return texture2DRect(dataTex, xy + vec2(.5, .5)).r;
}

vec4 pos(vec2 xy, float h)
{
    vec4 p = origin + vec4(dist * xy, h, 0.);
    return p;
}

void createVertex(vec2 xy)
{
    float h = getHeight(xy);
    gl_Position = gl_ModelViewProjectionMatrix * pos(xy, h);
    gl_ClipVertex = gl_Position;
    V = gl_Position.xyz / gl_Position.w;

    float dx = 0., dy = 0.;
    float hn = h, hs = h, he = h, hw = h;
    if (xy.x >= 1)
    {
        he = getHeight(xy - vec2(1, 0));
        dx += dist.x;
    }
    if (xy.x < size.x)
    {
        hw = getHeight(xy + vec2(1, 0));
        dx += dist.x;
    }
    if (xy.y >= 1)
    {
        hs = getHeight(xy + vec2(0, 1));
        dy += dist.y;
    }
    if (xy.y < size.y)
    {
        hn = getHeight(xy + vec2(0, 1));
        dy += dist.y;
    }
    N = normalize(gl_NormalMatrix * vec3((hw - he) / dx, (hn - hs) / dy, 1.));

    if (dataValid)
        data = getData(xy);
    else
        data = 0.;
    EmitVertex();
}

void main()
{
    vec2 xy = gl_in[0].gl_Position.xy;

    for (int i = 0; i < 4; ++i)
        createVertex(xy + off[i]);
    EndPrimitive();
}
