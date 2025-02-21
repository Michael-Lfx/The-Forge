/*
 * Copyright (c) 2018-2021 Confetti Interactive Inc.
 *
 * This is a part of Aura.
 *
 * This file(code) is licensed under a
 * Creative Commons Attribution-NonCommercial 4.0 International License
 *
 *   (https://creativecommons.org/licenses/by-nc/4.0/legalcode)
 *
 * Based on a work at https://github.com/ConfettiFX/The-Forge.
 * You may not use the material for commercial purposes.
*/

#ifndef SHADING_H
#define SHADING_H

#include "shader_defs.h"

#define SHADOW_PCF 0
#define SHADOW_ESM 1
#define SHADOW SHADOW_PCF
#define NUM_SHADOW_SAMPLES 32

STATIC const float NUM_SHADOW_SAMPLES_INV = 0.03125f;

#if NUM_SHADOW_SAMPLES == 16
STATIC const float shadowSamples[NUM_SHADOW_SAMPLES * 2] =
{
	-0.17466460f, -0.7913184f,
	 0.08863912f, -0.898169f,
	 0.17484090f, -0.5252063f,
	 0.45293190f, -0.384986f,
	 0.38576580f, -0.9096935f,
	 0.76801100f, -0.4906538f,
	 0.69465550f,  0.1605866f,
	 0.79865440f,  0.5325912f,
	 0.28476930f,  0.2293397f,
	-0.43578930f, -0.3808875f,
	-0.13912900f,  0.2394065f,
	 0.42878540f,  0.899425f,
	-0.69243230f, -0.2203967f,
	-0.26117240f,  0.7359962f,
	-0.85010400f,  0.1263935f,
	-0.53809670f,  0.6264234f
};
#else
STATIC const float shadowSamples[NUM_SHADOW_SAMPLES * 2] =
{
	-0.17466460f, -0.7913184f,
	-0.12979200f, -0.4477116f,
	 0.08863912f, -0.898169f,
	-0.58914990f, -0.6781639f,
	 0.17484090f, -0.5252063f,
	 0.64833250f, -0.752117f,
	 0.45293190f, -0.384986f,
	 0.09757467f, -0.1166954f,
	 0.38576580f, -0.9096935f,
	 0.56130580f, -0.1283066f,
	 0.76801100f, -0.4906538f,
	 0.84994380f, -0.220937f,
	 0.69465550f,  0.1605866f,
	 0.96142970f,  0.05975229f,
	 0.79865440f,  0.5325912f,
	 0.45139650f,  0.5592551f,
	 0.28476930f,  0.2293397f,
	-0.21189960f, -0.1609127f,
	-0.43578930f, -0.3808875f,
	-0.46626720f, -0.05288446f,
	-0.13912900f,  0.2394065f,
	 0.17818530f,  0.5254948f,
	 0.42878540f,  0.899425f,
	 0.12893490f,  0.8724155f,
	-0.69243230f, -0.2203967f,
	-0.48997000f,  0.2795907f,
	-0.26117240f,  0.7359962f,
	-0.77041720f,  0.4233134f,
	-0.85010400f,  0.1263935f,
	-0.83452670f, -0.4991361f,
	-0.53809670f,  0.6264234f,
	-0.97693120f, -0.1550569f
};
#endif

STATIC const float PI = 3.1415926535897932384626422832795028841971f;

float2 LightingFunGGX_FV(float dotLH, float roughness)
{
	float alpha = roughness * roughness;

	//F
	float F_a, F_b;
	float dotLH5 = pow(saturate(1.0f - dotLH), 5.0f);
	F_a = 1.0f;
	F_b = dotLH5;

	//V
	float vis;
	float k  = alpha * 0.5f;
	float k2 = k * k;
	float invK2 = 1.0f - k2;
	vis = 1.0f / (dotLH*dotLH*invK2 + k2);

	return float2((F_a - F_b)*vis, F_b*vis);
}

