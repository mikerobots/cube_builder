// Standard library includes first
#include <algorithm>
#include <filesystem>
#include <cctype>
#include <vector>
#include <stack>
#include <mutex>
#include <memory>

// Application headers
#include "cli/AutoComplete.h"
#include "cli/CommandProcessor.h"
#include "cli/Application.h"

// Core includes
#include "groups/GroupManager.h"

namespace VoxelEditor {

AutoComplete::AutoComplete(CommandProcessor* processor, Application* app)
    : m_processor(processor), m_app(app) {
}

void AutoComplete::reset() {
    m_input.clear();
    m_cursorPos = 0;
    m_completions.clear();
    m_currentIndex = -1;
    m_completionsValid = false;
}

void AutoComplete::setInput(const std::string& input) {
    if (m_input != input) {
        m_input = input;
        m_completionsValid = false;
    }
}

void AutoComplete::setCursorPosition(size_t pos) {
    if (m_cursorPos != pos) {
        m_cursorPos = std::min(pos, m_input.length());
        m_completionsValid = false;
    }
}

std::vector<std::string> AutoComplete::getCompletions() const {
    updateCompletions();
    return m_completions;
}

std::string AutoComplete::getCompletion(size_t index) const {
    updateCompletions();
    if (index < m_completions.size()) {
        return m_completions[index];
    }
    return "";
}

std::string AutoComplete::applyCompletion(size_t index) const {
    updateCompletions();
    if (index >= m_completions.size()) {
        return m_input;
    }
    
    auto [before, after] = splitAtCursor();
    std::string partial = getPartialAtCursor();
    
    // Find where partial starts
    size_t partialStart = m_cursorPos;
    if (partialStart > partial.length()) {
        partialStart -= partial.length();
    } else {
        partialStart = 0;
    }
    
    // Build completed string
    std::string result = m_input.substr(0, partialStart);
    result += m_completions[index];
    result += after;
    
    return result;
}

void AutoComplete::nextCompletion() {
    updateCompletions();
    if (m_completions.empty()) return;
    
    m_currentIndex++;
    if (m_currentIndex >= static_cast<int>(m_completions.size())) {
        m_currentIndex = 0;
    }
}

void AutoComplete::previousCompletion() {
    updateCompletions();
    if (m_completions.empty()) return;
    
    m_currentIndex--;
    if (m_currentIndex < 0) {
        m_currentIndex = static_cast<int>(m_completions.size()) - 1;
    }
}

std::string AutoComplete::getCurrentCompletion() const {
    updateCompletions();
    if (m_currentIndex >= 0 && m_currentIndex < static_cast<int>(m_completions.size())) {
        return applyCompletion(m_currentIndex);
    }
    return m_input;
}

bool AutoComplete::hasCompletions() const {
    updateCompletions();
    return !m_completions.empty();
}

std::vector<std::string> AutoComplete::getFileCompletions(const std::string& partial) const {
    std::vector<std::string> completions;
    
    namespace fs = std::filesystem;
    
    // Determine directory and filename parts
    fs::path partialPath(partial);
    fs::path dir = partialPath.parent_path();
    std::string filePrefix = partialPath.filename().string();
    
    if (dir.empty()) {
        dir = fs::current_path();
    }
    
    try {
        for (const auto& entry : fs::directory_iterator(dir)) {
            std::string filename = entry.path().filename().string();
            if (filename.starts_with(filePrefix)) {
                fs::path fullPath = dir / filename;
                if (entry.is_directory()) {
                    completions.push_back(fullPath.string() + "/");
                } else {
                    completions.push_back(fullPath.string());
                }
            }
        }
    } catch (...) {
        // Directory doesn't exist or not accessible
    }
    
    std::sort(completions.begin(), completions.end());
    return completions;
}

std::vector<std::string> AutoComplete::getGroupCompletions(const std::string& partial) const {
    std::vector<std::string> completions;
    
    if (m_app && m_app->getGroupManager()) {
        auto groupIds = m_app->getGroupManager()->getAllGroupIds();
        for (auto id : groupIds) {
            auto group = m_app->getGroupManager()->getGroup(id);
            if (group) {
                std::string name = group->getName();
                if (name.starts_with(partial)) {
                    completions.push_back(name);
                }
            }
        }
    }
    
    std::sort(completions.begin(), completions.end());
    return completions;
}

std::vector<std::string> AutoComplete::getViewCompletions(const std::string& partial) const {
    std::vector<std::string> views = {
        "front", "back", "left", "right", "top", "bottom", "iso", "default"
    };
    
    std::vector<std::string> completions;
    for (const auto& view : views) {
        if (view.starts_with(partial)) {
            completions.push_back(view);
        }
    }
    
    return completions;
}

std::vector<std::string> AutoComplete::getResolutionCompletions(const std::string& partial) const {
    std::vector<std::string> resolutions = {
        "1cm", "2cm", "4cm", "8cm", "16cm", "32cm", "64cm", "128cm", "256cm", "512cm"
    };
    
    std::vector<std::string> completions;
    for (const auto& res : resolutions) {
        if (res.starts_with(partial)) {
            completions.push_back(res);
        }
    }
    
    return completions;
}

void AutoComplete::updateCompletions() const {
    if (m_completionsValid) return;
    
    m_completions.clear();
    m_currentIndex = -1;
    
    // Parse input to determine context
    auto tokens = m_processor->parseInput(m_input.substr(0, m_cursorPos));
    
    if (tokens.empty() || (tokens.size() == 1 && m_cursorPos <= tokens[0].length())) {
        // Completing command name
        std::string partial = tokens.empty() ? "" : tokens[0];
        m_completions = m_processor->getCommandCompletions(partial);
    } else {
        // Completing arguments
        const std::string& cmdName = tokens[0];
        auto cmd = m_processor->getCommand(cmdName);
        if (!cmd) {
            m_completionsValid = true;
            return;
        }
        
        // Determine which argument we're completing
        size_t argIndex = tokens.size() - 2;
        if (m_input[m_cursorPos - 1] == ' ') {
            argIndex++;
        }
        
        if (argIndex < cmd->arguments.size()) {
            const auto& arg = cmd->arguments[argIndex];
            std::string partial = (argIndex + 1 < tokens.size()) ? tokens[argIndex + 1] : "";
            
            // Context-aware completions based on command and argument
            if (cmdName == "open" || cmdName == "save" || cmdName == "saveas" || cmdName == "export") {
                m_completions = getFileCompletions(partial);
            } else if ((cmdName == "group" || cmdName == "hide" || cmdName == "show") && arg.name == "name") {
                m_completions = getGroupCompletions(partial);
            } else if (cmdName == "camera" && arg.name == "preset") {
                m_completions = getViewCompletions(partial);
            } else if (cmdName == "resolution" && arg.name == "size") {
                m_completions = getResolutionCompletions(partial);
            }
        }
    }
    
    m_completionsValid = true;
}

std::string AutoComplete::getPartialAtCursor() const {
    if (m_cursorPos == 0) return "";
    
    size_t start = m_cursorPos;
    while (start > 0 && !std::isspace(m_input[start - 1])) {
        start--;
    }
    
    return m_input.substr(start, m_cursorPos - start);
}

std::pair<std::string, std::string> AutoComplete::splitAtCursor() const {
    std::string before = m_input.substr(0, m_cursorPos);
    std::string after = m_input.substr(m_cursorPos);
    return {before, after};
}

// LineEditor implementation

LineEditor::LineEditor() {
}

void LineEditor::clear() {
    m_line.clear();
    m_cursor = 0;
}

void LineEditor::insert(char c) {
    m_line.insert(m_cursor, 1, c);
    m_cursor++;
}

void LineEditor::insert(const std::string& str) {
    m_line.insert(m_cursor, str);
    m_cursor += str.length();
}

void LineEditor::backspace() {
    if (m_cursor > 0) {
        m_line.erase(m_cursor - 1, 1);
        m_cursor--;
    }
}

void LineEditor::deleteChar() {
    if (m_cursor < m_line.length()) {
        m_line.erase(m_cursor, 1);
    }
}

void LineEditor::moveLeft() {
    if (m_cursor > 0) {
        m_cursor--;
    }
}

void LineEditor::moveRight() {
    if (m_cursor < m_line.length()) {
        m_cursor++;
    }
}

void LineEditor::moveHome() {
    m_cursor = 0;
}

void LineEditor::moveEnd() {
    m_cursor = m_line.length();
}

void LineEditor::moveWordLeft() {
    if (m_cursor > 0) {
        m_cursor = findWordBoundaryLeft(m_cursor - 1);
    }
}

void LineEditor::moveWordRight() {
    if (m_cursor < m_line.length()) {
        m_cursor = findWordBoundaryRight(m_cursor + 1);
    }
}

void LineEditor::addToHistory(const std::string& line) {
    if (!line.empty() && (m_history.empty() || m_history.back() != line)) {
        m_history.push_back(line);
        if (m_history.size() > m_maxHistory) {
            m_history.pop_front();
        }
    }
    m_historyIndex = -1;
    m_savedLine.clear();
}

void LineEditor::historyUp() {
    if (m_history.empty()) return;
    
    if (m_historyIndex == -1) {
        m_savedLine = m_line;
        m_historyIndex = static_cast<int>(m_history.size()) - 1;
    } else if (m_historyIndex > 0) {
        m_historyIndex--;
    }
    
    if (m_historyIndex >= 0 && m_historyIndex < static_cast<int>(m_history.size())) {
        m_line = m_history[m_historyIndex];
        m_cursor = m_line.length();
    }
}

void LineEditor::historyDown() {
    if (m_historyIndex == -1) return;
    
    m_historyIndex++;
    
    if (m_historyIndex >= static_cast<int>(m_history.size())) {
        m_line = m_savedLine;
        m_historyIndex = -1;
        m_savedLine.clear();
    } else {
        m_line = m_history[m_historyIndex];
    }
    
    m_cursor = m_line.length();
}

void LineEditor::setLine(const std::string& line) {
    m_line = line;
    m_cursor = std::min(m_cursor, m_line.length());
}

std::string LineEditor::getDisplay() const {
    return m_line;
}

size_t LineEditor::getDisplayCursor() const {
    return m_cursor;
}

bool LineEditor::isWordChar(char c) const {
    return std::isalnum(c) || c == '_' || c == '-' || c == '.';
}

size_t LineEditor::findWordBoundaryLeft(size_t pos) const {
    // Skip non-word chars
    while (pos > 0 && !isWordChar(m_line[pos])) {
        pos--;
    }
    
    // Skip word chars
    while (pos > 0 && isWordChar(m_line[pos - 1])) {
        pos--;
    }
    
    return pos;
}

size_t LineEditor::findWordBoundaryRight(size_t pos) const {
    // Skip word chars
    while (pos < m_line.length() && isWordChar(m_line[pos])) {
        pos++;
    }
    
    // Skip non-word chars
    while (pos < m_line.length() && !isWordChar(m_line[pos])) {
        pos++;
    }
    
    return pos;
}

} // namespace VoxelEditor