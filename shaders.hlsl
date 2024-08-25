//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

float4x4 CreateTranslationMatrix(float3 translation)
{
    return float4x4(
        1.0f, 0.0f, 0.0f, 0.0f, // 첫 번째 행
        0.0f, 1.0f, 0.0f, 0.0f, // 두 번째 행
        0.0f, 0.0f, 1.0f, 0.0f, // 세 번째 행
        translation.x, translation.y, translation.z, 1.0f // 네 번째 행
    );
}
cbuffer SceneConstantBuffer : register(b0)
{
    matrix model[4]; // -> 16
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD;
};

Texture2D g_texture : register(t0);
SamplerState g_sampler : register(s0);

PSInput VSMain(float4 position : POSITION, float4 uv : TEXCOORD, uint instanceID : SV_InstanceID)
{
    PSInput result;
    //result.position = position;
    result.position = mul(position, model[instanceID]);
    result.uv = uv;
    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return g_texture.Sample(g_sampler, input.uv);
}
