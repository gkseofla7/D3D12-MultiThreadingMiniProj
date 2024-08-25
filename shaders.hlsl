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
    matrix model; // -> 16
    float4 padding[12];
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
    result.position = mul(position, model);
    int bIsNegative = 1;
    if (instanceID%2 == 1)
    {
        bIsNegative = -1;

    }
    float4x4 Transition = CreateTranslationMatrix(float3(bIsNegative * (instanceID * 2345.0) % 1.0, bIsNegative * (instanceID * 2345.0) % 1.0, bIsNegative * (instanceID * 2345.0) % 1.0));
    result.position = mul(result.position, Transition);
    result.uv = uv;
    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return g_texture.Sample(g_sampler, input.uv);
}
