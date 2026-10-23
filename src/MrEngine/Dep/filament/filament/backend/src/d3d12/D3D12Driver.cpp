
#include "D3D12Driver.h"
#include "D3D12DriverFactory.h"
#include "CommandStreamDispatcher.h"
#include "D3D12Context.h"
#include "D3D12Handles.h"
#include "D3D12DescriptorCache.h"
#include <utils/unwindows.h>

namespace filament
{
	namespace backend
	{
		Driver* D3D12DriverFactory::create(backend::D3D12Platform* platform)
		{
			return D3D12Driver::create(platform);
		}

		Driver* D3D12Driver::create(backend::D3D12Platform* platform)
		{
			assert(platform);
			return new D3D12Driver(platform);
		}

		D3D12Driver::D3D12Driver(backend::D3D12Platform* platform) noexcept:
			DriverBase(new ConcreteDispatcher<D3D12Driver>()),
			m_platform(*platform),
			m_context(new D3D12Context())
		{
		
		}

		D3D12Driver::~D3D12Driver() noexcept
		{
			delete m_context;
		}

		ShaderModel D3D12Driver::getShaderModel() const noexcept
		{
			return ShaderModel::GL_CORE_41;
		}

		void D3D12Driver::setPresentationTime(int64_t monotonic_clock_ns)
		{

		}