float LightingFuncGGX_D(float dotNH, float roughness)
{
	float alpha = roughness * roughness;
	float alphaSqr = alpha * alpha;
	float denom = max(dotNH * dotNH * (alphaSqr - 1.0f) + 1.0f, 0.001f);

	return alphaSqr / (PI*denom*denom);
}

float3 GGX_Spec(float3 Normal, float3 HalfVec, float Roughness, float3 SpecularColor, float2 paraFV)
{
	float NoH = saturate(dot(Normal, HalfVec));
	float NoH2 = NoH * NoH;
	float NoH4 = NoH2 * NoH2;
	float D = LightingFuncGGX_D(NoH4, Roughness);
	float2 FV_helper = paraFV;

	//float3 F0 = SpecularColor;
	float3 FV = SpecularColor * FV_helper.x + float3(FV_helper.y, FV_helper.y, FV_helper.y);

	return D * FV;
}

float3 FresnelSchlickRoughness(float cosTheta, float3 F0, float roughness)
{
	//return F0 + (max(float3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
	float3 ret = f3(0.0f);
	float powTheta = pow(1.0f - cosTheta, 5.0f);
	float oneMinusRough = float(1.0f - roughness);

	ret.x = F0.x + (max(oneMinusRough, F0.x) - F0.x) * powTheta;
	ret.y = F0.y + (max(oneMinusRough, F0.y) - F0.y) * powTheta;
	ret.z = F0.z + (max(oneMinusRough, F0.z) - F0.z) * powTheta;

	return ret;
}

// GGX / Trowbridge-Reitz
// [Walter et al. 2007, "Microfacet models for refraction through rough surfaces"]
float D_GGX(float a2, float NoH)
{
	float d = (NoH * a2 - NoH) * NoH + 1.0f; // 2 mad
	return a2 / (PI*d*d);                    // 4 mul, 1 rcp
}

// Appoximation of joint Smith term for GGX
// [Heitz 2014, "Understanding the Masking-Shadowing Function in Microfacet-Based BRDFs"]
float Vis_SmithJointApprox(float a2, float NoV, float NoL)
{
	float a = sqrt(a2);
	float Vis_SmithV = NoL * (NoV * (1.0f - a) + a);
	float Vis_SmithL = NoV * (NoL * (1.0f - a) + a);
	return 0.5f * rcp(max(Vis_SmithV + Vis_SmithL, 0.001f));
}

float Pow5(float x)
{
	float xx = x * x;
	return xx * xx * x;
}

// [Schlick 1994, "An Inexpensive BRDF Model for Physically-Based Rendering"]
float3 F_Schlick(float3 SpecularColor, float VoH)
{
	float Fc = Pow5(1.0f - VoH);		// 1 sub, 3 mul
										//return Fc + (1 - Fc) * SpecularColor;		// 1 add, 3 mad

	// Anything less than 2% is physically impossible and is instead considered to be shadowing
	//return saturate(50.0f * SpecularColor.g) * Fc + (1.0f - Fc) * SpecularColor;
	return saturate(SpecularColor.g) * Fc + (1.0f - Fc) * SpecularColor;

}

float3 SpecularGGX(float Roughness, inout(float3) SpecularColor, float NoL, float Nov, float NoH, float VoH)
{
	float a = Roughness * Roughness;
	float a2 = a * a;
	//float Energy = EnergyNormalization(a2, Context.VoH, AreaLight);

	// Generalized microfacet specular
	//float D = D_GGX(a2, Context.NoH) * Energy;
	float D = D_GGX(a2, NoH);
	float Vis = Vis_SmithJointApprox(a2, Nov, NoL);
	SpecularColor = F_Schlick(SpecularColor, VoH);

	return (D * Vis) * SpecularColor;
}

