#ifndef _EAGE_RENDERER_H
#define _EAGE_RENDERER_H

#include <memory>
#include <vector>
#include <span>

#include <vulkan/vulkan.hpp>

struct SDL_Window;

namespace graphics
{
	class VulkanContext;
	class VulkanSwapChain;
	class VulkanCommandContext;
	class VulkanSampler;
	class DynamicDescriptorAllocator;
	class Renderable;
	class ImGUILifetime;
	struct VMAWrapper;
	struct ManagedImage;
	struct Vertex;
	struct GPUMeshBuffers;
	struct GPUImageBuffers;
	struct ManagedBuffer;
	class AbstractCamera;

	///
	/// Renderer class
	///
	class Renderer
	{
	public:
		struct Frame
		{
			std::unique_ptr<VulkanCommandContext> command_context;
			std::unique_ptr<DynamicDescriptorAllocator> descriptor_allocator;
			vk::UniqueDescriptorSet renderable_descriptor_set;
			std::unique_ptr<ManagedBuffer, std::function<void(ManagedBuffer*)>> renderable_uniform_buffer;
		};

		///
		/// Constructor
		///
		Renderer( SDL_Window& window );

		///
		/// Destructor
		///
		virtual ~Renderer();

		///
		/// Initialize the renderer
		///
		/// @return
		///  true if successful, false otherwise
		///
		bool Init();

		///
		/// Output the rendered frame
		///
		void Render();

		///
		/// Add a renderable to the render queue
		///
		/// @param renderable
		///  The renderable to add
		///
		void AddToRenderQueue( std::shared_ptr<Renderable> renderable );

		///
		/// Wait for the renderer to be idle
		///
		void WaitForIdle();

		///
		/// Immediately upload a mesh to the GPU
		///
		std::unique_ptr<GPUMeshBuffers> UploadMesh( std::span<uint32_t> indices, std::span<Vertex> vertices );

		///
		/// Immediately upload an image to the GPU
		///
		template <typename T>
		std::unique_ptr<GPUImageBuffers> UploadImage( 
			std::span<T> image_data,
			uint32_t width, uint32_t height,
			vk::Format format,
			vk::ImageUsageFlags usage,
			vk::ImageAspectFlags aspect_flags,
			uint32_t mip_levels );

		void SetCamera( std::shared_ptr<AbstractCamera> camera );

		std::unique_ptr<VulkanSampler> CreateSampler( vk::Filter min_filter, vk::Filter mag_filter );

		vk::Device& GetDevice();

	private:
		Frame& GetCurrentFrame();
		
		void Submit();

		void Present( uint32_t image_index );

		void InitFrameResources();
		void InitDescriptors();
		void InitPipelines();
		bool InitMeshPipeline();
		void InitImGUI();

		void ImmediateSubmit( std::function<void( vk::CommandBuffer& )> work );

		void PrepareImGUI();

		void DrawRenderables( vk::CommandBuffer& cmd );

		SDL_Window& mWindow;
		std::unique_ptr<VulkanContext>		mContext;
		std::unique_ptr<VulkanSwapChain>	mSwapChain;

		std::unique_ptr<VMAWrapper> mVMA;
		std::unique_ptr<ManagedImage, std::function<void(ManagedImage*)>> mRenderImage;
		std::unique_ptr<ManagedImage, std::function<void(ManagedImage*)>> mDepthImage;

		std::unique_ptr<DynamicDescriptorAllocator> 	mGlobalDescriptorAllocator; //< Don't use this for per frame data
		vk::UniqueDescriptorSet 						mRenderImageDescriptorSet;
		vk::UniqueDescriptorSetLayout					mRenderImageDescriptorSetLayout;

		std::unique_ptr<VulkanCommandContext> mImmidiateCommandContext;
		std::unique_ptr<ImGUILifetime> mImGUILifetime;

		vk::UniquePipeline mMeshPipeline;
		vk::UniquePipelineLayout mMeshPipelineLayout;

		std::vector<std::shared_ptr<Renderable>> mRenderQueue;
		vk::UniqueDescriptorSetLayout mRenderableFixedDescriptorSetLayout;

		std::vector<Frame>					mFrames;
		int64_t								mFrameNumber;

		std::shared_ptr<AbstractCamera> mCamera;
	};
}

#endif // _EAGE_RENDERER_H