		void D3D12Driver::beginFrame(int64_t monotonic_clock_ns, uint32_t frame_id)
		{
			// With MSAA the swapchain is only a resolve destination (handled in commit).
			// Without MSAA, transition the backbuffer to RENDER_TARGET for direct drawing.
			if (m_context->msaaSamples <= 1)
			{
				SwapChainBuffer& scb = m_context->current_swap_chain->m_backbuffers[m_context->m_frameIndex];
				m_context->m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(scb.buffer.Get(),
					D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
			}
		}

		void D3D12Driver::endFrame(uint32_t frame_id)
		{
			// Descriptor / buddy reclaim happens in commit after the frame fence wait,
			// so we never reset slots still referenced by in-flight GPU work.
			(void)frame_id;
		}

		void D3D12Driver::flush(int dummy)
		{

		}

		void D3D12Driver::createVertexBufferR(
			Handle<HwVertexBuffer> vbh,
			uint8_t buffer_count,
			uint8_t attribute_count,
			uint32_t vertex_count,
			AttributeArray attributes,
			BufferUsage usage)
		{
			construct_handle<D3D12VertexBuffer>(
				m_handle_map,
				vbh,
				m_context,
				buffer_count,
				attribute_count,
				vertex_count,
				attributes,
				usage);
		}

		void D3D12Driver::createIndexBufferR(
			Handle<HwIndexBuffer> ibh,
			ElementType element_type,
			uint32_t index_count,
			BufferUsage usage)
		{
			construct_handle<D3D12IndexBuffer>(
				m_handle_map,
				ibh,
				m_context,
				element_type,
				index_count,
				usage);
		}

		void D3D12Driver::createTextureR(
			Handle<HwTexture> th,
			SamplerType target,
			uint8_t levels,
			TextureFormat format,
			uint8_t samples,
			uint32_t width,
			uint32_t height,
			uint32_t depth,
			TextureUsage usage)
		{
			construct_handle<D3D12Texture>(m_handle_map, th, m_context, target, levels, format, samples, width, height, depth, usage);
		}

		void D3D12Driver::createSamplerGroupR(Handle<HwSamplerGroup> sgh, size_t size)
		{
			construct_handle<D3D12SamplerGroup>(m_handle_map, sgh, m_context, size);
		}

		void D3D12Driver::createUniformBufferR(Handle<HwUniformBuffer> ubh, size_t size, BufferUsage usage)
		{
			construct_handle<D3D12UniformBuffer>(m_handle_map, ubh, m_context, size, usage);
		}

		void D3D12Driver::createRenderPrimitiveR(Handle<HwRenderPrimitive> rph, int dummy)
		{
			construct_handle<D3D12RenderPrimitive>(m_handle_map, rph, m_context);
		}

		void D3D12Driver::createProgramR(Handle<HwProgram> ph, Program&& program)
		{
			construct_handle<D3D12Program>(m_handle_map, ph, m_context, std::move(program));
		}

		void D3D12Driver::createDefaultRenderTargetR(Handle<HwRenderTarget> rth, int dummy)
		{
			construct_handle<D3D12RenderTarget>(m_handle_map, rth, m_context);
		}

		void D3D12Driver::createRenderTargetR(
			Handle<HwRenderTarget> rth,
			TargetBufferFlags flags,
			uint32_t width,
			uint32_t height,
			uint8_t samples,
			TargetBufferInfo color,
			TargetBufferInfo depth,
			TargetBufferInfo stencil)
		{
			auto render_target = construct_handle<D3D12RenderTarget>(
				m_handle_map,
				rth,
				m_context,
				flags,
				width,
				height,
				samples,
				color,
				depth,
				stencil);

			render_target->width = width;
			render_target->height = height;
			render_target->samples = samples > 0 ? samples : 1;

			if (flags & TargetBufferFlags::COLOR)
			{
				auto color_texture = handle_cast<D3D12Texture>(m_handle_map, color.handle);
				render_target->colorTex = color_texture;
				render_target->colorTexture = color_texture->texture;

				D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
				rtvDesc.Format = m_context->GetTextureViewFormat(color_texture->format);
				rtvDesc.ViewDimension = (samples > 1) ? D3D12_RTV_DIMENSION_TEXTURE2DMS : D3D12_RTV_DIMENSION_TEXTURE2D;
				if (samples <= 1) {
					rtvDesc.Texture2D.MipSlice = color.level;
				}
				render_target->RTV = std::make_unique<MD3D12RenderTargetView>(
					m_context, rtvDesc, color_texture->texture.Get());
				render_target->colorFormat = rtvDesc.Format;
			}
			if ((flags & TargetBufferFlags::DEPTH) ||
				(flags & TargetBufferFlags::STENCIL))
			{
				D3D12Texture* texture = nullptr;
				UINT level = 0;

				if (flags & TargetBufferFlags::DEPTH)
				{
					texture = handle_cast<D3D12Texture>(m_handle_map, depth.handle);
					level = depth.level;
				}
				else if (flags & TargetBufferFlags::STENCIL)
				{
					texture = handle_cast<D3D12Texture>(m_handle_map, stencil.handle);
					level = stencil.level;
				}

				render_target->depthTex = texture;
				render_target->depthStencilTexture = texture->texture;

				D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
				dsvDesc.Format = m_context->GetDepthViewFormat(texture->format);
				dsvDesc.ViewDimension = (samples > 1) ? D3D12_DSV_DIMENSION_TEXTURE2DMS : D3D12_DSV_DIMENSION_TEXTURE2D;
				if (samples <= 1) {
					dsvDesc.Texture2D.MipSlice = level;
				}
				render_target->DSV = std::make_unique<MD3D12DepthStencilView>(
					m_context, dsvDesc, texture->texture.Get());
				render_target->depthFormat = dsvDesc.Format;
			}
		}

		void D3D12Driver::createFenceR(Handle<HwFence> fh, int dummy)
		{

		}

		void D3D12Driver::createSwapChainR(Handle<HwSwapChain> sch, void* native_window, uint64_t flags)
		{
			construct_handle<D3D12SwapChain>(m_handle_map, sch, m_context, native_window);
		}

		void D3D12Driver::createStreamFromTextureIdR(
			Handle<HwStream> stream,
			intptr_t externalTextureId,
			uint32_t width,
			uint32_t height)
		{

		}

		Handle<HwVertexBuffer> D3D12Driver::createVertexBufferS() noexcept
		{
			return alloc_handle<D3D12VertexBuffer, HwVertexBuffer>();
		}

		Handle<HwIndexBuffer> D3D12Driver::createIndexBufferS() noexcept
		{
			return alloc_handle<D3D12IndexBuffer, HwIndexBuffer>();
		}

		Handle<HwTexture> D3D12Driver::createTextureS() noexcept
		{
			return alloc_handle<D3D12Texture, HwTexture>();
		}

		Handle<HwSamplerGroup> D3D12Driver::createSamplerGroupS() noexcept
		{
			return alloc_handle<D3D12SamplerGroup, HwSamplerGroup>();
		}

		Handle<HwUniformBuffer> D3D12Driver::createUniformBufferS() noexcept
		{
			return alloc_handle<D3D12UniformBuffer, HwUniformBuffer>();
		}

		Handle<HwRenderPrimitive> D3D12Driver::createRenderPrimitiveS() noexcept
		{
			return alloc_handle<D3D12RenderPrimitive, HwRenderPrimitive>();
		}

		Handle<HwProgram> D3D12Driver::createProgramS() noexcept
		{
			return alloc_handle<D3D12Program, HwProgram>();
		}

		Handle<HwRenderTarget> D3D12Driver::createDefaultRenderTargetS() noexcept
		{
			return alloc_handle<D3D12RenderTarget, HwRenderTarget>();
		}

		Handle<HwRenderTarget> D3D12Driver::createRenderTargetS() noexcept
		{
			return alloc_handle<D3D12RenderTarget, HwRenderTarget>();
		}

		Handle<HwFence> D3D12Driver::createFenceS() noexcept
		{
			return Handle<HwFence>();
		}

		Handle<HwSwapChain> D3D12Driver::createSwapChainS() noexcept
		{
			return alloc_handle<D3D12SwapChain, HwSwapChain>();
		}

		Handle<HwStream> D3D12Driver::createStreamFromTextureIdS() noexcept
		{
			return Handle<HwStream>();
		}

		// Buffer memory comes from the buddy allocators, whose frees are already deferred
		// per frame slot, so these no longer need a full GPU stall.
		void D3D12Driver::destroyVertexBuffer(Handle<HwVertexBuffer> vbh)
		{
			destruct_handle<D3D12VertexBuffer>(m_handle_map, vbh);
		}

		void D3D12Driver::destroyIndexBuffer(Handle<HwIndexBuffer> ibh)
		{
			destruct_handle<D3D12IndexBuffer>(m_handle_map, ibh);
		}

		void D3D12Driver::destroyRenderPrimitive(Handle<HwRenderPrimitive> rph)
		{
			destruct_handle<D3D12RenderPrimitive>(m_handle_map, rph);
		}

		void D3D12Driver::destroyProgram(Handle<HwProgram> ph)
		{
			if (!ph)
			{
				return;
			}
			auto program = handle_cast<D3D12Program>(m_handle_map, ph);
			struct Hold {
				ComPtr<ID3D12RootSignature> root;
				decltype(D3D12Program::m_psoCache) psos;
			};
			auto hold = std::make_shared<Hold>();
			hold->root = std::move(program->m_RootSignature);
			hold->psos = std::move(program->m_psoCache);
			m_context->DeferRelease(hold);
			destruct_handle<D3D12Program>(m_handle_map, ph);
		}

		void D3D12Driver::destroySamplerGroup(Handle<HwSamplerGroup> sgh)
		{
			destruct_handle<D3D12SamplerGroup>(m_handle_map, sgh);
		}

		void D3D12Driver::destroyUniformBuffer(Handle<HwUniformBuffer> ubh)
		{
			if (!ubh)
			{
				return;
			}
			auto uniform_buffer = handle_cast<D3D12UniformBuffer>(m_handle_map, ubh);
			struct Hold { MD3D12ConstantBufferRef frames[D3D12Context::NumFrames]; };
			auto hold = std::make_shared<Hold>();
			for (UINT i = 0; i < D3D12Context::NumFrames; ++i)
			{
				hold->frames[i] = std::move(uniform_buffer->frames[i]);
			}
			m_context->DeferRelease(hold);
			destruct_handle<D3D12UniformBuffer>(m_handle_map, ubh);
		}

		void D3D12Driver::destroyTexture(Handle<HwTexture> th)
		{
			if (!th)
			{
				return;
			}
			auto texture = handle_cast<D3D12Texture>(m_handle_map, th);
			struct Hold {
				ComPtr<ID3D12Resource> resource;
				std::unique_ptr<MD3D12ShaderResourceView> srv;
				std::unique_ptr<MD3D12DepthStencilView> dsv;
				std::unique_ptr<MD3D12UnorderedAccessView> uav;
			};
			auto hold = std::make_shared<Hold>();
			hold->resource = std::move(texture->texture);
			hold->srv = std::move(texture->SRV);
			hold->dsv = std::move(texture->DSV);
			hold->uav = std::move(texture->UAV);
			m_context->DeferRelease(hold);
			destruct_handle<D3D12Texture>(m_handle_map, th);
		}

		void D3D12Driver::destroyRenderTarget(Handle<HwRenderTarget> rth)
		{
			if (!rth)
			{
				return;
			}
			auto render_target = handle_cast<D3D12RenderTarget>(m_handle_map, rth);
			struct Hold {
				ComPtr<ID3D12Resource> color;
				ComPtr<ID3D12Resource> depth;
				std::unique_ptr<MD3D12RenderTargetView> rtv;
				std::unique_ptr<MD3D12DepthStencilView> dsv;
			};
			auto hold = std::make_shared<Hold>();
			hold->color = std::move(render_target->colorTexture);
			hold->depth = std::move(render_target->depthStencilTexture);
			hold->rtv = std::move(render_target->RTV);
			hold->dsv = std::move(render_target->DSV);
			m_context->DeferRelease(hold);
			destruct_handle<D3D12RenderTarget>(m_handle_map, rth);
		}

		void D3D12Driver::destroySwapChain(Handle<HwSwapChain> sch)
		{
			m_context->waitForGPU();
			for (UINT i = 0; i < D3D12Context::NumFrames; ++i)
			{
				m_context->m_framebuffers[i] = FrameBuffer{};
			}
			destruct_handle<D3D12SwapChain>(m_handle_map, sch);
		}

		void D3D12Driver::destroyStream(Handle<HwStream> sh)
		{

		}

		void D3D12Driver::terminate()
		{

		}

		Handle<HwStream> D3D12Driver::createStream(void* stream)
		{
			return Handle<HwStream>();
		}

		void D3D12Driver::setStreamDimensions(Handle<HwStream> stream, uint32_t width, uint32_t height)
		{

		}

		int64_t D3D12Driver::getStreamTimestamp(Handle<HwStream> stream)
		{
			return 0;
		}

		void D3D12Driver::updateStreams(backend::DriverApi* driver)
		{

		}

		void D3D12Driver::destroyFence(Handle<HwFence> fh)
		{

		}

		FenceStatus D3D12Driver::wait(Handle<HwFence> fh, uint64_t timeout)
		{
			return FenceStatus::ERROR;
		}

		bool D3D12Driver::isTextureFormatSupported(TextureFormat format)
		{
			UINT support = 0;
			D3D12_FEATURE_DATA_FORMAT_SUPPORT formatInfo = { m_context->GetTextureFormat(format) };
			if (SUCCEEDED(m_context->m_device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &formatInfo, sizeof(formatInfo))))
			{
				if (formatInfo.Support1 & D3D12_FORMAT_SUPPORT1_TEXTURE2D)
				{
					formatInfo = { m_context->GetTextureViewFormat(format) };
					if (SUCCEEDED(m_context->m_device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &formatInfo, sizeof(formatInfo))))
					{
						if (formatInfo.Support1 & D3D12_FORMAT_SUPPORT1_SHADER_SAMPLE)
							return true;
					}
				}
			}
			return false;
		}