float3 PBR(float NoL, float NoV, float3 LightVec, float3 ViewVec,
	float3 HalfVec, float3 NormalVec, float3 ReflectVec, inout(float3) albedo,
	inout(float3) specColor, float Roughness, float Metallic, bool isBackface, float shadowFactor)
{
	//float3 diffuseTerm = float3(0.0f, 0.0f, 0.0f);
	float3 specularTerm = f3(0.0f);

	//float LoH = clamp(dot(LightVec, HalfVec), 0.0f, 1.0f);

	float NoH = saturate(dot(NormalVec, HalfVec));
	float VoH = saturate(dot(ViewVec, HalfVec));

	//DIFFUSE
	specColor = lerp(0.08f * specColor.rgb, albedo, Metallic);
	albedo = (1.0f - Metallic) * albedo;

	//SPECULAR
	if (!isBackface)
		specularTerm = lerp(f3(0.0f), SpecularGGX(Roughness, specColor, NoL, NoV, NoH, VoH), shadowFactor);

	return (albedo + specularTerm);
}

float3 PBR(float NoL, float NoV, float3 LightVec, float3 ViewVec,
	float3 HalfVec, float3 NormalVec, float3 ReflectVec, inout(float3) albedo,
	inout(float3) specColor, float Roughness, float Metallic, bool isBackface)
{
	//float3 diffuseTerm = float3(0.0f, 0.0f, 0.0f);
	float3 specularTerm = f3(0.0f);

	//float LoH = clamp(dot(LightVec, HalfVec), 0.0f, 1.0f);

	float NoH = saturate(dot(NormalVec, HalfVec));
	float VoH = saturate(dot(ViewVec, HalfVec));

	//DIFFUSE
	specColor = lerp(0.08f * specColor.rgb, albedo, Metallic);
	albedo = (1.0f - Metallic) * albedo;

	//SPECULAR
	if (!isBackface)
		specularTerm = SpecularGGX(Roughness, specColor, NoL, NoV, NoH, VoH);

	return (albedo + specularTerm);
}

float random(float3 seed, float3 freq)
{
	// project seed on random constant vector
	float dt = dot(floor(seed * freq), float3(53.1215f, 21.1352f, 9.1322f));
	// return only the fractional part
	return frac(sin(dt) * 2105.2354f);
}

float3 calculateSpecular(float3 specularColor, float3 camPos, float3 pixelPos, float3 normalizedDirToLight, float3 normal)
{
	float3 viewVec = normalize(camPos - pixelPos);
	float3 halfVec = normalize(viewVec + normalizedDirToLight);
	float specIntensity = 128.0f;
	float specular = pow(saturate(dot(halfVec, normal)), specIntensity);

	return specularColor * specular;
}

float linearDepth(float depth)
{
	//float f = 2000.0;
	//float n = 10.0;
	return (20.0f) / (2010.0f - depth * (1990.0f));
}


