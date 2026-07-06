#include "FrameThumbnail.h"

#include <imgui/imgui.h>
#include <imgui/imgui_impl_vulkan.h>

using namespace animtool;

FrameThumbnail::~FrameThumbnail()
{
	ReleaseImGuiTexture();
}

FrameThumbnail::FrameThumbnail( FrameThumbnail&& other ) noexcept
	: mSourcePath( std::move( other.mSourcePath ) )
	, mFrameIndexInSource( other.mFrameIndexInSource )
	, mWidth( other.mWidth )
	, mHeight( other.mHeight )
	, mDelayMs( other.mDelayMs )
	, mGpuImage( std::move( other.mGpuImage ) )
	, mBindlessTextureIndex( other.mBindlessTextureIndex )
	, mImGuiDescriptor( other.mImGuiDescriptor )
{
	other.mFrameIndexInSource = 0;
	other.mWidth = 0;
	other.mHeight = 0;
	other.mDelayMs = 100;
	other.mBindlessTextureIndex = 0;
	other.mImGuiDescriptor = VK_NULL_HANDLE;
}

FrameThumbnail&
FrameThumbnail::operator=( FrameThumbnail&& other ) noexcept
{
	if( this == &other )
	{
		return *this;
	}

	ReleaseImGuiTexture();

	mSourcePath = std::move( other.mSourcePath );
	mFrameIndexInSource = other.mFrameIndexInSource;
	mWidth = other.mWidth;
	mHeight = other.mHeight;
	mDelayMs = other.mDelayMs;
	mGpuImage = std::move( other.mGpuImage );
	mBindlessTextureIndex = other.mBindlessTextureIndex;
	mImGuiDescriptor = other.mImGuiDescriptor;

	other.mFrameIndexInSource = 0;
	other.mWidth = 0;
	other.mHeight = 0;
	other.mDelayMs = 100;
	other.mBindlessTextureIndex = 0;
	other.mImGuiDescriptor = VK_NULL_HANDLE;

	return *this;
}

void
FrameThumbnail::Upload(
	eage::graphics::Renderer& renderer,
	const std::filesystem::path& source_path,
	const assets::ImageLoader::Image& image,
	int frame_index_in_source,
	int delay_ms )
{
	ReleaseImGuiTexture();

	mSourcePath = source_path;
	mFrameIndexInSource = frame_index_in_source;
	mWidth = image.width;
	mHeight = image.height;
	mDelayMs = delay_ms;

	mGpuImage = renderer.UploadImage(
		const_cast<void*>( static_cast<const void*>( image.data.data() ) ),
		image.data.size(),
		static_cast<uint32_t>( image.width ),
		static_cast<uint32_t>( image.height ),
		vk::Format::eR8G8B8A8Srgb,
		vk::ImageUsageFlagBits::eSampled,
		vk::ImageAspectFlagBits::eColor,
		1 );

	mBindlessTextureIndex = renderer.RegisterBindlessTexture(
		mGpuImage->image_view.get(),
		renderer.GetDefaultSampler() );

	mImGuiDescriptor = ImGui_ImplVulkan_AddTexture(
		renderer.GetDefaultSampler(),
		mGpuImage->image_view.get(),
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
