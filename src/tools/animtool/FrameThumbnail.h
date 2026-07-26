#ifndef _ANIMTOOL_FRAME_THUMBNAIL_H_
#define _ANIMTOOL_FRAME_THUMBNAIL_H_

#include <filesystem>

#include <imgui/vendor/imgui.h>
#include <vulkan/vulkan.hpp>

#include <assets/ImageLoader.h>
#include <graphics/ManagedVulkanResources.h>
#include <graphics/Renderer.h>

namespace animtool
{

class FrameThumbnail
{
public:
	FrameThumbnail() = default;
	~FrameThumbnail();

	FrameThumbnail( const FrameThumbnail& ) = delete;
	FrameThumbnail& operator=( const FrameThumbnail& ) = delete;
	FrameThumbnail( FrameThumbnail&& other ) noexcept;
	FrameThumbnail& operator=( FrameThumbnail&& other ) noexcept;

	void Upload(
		eage::graphics::Renderer& renderer,
		const std::filesystem::path& source_path,
		const assets::ImageLoader::Image& image,
		int frame_index_in_source = 0,
		int delay_ms = 100 );
	void ReleaseImGuiTexture();

	ImTextureID GetTextureId() const;
	uint32_t GetBindlessTextureIndex() const { return mBindlessTextureIndex; }

	const std::filesystem::path& GetSourcePath() const { return mSourcePath; }
	int GetFrameIndexInSource() const { return mFrameIndexInSource; }
	int GetWidth() const { return mWidth; }
	int GetHeight() const { return mHeight; }
	int GetDelayMs() const { return mDelayMs; }
	int& GetDelayMsMutable() { return mDelayMs; }
	void SetDelayMs( int delay_ms ) { mDelayMs = delay_ms; }

private:
	std::filesystem::path mSourcePath;
	int mFrameIndexInSource = 0;
	int mWidth = 0;
	int mHeight = 0;
	int mDelayMs = 100;

	eage::graphics::ManagedImage::Ptr mGpuImage;
	uint32_t mBindlessTextureIndex = 0;
	VkDescriptorSet mImGuiDescriptor = VK_NULL_HANDLE;
};

}

#endif // _ANIMTOOL_FRAME_THUMBNAIL_H_