		bool D3D12Driver::isRenderTargetFormatSupported(TextureFormat format)
		{
			if (!this->isTextureFormatSupported(format))
			{
				return false;
			}

			switch (format)
			{
			case TextureFormat::DEPTH16:
			case TextureFormat::DEPTH24:
			case TextureFormat::DEPTH24_STENCIL8:
			case TextureFormat::DEPTH32F:
			case TextureFormat::DEPTH32F_STENCIL8:
			{
				D3D12_FEATURE_DATA_FORMAT_SUPPORT info = { m_context->GetDepthViewFormat(format) };
				return info.Format != DXGI_FORMAT_UNKNOWN &&
					SUCCEEDED(m_context->m_device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &info, sizeof(info))) &&
					(info.Support1 & D3D12_FORMAT_SUPPORT1_DEPTH_STENCIL);
			}
			default:
			{
				D3D12_FEATURE_DATA_FORMAT_SUPPORT info = { m_context->GetTextureViewFormat(format) };
				return info.Format != DXGI_FORMAT_UNKNOWN &&
					SUCCEEDED(m_context->m_device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &info, sizeof(info))) &&
					(info.Support1 & D3D12_FORMAT_SUPPORT1_RENDER_TARGET);
			}
			}
		}

		bool D3D12Driver::isFrameTimeSupported()
		{
			return false;
		}

		void D3D12Driver::updateVertexBuffer(
			Handle<HwVertexBuffer> vbh,
			size_t index,
			BufferDescriptor&& data,
			uint32_t offset)
		{
			auto buffer = handle_cast<D3D12VertexBuffer>(m_handle_map, vbh);
			buffer->Update(m_context, index, data, offset);
			this->scheduleDestroy(std::move(data));
		}

		void D3D12Driver::updateIndexBuffer(
			Handle<HwIndexBuffer> ibh,
			BufferDescriptor&& data,
			uint32_t offset)
		{
			auto buffer = handle_cast<D3D12IndexBuffer>(m_handle_map, ibh);
			buffer->Update(m_context, data, offset);
			this->scheduleDestroy(std::move(data));
		}

		void D3D12Driver::updateTexture(
			Handle<HwTexture> th,
			int layer, int level,
			int x, int y,
			int w, int h,
			PixelBufferDescriptor&& data)
		{
			auto texture = handle_cast<D3D12Texture>(m_handle_map, th);
			texture->UpdateTexture(
				m_context,
				layer, level,
				x, y,
				w, h,
				data);
			this->scheduleDestroy(std::move(data));
		}

		void D3D12Driver::update2DImage(
			Handle<HwTexture> th,
			uint32_t level,
			uint32_t x, uint32_t y,
			uint32_t width, uint32_t height,
			PixelBufferDescriptor&& data)
		{
			auto texture = handle_cast<D3D12Texture>(m_handle_map, th);
			texture->UpdateTexture(
				m_context,
				0, level,
				x, y,
				width, height,
				data);
			this->scheduleDestroy(std::move(data));
		}

		void D3D12Driver::updateCubeImage(
			Handle<HwTexture> th,
			uint32_t level,
			PixelBufferDescriptor&& data,
			FaceOffsets face_offsets)
		{
			auto texture = handle_cast<D3D12Texture>(m_handle_map, th);
			for (int i = 0; i < 6; ++i)
			{
				auto buffer = static_cast<uint8_t*>(data.buffer) + face_offsets[i];
				if (data.type == PixelDataType::COMPRESSED)
				{
					const uint32_t mipW = (std::max)(1u, texture->width >> level);
					const uint32_t mipH = (std::max)(1u, texture->height >> level);
					texture->UpdateTexture(
						m_context,
						i, level,
						0, 0,
						mipW, mipH,
						PixelBufferDescriptor(buffer, data.size / 6, data.compressedFormat, data.imageSize, nullptr));
				}
				else
				{
					const uint32_t mipW = (std::max)(1u, texture->width >> level);
					const uint32_t mipH = (std::max)(1u, texture->height >> level);
					texture->UpdateTexture(
						m_context,
						i, level,
						0, 0,
						mipW, mipH,
						PixelBufferDescriptor(buffer, data.size / 6, data.format, data.type));
				}
			}
			this->scheduleDestroy(std::move(data));
		}

		void D3D12Driver::copyTexture(
			Handle<HwTexture> th_dst, int dst_layer, int dst_level,
			Offset3D dst_offset,
			Offset3D dst_extent,
			Handle<HwTexture> th_src, int src_layer, int src_level,
			Offset3D src_offset,
			Offset3D src_extent,
			SamplerMagFilter blit_filter)
		{
			auto dst = handle_cast<D3D12Texture>(m_handle_map, th_dst);
			auto src = handle_cast<D3D12Texture>(m_handle_map, th_src);
			dst->CopyTexture(
				m_context,
				dst_layer, dst_level,
				dst_offset, dst_extent,
				src,
				src_layer, src_level,
				src_offset, src_extent);
		}

		void D3D12Driver::copyTextureToMemory(
			Handle<HwTexture> th,
			int layer, int level,
			Offset3D offset,
			Offset3D extent,
			PixelBufferDescriptor&& p)
		{
			auto texture = handle_cast<D3D12Texture>(m_handle_map, th);
			texture->CopyTextureToMemory(
				m_context,
				layer, level,
				offset, extent,
				p);
			this->scheduleDestroy(std::move(p));
		}

		void D3D12Driver::setupExternalImage(void* image)
		{

		}

		void D3D12Driver::cancelExternalImage(void* image)
		{

		}

		void D3D12Driver::setExternalImage(Handle<HwTexture> th, void* image)
		{

		}

		void D3D12Driver::setExternalStream(Handle<HwTexture> th, Handle<HwStream> sh)
		{

		}

		void D3D12Driver::generateMipmaps(Handle<HwTexture> th)
		{
			auto texture = handle_cast<D3D12Texture>(m_handle_map, th);
			texture->GenerateMipmaps(m_context);
		}

		bool D3D12Driver::canGenerateMipmaps()
		{
			// D3D12Texture::GenerateMipmaps has no implementation; callers must upload mips themselves.
			return false;
		}

		void D3D12Driver::loadUniformBuffer(Handle<HwUniformBuffer> ubh, BufferDescriptor&& data)
		{
			auto uniform_buffer = handle_cast<D3D12UniformBuffer>(m_handle_map, ubh);
			uniform_buffer->Load(m_context, data);
			this->scheduleDestroy(std::move(data));
		}

		void D3D12Driver::updateSamplerGroup(Handle<HwSamplerGroup> sgh, SamplerGroup&& sg)
		{
			auto sampler_group = handle_cast<D3D12SamplerGroup>(m_handle_map, sgh);
			sampler_group->Update(m_context, std::move(sg));
		}

		void D3D12Driver::beginRenderPass(
			Handle<HwRenderTarget> rth,
			const RenderPassParams& params)
		{
			auto render_target = handle_cast<D3D12RenderTarget>(m_handle_map, rth);
			m_context->current_render_target = render_target;
			m_context->current_render_pass_flags = params.flags;

			FrameBuffer fb;

			uint32_t target_height = 0;

			D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = {};
			D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = {};
			bool hasColor = false;
			bool hasDepth = false;

			// set render target
			if (render_target->default_render_target)
			{
				UINT frameIndex = m_context->m_frameIndex;
				FrameBuffer& msaaFb = m_context->m_framebuffers[frameIndex];

				if (m_context->msaaSamples > 1 && msaaFb.colorTexture)
				{
					fb.colorTexture = msaaFb.colorTexture;
					fb.depthStencilTexture = msaaFb.depthStencilTexture;
					rtvHandle = msaaFb.RTV->GetDescriptorHandle();
					dsvHandle = msaaFb.DSV->GetDescriptorHandle();
					hasColor = true;
					hasDepth = true;
					target_height = msaaFb.height;
					render_target->samples = msaaFb.samples;
					render_target->height = msaaFb.height;
					render_target->width = msaaFb.width;
				}
				else
				{
					SwapChainBuffer& scb = m_context->current_swap_chain->m_backbuffers[frameIndex];
					fb.colorTexture = scb.buffer;
					rtvHandle = scb.RTV->GetDescriptorHandle();
					hasColor = true;

					if (render_target->depthStencilTexture == nullptr)
					{
						DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = { };
						m_context->current_swap_chain->m_swapChain->GetDesc1(&swap_chain_desc);

						render_target->CreateDepth(
							m_context,
							DXGI_FORMAT_D24_UNORM_S8_UINT,
							1,
							swap_chain_desc.Width,
							swap_chain_desc.Height);
						render_target->height = swap_chain_desc.Height;
						render_target->width = swap_chain_desc.Width;
						render_target->samples = 1;
					}

					fb.depthStencilTexture = render_target->depthStencilTexture;
					dsvHandle = render_target->DSV->GetDescriptorHandle();
					hasDepth = true;
					target_height = render_target->height;
				}
			}
			else
			{
				fb.colorTexture = render_target->colorTexture;
				fb.depthStencilTexture = render_target->depthStencilTexture;
				if (render_target->RTV)
				{
					rtvHandle = render_target->RTV->GetDescriptorHandle();
					hasColor = render_target->colorTexture != nullptr;
				}
				if (render_target->DSV)
				{
					dsvHandle = render_target->DSV->GetDescriptorHandle();
					hasDepth = render_target->depthStencilTexture != nullptr;
				}
				target_height = render_target->height;
				if (render_target->colorTex)
				{
					render_target->colorTex->TransitionTo(m_context, D3D12_RESOURCE_STATE_RENDER_TARGET);
				}
				if (render_target->depthTex)
				{
					render_target->depthTex->TransitionTo(m_context, D3D12_RESOURCE_STATE_DEPTH_WRITE);
				}
			}

			if (hasColor && hasDepth)
			{
				m_context->m_commandList->OMSetRenderTargets(1, &rtvHandle, false, &dsvHandle);
			}
			else if (hasDepth)
			{
				m_context->m_commandList->OMSetRenderTargets(0, nullptr, false, &dsvHandle);
			}
			else if (hasColor)
			{
				m_context->m_commandList->OMSetRenderTargets(1, &rtvHandle, false, nullptr);
			}

			// set viewport
			D3D12_VIEWPORT viewport = CD3DX12_VIEWPORT(
				(float) params.viewport.left,
				(float) (target_height - (params.viewport.bottom + params.viewport.height)),
				(float) params.viewport.width,
				(float) params.viewport.height
			);
			m_context->m_commandList->RSSetViewports(1, &viewport);

			// clear
			if (params.flags.clear & filament::backend::TargetBufferFlags::COLOR)
			{
				if (hasColor)
				{
					m_context->m_commandList->ClearRenderTargetView(rtvHandle, (float*) &params.clearColor, 0, nullptr);
				}
			}
			if (params.flags.clear & filament::backend::TargetBufferFlags::DEPTH ||
				params.flags.clear & filament::backend::TargetBufferFlags::STENCIL)
			{
				D3D12_CLEAR_FLAGS flags = (D3D12_CLEAR_FLAGS)0;
				if (params.flags.clear & filament::backend::TargetBufferFlags::DEPTH)
				{
					flags |= D3D12_CLEAR_FLAG_DEPTH;
				}
				if (params.flags.clear & filament::backend::TargetBufferFlags::STENCIL)
				{
					flags |= D3D12_CLEAR_FLAG_STENCIL;
				}

				if (hasDepth)
				{
					m_context->m_commandList->ClearDepthStencilView(
						dsvHandle, flags, params.clearDepth, 0, 0, nullptr);
				}
			}
			m_context->m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		}

		void D3D12Driver::endRenderPass(int dummy)
		{
			auto* rt = m_context->current_render_target;
			if (rt && !rt->default_render_target)
			{
				// Make attachments sampleable again for subsequent passes / shadow sampling.
				if (rt->colorTex && (rt->colorTex->usage & TextureUsage::SAMPLEABLE))
				{
					rt->colorTex->TransitionTo(m_context, rt->colorTex->IdleState());
				}
				if (rt->depthTex && (rt->depthTex->usage & TextureUsage::SAMPLEABLE))
				{
					rt->depthTex->TransitionTo(m_context, rt->depthTex->IdleState());
				}
			}

			m_context->current_render_target = nullptr;
			m_context->current_render_pass_flags = { };

			for (size_t i = 0; i < m_context->uniform_buffer_bindings.size(); ++i)
			{
				m_context->uniform_buffer_bindings[i] = { };
			}

			for (size_t i = 0; i < m_context->sampler_group_binding.size(); ++i)
			{
				m_context->sampler_group_binding[i].sampler_group = SamplerGroupHandle();
			}
		}

		void D3D12Driver::discardSubRenderTargetBuffers(
			Handle<HwRenderTarget> rth,
			TargetBufferFlags targetBufferFlags,
			uint32_t left,
			uint32_t bottom,
			uint32_t width,
			uint32_t height)
		{

		}

		void D3D12Driver::setRenderPrimitiveBuffer(
			Handle<HwRenderPrimitive> rph,
			Handle<HwVertexBuffer> vbh,
			Handle<HwIndexBuffer> ibh,
			uint32_t enabled_attributes)
		{
			auto primitive = handle_cast<D3D12RenderPrimitive>(m_handle_map, rph);
			auto vertex_buffer = handle_cast<D3D12VertexBuffer>(m_handle_map, vbh);
			primitive->SetBuffer(m_context, vbh, ibh, enabled_attributes, vertex_buffer->vertexCount);
		}

		void D3D12Driver::setRenderPrimitiveRange(
			Handle<HwRenderPrimitive> rph,
			PrimitiveType pt,
			uint32_t offset,
			uint32_t min_index,
			uint32_t max_index,
			uint32_t count)
		{
			auto primitive = handle_cast<D3D12RenderPrimitive>(m_handle_map, rph);
			primitive->SetRange(m_context, pt, offset, min_index, max_index, count);
		}

		void D3D12Driver::setViewportScissor(
			int32_t left,
			int32_t bottom,
			uint32_t width,
			uint32_t height)
		{
			const LONG top = (LONG)(m_context->current_render_target->height - (bottom + height));
			CD3DX12_RECT rect(
				(LONG)left,
				top,
				(LONG)left + (LONG)width,
				top + (LONG)height);
			m_context->m_commandList->RSSetScissorRects(1, &rect);
		}

		void D3D12Driver::makeCurrent(Handle<HwSwapChain> sch_draw, Handle<HwSwapChain> sch_read)
		{
			auto swap_chain = handle_cast<D3D12SwapChain>(m_handle_map, sch_draw);
			if (m_context->current_swap_chain != swap_chain)
			{
				m_context->current_swap_chain = swap_chain;
			}

			// Flush any init / inter-frame uploads that were recorded since the last
			// execute, then wait so this frame's allocator is idle before Reset.
			if (m_context->m_commandListRecording)
			{
				m_context->executeCommandList(false);
				m_context->waitForGPU();
			}

			ID3D12CommandAllocator* commandAllocator = m_context->m_commandAllocators[m_context->m_frameIndex].Get();
			commandAllocator->Reset();
			m_context->ensureCommandListRecording();
		}

		void D3D12Driver::commit(Handle<HwSwapChain> sch)
		{
			SwapChainBuffer& scb = m_context->current_swap_chain->m_backbuffers[m_context->m_frameIndex];
			FrameBuffer& msaaFb = m_context->m_framebuffers[m_context->m_frameIndex];

			if (m_context->msaaSamples > 1 && msaaFb.colorTexture)
			{
				// Resolve MSAA color into the swapchain backbuffer, then present.
				m_context->m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
					msaaFb.colorTexture.Get(),
					D3D12_RESOURCE_STATE_RENDER_TARGET,
					D3D12_RESOURCE_STATE_RESOLVE_SOURCE));
				m_context->m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
					scb.buffer.Get(),
					D3D12_RESOURCE_STATE_PRESENT,
					D3D12_RESOURCE_STATE_RESOLVE_DEST));
				m_context->m_commandList->ResolveSubresource(
					scb.buffer.Get(), 0,
					msaaFb.colorTexture.Get(), 0,
					DXGI_FORMAT_R8G8B8A8_UNORM);
				m_context->m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
					msaaFb.colorTexture.Get(),
					D3D12_RESOURCE_STATE_RESOLVE_SOURCE,
					D3D12_RESOURCE_STATE_RENDER_TARGET));
				m_context->m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
					scb.buffer.Get(),
					D3D12_RESOURCE_STATE_RESOLVE_DEST,
					D3D12_RESOURCE_STATE_PRESENT));
			}
			else
			{
				m_context->m_commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(scb.buffer.Get(),
					D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
			}

			m_context->executeCommandList(false);
			auto swap_chain = handle_cast<D3D12SwapChain>(m_handle_map, sch);

			DXGI_PRESENT_PARAMETERS dpp = {};
			swap_chain->m_swapChain->Present1(0, DXGI_PRESENT_ALLOW_TEARING, &dpp);

			// Monotonic fence: never Signal(0) (CreateFence(0) already completed that value).
			const UINT64 submittedFence = ++m_context->m_lastSignaledFence;
			m_context->m_commandQueue->Signal(m_context->m_fence.Get(), submittedFence);
			m_context->m_fenceValues[m_context->m_frameIndex] = submittedFence;

			m_context->m_frameIndex = swap_chain->m_swapChain->GetCurrentBackBufferIndex();
			const UINT64 waitValue = m_context->m_fenceValues[m_context->m_frameIndex];
			if (m_context->m_fence->GetCompletedValue() < waitValue) {
				m_context->m_fence->SetEventOnCompletion(waitValue, m_context->m_fenceCompletionEvent);
				WaitForSingleObjectEx(m_context->m_fenceCompletionEvent, INFINITE, FALSE);
			}

			// Only the slot we just waited on is idle; the frame just submitted keeps its own.
			m_context->ReclaimFrameSlot(m_context->m_frameIndex);
		}

		void D3D12Driver::bindUniformBuffer(size_t index, Handle<HwUniformBuffer> ubh)
		{
			auto uniform_buffer = handle_cast<D3D12UniformBuffer>(m_handle_map, ubh);

			assert(index < m_context->uniform_buffer_bindings.size());
			
			m_context->uniform_buffer_bindings[index].ConstantBufferRef = uniform_buffer->GetCurrent(m_context);
			m_context->uniform_buffer_bindings[index].offset = 0;
			m_context->uniform_buffer_bindings[index].size = uniform_buffer->size;
		}

		void D3D12Driver::bindUniformBufferRange(
			size_t index,
			Handle<HwUniformBuffer> ubh,
			size_t offset,
			size_t size)
		{
			auto uniform_buffer = handle_cast<D3D12UniformBuffer>(m_handle_map, ubh);

			assert(index < m_context->uniform_buffer_bindings.size());
			assert(offset < uniform_buffer->size);
			assert(offset + size <= uniform_buffer->size);

			m_context->uniform_buffer_bindings[index].ConstantBufferRef = uniform_buffer->GetCurrent(m_context);
			m_context->uniform_buffer_bindings[index].offset = offset;
			m_context->uniform_buffer_bindings[index].size = size;
		}

		void D3D12Driver::setUniformVector(
			backend::ProgramHandle ph,
			std::string name,
			size_t count,
			backend::BufferDescriptor&& data)
		{

		}

		void D3D12Driver::setUniformMatrix(
			backend::ProgramHandle ph,
			std::string name,
			size_t count,
			backend::BufferDescriptor&& data)
		{

		}

		void D3D12Driver::bindSamplers(size_t index, Handle<HwSamplerGroup> sgh)
		{
			auto sampler_group = handle_cast<D3D12SamplerGroup>(m_handle_map, sgh);

			assert(index < m_context->sampler_group_binding.size());

			m_context->sampler_group_binding[index].sampler_group = sgh;
		}

		void D3D12Driver::insertEventMarker(const char* string, size_t len)
		{

		}

		void D3D12Driver::pushGroupMarker(const char* string, size_t len)
		{

		}

		void D3D12Driver::popGroupMarker(int dummy)
		{

		}

		void D3D12Driver::readPixels(
			Handle<HwRenderTarget> src,
			uint32_t x,
			uint32_t y,
			uint32_t width,
			uint32_t height,
			PixelBufferDescriptor&& data)
		{
			auto rt = handle_cast<D3D12RenderTarget>(m_handle_map, src);
			// Only single-sampled offscreen targets own a readable engine texture.
			if (!rt || !rt->colorTex || rt->samples > 1 || width == 0 || height == 0 || y + height > rt->height) {
				return;
			}

			// Filament rects are bottom-left based; D3D textures are top-left based.
			const uint32_t top = rt->height - (y + height);
			rt->colorTex->CopyTextureToMemory(m_context, 0, 0,
				Offset3D({ (int)x, (int)top, 0 }), Offset3D({ (int)width, (int)height, 1 }), data);

			// Match glReadPixels: the bottom row comes first in the user buffer.
			const size_t rowBytes = data.size / height;
			if (rowBytes > 0)
			{
				std::vector<uint8_t> tmp(rowBytes);
				uint8_t* rows = static_cast<uint8_t*>(data.buffer);
				for (uint32_t i = 0; i < height / 2; ++i)
				{
					uint8_t* a = rows + i * rowBytes;
					uint8_t* b = rows + (height - 1 - i) * rowBytes;
					memcpy(tmp.data(), a, rowBytes);
					memcpy(a, b, rowBytes);
					memcpy(b, tmp.data(), rowBytes);
				}
			}
		}

		void D3D12Driver::readStreamPixels(
			Handle<HwStream> sh,
			uint32_t x,
			uint32_t y,
			uint32_t width,
			uint32_t height,
			PixelBufferDescriptor&& data)
		{

		}

		void D3D12Driver::blit(
			TargetBufferFlags buffers,
			Handle<HwRenderTarget> dst,
			backend::Viewport dstRect,
			Handle<HwRenderTarget> src,
			backend::Viewport srcRect,
			SamplerMagFilter filter)
		{
			if (!(buffers & TargetBufferFlags::COLOR)) {
				return;
			}
			auto dstRt = handle_cast<D3D12RenderTarget>(m_handle_map, dst);
			auto srcRt = handle_cast<D3D12RenderTarget>(m_handle_map, src);
			if (!dstRt || !srcRt || !dstRt->colorTex || !srcRt->colorTex || dstRt->colorTex == srcRt->colorTex) {
				return;
			}
			// Copies and resolves can't scale; a scaling blit would need a draw.
			if (dstRect.width != srcRect.width || dstRect.height != srcRect.height) {
				return;
			}

			D3D12Texture* dstTex = dstRt->colorTex;
			D3D12Texture* srcTex = srcRt->colorTex;
			const int srcTop = (int)srcRt->height - (srcRect.bottom + (int)srcRect.height);
			const int dstTop = (int)dstRt->height - (dstRect.bottom + (int)dstRect.height);

			if (srcRt->samples > 1 && dstRt->samples <= 1)
			{
				// ResolveSubresource has no rect; only whole-surface resolves are possible.
				if (srcRect.left != 0 || srcTop != 0 || dstRect.left != 0 || dstTop != 0 ||
					srcRect.width != srcRt->width || srcRect.height != srcRt->height ||
					dstRt->width != srcRt->width || dstRt->height != srcRt->height)
				{
					return;
				}
				m_context->ensureCommandListRecording();
				srcTex->TransitionTo(m_context, D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
				dstTex->TransitionTo(m_context, D3D12_RESOURCE_STATE_RESOLVE_DEST);
				m_context->m_commandList->ResolveSubresource(dstTex->texture.Get(), 0, srcTex->texture.Get(), 0, dstRt->colorFormat);
				dstTex->TransitionTo(m_context, dstTex->IdleState());
				srcTex->TransitionTo(m_context, srcTex->IdleState());
				return;
			}

			if (srcRt->samples != dstRt->samples) {
				return;
			}
			dstTex->CopyTexture(m_context,
				0, 0,
				Offset3D({ dstRect.left, dstTop, 0 }),
				Offset3D({ (int)dstRect.width, (int)dstRect.height, 1 }),
				srcTex,
				0, 0,
				Offset3D({ srcRect.left, srcTop, 0 }),
				Offset3D({ (int)srcRect.width, (int)srcRect.height, 1 }));
		}

		void D3D12Driver::draw(backend::PipelineState ps, Handle<HwRenderPrimitive> rph)
		{
			auto program = handle_cast<D3D12Program>(m_handle_map, ps.program);
			auto primitive = handle_cast<D3D12RenderPrimitive>(m_handle_map, rph);
			auto vertex_buffer = handle_cast<D3D12VertexBuffer>(m_handle_map, primitive->vertex_buffer);
			auto index_buffer = handle_cast<D3D12IndexBuffer>(m_handle_map, primitive->index_buffer);

			auto DescriptorCache = m_context->GetDescriptorCache();
			auto get_format = [](ElementType type) {
				switch (type)
				{
				case ElementType::FLOAT2: return DXGI_FORMAT_R32G32_FLOAT;
				case ElementType::FLOAT3: return DXGI_FORMAT_R32G32B32_FLOAT;
				case ElementType::FLOAT4: return DXGI_FORMAT_R32G32B32A32_FLOAT;
				case ElementType::UBYTE4: return DXGI_FORMAT_R8G8B8A8_UINT;
				default: assert(false); return DXGI_FORMAT_UNKNOWN;
				}
			};
			// Programs are shared by meshes with different vertex formats, so the layout is per draw.
			std::vector<D3D12_INPUT_ELEMENT_DESC> inputLayout;
			uint64_t inputLayoutKey = 14695981039346656037ull;
			auto hashValue = [&inputLayoutKey](uint64_t v) {
				inputLayoutKey ^= v;
				inputLayoutKey *= 1099511628211ull;
			};
			for (size_t i = 0; i < vertex_buffer->attributes.size(); ++i)
			{
				if (primitive->enabled_attributes & (1 << i))
				{
					const auto& attribute = vertex_buffer->attributes[i];
					D3D12_INPUT_ELEMENT_DESC desc = { };
					desc.SemanticName = "TEXCOORD";
					desc.SemanticIndex = (UINT)i;
					desc.Format = get_format(attribute.type);
					desc.InputSlot = (UINT)0;
					desc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
					desc.AlignedByteOffset = attribute.offset;
					inputLayout.push_back(desc);
					hashValue((uint64_t(i) << 40) | (uint64_t(desc.Format) << 32) | desc.AlignedByteOffset);
				}
			}
			m_context->m_commandList->SetPipelineState(program->GetPSO(m_context, ps, inputLayout, inputLayoutKey).Get());
			m_context->m_commandList->SetGraphicsRootSignature(program->m_RootSignature.Get());
			UINT indexTable = 0;
			//ID3D12DescriptorHeap* descriptorHeaps[] = {
			//	m_context->m_descHeapCBV_SRV_UAV.heap.Get()
			//};
			//m_context->m_commandList->SetDescriptorHeaps(1, descriptorHeaps);
			auto CacheCbvSrvUavDescriptorHeap = m_context->GetDescriptorCache()->GetCacheCbvSrvUavDescriptorHeap();
			ID3D12DescriptorHeap* DescriptorHeaps[] = { CacheCbvSrvUavDescriptorHeap.Get(), m_context->GetSamplerHeap() };
			m_context->m_commandList->SetDescriptorHeaps(2, DescriptorHeaps);

			auto commandList = m_context->m_commandList.Get();
			auto bindCbvs = [&](const std::vector<D3D12_SHADER_INPUT_BIND_DESC>& cbvs, std::map<UINT, UINT>& cbvMap) {
				for (const auto& cbv : cbvs)
				{
					const auto& binding = m_context->uniform_buffer_bindings[cbv.BindPoint];
					if (binding.ConstantBufferRef)
						commandList->SetGraphicsRootConstantBufferView(cbvMap[cbv.BindPoint], binding.ConstantBufferRef->ResourceLocation.GPUVirtualAddress);
				}
			};
			bindCbvs(program->vert_cbv, program->vert_cbv_map);
			bindCbvs(program->frag_cbv, program->frag_cbv_map);

			// Texture t<N> is always paired with sampler s<N>, whichever stage declares them.
			std::vector<bool> rootBound(program->vert_cbv.size() + program->vert_srv.size() + program->vert_sam.size() +
				program->frag_cbv.size() + program->frag_srv.size() + program->frag_sam.size(), false);
			auto bindTable = [&](std::map<UINT, UINT>& map, UINT binding, D3D12_GPU_DESCRIPTOR_HANDLE handle) {
				auto it = map.find(binding);
				if (it != map.end())
				{
					commandList->SetGraphicsRootDescriptorTable(it->second, handle);
					rootBound[it->second] = true;
				}
			};

			const auto& samplers = program->info.getSamplerGroupInfo();
			for (size_t i = 0; i < samplers.size() && i < m_context->sampler_group_binding.size(); ++i)
			{
				if (samplers[i].empty() || !m_context->sampler_group_binding[i].sampler_group)
					continue;

				auto sampler_group = handle_cast<D3D12SamplerGroup>(m_handle_map, m_context->sampler_group_binding[i].sampler_group);
				const auto* groupSamplers = sampler_group->sb->getSamplers();
				const size_t groupSize = sampler_group->sb->getSize();
				for (size_t j = 0; j < samplers[i].size() && j < groupSize; ++j)
				{
					const auto& s = groupSamplers[j];
					if (!s.t)
						continue;

					auto texture = handle_const_cast<D3D12Texture>(m_handle_map, s.t);
					if (!texture->SRV)
						continue;

					const UINT binding = samplers[i][j].binding;
					const bool usedByVs = program->vert_srv_map.count(binding) || program->vert_sam_map.count(binding);
					const bool usedByPs = program->frag_srv_map.count(binding) || program->frag_sam_map.count(binding);
					if (!usedByVs && !usedByPs)
						continue;

					std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> SrcDescriptors{ texture->SRV->GetDescriptorHandle() };
					const auto srvGpu = DescriptorCache->AppendCbvSrvUavDescriptors(SrcDescriptors);
					const auto samplerGpu = m_context->GetSampler(s.s);
					bindTable(program->vert_srv_map, binding, srvGpu);
					bindTable(program->frag_srv_map, binding, srvGpu);
					bindTable(program->vert_sam_map, binding, samplerGpu);
					bindTable(program->frag_sam_map, binding, samplerGpu);
				}
			}

			// Unset descriptor tables are invalid at draw time, so give leftover slots safe defaults.
			auto bindNullSrvs = [&](const std::vector<D3D12_SHADER_INPUT_BIND_DESC>& srvs, std::map<UINT, UINT>& map) {
				for (const auto& srv : srvs)
				{
					const UINT root = map[srv.BindPoint];
					if (rootBound[root])
						continue;
					std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> SrcDescriptors{ m_context->GetNullSRV(srv.Dimension) };
					commandList->SetGraphicsRootDescriptorTable(root, DescriptorCache->AppendCbvSrvUavDescriptors(SrcDescriptors));
				}
			};
			auto bindDefaultSamplers = [&](const std::vector<D3D12_SHADER_INPUT_BIND_DESC>& sams, std::map<UINT, UINT>& map) {
				for (const auto& sam : sams)
				{
					const UINT root = map[sam.BindPoint];
					if (rootBound[root])
						continue;
					SamplerParams params;
					params.filterMag = SamplerMagFilter::LINEAR;
					params.filterMin = SamplerMinFilter::LINEAR_MIPMAP_LINEAR;
					commandList->SetGraphicsRootDescriptorTable(root, m_context->GetSampler(params));
				}
			};
			bindNullSrvs(program->vert_srv, program->vert_srv_map);
			bindNullSrvs(program->frag_srv, program->frag_srv_map);
			bindDefaultSamplers(program->vert_sam, program->vert_sam_map);
			bindDefaultSamplers(program->frag_sam, program->frag_sam_map);

			m_context->SetVertexBuffer(vertex_buffer->VertexBufferRefArray[0], 0, vertex_buffer->stride, vertex_buffer->size);
			m_context->SetIndexBuffer(index_buffer->IndexBufferRef, 0, index_buffer->indexFormat, index_buffer->indexDataSize);
			//m_context->m_commandList->IASetVertexBuffers(0, vertex_buffer->vbvs.size(), &vertex_buffer->vbvs[0]);
			//m_context->m_commandList->IASetIndexBuffer(&index_buffer->ibv);

			m_context->m_commandList->DrawIndexedInstanced(primitive->count, 1, primitive->offset, 0, 0);
		}
	}
}
