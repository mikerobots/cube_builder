#pragma once

#include "VoxelTypes.h"
#include "../../foundation/memory/MemoryPool.h"
#include "../../foundation/math/Vector3i.h"
#include <memory>
#include <array>
#include <vector>
#include <iostream>

namespace VoxelEditor {
namespace VoxelData {

// Forward declaration
class SparseOctree;

// Octree node representing an 8-child spatial subdivision
class OctreeNode {
public:
    OctreeNode() : m_isLeaf(true), m_hasVoxel(false), m_voxelPos(-1, -1, -1) {
        m_children.fill(nullptr);
    }
    
    ~OctreeNode(); // Implementation in .cpp file to avoid forward declaration issues
    
    bool isLeaf() const { return m_isLeaf; }
    bool hasVoxel() const { return m_hasVoxel; }
    
    void setVoxel(bool value, const Math::Vector3i& pos = Math::Vector3i(-1, -1, -1)) {
        m_hasVoxel = value;
        m_voxelPos = pos;
    }
    
    Math::Vector3i getVoxelPos() const { return m_voxelPos; }
    
    // Get child index for a position within this node's bounds
    static int getChildIndex(const Math::Vector3i& pos, const Math::Vector3i& center) {
        int index = 0;
        if (pos.x >= center.x) index |= 1;
        if (pos.y >= center.y) index |= 2;
        if (pos.z >= center.z) index |= 4;
        return index;
    }
    
    // Calculate child center given parent center and child index
    static Math::Vector3i getChildCenter(const Math::Vector3i& parentCenter, int childIndex, int halfSize) {
        Math::Vector3i offset(
            (childIndex & 1) ? halfSize : -halfSize,
            (childIndex & 2) ? halfSize : -halfSize,
            (childIndex & 4) ? halfSize : -halfSize
        );
        return parentCenter + offset;
    }
    
    OctreeNode* getChild(int index) const {
        return (index >= 0 && index < 8) ? m_children[index] : nullptr;
    }
    
    void setChild(int index, OctreeNode* child) {
        if (index >= 0 && index < 8) {
            m_children[index] = child;
            if (child != nullptr) {
                m_isLeaf = false;
            }
        }
    }
    
    // Check if node has any non-null children
    bool hasChildren() const {
        if (m_isLeaf) return false;
        for (const auto& child : m_children) {
            if (child != nullptr) return true;
        }
        return false;
    }
    
    // Count non-null children
    int getChildCount() const {
        if (m_isLeaf) return 0;
        int count = 0;
        for (const auto& child : m_children) {
            if (child != nullptr) count++;
        }
        return count;
    }
    
    // Remove all children and mark as leaf
    void clearChildren();
    
private:
    bool m_isLeaf;
    bool m_hasVoxel;
    Math::Vector3i m_voxelPos;  // Position of the voxel (only valid if m_hasVoxel is true)
    std::array<OctreeNode*, 8> m_children;
    
    friend class SparseOctree;
};

class SparseOctree {
public:
    SparseOctree(int maxDepth = 10) : m_root(nullptr), m_maxDepth(maxDepth), m_nodeCount(0) {
        // Calculate root bounds based on max depth
        int rootSize = 1 << maxDepth;  // 2^maxDepth
        // Root center should be at the middle of the space
        // For an 8x8x8 octree (depth=3), center is at (4,4,4)
        m_rootCenter = Math::Vector3i(rootSize / 2, rootSize / 2, rootSize / 2);
        m_rootSize = rootSize;
    }
    
    ~SparseOctree() {
        clear();
    }
    
    // Non-copyable
    SparseOctree(const SparseOctree&) = delete;
    SparseOctree& operator=(const SparseOctree&) = delete;
    
    // Set a voxel at the given position
    bool setVoxel(const Math::Vector3i& pos, bool value) {
        if (!isPositionValid(pos)) {
            return false;
        }
        
        if (value) {
            return insertVoxel(pos);
        } else {
            return removeVoxel(pos);
        }
    }
    
