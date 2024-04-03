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
	class DescriptorAllocator;
	class Renderable;
	class ImGUILifetime;
	struct VMAWrapper;
	class ManagedImage;
	struct Vertex;
	struct GPUMeshBuffers;

	///
	/// Renderer class
	///
	class Renderer
	{
	public:
		struct Frame
		{
			std::vector<std::shared_ptr<Renderable>> renderables;
			std::unique_ptr<VulkanCommandContext> command_context;
		};

		///
		/// Constructor
		///
		Renderer();

		///
		/// Destructor
		///
		virtual ~Renderer();

		///
		/// Initialize the renderer
		///
		///	@param window
		///	 The window to render to
		///
		/// @return
		///  true if successful, false otherwise
		///
		bool Init( SDL_Window& window );

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

	private:
		Frame& GetCurrentFrame();
		
		void Submit();

		void Present( uint32_t image_index );

		void InitDescriptors();
		void InitPipelines();
		bool InitMeshPipeline();
		void InitImGUI( SDL_Window& window );
		void InitData();

		void ImmediateSubmit( std::function<void( vk::CommandBuffer& )> work );

		void PrepareImGUI();

		void DrawGeometry( vk::CommandBuffer& cmd );

		std::unique_ptr<GPUMeshBuffers> UploadMesh( std::span<uint32_t> indices, std::span<Vertex> vertices );

		std::unique_ptr<VulkanContext>		mContext;
		std::unique_ptr<VulkanSwapChain>	mSwapChain;
		std::vector<Frame>					mFrames;
		int64_t								mFrameNumber;

		std::unique_ptr<VMAWrapper> mVMA;
		std::unique_ptr<ManagedImage> mRenderImage;
		std::unique_ptr<ManagedImage> mDepthImage;

		std::unique_ptr<DescriptorAllocator> 	mDescriptorAllocator;
		vk::UniqueDescriptorSet 				mRenderImageDescriptorSet;
		vk::UniqueDescriptorSetLayout			mRenderImageDescriptorSetLayout;

		std::unique_ptr<VulkanCommandContext> mImmidiateCommandContext;
		std::unique_ptr<ImGUILifetime> mImGUILifetime;

		vk::UniquePipeline mMeshPipeline;
		vk::UniquePipelineLayout mMeshPipelineLayout;
		std::unique_ptr<GPUMeshBuffers> mRectangleMeshes;
	};
}

#endif // _EAGE_RENDERER_H