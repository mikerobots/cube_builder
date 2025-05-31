#include <gtest/gtest.h>
#include "../Logger.h"
#include <sstream>
#include <thread>
#include <chrono>
#include <filesystem>

using namespace VoxelEditor::Logging;

class TestLogOutput : public LogOutput {
public:
    TestLogOutput(const std::string& name = "Test") : m_name(name) {}
    
    void write(const LogMessage& message) override {
        m_messages.push_back(message);
    }
    
    std::string getName() const override {
        return m_name;
    }
    
    const std::vector<LogMessage>& getMessages() const {
        return m_messages;
    }
    
    void clear() {
        m_messages.clear();
    }
    
private:
    std::string m_name;
    std::vector<LogMessage> m_messages;
};

class LoggerTest : public ::testing::Test {
protected:
    void SetUp() override {
        Logger::getInstance().clearOutputs();
        Logger::getInstance().setLevel(LogLevel::Debug);
        
        m_testOutput = std::make_unique<TestLogOutput>();
        m_testOutputPtr = m_testOutput.get();
        Logger::getInstance().addOutput(std::move(m_testOutput));
    }
    
    void TearDown() override {
        Logger::getInstance().clearOutputs();
        Logger::getInstance().setLevel(LogLevel::Info);
    }
    
    TestLogOutput* m_testOutputPtr = nullptr;
    std::unique_ptr<TestLogOutput> m_testOutput;
};

TEST_F(LoggerTest, BasicLogging) {
    Logger& logger = Logger::getInstance();
    
    logger.debug("Debug message");
    logger.info("Info message");
    logger.warning("Warning message");
    logger.error("Error message");
    
    const auto& messages = m_testOutputPtr->getMessages();
    ASSERT_EQ(messages.size(), 4);
    
    EXPECT_EQ(messages[0].level, LogLevel::Debug);
    EXPECT_EQ(messages[0].message, "Debug message");
    
    EXPECT_EQ(messages[1].level, LogLevel::Info);
    EXPECT_EQ(messages[1].message, "Info message");
    
    EXPECT_EQ(messages[2].level, LogLevel::Warning);
    EXPECT_EQ(messages[2].message, "Warning message");
    
    EXPECT_EQ(messages[3].level, LogLevel::Error);
    EXPECT_EQ(messages[3].message, "Error message");
}

TEST_F(LoggerTest, LogLevelFiltering) {
    Logger& logger = Logger::getInstance();
    
    logger.setLevel(LogLevel::Warning);
    
    logger.debug("Debug message");
    logger.info("Info message");
    logger.warning("Warning message");
    logger.error("Error message");
    
    const auto& messages = m_testOutputPtr->getMessages();
    ASSERT_EQ(messages.size(), 2);
    
    EXPECT_EQ(messages[0].level, LogLevel::Warning);
    EXPECT_EQ(messages[1].level, LogLevel::Error);
}

TEST_F(LoggerTest, ComponentLogging) {
    Logger& logger = Logger::getInstance();
    
    logger.info("Test message", "TestComponent");
    
    const auto& messages = m_testOutputPtr->getMessages();
    ASSERT_EQ(messages.size(), 1);
    
    EXPECT_EQ(messages[0].component, "TestComponent");
    EXPECT_EQ(messages[0].message, "Test message");
}

TEST_F(LoggerTest, FormattedLogging) {
    Logger& logger = Logger::getInstance();
    
    logger.infof("Value: %d, Name: %s", 42, "Test");
    
    const auto& messages = m_testOutputPtr->getMessages();
    ASSERT_EQ(messages.size(), 1);
    
    EXPECT_EQ(messages[0].message, "Value: 42, Name: Test");
}

TEST_F(LoggerTest, ComponentFormattedLogging) {
    Logger& logger = Logger::getInstance();
    
    logger.infofc("TestComponent", "Value: %d", 100);
    
    const auto& messages = m_testOutputPtr->getMessages();
    ASSERT_EQ(messages.size(), 1);
    
    EXPECT_EQ(messages[0].component, "TestComponent");
    EXPECT_EQ(messages[0].message, "Value: 100");
}

