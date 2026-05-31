---
name: utility-module
description: "USE WHEN: writing, reading, or modifying code in src/core/utility/. Covers Logger (stream and string macros, severity levels), JsonParser (RapidJSON helpers), Filesystem (file existence check), and Pointers (make_resource RAII helper)."
---

# Utility Module

## Purpose

Cross-cutting helpers used throughout the engine. No module-specific logic — safe to include anywhere in `core/`.

## Logger (`Logger.h`)

Severity levels: `TRACE < DEBUG < INFO < WARNING < EAGE_ERROR`.  
Default cutoff at compile time: `INFO` (override with preprocessor defines).

```cpp
// Stream-style (preferred for dynamic messages)
LOG() << "Loaded " << count << " assets";
LOG( LOG_LEVEL::DEBUG ) << "Frame time: " << dt;
LOG_ERROR() << "Failed to open: " << path;   // also triggers EAGE_ASSERT in debug

// String-style (for simple literals)
LOG( "Initializing renderer ..." );
LOG_ERROR( "Swap chain creation failed." );
```

**Severity preprocessor defines** (set in CMake or project settings):
- `LOGGING_LEVEL_ALL` / `LOGGING_LEVEL_TRACE` — show everything
- `LOGGING_LEVEL_DEBUG`
- `LOGGING_LEVEL_WARN`
- `LOGGING_LEVEL_ERROR`
- `LOGGING_LEVEL_NONE` — silence all

`LOG_ERROR()` stream variant sets `mShouldAssert = true` — destructor calls `EAGE_ASSERT(false, message)` in debug builds.

## JsonParser (`JsonParser.h`)

Thin helpers over RapidJSON. All functions log errors internally.

```cpp
rapidjson::Document doc;
if( utility::parse_json_document( doc, "data.json" ) )
{
    const auto& obj   = utility::get_json_object( doc, "frames" );
    int         count = utility::get_json_int( doc, "count" );
    float       scale = utility::get_json_float( doc, "scale" );
    std::string name  = utility::get_json_string( doc, "name" );
}
```

Returns safe defaults (`0`, `0.0f`, `""`, `empty_json_value`) and logs `LOG_ERROR` if a key is missing or has the wrong type.

## Filesystem (`Filesystem.h`)

```cpp
if( utility::is_file_exist( path ) )
{
    // safe to open
}
```

Wraps `std::filesystem::exists`. Header-only.

## Pointers (`Pointers.h`)

`make_resource` — creates a `shared_ptr` from a C-style creator/destructor pair (for third-party C APIs):

```cpp
auto engine = utility::make_resource(
    ma_engine_init,   // creator
    ma_engine_uninit, // destructor
    &config           // args...
);
// engine is shared_ptr<ma_engine>; uninit called automatically on destruction
```

## DOs

- Prefer `LOG() <<` stream style for messages with runtime data — avoids string concatenation overhead when below cutoff level
- Use `parse_json_document` + helper getters instead of raw RapidJSON API — they handle missing-key logging uniformly
- Use `is_file_exist` before attempting to open asset files to produce clearer error messages

## DON'Ts

- Don't call `GetLogger().Log()` directly — use the `LOG` / `LOG_ERROR` macros/functions
- Don't include `Logger.h` in headers that are included widely in hot paths — the static logger initialization has a small overhead
- Don't use `make_resource` for objects that need non-default construction beyond what the creator function supports — use a custom RAII wrapper instead
