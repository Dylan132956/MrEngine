
#include "D3D11Context.h"
#include <algorithm>
#include <assert.h>
#include <cfloat>
#include <cstdio>

namespace filament
{
	namespace backend
	{
		static bool SDKLayersAvailable()
		{
			HRESULT hr = D3D11CreateDevice(
				nullptr,
				D3D_DRIVER_TYPE_NULL,       // 无需创建实际硬件设备。
				0,
				D3D11_CREATE_DEVICE_DEBUG,  // 请检查 SDK 层。
				nullptr,                    // 任何功能级别都会这样。
				0,
				D3D11_SDK_VERSION,          // 对于 Microsoft Store 应用，始终将此值设置为 D3D11_SDK_VERSION。
				nullptr,                    // 无需保留 D3D 设备引用。
				nullptr,                    // 无需知道功能级别。
				nullptr                     // 无需保留 D3D 设备上下文引用。
			);

			return SUCCEEDED(hr);
		}

		D3D11Context::D3D11Context()
		{
			UINT creation_flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#if defined(_DEBUG)
			sdk_layers_available = SDKLayersAvailable();
			if (sdk_layers_available)
			{
				creation_flags |= D3D11_CREATE_DEVICE_DEBUG;
			}
#endif

			D3D_FEATURE_LEVEL feature_levels[] =
			{
				D3D_FEATURE_LEVEL_12_1,
				D3D_FEATURE_LEVEL_12_0,
				D3D_FEATURE_LEVEL_11_1,
				D3D_FEATURE_LEVEL_11_0,
				D3D_FEATURE_LEVEL_10_1,
				D3D_FEATURE_LEVEL_10_0,
				D3D_FEATURE_LEVEL_9_3,
				D3D_FEATURE_LEVEL_9_2,
				D3D_FEATURE_LEVEL_9_1
			};

			ID3D11DeviceContext* context_0 = nullptr;
			ID3D11Device* device_0 = nullptr;
			HRESULT hr = D3D11CreateDevice(
				nullptr,					// 指定 nullptr 以使用默认适配器。
				D3D_DRIVER_TYPE_HARDWARE,	// 创建使用硬件图形驱动程序的设备。
				0,							// 应为 0，除非驱动程序是 D3D_DRIVER_TYPE_SOFTWARE。
				creation_flags,				// 设置调试和 Direct2D 兼容性标志。
				feature_levels,				// 此应用程序可以支持的功能级别的列表。
				ARRAYSIZE(feature_levels),	// 上面的列表的大小。
				D3D11_SDK_VERSION,			// 对于 Microsoft Store 应用，始终将此值设置为 D3D11_SDK_VERSION。
				&device_0,					// 返回创建的 Direct3D 设备。
				&feature_level,				// 返回所创建设备的功能级别。
				&context_0					// 返回设备的即时上下文。
			);

			if (FAILED(hr))
			{
				hr = D3D11CreateDevice(
					nullptr,
					D3D_DRIVER_TYPE_WARP, // 创建 WARP 设备而不是硬件设备。
					0,
					creation_flags,
					feature_levels,
					ARRAYSIZE(feature_levels),
					D3D11_SDK_VERSION,
					&device_0,
					&feature_level,
					&context_0
				);

				assert(SUCCEEDED(hr));
			}

			device_0->QueryInterface(__uuidof(ID3D11Device3), (void**) &device);
			context_0->QueryInterface(__uuidof(ID3D11DeviceContext3), (void**) &context);

			SAFE_RELEASE(device_0);
			SAFE_RELEASE(context_0);

			// Determine maximum supported MSAA level for the swapchain color format.
			UINT samples;
			for (samples = maxSamples; samples > 1; samples /= 2) {
				UINT colorLevels = 0;
				UINT depthLevels = 0;
				device->CheckMultisampleQualityLevels(DXGI_FORMAT_B8G8R8A8_UNORM, samples, &colorLevels);
				device->CheckMultisampleQualityLevels(DXGI_FORMAT_D24_UNORM_S8_UINT, samples, &depthLevels);
				if (colorLevels > 0 && depthLevels > 0) {
					break;
				}
			}
			maxSamples = samples;
			if (msaaSamples > maxSamples) {
				msaaSamples = maxSamples;
			}
			std::printf("D3D11 MSAA samples: %u (max %u)\n", msaaSamples, maxSamples);
		}