TEST_F(LoggerTest, MultipleOutputs) {
    Logger& logger = Logger::getInstance();
    
    auto secondOutput = std::make_unique<TestLogOutput>("Second");
    TestLogOutput* secondPtr = secondOutput.get();
    logger.addOutput(std::move(secondOutput));
    
    EXPECT_EQ(logger.getOutputCount(), 2);
    
    logger.info("Test message");
    
    EXPECT_EQ(m_testOutputPtr->getMessages().size(), 1);
    EXPECT_EQ(secondPtr->getMessages().size(), 1);
    
    logger.removeOutput("Second");
    EXPECT_EQ(logger.getOutputCount(), 1);
}

TEST_F(LoggerTest, ThreadSafety) {
    Logger& logger = Logger::getInstance();
    
    const int numThreads = 4;
    const int messagesPerThread = 100;
    std::vector<std::thread> threads;
    
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&logger, t, messagesPerThread]() {
            for (int i = 0; i < messagesPerThread; ++i) {
                logger.infof("Thread %d, Message %d", t, i);
                std::this_thread::sleep_for(std::chrono::microseconds(1));
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    const auto& messages = m_testOutputPtr->getMessages();
    EXPECT_EQ(messages.size(), numThreads * messagesPerThread);
    
    // Verify all messages are present
    std::vector<std::string> expectedMessages;
    for (int t = 0; t < numThreads; ++t) {
        for (int i = 0; i < messagesPerThread; ++i) {
            expectedMessages.push_back("Thread " + std::to_string(t) + ", Message " + std::to_string(i));
        }
    }
    
    std::vector<std::string> actualMessages;
    for (const auto& msg : messages) {
        actualMessages.push_back(msg.message);
    }
    
    std::sort(expectedMessages.begin(), expectedMessages.end());
    std::sort(actualMessages.begin(), actualMessages.end());
    
    EXPECT_EQ(actualMessages, expectedMessages);
}

TEST_F(LoggerTest, ConsoleOutput) {
    Logger& logger = Logger::getInstance();
    logger.clearOutputs();
    
    auto consoleOutput = std::make_unique<ConsoleOutput>();
    logger.addOutput(std::move(consoleOutput));
    
    // Just verify it doesn't crash
    logger.info("Console test message");
    logger.flush();
}

TEST_F(LoggerTest, FileOutput) {
    const std::string testFile = "test_log.txt";
    
    // Clean up any existing file
    if (std::filesystem::exists(testFile)) {
        std::filesystem::remove(testFile);
    }
    
    {
        Logger& logger = Logger::getInstance();
        logger.clearOutputs();
        
        auto fileOutput = std::make_unique<FileOutput>(testFile);
        logger.addOutput(std::move(fileOutput));
        
        logger.info("Test file message 1");
        logger.warning("Test file message 2", "FileTest");
        logger.flush();
    }
    
    // Verify file was created and contains expected content
    EXPECT_TRUE(std::filesystem::exists(testFile));
    
    std::ifstream file(testFile);
    std::string line1, line2;
    std::getline(file, line1);
    std::getline(file, line2);
    
    EXPECT_TRUE(line1.find("Test file message 1") != std::string::npos);
    EXPECT_TRUE(line2.find("Test file message 2") != std::string::npos);
    EXPECT_TRUE(line2.find("FileTest") != std::string::npos);
    
    // Clean up
    std::filesystem::remove(testFile);
}

TEST_F(LoggerTest, Macros) {
    LOG_INFO("Macro test message");
    LOG_DEBUG_C("MacroComponent", "Debug macro test");
    
    const auto& messages = m_testOutputPtr->getMessages();
    ASSERT_EQ(messages.size(), 2);
    
    EXPECT_EQ(messages[0].message, "Macro test message");
    EXPECT_EQ(messages[1].component, "MacroComponent");
    EXPECT_EQ(messages[1].message, "Debug macro test");
}