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
| `Renderer` | Owns all GPU resources; orchestrates frame loop (MAX_FRAMES_IN_FLIGHT=2); provides `ImmediateSubmit()` |
| `VulkanContext` | Initialises Vulkan instance, physical/logical device, queues, surface |
| `AbstractRenderPass` | Base for all render passes; two-phase: `Prepare()` (CPU) → `Execute()` (GPU) |
| `SceneRenderPass` | Main geometry pass; sorts queue back-to-front for correct alpha blending |
| `Material` | Runtime instance = shared `RenderPipeline` + unique `StaticDescriptor` |
| `MaterialBuilder` | Fluent builder for `MaterialProperty` → compiles to `Material` |
| `StaticDescriptor` | Single descriptor set for textures/constants (don't change per-frame) |
| `DynamicDescriptor` | Per-frame descriptor sets with dynamic offsets (matrices, per-object data) |
| `DynamicDescriptorAllocator` | Pool-based allocator; grows on demand, resets each frame |
| `ManagedImage` / `ManagedBuffer` | RAII GPU resources via VMA; never use raw `vkCreateImage` |
| `VulkanPipelineBuilder` | Fluent builder for `vk::Pipeline`; encapsulates state (blend, depth, topology) |
| `ShaderReflection` | SPIR-V reflection to auto-generate descriptor set layouts from shaders |

## Descriptor Set Convention

Shader descriptor sets follow a fixed binding convention:

| Set | Content | Type |
|---|---|---|
| 0 | Global scene data (`SceneGlobalData`: view/proj matrices) | `DynamicDescriptor` |
| 1 | Per-object data (`MeshUniformData`: model matrix + vertex buffer address) | `DynamicDescriptor` |
| 2 | Material-specific textures / constants | `StaticDescriptor` |

## Resource Ownership

- **Unique ownership** for frame resources and GPU objects: `std::unique_ptr<VulkanCommandContext>`
- **Shared ownership** for pipelines (shared across material instances): `std::shared_ptr<RenderPipeline>`
- **Per-instance** descriptor: each `Material` owns its own `StaticDescriptor`
- **Per-frame** allocators: `DynamicDescriptorAllocator` resets at frame start

## Key Flows

### Creating a Material
```cpp
auto mat = MaterialBuilder()
    .SetShaders( "vert.spv", "frag.spv" )
    .AddTexture( "albedo.png" )
    .SetAlphaBlending()
    .Build( renderer );
```

### Submitting a Draw
```cpp
renderer.Submit( RenderInfo{
    .material        = mat,
    .mesh            = mesh_buffers,
    .object_descriptor = dynamic_descriptor,
    .model_matrix    = transform
} );
```

### Adding a Custom Render Pass
```cpp
class MyPass : public AbstractRenderPass {
    void Prepare( uint32_t frame_index ) override { /* CPU setup */ }
    void Execute( vk::CommandBuffer cmd, const ExecutionContext& ctx ) override { /* Record commands */ }
};
renderer.AddRenderPass( std::make_unique<MyPass>() );
```

### One-Off GPU Work (e.g., texture uploads)
```cpp
renderer.ImmediateSubmit( [&]( vk::CommandBuffer cmd ) {
    // copy data, transition image layouts, etc.
} );
```

## DOs

- Use `MaterialBuilder` for all material creation — never construct `Material` directly
- Use `DynamicDescriptor` for per-frame data (matrices, lighting, time)
- Use `StaticDescriptor` for textures and data that doesn't change per-frame
- Extend `AbstractRenderPass` for any custom rendering step; register via `Renderer::AddRenderPass()`
- Use `Renderer::ImmediateSubmit()` for one-off GPU submissions (uploads, transitions)
- Allocate GPU memory through `ManagedImage::Create()` / `ManagedBuffer::Create()` — they handle VMA cleanup
- Sort render queue by depth (back-to-front) when alpha blending is involved

## DON'Ts

- Don't create raw Vulkan objects directly — use the `Managed*` and `Vulkan*Builder` helpers
- Don't write to or modify a descriptor set after pipeline creation; create a new `Material` instance instead
- Don't bypass `Renderer` for GPU submissions — this breaks synchronisation
- Don't manually manage VMA allocations; only use `ManagedBuffer` / `ManagedImage`
- Don't use `StaticDescriptor` for data that varies per-frame — that breaks double-buffering
- Don't forget dynamic offsets when binding per-frame `DynamicDescriptor` sets