		void D3D11Context::releaseMSAA()
		{
			SAFE_RELEASE(msaaColorView);
			SAFE_RELEASE(msaaDepthView);
			SAFE_RELEASE(msaaColor);
			SAFE_RELEASE(msaaDepth);
			msaaWidth = 0;
			msaaHeight = 0;
			msaaColorFormat = DXGI_FORMAT_UNKNOWN;
		}

		void D3D11Context::ensureMSAA(UINT width, UINT height, DXGI_FORMAT colorFormat)
		{
			if (msaaSamples <= 1) {
				return;
			}
			if (msaaColor && msaaWidth == width && msaaHeight == height && msaaColorFormat == colorFormat) {
				return;
			}
			releaseMSAA();
			msaaWidth = width;
			msaaHeight = height;
			msaaColorFormat = colorFormat;

			D3D11_TEXTURE2D_DESC1 colorDesc = {};
			colorDesc.Width = width;
			colorDesc.Height = height;
			colorDesc.MipLevels = 1;
			colorDesc.ArraySize = 1;
			colorDesc.Format = colorFormat;
			colorDesc.SampleDesc.Count = msaaSamples;
			colorDesc.SampleDesc.Quality = 0;
			colorDesc.Usage = D3D11_USAGE_DEFAULT;
			colorDesc.BindFlags = D3D11_BIND_RENDER_TARGET;
			HRESULT hr = device->CreateTexture2D1(&colorDesc, nullptr, &msaaColor);
			assert(SUCCEEDED(hr));

			D3D11_RENDER_TARGET_VIEW_DESC1 rtvDesc = {};
			rtvDesc.Format = colorFormat;
			rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
			hr = device->CreateRenderTargetView1(msaaColor, &rtvDesc, &msaaColorView);
			assert(SUCCEEDED(hr));

			D3D11_TEXTURE2D_DESC1 depthDesc = colorDesc;
			depthDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
			depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
			hr = device->CreateTexture2D1(&depthDesc, nullptr, &msaaDepth);
			assert(SUCCEEDED(hr));

			D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
			dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
			dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
			hr = device->CreateDepthStencilView(msaaDepth, &dsvDesc, &msaaDepthView);
			assert(SUCCEEDED(hr));
		}

		D3D11Context::~D3D11Context()
		{
			releaseMSAA();

			for (auto i : samplers)
			{
				SAFE_RELEASE(i.second);
			}
			samplers.clear();

			for (auto i : rasterizer_states)
			{
				SAFE_RELEASE(i.second.raster);
				SAFE_RELEASE(i.second.blend);
				SAFE_RELEASE(i.second.depth);
			}
			rasterizer_states.clear();

			SAFE_RELEASE(device);
			SAFE_RELEASE(context);
		}

