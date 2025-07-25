#version 450
const float PI = 3.14159265359;

layout (binding = 0) uniform sampler2D samplerPosition;
layout (binding = 1) uniform sampler2D samplerNormal;
layout (binding = 2) uniform sampler2D samplerAlbedo;
layout (binding = 3) uniform sampler2D samplerSSAO;
layout (binding = 4) uniform sampler2D samplerMaterials;
layout (binding = 5) uniform samplerCube samplerReflectiveCubeMap;
layout (binding = 6) uniform sampler2D samplerReflectionMask;

layout(location = 0) in vec2 inTexCoord;           

struct LightData{
    vec4    positionAndLightType;
    vec4    colorAndAmbientStrength;
    vec4    CameraPositionAndLightIntensity;
    mat4    LightProjectionViewMatrix;

};
layout (binding = 7) uniform LightUniformBuffer {
   
   LightData lights[3];
};

layout (location = 0) out vec4 outFragcolor;

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}  

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float safeRoughness = max(roughness, 0.05);
    float a      = safeRoughness*safeRoughness;
    float a2     = a*a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;
	
    float num   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float num   = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2  = GeometrySchlickGGX(NdotV, roughness);
    float ggx1  = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

void main() {

   //Defaults/////////////////////////////
    vec3  LightDir = vec3(1,1,1);

    const float Constant   = 1.0;
    const float Linear     = 0.09;
    const float Quadratic  = 0.032;

    vec3 radiance      = vec3(0.0);
    vec3 totalLighting = vec3(0.0);
    ///////////////////////////////////////

    vec3  WorldPos     = texture(samplerPosition,inTexCoord).rgb;
    vec3  Normal       = normalize(texture(samplerNormal,inTexCoord).rgb);
    vec3  Albedo       = texture(samplerAlbedo,inTexCoord).rgb;
    float SSAO         = texture(samplerSSAO,inTexCoord).r;
    float Metallic     = texture(samplerMaterials,inTexCoord).r;
    float Roughness    = texture(samplerMaterials,inTexCoord).g;
    float AO           = texture(samplerMaterials,inTexCoord).b;
    vec2  ReflectionMask     = texture(samplerReflectionMask,inTexCoord).rg;

    float ambientOcclusion = AO * SSAO;

    vec3  ViewDir    = normalize(lights[0].CameraPositionAndLightIntensity.xyz -  WorldPos);


    //Reflection Calc
    float mipLevel = Roughness * float(6);
    vec3 cR = reflect (-ViewDir, normalize(Normal));
    vec3 Reflection = texture(samplerReflectiveCubeMap, cR,mipLevel).rgb;

    vec3 ambientvalue = vec3(0.25);
    vec3 Ambient = Albedo * ambientvalue * ambientOcclusion;

  for (int i = 0; i < 3; i++) {

     float Attenuation = 1.0;

     LightData light = lights[i];

     vec3 Lo      = vec3(0.0);


      if(light.positionAndLightType.w == 0){

         LightDir = normalize(-light.positionAndLightType.xyz);
         radiance = light.colorAndAmbientStrength.rgb ;

       }
      else if (light.positionAndLightType.w == 1){
               
               vec3 LightPos     = light.positionAndLightType.xyz;
               LightDir          = normalize(LightPos - WorldPos);
               float Distance    = length(LightPos -  WorldPos);
               Attenuation       = 1.0 / (Constant + Linear * Distance + Quadratic * (Distance * Distance));
               radiance          = light.colorAndAmbientStrength.rgb * Attenuation;
    }  

    vec3 F0          = vec3(0.04); 
         F0          = mix(F0, Albedo, Metallic);
    vec3 halfwayDir  = normalize(LightDir + ViewDir);
    
    vec3 F    = fresnelSchlick(max(dot(halfwayDir, ViewDir), 0.0), F0);//Calculates how much light is reflected vs. refracted on a surface based on the view angle.
    float NDF = DistributionGGX(Normal, halfwayDir, Roughness); //describes how microfacet normal are distributed on a rough surface       
    float G   = GeometrySmith(Normal, ViewDir, LightDir, Roughness);// models shadowing and masking

    vec3 numerator    = NDF * G * F;
    float denominator = 4.0 * max(dot(Normal, ViewDir), 0.0) * max(dot(Normal, LightDir), 0.0)  + 0.0001;
    vec3 specular = (numerator / denominator) ;
    
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
  
    kD *= 1.0 - Metallic;	

     float NdotL = max(dot(Normal, LightDir), 0.0);        
     Lo += (kD * Albedo / PI + specular) * radiance * NdotL;


    totalLighting += Lo * light.CameraPositionAndLightIntensity.w;  
  }
  
  vec3 finalColor;

  if(ReflectionMask.x == 1){

    // Compute Fresnel for reflection
   vec3 F0 = vec3(0.04);
   F0 = mix(F0, Albedo, Metallic);
   float NdotV = max(dot(Normal, ViewDir), 0.0);
   vec3 F = fresnelSchlick(NdotV, F0);
   
   // Add environment reflection with Fresnel weighting
   vec3 envSpecular = Reflection * F;
   
    finalColor = Ambient + totalLighting + envSpecular;
  }else{
  
     finalColor = Ambient + totalLighting;
  }

   outFragcolor = vec4(finalColor, 1.0);
}