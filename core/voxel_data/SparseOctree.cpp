#include "SparseOctree.h"

using namespace VoxelEditor::Math;

namespace VoxelEditor {
namespace VoxelData {

// OctreeNode destructor implementation
OctreeNode::~OctreeNode() {
    if (!m_isLeaf) {
        for (auto& child : m_children) {
            if (child) {
                SparseOctree::deallocateNode(child);
            }
        }
    }
}

// OctreeNode clearChildren implementation
void OctreeNode::clearChildren() {
    if (!m_isLeaf) {
        for (auto& child : m_children) {
            if (child) {
                SparseOctree::deallocateNode(child);
                child = nullptr;
            }
        }
        m_isLeaf = true;
    }
}

// Static member definition
std::unique_ptr<Memory::TypedMemoryPool<OctreeNode>> SparseOctree::s_nodePool = nullptr;

void SparseOctree::initializePool(size_t initialSize) {
    if (!s_nodePool) {
        s_nodePool = std::make_unique<Memory::TypedMemoryPool<OctreeNode>>(initialSize);
    }
}

void SparseOctree::shutdownPool() {
    s_nodePool.reset();
}

OctreeNode* SparseOctree::allocateNode() {
    if (!s_nodePool) {
        initializePool();
    }
    
    OctreeNode* node = s_nodePool->construct();
    return node;
}

void SparseOctree::deallocateNode(OctreeNode* node) {
    if (node && s_nodePool) {
        s_nodePool->destroy(node);
    }
}

}
}