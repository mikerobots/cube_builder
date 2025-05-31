# Logging System Foundation

## Purpose
Provides multi-level logging, performance profiling, debug output management, and structured logging across all components.

## Key Components

### Logger
- Multi-level logging (Debug, Info, Warning, Error)
- Multiple output targets (console, file, network)
- Thread-safe logging operations
- Structured logging support

### LogFormatter
- Configurable log message formatting
- Timestamp and thread information
- Component identification
- Performance metrics integration

### PerformanceProfiler
- Function and scope timing
- Memory allocation tracking
- GPU timing integration
- Statistical analysis

## Interface Design

```cpp
class Logger {
public:
    enum class Level {
        Debug = 0,
        Info = 1,
        Warning = 2,
        Error = 3,
        None = 4
    };
    
    static Logger& getInstance();
    
    void setLevel(Level level);
    void addOutput(std::unique_ptr<LogOutput> output);
    void removeOutput(const std::string& name);
    
    void debug(const std::string& message, const std::string& component = "");
    void info(const std::string& message, const std::string& component = "");
    void warning(const std::string& message, const std::string& component = "");
    void error(const std::string& message, const std::string& component = "");
    
    template<typename... Args>
    void debug(const std::string& format, Args&&... args);
    
    template<typename... Args>
    void info(const std::string& format, Args&&... args);
    
    template<typename... Args>
    void warning(const std::string& format, Args&&... args);
    
    template<typename... Args>
    void error(const std::string& format, Args&&... args);
    
private:
    Level m_level = Level::Info;
    std::vector<std::unique_ptr<LogOutput>> m_outputs;
    std::mutex m_mutex;
    
    void log(Level level, const std::string& message, const std::string& component);
};

class PerformanceProfiler {
public:
    static PerformanceProfiler& getInstance();
    
    void beginSection(const std::string& name);
    void endSection(const std::string& name);
    
    void recordMemoryAllocation(size_t size, const std::string& category);
    void recordMemoryDeallocation(size_t size, const std::string& category);
    
    struct ProfileData {
        std::string name;
        double totalTime;
        double averageTime;
        uint64_t callCount;
        double minTime;
        double maxTime;
    };
    
    std::vector<ProfileData> getResults() const;
    void reset();
    void saveReport(const std::string& filename) const;
    
private:
    std::unordered_map<std::string, ProfileSection> m_sections;
    std::stack<std::string> m_sectionStack;
    mutable std::mutex m_mutex;
};

// RAII profiling helper
class ScopedProfiler {
public:
    ScopedProfiler(const std::string& name);
    ~ScopedProfiler();
    
private:
    std::string m_name;
};

#define PROFILE_SCOPE(name) ScopedProfiler _prof(name)
#define PROFILE_FUNCTION() ScopedProfiler _prof(__FUNCTION__)
```

## Dependencies
- Standard C++ threading library
- Platform-specific high-resolution timers
- Optional network logging support