#include "stdafx.h"
#include "FrameResource.h"


FrameResource::FrameResource(ID3D12Device* pDevice, ID3D12PipelineState* pPso, ID3D12DescriptorHeap* pCbvSrvHeap, D3D12_VIEWPORT* pViewport, UINT frameResourceIndex) :
    m_fenceValue(0),
    m_pipelineState(pPso)
{
    for (UINT i = 0; i < CommandListCount; i++)
    {
        ThrowIfFailed(pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocators[i])));
        ThrowIfFailed(pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocators[i].Get(), m_pipelineState.Get(), IID_PPV_ARGS(&m_commandLists[i])));
        NAME_D3D12_OBJECT_INDEXED(m_commandLists, i);
        // Close these command lists; don't record into them for now.
        ThrowIfFailed(m_commandLists[i]->Close());
    }

    for (UINT i = 0; i < NumContexts; i++)
    {
        // Create command list allocators for worker threads. One alloc is 
        ThrowIfFailed(pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_sceneCommandAllocators[i])));

        ThrowIfFailed(pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_sceneCommandAllocators[i].Get(), m_pipelineState.Get(), IID_PPV_ARGS(&m_sceneCommandLists[i])));

        NAME_D3D12_OBJECT_INDEXED(m_sceneCommandLists, i);

        // Close these command lists; don't record into them for now. We will 
        // reset them to a recording state when we start the render loop.
        ThrowIfFailed(m_sceneCommandLists[i]->Close());
    }

    // Get a handle to the start of the descriptor heap then offset it 
    // based on the existing textures and the frame resource index. Each 
    // frame has 1 SRV (shadow tex) and 2 CBVs.
    const UINT textureCount = 1;    // Diffuse + normal textures near the start of the heap.  Ideally, track descriptor heap contents/offsets at a higher level.
    const UINT cbvSrvDescriptorSize = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    CD3DX12_CPU_DESCRIPTOR_HANDLE cbvSrvCpuHandle(pCbvSrvHeap->GetCPUDescriptorHandleForHeapStart());
    CD3DX12_GPU_DESCRIPTOR_HANDLE cbvSrvGpuHandle(pCbvSrvHeap->GetGPUDescriptorHandleForHeapStart());
    // �ؽ�ó ���� ��� + Frame ������ ��� �����(�ϳ� �� �ִٰ� �����ѵ��ѵ�,,)
    cbvSrvCpuHandle.Offset(textureCount + (frameResourceIndex), cbvSrvDescriptorSize);
    cbvSrvGpuHandle.Offset(textureCount + (frameResourceIndex), cbvSrvDescriptorSize);
    
    // Create the constant buffers.
    {
        const UINT constantBufferSize = (sizeof(SceneConstantBuffer) + (D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1)) & ~(D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT - 1); // must be a multiple 256 bytes
        ThrowIfFailed(pDevice->CreateCommittedResource(
            &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(constantBufferSize),
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&m_sceneConstantBuffer)));

        // Map the constant buffers and cache their hseap pointers.
        CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
        ThrowIfFailed(m_sceneConstantBuffer->Map(0, &readRange, reinterpret_cast<void**>(&mp_sceneConstantBufferWO)));

        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
        cbvDesc.SizeInBytes = constantBufferSize;

        // Describe and create the scene constant buffer view (CBV) and 
        // cache the GPU descriptor handle.
        cbvDesc.BufferLocation = m_sceneConstantBuffer->GetGPUVirtualAddress();
        pDevice->CreateConstantBufferView(&cbvDesc, cbvSrvCpuHandle);
        m_sceneCbvHandle = cbvSrvGpuHandle;
    }

    // Batch up command lists for execution later.
    {
        const UINT batchSize = _countof(m_sceneCommandLists) + 2;
        m_batchSubmit[0] = m_commandLists[CommandListPre].Get();
        memcpy(m_batchSubmit + 1, m_sceneCommandLists, _countof(m_sceneCommandLists) * sizeof(ID3D12CommandList*));
        m_batchSubmit[batchSize - 1] = m_commandLists[CommandListPost].Get();
    }
}

FrameResource::~FrameResource()
{
    for (int i = 0; i < CommandListCount; i++)
    {
        m_commandAllocators[i] = nullptr;
        m_commandLists[i] = nullptr;
    }

    m_sceneConstantBuffer = nullptr;

    for (int i = 0; i < NumContexts; i++)
    {
        m_sceneCommandLists[i] = nullptr;
        m_sceneCommandAllocators[i] = nullptr;
    }

}

// Builds and writes constant buffers from scratch to the proper slots for 
// this frame resource.
void FrameResource::WriteConstantBuffers(XMMATRIX inMatrix)
{
    SceneConstantBuffer sceneConsts = {};
    sceneConsts.model = inMatrix;
    memcpy(mp_sceneConstantBufferWO, &sceneConsts, sizeof(SceneConstantBuffer));
}

void FrameResource::Init()
{
    // Reset the command allocators and lists for the main thread.
    for (int i = 0; i < CommandListCount; i++)
    {
        ThrowIfFailed(m_commandAllocators[i]->Reset());
        ThrowIfFailed(m_commandLists[i]->Reset(m_commandAllocators[i].Get(), m_pipelineState.Get()));
    }

    // Reset the worker command allocators and lists.
    for (int i = 0; i < NumContexts; i++)
    {
        ThrowIfFailed(m_sceneCommandAllocators[i]->Reset());
        ThrowIfFailed(m_sceneCommandLists[i]->Reset(m_sceneCommandAllocators[i].Get(), m_pipelineState.Get()));
    }
}

// Sets up the descriptor tables for the worker command list to use 
// resources provided by frame resource.
void FrameResource::Bind(ID3D12GraphicsCommandList* pCommandList, D3D12_CPU_DESCRIPTOR_HANDLE* pRtvHandle)
{
    // with rendering to the render target enabled.
    pCommandList->SetGraphicsRootDescriptorTable(1, m_sceneCbvHandle);

    assert(pRtvHandle != nullptr);

    pCommandList->OMSetRenderTargets(1, pRtvHandle, FALSE, nullptr);
}
