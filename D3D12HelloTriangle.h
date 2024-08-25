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

#pragma once

#include "DXSample.h"

using namespace DirectX;

// Note that while ComPtr is used to manage the lifetime of resources on the CPU,
// it has no understanding of the lifetime of resources on the GPU. Apps must account
// for the GPU lifetime of resources to avoid destroying objects that may still be
// referenced by the GPU.
// An example of this can be found in the class method: OnDestroy().
using Microsoft::WRL::ComPtr;

class FrameResource;


struct SceneConstantBuffer {
    XMMATRIX model;// -> 16
    float padding[48];
};
static_assert((sizeof(SceneConstantBuffer) % 256) == 0, "BasicVertexConstantData size must be 256-byte aligned");

class D3D12HelloTriangle : public DXSample
{
public:
    D3D12HelloTriangle(UINT width, UINT height, std::wstring name);

    static D3D12HelloTriangle* Get() { return s_app; }

    virtual void OnInit();
    virtual void OnUpdate();
    virtual void OnRender();
    virtual void OnDestroy();
    void BeginFrame();
    void EndFrame();
private:

    struct Vertex
    {
        XMFLOAT3 position;
        XMFLOAT2 uv;
    };

    // Pipeline objects.
    CD3DX12_VIEWPORT m_viewport;
    CD3DX12_RECT m_scissorRect;
    ComPtr<IDXGISwapChain3> m_swapChain;
    ComPtr<ID3D12Device> m_device;
    ComPtr<ID3D12Resource> m_renderTargets[FrameCount];
    ComPtr<ID3D12CommandAllocator> m_commandAllocator;
    ComPtr<ID3D12CommandQueue> m_commandQueue;
    ComPtr<ID3D12RootSignature> m_rootSignature;
    ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
    ComPtr<ID3D12DescriptorHeap> m_srvHeap;
    ComPtr<ID3D12DescriptorHeap> m_cbvHeap;
    ComPtr<ID3D12PipelineState> m_pipelineState;
    ComPtr<ID3D12GraphicsCommandList> m_commandList;
    UINT m_rtvDescriptorSize;

    // App resources.
    ComPtr<ID3D12Resource> m_vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
    ComPtr<ID3D12Resource> m_IndexBuffer;
    D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;

    ComPtr<ID3D12Resource> m_texture;

    // Frame resources.
    FrameResource* m_frameResources[FrameCount];
    FrameResource* m_pCurrentFrameResource;
    int m_currentFrameResourceIndex;

    // Synchronization objects.
    UINT m_frameIndex;
    HANDLE m_fenceEvent;
    ComPtr<ID3D12Fence> m_fence;
    UINT64 m_fenceValues;
    UINT64 m_fenceValue = 0;

    HANDLE m_workerBeginRenderFrame[NumContexts];
    HANDLE m_workerFinishedRenderFrame[NumContexts];
    HANDLE m_threadHandles[NumContexts];

    // Singleton object so that worker threads can share members.
    static D3D12HelloTriangle* s_app;
    struct ThreadParameter
    {
        int threadIndex;
    };
    ThreadParameter m_threadParameters[NumContexts];
private:
    void LoadPipeline();
    void LoadAssets();
    void LoadContexts();
    //void PopulateCommandList();
    void WaitForGpu();

    void RestoreD3DResources();
    void ReleaseD3DResources();
    void WorkerThread(int threadIndex);
    void SetCommonPipelineState(ID3D12GraphicsCommandList* pCommandList);

    // Support
    void ReadImage(const std::string filename, std::vector<uint8_t>& image,
        int& width, int& height);
};
