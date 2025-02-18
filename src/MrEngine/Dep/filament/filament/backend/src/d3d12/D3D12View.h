#pragma once

#include "D3D12/D3D12HeapSlotAllocator.h"
namespace filament
{
	namespace backend
	{
		class D3D12Context;

		class MD3D12View
		{
		public:
			MD3D12View(D3D12Context* InContext, D3D12_DESCRIPTOR_HEAP_TYPE InType, ID3D12Resource* InResource);

			virtual ~MD3D12View();

			D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandle() const { return HeapSlot.Handle; }

		private:
			void Destroy();

		protected:
			D3D12Context* m_context = nullptr;

			MD3D12HeapSlotAllocator* HeapSlotAllocator = nullptr;

			ID3D12Resource* Resource = nullptr;

			MD3D12HeapSlotAllocator::HeapSlot HeapSlot;

			D3D12_DESCRIPTOR_HEAP_TYPE Type;
		};

		class MD3D12ShaderResourceView : public MD3D12View
		{
		public:
			MD3D12ShaderResourceView(D3D12Context* InContext, const D3D12_SHADER_RESOURCE_VIEW_DESC& Desc, ID3D12Resource* InResource);

			virtual ~MD3D12ShaderResourceView();

		protected:
			void CreateShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC& Desc);
		};

		class MD3D12RenderTargetView : public MD3D12View
		{
		public:
			MD3D12RenderTargetView(D3D12Context* InContext, const D3D12_RENDER_TARGET_VIEW_DESC& Desc, ID3D12Resource* InResource);

			virtual ~MD3D12RenderTargetView();

		protected:
			void CreateRenderTargetView(const D3D12_RENDER_TARGET_VIEW_DESC& Desc);
		};


		class MD3D12DepthStencilView : public MD3D12View
		{
		public:
			MD3D12DepthStencilView(D3D12Context* InContext, const D3D12_DEPTH_STENCIL_VIEW_DESC& Desc, ID3D12Resource* InResource);

			virtual ~MD3D12DepthStencilView();

		protected:
			void CreateDepthStencilView(const D3D12_DEPTH_STENCIL_VIEW_DESC& Desc);
		};

		class MD3D12UnorderedAccessView : public MD3D12View
		{
		public:
			MD3D12UnorderedAccessView(D3D12Context* InContext, const D3D12_UNORDERED_ACCESS_VIEW_DESC& Desc, ID3D12Resource* InResource);

			virtual ~MD3D12UnorderedAccessView();

		protected:
			void CreateUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC& Desc);
		};
	}
}