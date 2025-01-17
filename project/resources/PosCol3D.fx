//-----------------
// Matrices
//-----------------
float4x4 gWorldViewProj : WorldViewProjection;
float4x4 gWorld : World;

//-----------------
// Textures
//-----------------
Texture2D gDiffuseMap : DiffuseMap;
Texture2D gNormalMap : NormalMap;
Texture2D gSpecularMap : SpecularMap;
Texture2D gGlossinessMap : GlossinessMap;

//-----------------
// Light Direction
//-----------------
//float3 gLightDir : LightDir;
// Hardcode for now: 
float3 gLightDir = { 0.577f, -0.577f, 0.577f };

//-----------------
// Camera
//-----------------
float3 gCamera : Camera;

//-----------------
// Handy Variables
//-----------------
float gPI = 3.14159265358979323846f;
float gLightIntensity = 7.0f;
float gShininess = 25.f;

//-----------------
// Sampler States
//-----------------
SamplerState samPoint
{
    Filter = MIN_MAG_MIP_POINT; // options are: POINT, LINEAR, ANISOTROPIC
    AddressU = Wrap; // options are: WRAP, MIRROR, CLAMP, BORDER
    AddressV = Wrap;
};

SamplerState samLinear
{
    Filter = MIN_MAG_MIP_LINEAR; // options are: POINT, LINEAR, ANISOTROPIC
    AddressU = Wrap; // options are: WRAP, MIRROR, CLAMP, BORDER
    AddressV = Wrap;
};

SamplerState samAnisotropic
{
    Filter = ANISOTROPIC; // options are: POINT, LINEAR, ANISOTROPIC
    AddressU = Wrap; // options are: WRAP, MIRROR, CLAMP, BORDER
    AddressV = Wrap;
};

//-----------------
// input/output structs
//-----------------
struct VS_INPUT
{
    float3 Position : POSITION;
    float3 Color : COLOR;
    float2 TexCoord : TEXCOORD;
    float3 Normal : NORMAL;
    float3 Tangent : TANGENT;
};

struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
    float3 Color : COLOR;
    float2 TexCoord : TEXCOORD;
    float3 Normal : NORMAL;
    float3 Tangent : TANGENT;
};

//-----------------
// Vertex Shader
//-----------------
VS_OUTPUT VS(VS_INPUT input)
{
    VS_OUTPUT output = (VS_OUTPUT) 0;
    output.Position = mul(float4(input.Position, 1.f), gWorldViewProj);
    output.Color = input.Color;
    output.TexCoord = input.TexCoord;
    output.Normal = mul(normalize(input.Normal), (float3x3) gWorld);
    output.Tangent = mul(normalize(input.Tangent), (float3x3) gWorld);
    return output;
}

//-----------------
// Pixel Shader
//-----------------
float4 PSPoint(VS_OUTPUT input) : SV_TARGET
{
    return gDiffuseMap.Sample(samPoint, input.TexCoord);
}

float4 PSLinear(VS_OUTPUT input) : SV_TARGET
{
    return gDiffuseMap.Sample(samLinear, input.TexCoord);
}
float4 PSAnisotropic(VS_OUTPUT input) : SV_TARGET
{
    return gDiffuseMap.Sample(samAnisotropic, input.TexCoord);
}

//-----------------
// Techniques
//-----------------
technique11 PointTechnique
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PSPoint()));
    }
}
technique11 LinearTechnique
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PSLinear()));
    }
}
technique11 AnisotropicTechnique
{
    pass P0
    {
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetGeometryShader(NULL);
        SetPixelShader(CompileShader(ps_5_0, PSAnisotropic()));
    }
}


//-----------------
// Calculate Shaders
//-----------------
//float3 CalculateDiffuse(float3 normal, float3 lightDir, float3 diffuseMap)
//{
//    float diffuse = saturate(dot(normal, lightDir));
//    return diffuseMap * diffuse;
//}
//
//float3 CalculateNormal(VS_OUTPUT input)
//{
//    float3 binormal = cross(input.Normal, input.Tangent);
//    float4x4 tangentSpaceAxis = float4x4(float4(input.Tangent, 0.0f), float4(binormal, 0.0f), float4(input.Normal, 0.0), float4(0.0f, 0.0f, 0.0f, 1.0f));
//
//    float3 sampledNormal = gNormalMap.Sample(samPoint, input.TexCoord).rgb;
//    sampledNormal = 2 * sampledNormal - float3(1.f, 1.f, 1.f);
//    
//    return mul(float4(sampledNormal, 0.0f), tangentSpaceAxis).xyz;
//}
//
//float3 CalculateSpecular(float3 normal, float3 lightDir, float3 viewDir, float3 specularMap)
//{
//    float3 halfVector = normalize(lightDir + viewDir);
//    float specular = pow(saturate(dot(normal, halfVector)), gShininess);
//    return specularMap * specular;
//}
//
//float3 CalculateGlossiness(float3 normal, float3 lightDir, float3 viewDir, float3 glossinessMap)
//{
//    float3 halfVector = normalize(lightDir + viewDir);
//    float glossiness = pow(saturate(dot(normal, halfVector)), gShininess);
//    return glossinessMap * glossiness;
//}