float3 calculateIllumination(
	float3 normal,
	float3 ViewVec,
	float3 HalfVec,
	float3 ReflectVec,
	float NoL,
	float NoV,
	float3 camPos, float esmControl,
	float3 normalizedDirToLight, float4 posLS, float3 position,
	Tex2D(float) shadowMap,
	float3 albedo,
	float3 specularColor,
	float Roughness,
	float Metallic,
	SamplerState sh,
	bool isBackface,
	float isPBR,
	float shadowFactor)
{
	// Project pixel position post-perspective division coordinates and map to [0..1] range to access the shadow map
	posLS /= posLS.w;
	posLS.y *= -1.0f;
	posLS.xy = posLS.xy * 0.5 + f2(0.5f);
	shadowFactor = 0.0f;

	float2 Gaps = float2(0.0009765625f, 0.0009765625f);
	float2 HalfGaps = float2(0.00048828125f, 0.00048828125f);

	posLS.xy += HalfGaps;

	if (all(GreaterThan(posLS.xy, 0.0f)) && all(LessThan(posLS.xy, 1.0f)))
	{
#if SHADOW == SHADOW_PCF
		// waste of shader cycles
		// Perform percentage-closer shadows with randomly rotated poisson kernel
		float shadowFilterSize = 0.0016f;
		float angle = random(position, f3(20.0f));
		float s = sin(angle);
		float c = cos(angle);

		for (int i = 0; i < NUM_SHADOW_SAMPLES; ++i)
		{
			float2 offset = float2(shadowSamples[i * 2], shadowSamples[i * 2 + 1]);
			offset = float2(offset.x * c + offset.y * s, offset.x * -s + offset.y * c);
			offset *= shadowFilterSize;
			float shadowMapValue = SampleLvlTex2D(shadowMap, sh, posLS.xy + offset, 0).x;
			shadowFactor += (shadowMapValue < posLS.z - 0.002f ? 0.0f : 1.0f);
		}
		shadowFactor *= NUM_SHADOW_SAMPLES_INV;
#elif SHADOW == SHADOW_ESM
		// ESM
		float CShadow  = linearDepth(SampleLvlTex2D(shadowMap, sh, posLS.xy, 0).x);
		float LShadow  = linearDepth(SampleLvlTex2D(shadowMap, sh, posLS.xy + float2(-Gaps.x, 0.0f), 0).x);
		float RShadow  = linearDepth(SampleLvlTex2D(shadowMap, sh, posLS.xy + float2( Gaps.x, 0.0f), 0).x);
		float LTShadow = linearDepth(SampleLvlTex2D(shadowMap, sh, posLS.xy + float2(-Gaps.x, Gaps.y), 0).x);
		float RTShadow = linearDepth(SampleLvlTex2D(shadowMap, sh, posLS.xy + float2( Gaps.x, Gaps.y), 0).x);
		float TShadow  = linearDepth(SampleLvlTex2D(shadowMap, sh, posLS.xy + float2(0.0f,  Gaps.y), 0).x);
		float BShadow  = linearDepth(SampleLvlTex2D(shadowMap, sh, posLS.xy + float2(0.0f, -Gaps.y), 0).x);
		float LBShadow = linearDepth(SampleLvlTex2D(shadowMap, sh, posLS.xy + float2(-Gaps.x, -Gaps.y), 0).x);
		float RBShadow = linearDepth(SampleLvlTex2D(shadowMap, sh, posLS.xy + float2(Gaps.x,  -Gaps.y), 0).x);

		float nbShadow = LShadow + RShadow + TShadow + BShadow + LTShadow + RTShadow + LBShadow + RBShadow;

		float avgShadowDepthSample = (CShadow + nbShadow) / 9.0f;
		//float avgShadowDepthSample = CShadow;

		float linearD = linearDepth(posLS.z);
		float esm = exp(-esmControl * linearD) * exp(esmControl * avgShadowDepthSample);

		shadowFactor = saturate(esm);
#endif
	}

	float3 finalColor;

	if (isPBR > 0.5f)
	{
		finalColor = PBR(NoL, NoV, -normalizedDirToLight, ViewVec, HalfVec, normal, ReflectVec, albedo, specularColor, Roughness, Metallic, isBackface, shadowFactor);
	}
	else
	{
		specularColor = calculateSpecular(specularColor, camPos, position, -normalizedDirToLight, normal);
		finalColor    = albedo + lerp(f3(0.0f), specularColor, shadowFactor);
	}

	finalColor *= shadowFactor;

	return finalColor;
}

float3 pointLightShade(
	float3 normal,
	float3 ViewVec,
	float3 HalfVec,
	float3 ReflectVec,
	float NoL,
	float NoV,
	float3 lightPos,
	float3 lightCol,
	float3 camPos,
	float3 normalizedDirToLight, float4 posLS, float3 position,
	float3 albedo,
	float3 specularColor,
	float Roughness,
	float Metallic,
	bool isBackface,
	float isPBR)
{
	float3 lVec = (lightPos - position) * (1.0f / LIGHT_SIZE);
	float atten = saturate(1.0f - dot(lVec, lVec));

	float3 finalColor;

	if (isPBR > 0.5f)
	{
		finalColor = PBR(NoL, NoV, -normalizedDirToLight, ViewVec, HalfVec, normal, ReflectVec, albedo, specularColor, Roughness, Metallic, isBackface);
	}
	else
	{
		specularColor = calculateSpecular(specularColor, camPos, position, -normalizedDirToLight, normal);

		finalColor = albedo + specularColor;
	}

	return lightCol * finalColor * atten;
}

#endif // SHADING_H
