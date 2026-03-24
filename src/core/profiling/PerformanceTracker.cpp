#include "PerformanceTracker.h"

#include <imgui/imgui.h>
#include <graphics/Renderer.h>

using namespace eage::profiling;

PerformanceTracker::PerformanceTracker( graphics::Renderer& renderer )
	: mRenderer( renderer )
	, mLastFrameTime(std::chrono::steady_clock::now())
{
}

void
PerformanceTracker::Update()
{
	auto current_time = std::chrono::steady_clock::now();
	auto frame_duration = std::chrono::duration<float, std::milli>(current_time - mLastFrameTime);
	mLastFrameTime = current_time;
	
	// Update frame time history for moving average
	mFrameTimeHistory[mFrameHistoryIndex] = frame_duration.count();
	mFrameHistoryIndex = (mFrameHistoryIndex + 1) % FRAME_HISTORY_SIZE;
	
	// Calculate average frame time
	float total_frame_time = 0.0f;
	for(size_t i = 0; i < FRAME_HISTORY_SIZE; ++i)
	{
		total_frame_time += mFrameTimeHistory[i];
	}
	mCPUFrameTime = total_frame_time / FRAME_HISTORY_SIZE;
	mFPS = 1000.0f / mCPUFrameTime;
	
	// Get actual GPU frame time from renderer
	mGPUFrameTime = mRenderer.GetGPUFrameTime();
}

void
PerformanceTracker::DrawDebugGUI()
{
	if (!mEnabled) return;
	
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | 
							ImGuiWindowFlags_NoResize | 
							ImGuiWindowFlags_NoMove | 
							ImGuiWindowFlags_NoSavedSettings |
							ImGuiWindowFlags_AlwaysAutoResize;
	
	ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
	
	if (ImGui::Begin("Performance Metrics", nullptr, flags))
	{
		ImGui::Text("FPS: %.1f", mFPS);
		ImGui::Text("CPU frame time: %.1f ms", mCPUFrameTime);
		ImGui::Text("GPU frame time: %.1f ms", mGPUFrameTime);

		// Set green color for plot lines
		ImGui::PushStyleColor(ImGuiCol_PlotLines, IM_COL32(0, 255, 0, 255)); // Bright green
		
		// Add frame time graph
		ImGui::PlotLines( "Frame Time", mFrameTimeHistory, static_cast<int>( FRAME_HISTORY_SIZE ), 
						  static_cast<int>( mFrameHistoryIndex ), nullptr, 0.0f, 33.33f, ImVec2(200, 50) );

		// Restore original colors
		ImGui::PopStyleColor(1);
	}
	ImGui::End();
}