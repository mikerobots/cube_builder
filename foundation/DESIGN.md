# Foundation Layer Design Document
*Updated: June 22, 2025*

## Overview

The Foundation Layer provides core infrastructure and utilities that support the entire voxel editor system. It consists of 5 major subsystems that handle fundamental operations like configuration management, event communication, logging, mathematical operations, and memory management.

This layer has no dependencies on other parts of the system and serves as the bedrock for both the Core Library and Application Layer.

## Architecture Principles

### Independence
- Zero dependencies on core or application layers
- Self-contained utilities and services
- Platform-agnostic implementations where possible

### Performance
- Memory-efficient implementations optimized for VR constraints (<4GB total)
- Object pooling for frequently allocated objects
- Thread-safe components where needed

### Reliability
- Comprehensive error handling and validation
- Extensive unit test coverage (12 unit tests across all subsystems)
- Consistent API patterns across all components

## Foundation Subsystems

### 1. Configuration Management (`foundation/config/`)

**Purpose**: Hierarchical configuration system with type-safe access and validation

#### Key Components:
- **ConfigManager**: Central configuration coordination with JSON file support
- **ConfigValue**: Strongly-typed configuration values with validation and change notifications
- **ConfigSection**: Hierarchical configuration sections with dot-notation access

#### Design Features:
- Type-safe configuration access with compile-time validation
- Hierarchical section support (e.g., `render.quality.shadows`)
- JSON-based configuration files with hot-reload capability
- Change notification system for reactive configuration updates
- Default value support with override capabilities

#### API Pattern:
```cpp
class ConfigManager {
public:
    bool loadFromFile(const std::string& filename);
    template<typename T> T getValue(const std::string& key, const T& defaultValue = T{});
    template<typename T> void setValue(const std::string& key, const T& value);
    void subscribe(const std::string& key, ConfigChangeCallback callback);
};
```

#### Test Coverage:
- **test_unit_foundation_config_manager.cpp**: Manager functionality and file I/O
- **test_unit_foundation_config_sections.cpp**: Hierarchical section management
- **test_unit_foundation_config_values.cpp**: Type safety and validation

### 2. Event System (`foundation/events/`)

**Purpose**: Decoupled component communication through type-safe event dispatching

#### Key Components:
- **EventDispatcher**: Central event coordination with subscription management
- **EventBase**: Base event interface with type identification
- **EventHandler**: Event subscription and handling interface
- **CommonEvents**: Standard event definitions used throughout the system

#### Design Features:
- Type-safe event dispatching with compile-time type checking
- Subscription/unsubscription pattern for loose coupling
- Event queuing and deferred processing support
- Priority-based event handling
- Thread-safe event dispatching for multi-threaded environments

#### API Pattern:
```cpp
class EventDispatcher {
public:
    template<typename EventType>
    void subscribe(EventHandler<EventType>* handler);
    
    template<typename EventType>
    void unsubscribe(EventHandler<EventType>* handler);
    
    template<typename EventType>
    void dispatch(const EventType& event);
    
    void processQueuedEvents();
};
```

#### Used Throughout System:
- Camera system: View change notifications
- Input system: Mouse/keyboard event distribution
- Voxel data: Change notifications for rendering updates
- Visual feedback: Real-time update coordination

#### Test Coverage:
- **test_unit_foundation_event_dispatcher.cpp**: Complete event system validation

### 3. Logging System (`foundation/logging/`)

**Purpose**: Multi-level logging with performance profiling and configurable output

#### Key Components:
- **Logger**: Multi-level logging with timestamp and context support
- **LogOutput**: Configurable output destinations (console, file, network)
- **PerformanceProfiler**: Performance measurement and bottleneck analysis

#### Design Features:
- Multiple log levels (Debug, Info, Warning, Error, Fatal)
- Configurable output destinations with format customization
- Performance profiling with microsecond precision
- Context-aware logging with subsystem identification
- Efficient string formatting with minimal allocation overhead

#### API Pattern:
```cpp
class Logger {
public:
    static void log(LogLevel level, const std::string& message);
    static void setLogLevel(LogLevel level);
    static void addOutput(std::unique_ptr<LogOutput> output);
    
    // Performance profiling
    static ScopedProfiler profile(const std::string& operation);
};

// Usage macros for convenience
#define LOG_DEBUG(msg) Logger::log(LogLevel::Debug, msg)
#define LOG_INFO(msg) Logger::log(LogLevel::Info, msg)
#define PROFILE(name) auto prof = Logger::profile(name)
```

#### Test Coverage:
- **test_unit_foundation_logger.cpp**: Logging functionality and output management
- **test_unit_foundation_performance_profiler.cpp**: Performance measurement accuracy

### 4. Math Utilities (`foundation/math/`)

**Purpose**: Comprehensive mathematical operations for 3D graphics and voxel calculations

#### Key Components:
- **Vector Classes**: Vector2f/i, Vector3f/i, Vector4f with full arithmetic operations
- **Matrix4f**: 4x4 transformation matrices with standard operations
- **Quaternion**: Rotation representation with slerp interpolation
- **BoundingBox**: Spatial bounds representation with intersection testing
- **CoordinateConverter**: Coordinate system transformations (world/screen/voxel)
- **Ray**: Ray-based calculations for picking and intersection operations

#### Design Features:
- Header-only implementation for maximum performance
- SIMD-optimized operations where available
- Consistent API patterns across all mathematical types
- Comprehensive operator overloading for natural syntax
- Specialized voxel coordinate conversions

