---
name: graphics-module
description: "USE WHEN: writing, reading, or modifying code in src/core/graphics/. Covers the Vulkan rendering backend architecture, key abstractions (Renderer, Material, RenderPass, Descriptor, Pipeline, Mesh), resource ownership patterns, and DOs/DON'Ts for safe and correct usage."
---

# Graphics Module

## Purpose

The graphics module is the Vulkan-based rendering backend for Vortex. It abstracts Vulkan complexity into engine-friendly APIs while maintaining performance through RAII resource management, pool-based descriptor allocation, and a layered architecture.

## Architecture Layers

```
Layer 5: Rendering Pipeline       Renderer (orchestrator)
         AbstractRenderPass        SceneRenderPass, ImGuiRenderPass

Layer 4: Material & Pipeline       Material, MaterialProperty, MaterialBuilder
         High-level abstraction    RenderPipeline, RenderInfo

Layer 3: Descriptor System         StaticDescriptor, DynamicDescriptor
         Set allocation/binding    DynamicDescriptorAllocator, DescriptorWriter

Layer 2: Resource Management       ManagedImage, ManagedBuffer (RAII + VMA)
         GPU memory lifecycle       VulkanShader, ShaderReflection

Layer 1: Vulkan Metal              VulkanContext, VulkanSwapChain
         Raw vk::raii types        VulkanCommandContext, VMAWrapper
```

**Engine-facing surface (prefer these):** `Renderer`, `Material`, `MaterialBuilder`, `AbstractRenderPass`  
**Vulkan internals (rarely touch directly):** Layers 1–2

## Key Classes

