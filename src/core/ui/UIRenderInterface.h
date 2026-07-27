#ifndef _EAGE_UI_RENDER_INTERFACE_H_
#define _EAGE_UI_RENDER_INTERFACE_H_

#include <cstdint>
#include <deque>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>
#include <RmlUi/Core/RenderInterface.h>

#include <graphics/ManagedVulkanResources.h>

namespace eage::graphics
{
	class Renderer;
	class CommandBuffer;
}

namespace eage::ui
{
	struct UIVertex
	{
		glm::vec2 position;
		glm::vec2 tex_coord;
		glm::vec4 colour;
	};

	struct UIPushConstants
	{
		glm::vec2 scale;
		glm::vec2 translation;
		uint64_t  vertex_buffer_address;
		uint32_t  texture_index;
		uint32_t  has_texture;
	};

	class UIRenderInterface final : public Rml::RenderInterface
	{
	public:
		explicit UIRenderInterface( graphics::Renderer& renderer );
		~UIRenderInterface() override;

		void BeginFrame( graphics::CommandBuffer& cmd, uint32_t target_width, uint32_t target_height );
		void EndFrame();
		void AdvanceFrame();

		void BindExternalImage( const std::string& name, graphics::ManagedImage& image );

		Rml::CompiledGeometryHandle CompileGeometry( Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices ) override;
		void RenderGeometry( Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation, Rml::TextureHandle texture ) override;
		void ReleaseGeometry( Rml::CompiledGeometryHandle geometry ) override;

		Rml::TextureHandle LoadTexture( Rml::Vector2i& texture_dimensions, const Rml::String& source ) override;
		Rml::TextureHandle GenerateTexture( Rml::Span<const Rml::byte> source, Rml::Vector2i source_dimensions ) override;
		void ReleaseTexture( Rml::TextureHandle texture ) override;

		void EnableScissorRegion( bool enable ) override;
		void SetScissorRegion( Rml::Rectanglei region ) override;

	private:
		struct GeometryRecord
		{
			graphics::ManagedBuffer::Ptr vertex_buffer;
			graphics::ManagedBuffer::Ptr index_buffer;
			uint64_t vertex_address = 0;
			uint32_t index_count = 0;
		};

		struct TextureRecord
		{
			uint32_t bindless_index = 0;
			graphics::ManagedImage::Ptr owned_image;
			bool external = false;
			uint32_t width = 0;
			uint32_t height = 0;
		};

		struct PendingGeometryRelease
		{
			uint64_t frame_number = 0;
			graphics::ManagedBuffer::Ptr vertex_buffer;
			graphics::ManagedBuffer::Ptr index_buffer;
		};

		bool EnsurePipeline();
		Rml::TextureHandle AllocateTextureHandle();
		void DrainDeferredReleases();
		static std::string basename_of( const std::string& path );

		graphics::Renderer& mRenderer;

		vk::UniqueShaderModule mVertModule;
		vk::UniqueShaderModule mFragModule;
		vk::UniquePipelineLayout mPipelineLayout;
		vk::UniquePipeline mPipeline;
		bool mPipelineReady = false;

		vk::CommandBuffer mActiveCmd;
		bool mHasActiveCmd = false;
		uint32_t mTargetWidth = 1;
		uint32_t mTargetHeight = 1;
		bool mScissorEnabled = false;
		vk::Rect2D mScissor{};

		uint64_t mFrameNumber = 0;
		Rml::CompiledGeometryHandle mNextGeometryHandle = 1;
		Rml::TextureHandle mNextTextureHandle = 1;

		std::unordered_map<Rml::CompiledGeometryHandle, GeometryRecord> mGeometries;
		std::unordered_map<Rml::TextureHandle, TextureRecord> mTextures;
		std::unordered_map<std::string, Rml::TextureHandle> mNamedTextures;
		std::deque<PendingGeometryRelease> mPendingReleases;
	};
}

#endif // _EAGE_UI_RENDER_INTERFACE_H_