		void D3D11Context::SetState(const backend::PipelineState& ps)
		{
			auto rs = rasterizer_states.find(ps.rasterState.u);
			if (rs == rasterizer_states.end())
			{
				D3D11_RASTERIZER_DESC raster_desc = { };
				raster_desc.FillMode = D3D11_FILL_SOLID;
				raster_desc.FrontCounterClockwise = !ps.rasterState.inverseFrontFaces;
				raster_desc.DepthClipEnable = TRUE;
				raster_desc.ScissorEnable = TRUE;

				switch (ps.rasterState.culling)
				{
					case CullingMode::NONE:
						raster_desc.CullMode = D3D11_CULL_NONE;
						break;
					case CullingMode::FRONT:
						raster_desc.CullMode = D3D11_CULL_FRONT;
						break;
					case CullingMode::BACK:
						raster_desc.CullMode = D3D11_CULL_BACK;
						break;
					default:
						assert(false);
						break;
				}

				ID3D11RasterizerState* raster = nullptr;
				HRESULT hr = device->CreateRasterizerState(&raster_desc, &raster);
				assert(SUCCEEDED(hr));

				auto get_blend_func = [](BlendFunction func) {
					switch (func)
					{
						case BlendFunction::ZERO: return D3D11_BLEND_ZERO;
						case BlendFunction::ONE: return D3D11_BLEND_ONE;
						case BlendFunction::SRC_COLOR: return D3D11_BLEND_SRC_COLOR;
						case BlendFunction::ONE_MINUS_SRC_COLOR: return D3D11_BLEND_INV_SRC_COLOR;
						case BlendFunction::DST_COLOR: return D3D11_BLEND_DEST_COLOR;
						case BlendFunction::ONE_MINUS_DST_COLOR: return D3D11_BLEND_INV_DEST_COLOR;
						case BlendFunction::SRC_ALPHA: return D3D11_BLEND_SRC_ALPHA;
						case BlendFunction::ONE_MINUS_SRC_ALPHA: return D3D11_BLEND_INV_SRC_ALPHA;
						case BlendFunction::DST_ALPHA: return D3D11_BLEND_DEST_ALPHA;
						case BlendFunction::ONE_MINUS_DST_ALPHA: return D3D11_BLEND_INV_DEST_ALPHA;
						case BlendFunction::SRC_ALPHA_SATURATE: return D3D11_BLEND_SRC_ALPHA_SAT;
						default: assert(false);
					}
					return D3D11_BLEND_ZERO;
				};
				auto get_blend_op = [](BlendEquation op) {
					switch (op)
					{
						case BlendEquation::ADD: return D3D11_BLEND_OP_ADD;
						case BlendEquation::SUBTRACT: return D3D11_BLEND_OP_SUBTRACT;
						case BlendEquation::REVERSE_SUBTRACT: return D3D11_BLEND_OP_REV_SUBTRACT;
						case BlendEquation::MIN: return D3D11_BLEND_OP_MIN;
						case BlendEquation::MAX: return D3D11_BLEND_OP_MAX;
						default: assert(false);
					}
					return D3D11_BLEND_OP_ADD;
				};

				D3D11_BLEND_DESC blend_desc = { };
				blend_desc.AlphaToCoverageEnable = ps.rasterState.alphaToCoverage;
				for (UINT i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
				{
					auto& target = blend_desc.RenderTarget[i];
					target.BlendEnable = ps.rasterState.hasBlending();
					target.SrcBlend = get_blend_func(ps.rasterState.blendFunctionSrcRGB);
					target.DestBlend = get_blend_func(ps.rasterState.blendFunctionDstRGB);
					target.SrcBlendAlpha = get_blend_func(ps.rasterState.blendFunctionSrcAlpha);
					target.DestBlendAlpha = get_blend_func(ps.rasterState.blendFunctionDstAlpha);
					target.BlendOp = get_blend_op(ps.rasterState.blendEquationRGB);
					target.BlendOpAlpha = get_blend_op(ps.rasterState.blendEquationAlpha);
					target.RenderTargetWriteMask = ps.rasterState.colorWrite ? D3D11_COLOR_WRITE_ENABLE_ALL : 0;
				}

				ID3D11BlendState* blend = nullptr;
				hr = device->CreateBlendState(&blend_desc, &blend);
				assert(SUCCEEDED(hr));

				auto get_depth_func = [](SamplerCompareFunc func) {
					switch (func)
					{
						case SamplerCompareFunc::LE: return D3D11_COMPARISON_LESS_EQUAL;
						case SamplerCompareFunc::GE: return D3D11_COMPARISON_GREATER_EQUAL;
						case SamplerCompareFunc::L: return D3D11_COMPARISON_LESS;
						case SamplerCompareFunc::G: return D3D11_COMPARISON_GREATER;
						case SamplerCompareFunc::E: return D3D11_COMPARISON_EQUAL;
						case SamplerCompareFunc::NE: return D3D11_COMPARISON_NOT_EQUAL;
						case SamplerCompareFunc::A: return D3D11_COMPARISON_ALWAYS;
						case SamplerCompareFunc::N: return D3D11_COMPARISON_NEVER;
						default: assert(false);
					}
					return D3D11_COMPARISON_LESS_EQUAL;
				};

				D3D11_DEPTH_STENCIL_DESC depth_desc = { };
				depth_desc.DepthEnable = TRUE;
				depth_desc.DepthWriteMask = ps.rasterState.depthWrite ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
				depth_desc.DepthFunc = get_depth_func(ps.rasterState.depthFunc);

				ID3D11DepthStencilState* depth = nullptr;
				hr = device->CreateDepthStencilState(&depth_desc, &depth);
				assert(SUCCEEDED(hr));

				D3D11Context::RenderState state;
				state.raster = raster;
				state.blend = blend;
				state.depth = depth;
				rasterizer_states.insert({ ps.rasterState.u, state });

				rs = rasterizer_states.find(ps.rasterState.u);
			}

			context->RSSetState(rs->second.raster);
			context->OMSetBlendState(rs->second.blend, nullptr, D3D11_DEFAULT_SAMPLE_MASK);
			context->OMSetDepthStencilState(rs->second.depth, D3D11_DEFAULT_STENCIL_REFERENCE);
		}

		ID3D11SamplerState* D3D11Context::GetSampler(const backend::SamplerParams& s)
		{
			auto find = samplers.find(s.u);
			if (find != samplers.end())
			{
				return find->second;
			}

			const bool minLinear =
				s.filterMin == SamplerMinFilter::LINEAR ||
				s.filterMin == SamplerMinFilter::LINEAR_MIPMAP_NEAREST ||
				s.filterMin == SamplerMinFilter::LINEAR_MIPMAP_LINEAR;
			const bool magLinear = s.filterMag == SamplerMagFilter::LINEAR;
			const bool mipLinear =
				s.filterMin == SamplerMinFilter::NEAREST_MIPMAP_LINEAR ||
				s.filterMin == SamplerMinFilter::LINEAR_MIPMAP_LINEAR;
			const bool hasMips =
				s.filterMin != SamplerMinFilter::NEAREST &&
				s.filterMin != SamplerMinFilter::LINEAR;
			const bool compare = s.compareMode == SamplerCompareMode::COMPARE_TO_TEXTURE;

			auto toAddress = [](SamplerWrapMode mode) {
				switch (mode)
				{
				case SamplerWrapMode::REPEAT: return D3D11_TEXTURE_ADDRESS_WRAP;
				case SamplerWrapMode::MIRRORED_REPEAT: return D3D11_TEXTURE_ADDRESS_MIRROR;
				default: return D3D11_TEXTURE_ADDRESS_CLAMP;
				}
			};
			auto toCompare = [](SamplerCompareFunc func) {
				switch (func)
				{
				case SamplerCompareFunc::LE: return D3D11_COMPARISON_LESS_EQUAL;
				case SamplerCompareFunc::GE: return D3D11_COMPARISON_GREATER_EQUAL;
				case SamplerCompareFunc::L: return D3D11_COMPARISON_LESS;
				case SamplerCompareFunc::G: return D3D11_COMPARISON_GREATER;
				case SamplerCompareFunc::E: return D3D11_COMPARISON_EQUAL;
				case SamplerCompareFunc::NE: return D3D11_COMPARISON_NOT_EQUAL;
				case SamplerCompareFunc::A: return D3D11_COMPARISON_ALWAYS;
				default: return D3D11_COMPARISON_NEVER;
				}
			};

			D3D11_SAMPLER_DESC sampler_desc = {};
			if (s.anisotropyLog2 > 0 && minLinear && magLinear)
			{
				sampler_desc.Filter = D3D11_ENCODE_ANISOTROPIC_FILTER(compare);
				sampler_desc.MaxAnisotropy = (std::min)(16u, 1u << s.anisotropyLog2);
			}
			else
			{
				sampler_desc.Filter = D3D11_ENCODE_BASIC_FILTER(
					minLinear ? D3D11_FILTER_TYPE_LINEAR : D3D11_FILTER_TYPE_POINT,
					magLinear ? D3D11_FILTER_TYPE_LINEAR : D3D11_FILTER_TYPE_POINT,
					mipLinear ? D3D11_FILTER_TYPE_LINEAR : D3D11_FILTER_TYPE_POINT,
					compare);
				sampler_desc.MaxAnisotropy = 1;
			}
			sampler_desc.AddressU = toAddress(s.wrapS);
			sampler_desc.AddressV = toAddress(s.wrapT);
			sampler_desc.AddressW = toAddress(s.wrapR);
			sampler_desc.ComparisonFunc = compare ? toCompare(s.compareFunc) : D3D11_COMPARISON_NEVER;
			sampler_desc.MinLOD = 0.0f;
			sampler_desc.MaxLOD = hasMips ? FLT_MAX : 0.0f;

			ID3D11SamplerState* sampler = nullptr;
			HRESULT hr = device->CreateSamplerState(&sampler_desc, &sampler);
			assert(SUCCEEDED(hr));

			samplers.insert({ s.u, sampler });
			return sampler;
		}

		DXGI_FORMAT D3D11Context::GetTextureFormat(TextureFormat format)
		{
			switch (format)
			{
				case TextureFormat::R8: return DXGI_FORMAT_R8_UNORM;
				case TextureFormat::RGBA8: return DXGI_FORMAT_R8G8B8A8_UNORM;
                case TextureFormat::R16F: return DXGI_FORMAT_R16_FLOAT;
                case TextureFormat::RGBA16F: return DXGI_FORMAT_R16G16B16A16_FLOAT;
                case TextureFormat::R32F: return DXGI_FORMAT_R32_FLOAT;
                case TextureFormat::RGBA32F: return DXGI_FORMAT_R32G32B32A32_FLOAT;
				case TextureFormat::DEPTH16: return DXGI_FORMAT_R16_TYPELESS;
				case TextureFormat::DEPTH24: return DXGI_FORMAT_UNKNOWN;
				case TextureFormat::DEPTH24_STENCIL8: return DXGI_FORMAT_R24G8_TYPELESS;
				case TextureFormat::DEPTH32F: return DXGI_FORMAT_R32_TYPELESS;
				case TextureFormat::DEPTH32F_STENCIL8: return DXGI_FORMAT_R32G8X24_TYPELESS;
				case TextureFormat::DXT1_RGB: return DXGI_FORMAT_BC1_UNORM;
				case TextureFormat::DXT1_RGBA: return DXGI_FORMAT_BC1_UNORM;
				case TextureFormat::DXT3_RGBA: return DXGI_FORMAT_BC2_UNORM;
				case TextureFormat::DXT5_RGBA: return DXGI_FORMAT_BC3_UNORM;
				default: assert(false); break;
			}
			return DXGI_FORMAT_UNKNOWN;
		}

		DXGI_FORMAT D3D11Context::GetTextureViewFormat(TextureFormat format)
		{
			switch (format)
			{
				case TextureFormat::R8: return DXGI_FORMAT_R8_UNORM;
				case TextureFormat::RGBA8: return DXGI_FORMAT_R8G8B8A8_UNORM;
                case TextureFormat::R16F: return DXGI_FORMAT_R16_FLOAT;
                case TextureFormat::RGBA16F: return DXGI_FORMAT_R16G16B16A16_FLOAT;
                case TextureFormat::R32F: return DXGI_FORMAT_R32_FLOAT;
                case TextureFormat::RGBA32F: return DXGI_FORMAT_R32G32B32A32_FLOAT;
				case TextureFormat::DEPTH16: return DXGI_FORMAT_R16_UNORM;
				case TextureFormat::DEPTH24: return DXGI_FORMAT_UNKNOWN;
				case TextureFormat::DEPTH24_STENCIL8: return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
				case TextureFormat::DEPTH32F: return DXGI_FORMAT_R32_FLOAT;
				case TextureFormat::DEPTH32F_STENCIL8: return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
				case TextureFormat::DXT1_RGB: return DXGI_FORMAT_BC1_UNORM;
				case TextureFormat::DXT1_RGBA: return DXGI_FORMAT_BC1_UNORM;
				case TextureFormat::DXT3_RGBA: return DXGI_FORMAT_BC2_UNORM;
				case TextureFormat::DXT5_RGBA: return DXGI_FORMAT_BC3_UNORM;
				default: assert(false); break;
			}
			return DXGI_FORMAT_UNKNOWN;
		}

		DXGI_FORMAT D3D11Context::GetDepthViewFormat(TextureFormat format)
		{
			switch (format)
			{
				case TextureFormat::DEPTH16: return DXGI_FORMAT_D16_UNORM;
				case TextureFormat::DEPTH24: return DXGI_FORMAT_UNKNOWN;
				case TextureFormat::DEPTH24_STENCIL8: return DXGI_FORMAT_D24_UNORM_S8_UINT;
				case TextureFormat::DEPTH32F: return DXGI_FORMAT_D32_FLOAT;
				case TextureFormat::DEPTH32F_STENCIL8: return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
				default: assert(false); break;
			}
			return DXGI_FORMAT_UNKNOWN;
		}
	}
}
