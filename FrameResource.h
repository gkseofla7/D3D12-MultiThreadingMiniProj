#pragma once
#include "stdafx.h"
#include "DXSampleHelper.h"
#include "D3D12HelloTriangle.h"

using namespace DirectX;
using namespace Microsoft::WRL;

class FrameResource
{
public:
	FrameResource(ID3D12Device* pDevice, ID3D12PipelineState* pPso, ID3D12DescriptorHeap* pCbvSrvHeap, D3D12_VIEWPORT* pViewport, UINT frameResourceIndex);
	~FrameResource();

	void Bind(ID3D12GraphicsCommandList* pCommandList, D3D12_CPU_DESCRIPTOR_HANDLE* pRtvHandle);
	void Init();
	void WriteConstantBuffers(XMMATRIX offset);
public:
	ID3D12CommandList* m_batchSubmit[NumContexts + CommandListCount];

	ComPtr<ID3D12CommandAllocator> m_commandAllocators[CommandListCount];
	ComPtr<ID3D12GraphicsCommandList> m_commandLists[CommandListCount];

	ComPtr<ID3D12CommandAllocator> m_sceneCommandAllocators[NumContexts];
	ComPtr<ID3D12GraphicsCommandList> m_sceneCommandLists[NumContexts];

	UINT64 m_fenceValue;
	SceneConstantBuffer* mp_sceneConstantBufferWO;        // WRITE-ONLY pointer to the scene pass constant buffer.
private:
	ComPtr<ID3D12PipelineState> m_pipelineState;
	ComPtr<ID3D12Resource> m_sceneConstantBuffer;
	
	D3D12_GPU_DESCRIPTOR_HANDLE m_sceneCbvHandle;

};