#### API Pattern:
```cpp
class Vector3f {
public:
    Vector3f(float x = 0, float y = 0, float z = 0);
    
    // Arithmetic operations
    Vector3f operator+(const Vector3f& other) const;
    Vector3f& operator+=(const Vector3f& other);
    
    // Utility functions
    float length() const;
    Vector3f normalized() const;
    float dot(const Vector3f& other) const;
    Vector3f cross(const Vector3f& other) const;
};

class Matrix4f {
public:
    static Matrix4f perspective(float fov, float aspect, float near, float far);
    static Matrix4f lookAt(const Vector3f& eye, const Vector3f& center, const Vector3f& up);
    
    Matrix4f operator*(const Matrix4f& other) const;
    Vector3f transform(const Vector3f& vector) const;
};
```

#### Critical for System:
- Camera view/projection matrix calculations
- Voxel world-to-grid coordinate conversions
- Ray-casting for mouse picking and face detection
- Spatial bounds checking and collision detection

#### Test Coverage:
- **test_unit_foundation_vectors.cpp**: Vector arithmetic and utility functions
- **test_unit_foundation_matrices.cpp**: Matrix operations and transformations
- **test_unit_foundation_coordinate_converter.cpp**: Coordinate system conversions
- **test_unit_foundation_quaternion.cpp**: Rotation operations and interpolation

### 5. Memory Management (`foundation/memory/`)

**Purpose**: Performance-optimized memory management with VR constraints

#### Key Components:
- **MemoryPool**: Object pooling for frequently allocated objects
- **MemoryTracker**: Memory usage monitoring and reporting
- **MemoryOptimizer**: Memory pressure detection and cleanup coordination

#### Design Features:
- Object pooling with configurable pool sizes
- Memory pressure detection with automatic cleanup triggers
- Allocation tracking for memory leak detection
- VR-optimized for <4GB total memory constraints
- Thread-safe pool management for multi-threaded access

#### API Pattern:
```cpp
template<typename T>
class MemoryPool {
public:
    MemoryPool(size_t initialSize = 64);
    
    T* acquire();
    void release(T* object);
    
    size_t getPoolSize() const;
    size_t getActiveCount() const;
};

class MemoryTracker {
public:
    static void recordAllocation(size_t size, const std::string& category);
    static void recordDeallocation(size_t size, const std::string& category);
    static MemoryReport generateReport();
};
```

#### Critical for VR Performance:
- Voxel object pooling for rapid placement/deletion
- Command object pooling for undo/redo operations
- Memory pressure monitoring for Meta Quest 3 constraints
- Allocation tracking for memory leak prevention

#### Test Coverage:
- **test_unit_foundation_memory_pool.cpp**: Pool allocation and lifecycle management
- **test_unit_foundation_memory_tracker.cpp**: Allocation tracking and reporting

## Cross-Cutting Concerns

### Thread Safety
Components designed for multi-threaded access:
- **EventDispatcher**: Thread-safe event queuing and processing
- **Logger**: Thread-safe log output with minimal contention
- **MemoryPool**: Thread-safe object acquisition and release

### Error Handling
Consistent error handling patterns:
- Return codes for recoverable errors
- Exceptions for programming errors and critical failures
- Comprehensive validation with descriptive error messages
- Graceful degradation where possible

### Performance Characteristics
- **Memory allocation**: Minimized through object pooling
- **String operations**: Optimized formatting with minimal copies
- **Mathematical operations**: SIMD optimization where available
- **Event processing**: Deferred processing to avoid frame drops

## Integration Patterns

### Configuration-Driven Behavior
Foundation components use the configuration system for runtime behavior:
- Logger output levels and destinations
- Memory pool sizes and thresholds
- Event processing queue sizes
- Performance profiling enable/disable

### Event-Driven Communication
Foundation events enable decoupled system communication:
- Configuration change notifications
- Memory pressure warnings
- Performance threshold alerts
- System state transitions

### Mathematical Foundation
Math utilities provide consistent coordinate systems:
- Centered coordinate system (origin at 0,0,0)
- Y-up orientation throughout the system
- 1cm precision for all voxel operations
- Consistent transformation matrices

## Testing Strategy

### Unit Test Coverage
All foundation components have comprehensive unit tests:
- **12 unit tests** covering all public APIs
- **Edge case validation** for boundary conditions
- **Performance benchmarks** for critical operations
- **Thread safety validation** for concurrent components

### Integration Testing
Foundation components are tested together:
- Configuration-driven logging behavior
- Event-based memory pressure handling
- Mathematical coordinate system consistency
- Cross-component error propagation

## Future Extensibility

### Platform Adaptation
Foundation designed for multi-platform deployment:
- Platform-specific implementations through interfaces
- Conditional compilation for platform optimizations
- Runtime capability detection and adaptation

### VR Optimization
Memory and performance optimizations for VR:
- Aggressive object pooling strategies
- Memory pressure detection and response
- Microsecond-precision performance profiling
- SIMD-optimized mathematical operations

### Monitoring and Diagnostics
Built-in observability for production deployment:
- Performance metrics collection
- Memory usage reporting
- Event processing statistics
- Error rate monitoring

The Foundation Layer provides a solid, well-tested base for the entire voxel editor system, ensuring consistent behavior, optimal performance, and reliable operation across all target platforms.