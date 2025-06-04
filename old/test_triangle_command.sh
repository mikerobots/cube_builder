#!/bin/bash
# Add a test triangle command to verify shader works

cat << 'EOF' > test_triangle_cmd.cpp
// Add to Commands.cpp - test triangle command
m_commandProcessor->registerCommand({
    "test",
    "Test rendering commands",
    CommandCategory::SYSTEM,
    {},
    {
        {"subcommand", "What to test: triangle", "string", true, ""}
    },
    [this](const CommandContext& ctx) {
        std::string subcommand = ctx.getArg(0);
        
        if (subcommand == "triangle") {
            // Create a simple triangle mesh
            Rendering::Mesh testMesh;
            
            // Three vertices in NDC space
            testMesh.vertices.resize(3);
            testMesh.vertices[0].position = Math::Vector3f(-0.5f, -0.5f, 0.0f);
            testMesh.vertices[0].normal = Math::Vector3f(0, 0, 1);
            testMesh.vertices[0].color = Rendering::Color(1.0f, 0.0f, 0.0f, 1.0f);
            testMesh.vertices[0].texCoords = Math::Vector2f(0, 0);
            
            testMesh.vertices[1].position = Math::Vector3f(0.5f, -0.5f, 0.0f);
            testMesh.vertices[1].normal = Math::Vector3f(0, 0, 1);
            testMesh.vertices[1].color = Rendering::Color(0.0f, 1.0f, 0.0f, 1.0f);
            testMesh.vertices[1].texCoords = Math::Vector2f(1, 0);
            
            testMesh.vertices[2].position = Math::Vector3f(0.0f, 0.5f, 0.0f);
            testMesh.vertices[2].normal = Math::Vector3f(0, 0, 1);
            testMesh.vertices[2].color = Rendering::Color(0.0f, 0.0f, 1.0f, 1.0f);
            testMesh.vertices[2].texCoords = Math::Vector2f(0.5f, 1);
            
            // Triangle indices
            testMesh.indices = {0, 1, 2};
            
            // Add to render list temporarily
            m_renderEngine->setupMeshBuffers(testMesh);
            m_testTriangleMesh = std::move(testMesh);
            m_renderTestTriangle = true;
            
            return CommandResult::Success("Test triangle enabled. Take a screenshot to see it.");
        }
        
        return CommandResult::Error("Unknown test subcommand");
    }
});
EOF

echo "Add this test command to Commands.cpp to debug shader rendering"