    // Get voxel value at position
    bool getVoxel(const Math::Vector3i& pos) const {
        if (!isPositionValid(pos)) {
            return false;
        }
        
        if (!m_root) {
            return false;
        }
        
        return findVoxel(m_root, pos, m_rootCenter, m_rootSize / 2, 0);
    }
    
    // Check if voxel exists at position
    bool hasVoxel(const Math::Vector3i& pos) const {
        return getVoxel(pos);
    }
    
    // Clear all voxels
    void clear() {
        if (m_root) {
            deallocateNode(m_root);
            m_root = nullptr;
        }
        m_nodeCount = 0;
    }
    
    // Get memory usage statistics
    size_t getMemoryUsage() const {
        return m_nodeCount * sizeof(OctreeNode);
    }
    
    size_t getNodeCount() const {
        return m_nodeCount;
    }
    
    size_t getVoxelCount() const {
        return getAllVoxels().size();
    }
    
    // Get all voxel positions
    std::vector<Math::Vector3i> getAllVoxels() const {
        std::vector<Math::Vector3i> voxels;
        if (m_root) {
            collectVoxels(m_root, m_rootCenter, m_rootSize / 2, 0, voxels);
        }
        return voxels;
    }
    
    // Optimize memory by removing empty branches
    void optimize() {
        if (m_root) {
            optimizeNode(m_root);
        }
    }
    
    // Static memory pool management
    static void initializePool(size_t initialSize = 1024);
    static void shutdownPool();
    static OctreeNode* allocateNode();
    static void deallocateNode(OctreeNode* node);
    
private:
    OctreeNode* m_root;
    Math::Vector3i m_rootCenter;
    int m_rootSize;
    int m_maxDepth;
    size_t m_nodeCount;
    
    static std::unique_ptr<Memory::TypedMemoryPool<OctreeNode>> s_nodePool;
    
    bool isPositionValid(const Math::Vector3i& pos) const {
        int halfRoot = m_rootSize / 2;
        return pos.x >= 0 && pos.x < m_rootSize &&
               pos.y >= 0 && pos.y < m_rootSize &&
               pos.z >= 0 && pos.z < m_rootSize;
    }
    
    bool insertVoxel(const Math::Vector3i& pos) {
        if (!m_root) {
            m_root = allocateNode();
            if (!m_root) return false;
            m_nodeCount++;
        }
        
        std::cout << "SparseOctree::insertVoxel(" << pos.x << "," << pos.y << "," << pos.z 
                  << ") - rootCenter: " << m_rootCenter.x << "," << m_rootCenter.y << "," << m_rootCenter.z
                  << ", rootSize: " << m_rootSize << ", maxDepth: " << m_maxDepth << std::endl;
        
        return insertVoxelRecursive(m_root, pos, m_rootCenter, m_rootSize / 2, 0);
    }
    
    bool insertVoxelRecursive(OctreeNode* node, const Math::Vector3i& pos, 
                            const Math::Vector3i& center, int halfSize, int depth) {
        if (depth >= m_maxDepth) {
            // At leaf level, set the voxel and store its position
            node->setVoxel(true, pos);
            std::cout << "SparseOctree: Inserted voxel (" << pos.x << "," << pos.y << "," << pos.z 
                      << ") at leaf with center (" << center.x << "," << center.y << "," << center.z 
                      << "), halfSize=" << halfSize << ", depth=" << depth << std::endl;
            return true;
        }
        
        int childIndex = OctreeNode::getChildIndex(pos, center);
        
        std::cout << "  Depth " << depth << ": pos(" << pos.x << "," << pos.y << "," << pos.z 
                  << ") center(" << center.x << "," << center.y << "," << center.z 
                  << ") halfSize=" << halfSize << " -> childIndex=" << childIndex << std::endl;
        
        OctreeNode* child = node->getChild(childIndex);
        
        if (!child) {
            child = allocateNode();
            if (!child) return false;
            m_nodeCount++;
            node->setChild(childIndex, child);
        }
        
        Math::Vector3i childCenter = OctreeNode::getChildCenter(center, childIndex, halfSize / 2);
        std::cout << "    -> childCenter(" << childCenter.x << "," << childCenter.y << "," << childCenter.z << ")" << std::endl;
        return insertVoxelRecursive(child, pos, childCenter, halfSize / 2, depth + 1);
    }
    
