#include "FrameThumbnail.h"

#include <imgui/imgui.h>
#include <imgui/imgui_impl_vulkan.h>

using namespace animtool;

FrameThumbnail::~FrameThumbnail()
{
	ReleaseImGuiTexture();
}

void
FrameThumbnail::Upload( eage::graphics::Renderer& renderer, const std::filesystem::path& source_path, const assets::ImageLoader::Image& image )
{
	ReleaseImGuiTexture();

	mSourcePath = source_path;
	mWidth = image.width;
	mHeight = image.height;

	mGpuImage = renderer.UploadImage(
		const_cast<void*>( static_cast<const void*>( image.data.data() ) ),
		image.data.size(),
		static_cast<uint32_t>( image.width ),
		static_cast<uint32_t>( image.height ),
		vk::Format::eR8G8B8A8Srgb,
		vk::ImageUsageFlagBits::eSampled,
		vk::ImageAspectFlagBits::eColor,
		1 );

	mSampler = renderer.CreateSampler( vk::Filter::eNearest, vk::Filter::eNearest );

	mImGuiDescriptor = ImGui_ImplVulkan_AddTexture(
		*mSampler,
		*mGpuImage->image_view,
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL );
}

void
FrameThumbnail::ReleaseImGuiTexture()
{
	if( mImGuiDescriptor != VK_NULL_HANDLE )
	{
		ImGui_ImplVulkan_RemoveTexture( mImGuiDescriptor );
		mImGuiDescriptor = VK_NULL_HANDLE;
	}
}

ImTextureID
FrameThumbnail::GetTextureId() const
{
	return reinterpret_cast<ImTextureID>( mImGuiDescriptor );
}
