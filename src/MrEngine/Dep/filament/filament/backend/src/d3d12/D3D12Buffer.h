#pragma once

#include "D3D12Resource.h"
#include "D3D12View.h"

namespace filament
{
	namespace backend
	{
		class MD3D12Buffer
		{
		public:
			MD3D12Buffer() {}

			virtual ~MD3D12Buffer() {}

			MD3D12Resource* GetResource() { return ResourceLocation.UnderlyingResource; }

		public:
			MD3D12ResourceLocation ResourceLocation;
		};

		class MD3D12ConstantBuffer : public MD3D12Buffer
		{

		};
		typedef std::shared_ptr<MD3D12ConstantBuffer> MD3D12ConstantBufferRef;


		class MD3D12StructuredBuffer : public MD3D12Buffer
		{
		public:
			MD3D12ShaderResourceView* GetSRV()
			{
				return SRV.get();
			}

			void SetSRV(std::unique_ptr<MD3D12ShaderResourceView>& InSRV)
			{
				SRV = std::move(InSRV);
			}

		private:
			std::unique_ptr<MD3D12ShaderResourceView> SRV = nullptr;
		};
		typedef std::shared_ptr<MD3D12StructuredBuffer> MD3D12StructuredBufferRef;


		class MD3D12RWStructuredBuffer : public MD3D12Buffer
		{
		public:
			MD3D12ShaderResourceView* GetSRV()
			{
				return SRV.get();
			}

			void SetSRV(std::unique_ptr<MD3D12ShaderResourceView>& InSRV)
			{
				SRV = std::move(InSRV);
			}

			MD3D12UnorderedAccessView* GetUAV()
			{
				return UAV.get();
			}

			void SetUAV(std::unique_ptr<MD3D12UnorderedAccessView>& InUAV)
			{
				UAV = std::move(InUAV);
			}

		private:
			std::unique_ptr<MD3D12ShaderResourceView> SRV = nullptr;

			std::unique_ptr<MD3D12UnorderedAccessView> UAV = nullptr;
		};
		typedef std::shared_ptr<MD3D12RWStructuredBuffer> MD3D12RWStructuredBufferRef;


		class MD3D12VertexBuffer : public MD3D12Buffer
		{

		};
		typedef std::shared_ptr<MD3D12VertexBuffer> MD3D12VertexBufferRef;


		class MD3D12IndexBuffer : public MD3D12Buffer
		{

		};
		typedef std::shared_ptr<MD3D12IndexBuffer> MD3D12IndexBufferRef;


		class MD3D12ReadBackBuffer : public MD3D12Buffer
		{

		};
		typedef std::shared_ptr<MD3D12ReadBackBuffer> MD3D12ReadBackBufferRef;

	}
}