    bool removeVoxel(const Math::Vector3i& pos) {
        if (!m_root) {
            return false;
        }
        
        return removeVoxelRecursive(m_root, pos, m_rootCenter, m_rootSize / 2, 0);
    }
    
    bool removeVoxelRecursive(OctreeNode* node, const Math::Vector3i& pos,
                            const Math::Vector3i& center, int halfSize, int depth) {
        if (depth >= m_maxDepth) {
            // At leaf level, remove the voxel
            node->setVoxel(false);
            return true;
        }
        
        int childIndex = OctreeNode::getChildIndex(pos, center);
        OctreeNode* child = node->getChild(childIndex);
        
        if (!child) {
            return false; // Voxel doesn't exist
        }
        
        Math::Vector3i childCenter = OctreeNode::getChildCenter(center, childIndex, halfSize / 2);
        bool removed = removeVoxelRecursive(child, pos, childCenter, halfSize / 2, depth + 1);
        
        // Check if child node is now empty and can be removed
        if (removed && canRemoveChild(child)) {
            deallocateNode(child);
            node->setChild(childIndex, nullptr);
        }
        
        return removed;
    }
    
    bool findVoxel(OctreeNode* node, const Math::Vector3i& pos,
                  const Math::Vector3i& center, int halfSize, int depth) const {
        if (depth >= m_maxDepth) {
            return node->hasVoxel();
        }
        
        int childIndex = OctreeNode::getChildIndex(pos, center);
        OctreeNode* child = node->getChild(childIndex);
        
        if (!child) {
            return false;
        }
        
        Math::Vector3i childCenter = OctreeNode::getChildCenter(center, childIndex, halfSize / 2);
        return findVoxel(child, pos, childCenter, halfSize / 2, depth + 1);
    }
    
    void collectVoxels(OctreeNode* node, const Math::Vector3i& center, int halfSize, int depth,
                      std::vector<Math::Vector3i>& voxels) const {
        if (depth >= m_maxDepth) {
            if (node->hasVoxel()) {
                // Use the stored voxel position
                Math::Vector3i voxelPos = node->getVoxelPos();
                voxels.push_back(voxelPos);
                
                // Debug output
                std::cout << "  Octree voxel at (" << voxelPos.x << ", " << voxelPos.y << ", " << voxelPos.z << ")" << std::endl;
            }
            return;
        }
        
        for (int i = 0; i < 8; ++i) {
            OctreeNode* child = node->getChild(i);
            if (child) {
                Math::Vector3i childCenter = OctreeNode::getChildCenter(center, i, halfSize / 2);
                collectVoxels(child, childCenter, halfSize / 2, depth + 1, voxels);
            }
        }
    }
    
    bool canRemoveChild(OctreeNode* node) const {
        if (node->isLeaf()) {
            return !node->hasVoxel();
        } else {
            return !node->hasChildren();
        }
    }
    
    void optimizeNode(OctreeNode* node) {
        if (node->isLeaf()) {
            return;
        }
        
        // First optimize all children
        for (int i = 0; i < 8; ++i) {
            OctreeNode* child = node->getChild(i);
            if (child) {
                optimizeNode(child);
                
                // Remove child if it's now empty
                if (canRemoveChild(child)) {
                    deallocateNode(child);
                    node->setChild(i, nullptr);
                }
            }
        }
        
        // If no children remain, mark as leaf
        if (!node->hasChildren()) {
            node->clearChildren();
        }
    }
};

}
}