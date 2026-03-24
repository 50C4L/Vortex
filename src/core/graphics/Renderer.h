#ifndef _EAGE_RENDERER_H
#define _EAGE_RENDERER_H

#include <memory>
#include <vector>

#include <vulkan/vulkan.hpp>

#include <graphics/RenderInfo.h>

struct SDL_Window;

namespace eage::graphics
{
	class VulkanContext;
	class VulkanSwapChain;
	class VulkanCommandContext;
	class VulkanSampler;
	class DynamicDescriptorAllocator;
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
		};

		static const int MAX_FRAMES_IN_FLIGHT = 2;

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
		/// Add a RenderInfo to the render queue
		///
		/// @param RenderInfo
		///  The RenderInfo to add
		///
		void AddToRenderQueue( RenderInfo render_info );

		///
		/// Wait for the renderer to be idle
		///
		void WaitForIdle();

		///
		/// Immediately upload a mesh to the GPU
		///
		std::unique_ptr<GPUMeshBuffers> UploadMesh( const std::vector<uint32_t>& indices, const std::vector<Vertex>& vertices );

		///
		/// Immediately upload an image to the GPU
		///
		std::unique_ptr<ManagedImage, std::function<void(ManagedImage*)>> UploadImage( 
			void* data,
			size_t size,
			uint32_t width, uint32_t height,
			vk::Format format,
			vk::ImageUsageFlags usage,
			vk::ImageAspectFlags aspect_flags,
			uint32_t mip_levels );

		vk::UniqueSampler CreateSampler( vk::Filter min_filter, vk::Filter mag_filter );

		vk::Device GetDevice();

		std::vector<Frame>& GetFrames();

		VMAWrapper& GetMemoryAllocator();

		vk::Format GetDepthFormat();
		vk::Format GetColorFormat();

		size_t GetCurrentFrameIndex() const;

		struct BuiltInDescriptorSetLayouts
		{
			vk::UniqueDescriptorSetLayout global;
			vk::UniqueDescriptorSetLayout per_object;
		};
		BuiltInDescriptorSetLayouts& GetBuiltInDescriptorSetLayouts();

		///
		/// Set the ImGUI render function, should use for debugging rendering or editor GUI rendering
		/// Gameplay GUI should be rendered with `RenderComponent`
		///
		void SetImGUIRenderFunction( std::function<void()> render_function );

		float GetGPUFrameTime() const { return mGPUFrameTime; }

	private:
		Frame& GetCurrentFrame();
		
		void Submit();

		void Present( uint32_t image_index );

		void InitFrameResources();
		void InitDescriptors();
		void InitImGUI();

		void ImmediateSubmit( std::function<void( vk::CommandBuffer& )> work );

		void PrepareImGUI();

		void DrawRenderQueue( vk::CommandBuffer& cmd );

		void InitGPUTiming();
		void UpdateGPUTiming();

		SDL_Window& mWindow;
		std::unique_ptr<VulkanContext>		mContext;
		std::unique_ptr<VulkanSwapChain>	mSwapChain;

		std::unique_ptr<VMAWrapper> mVMA;
		std::unique_ptr<ManagedImage, std::function<void(ManagedImage*)>> mRenderImage;
		std::unique_ptr<ManagedImage, std::function<void(ManagedImage*)>> mDepthImage;

		std::unique_ptr<DynamicDescriptorAllocator> 	mGlobalDescriptorAllocator; //< Don't use this for per frame data

		std::unique_ptr<VulkanCommandContext> mImmidiateCommandContext;
		std::unique_ptr<ImGUILifetime> mImGUILifetime;
		std::function<void()> mImGUIRenderFunction;

		std::vector<Frame>					mFrames;
		uint64_t							mFrameNumber;

		BuiltInDescriptorSetLayouts mBuiltInDescriptorSetLayouts;

		// The queue shold be clear first when destorying the renderer
		std::vector<RenderInfo> mRenderQueue;

		// GPU frame timing
		vk::UniqueQueryPool mTimestampQueryPool;
		std::array<uint64_t, MAX_FRAMES_IN_FLIGHT * 2> mTimestampResults = {}; // Start and end timestamps per frame
		float mGPUFrameTime = 0.0f;
		bool mTimestampQuerySupported = false;
	};
}

#endif // _EAGE_RENDERER_H