| Class | Responsibility |
|---|---|
| `Renderer` | Owns all GPU resources; orchestrates frame loop (MAX_FRAMES_IN_FLIGHT=2); provides `ImmediateSubmit()`, `GetGPUFrameTime()` |
| `VulkanContext` | Initialises Vulkan instance, physical/logical device, queues, surface |
| `AbstractRenderPass` | Base for all render passes; two-phase: `Prepare()` (CPU) → `Execute()` (GPU); declares `RenderPassDesc` |
| `SceneRenderPass` | Main geometry pass; accepts `RenderInfo` via `AddRenderInfo()`; owns off-screen color + depth targets |
| `Material` | Runtime instance = shared `RenderPipeline` + unique `StaticDescriptor` |
| `MaterialBuilder` | Fluent builder for `MaterialProperty` → compiles to `Material` |
| `StaticDescriptor` | Single descriptor set for textures/constants (don't change per-frame) |
| `DynamicDescriptor` | Per-frame descriptor sets with dynamic offsets (matrices, per-object data) |
| `DynamicDescriptorAllocator` | Pool-based allocator; grows on demand, resets each frame |
| `ManagedImage` / `ManagedBuffer` | RAII GPU resources via VMA; use `::Ptr` alias (`unique_ptr` with custom deleter) |
| `AbstractCamera` | Interface: `GetViewMatrix()`, `GetProjectionMatrix()`, `GetViewProjectionMatrix()`, `SetPosition()` |
| `OrthographicCamera` | Concrete `AbstractCamera`; constructed with left/right/bottom/top/near/far bounds |
| `VulkanPipelineBuilder` | Fluent builder for `vk::Pipeline`; encapsulates state (blend, depth, topology) |
| `ShaderReflection` | SPIR-V reflection to auto-generate descriptor set layouts from shaders |
| `BuiltInMeshes` | Helper: `made_rect_vertices( center, width, height )` → `BuiltInRect` (vertices + indices) |

## Descriptor Set Convention

Shader descriptor sets follow a fixed binding convention:

| Set | Content | Type |
|---|---|---|
| 0 | Global scene data (`SceneGlobalData`: view, proj, view_proj matrices, virtual_resolution) | `DynamicDescriptor` |
| 1 | Per-object data (`MeshUniformData`: model matrix, vertex buffer address, uv_rect) | `DynamicDescriptor` |
| 2 | Material-specific textures / constants | `StaticDescriptor` |

**`MeshUniformData`** (set 1):
```cpp
struct MeshUniformData {
    alignas(64) glm::mat4  model;
    alignas(8)  uint64_t   vertex_buffer_address;
    alignas(16) glm::vec4  uv_rect;  // xy = uv offset, zw = uv scale
};
```

**`SceneGlobalData`** (set 0):
```cpp
struct SceneGlobalData {
    alignas(64) glm::mat4  view;
    alignas(64) glm::mat4  proj;
    alignas(64) glm::mat4  view_proj;
    alignas(8)  glm::vec2  virtual_resolution;  // pixel-snapping grid size
};
```

## Resource Ownership

- **Unique ownership** for frame resources and GPU objects: `std::unique_ptr<VulkanCommandContext>`
- **Shared ownership** for pipelines (shared across material instances): `std::shared_ptr<RenderPipeline>`
- **Per-instance** descriptor: each `Material` owns its own `StaticDescriptor`
- **Per-frame** allocators: `DynamicDescriptorAllocator` resets at frame start
- **`ManagedImage::Ptr` / `ManagedBuffer::Ptr`**: `unique_ptr` with custom VMA deleter — always use these aliases, never raw `unique_ptr<ManagedImage>`

## Key Flows

### Creating a Material
```cpp
auto mat = MaterialBuilder()
    .SetShaders( "vert.spv", "frag.spv" )
    .AddTexture( "albedo.png" )
    .SetAlphaBlending()
    .Build( renderer );
```

### Submitting a Draw (via SceneRenderPass)

Draws are submitted to `SceneRenderPass`, not directly to `Renderer`. `Renderer::Submit()` is private.

```cpp
scene_render_pass.AddRenderInfo( RenderInfo{
    .material                  = mat.get(),
    .mesh_buffer               = mesh_buffers.get(),
    .mesh_descriptor           = &dynamic_descriptor,
    .mesh_uniform_data_dynamic = uniform_buffer.get(),
    .first_index               = 0,
    .index_count               = index_count,
    .vertex_offset             = 0,
    .model_matrix              = transform,
    .uv_rect                   = { 0.f, 0.f, 1.f, 1.f }  // offset + scale
} );
```

### Setting Up a Camera
```cpp
OrthographicCamera camera( 0.f, width, height, 0.f, -1.f, 1.f );
camera.SetPosition( { 0.f, 0.f, 0.f } );
// Pass view/proj to SceneGlobalData uniform buffer
```

### Adding a Custom Render Pass
```cpp
class MyPass : public AbstractRenderPass
{
    RenderPassDesc mDesc{};
public:
    const RenderPassDesc& GetDesc() const override { return mDesc; }
    void Prepare( size_t frame_index ) override { /* CPU setup */ }
    void Execute( vk::CommandBuffer& cmd, const ExecutionContext& ctx ) override { /* record cmds */ }
};

MyPass my_pass;
renderer.AddRenderPass( &my_pass );   // raw pointer — renderer does NOT own it
// ...
renderer.RemoveRenderPass( &my_pass );
```

### Creating a Rect Mesh
```cpp
auto rect = eage::graphics::made_rect_vertices( { 0.f, 0.f, 0.f }, width, height );
auto mesh = renderer.UploadMesh( rect.indices, rect.vertices );
```

### One-Off GPU Work (e.g., texture uploads)
```cpp
renderer.ImmediateSubmit( [&]( vk::CommandBuffer& cmd )
{
    // copy data, transition image layouts, etc.
} );
```

### GPU Frame Timing
```cpp
float gpu_ms = renderer.GetGPUFrameTime();  // milliseconds; 0 if unsupported
```

## DOs

- Use `MaterialBuilder` for all material creation — never construct `Material` directly
- Use `DynamicDescriptor` for per-frame data (matrices, lighting, time)
- Use `StaticDescriptor` for textures and data that doesn't change per-frame
- Extend `AbstractRenderPass` for any custom rendering step; register via `Renderer::AddRenderPass( &pass )` (raw pointer)
- Use `Renderer::ImmediateSubmit()` for one-off GPU submissions (uploads, transitions)
- Allocate GPU memory through `ManagedImage::Create()` / `ManagedBuffer::Create()` — store in `::Ptr` alias
- Submit draws via `SceneRenderPass::AddRenderInfo()` — fill all `RenderInfo` fields including `uv_rect`
- Use `made_rect_vertices()` for simple quad/sprite meshes
- Sort render queue by depth (back-to-front) when alpha blending is involved

## DON'Ts

- Don't call `renderer.Submit()` directly — it's private; draws go through render passes
- Don't pass `std::make_unique<Pass>()` to `AddRenderPass` — it takes a raw pointer; you own the lifetime
- Don't create raw Vulkan objects directly — use the `Managed*` and `Vulkan*Builder` helpers
- Don't write to or modify a descriptor set after pipeline creation; create a new `Material` instance instead
- Don't bypass `Renderer` for GPU submissions — this breaks synchronisation
- Don't use `unique_ptr<ManagedImage>` directly — always use `ManagedImage::Ptr` (custom deleter required)
- Don't use `StaticDescriptor` for data that varies per-frame — that breaks double-buffering
- Don't forget dynamic offsets when binding per-frame `DynamicDescriptor` sets
