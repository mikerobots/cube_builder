#pragma once

#include <string>
#include <vector>
#include <deque>

namespace VoxelEditor {

class CommandProcessor;
class Application;

class AutoComplete {
public:
    AutoComplete(CommandProcessor* processor, Application* app);
    ~AutoComplete() = default;
    
    // Input handling
    void reset();
    void setInput(const std::string& input);
    void setCursorPosition(size_t pos);
    
    // Completion
    std::vector<std::string> getCompletions() const;
    std::string getCompletion(size_t index) const;
    std::string applyCompletion(size_t index) const;
    
    // Tab cycling
    void nextCompletion();
    void previousCompletion();
    std::string getCurrentCompletion() const;
    bool hasCompletions() const;
    
    // Context-aware completions
    std::vector<std::string> getFileCompletions(const std::string& partial) const;
    std::vector<std::string> getGroupCompletions(const std::string& partial) const;
    std::vector<std::string> getViewCompletions(const std::string& partial) const;
    std::vector<std::string> getResolutionCompletions(const std::string& partial) const;
    
private:
    CommandProcessor* m_processor;
    Application* m_app;
    
    std::string m_input;
    size_t m_cursorPos = 0;
    
    mutable std::vector<std::string> m_completions;
    mutable int m_currentIndex = -1;
    mutable bool m_completionsValid = false;
    
    // Helper methods
    void updateCompletions() const;
    std::string getPartialAtCursor() const;
    std::pair<std::string, std::string> splitAtCursor() const;
};

// Line editor for interactive command input
class LineEditor {
public:
    LineEditor();
    ~LineEditor() = default;
    
    // Input handling
    void clear();
    void insert(char c);
    void insert(const std::string& str);
    void backspace();
    void deleteChar();
    void moveLeft();
    void moveRight();
    void moveHome();
    void moveEnd();
    void moveWordLeft();
    void moveWordRight();
    
    // History
    void addToHistory(const std::string& line);
    void historyUp();
    void historyDown();
    
    // Content access
    const std::string& getLine() const { return m_line; }
    size_t getCursor() const { return m_cursor; }
    void setLine(const std::string& line);
    
    // Display
    std::string getDisplay() const;
    size_t getDisplayCursor() const;
    
private:
    std::string m_line;
    size_t m_cursor = 0;
    
    std::deque<std::string> m_history;
    size_t m_maxHistory = 100;
    int m_historyIndex = -1;
    std::string m_savedLine;
    
    // Helper methods
    bool isWordChar(char c) const;
    size_t findWordBoundaryLeft(size_t pos) const;
    size_t findWordBoundaryRight(size_t pos) const;
};

} // namespace VoxelEditor