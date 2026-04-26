#ifndef MODEL_HPP
#define MODEL_HPP

#include<vector>
#include<string>
#include<memory>
#include<Eigen/Dense>

// ---- Data Structures ----

struct Triangle {
    int v0, v1, v2;   // 0-based vertex indices (0-based means 0-indexed)
};

struct Mesh {
  std::vector<Eigen::Vector3f> vertices;
  std::vector<Triangle> faces;
};

struct ModelNode {
    std::string name;
    Eigen::Vector3f position;
    Eigen::Vector3f rotation;
    Eigen::Vector3f scale;
    Eigen::Vector3f pre_position;

    std::vector<std::unique_ptr<ModelNode>> children;

    ModelNode(const std::string& name,
              const Eigen::Vector3f& pos = Eigen::Vector3f::Zero(),
              const Eigen::Vector3f& rot = Eigen::Vector3f::Zero(),
              const Eigen::Vector3f& scl = Eigen::Vector3f::Ones(), 
              const Eigen::Vector3f& pre_pos = Eigen::Vector3f::Zero())
        : name(name), position(pos), rotation(rot), scale(scl), pre_position(pre_pos)  {}

    ModelNode* addChild(const std::string& name,
                        const Eigen::Vector3f& pos,
                        const Eigen::Vector3f& scl,
                        const Eigen::Vector3f& rot = Eigen::Vector3f::Zero(), 
                        const Eigen::Vector3f& pre_pos = Eigen::Vector3f::Zero());
    
    void setYRotation(float y);
};

// Methods 
Eigen::Matrix4f translationMatrix(const Eigen::Vector3f& t);
Eigen::Matrix4f scalingMatrix(const Eigen::Vector3f& s);
Eigen::Matrix4f rotationX(float degrees);
Eigen::Matrix4f rotationY(float degrees);
Eigen::Matrix4f rotationZ(float degrees);
Eigen::Matrix4f rotationMatrix(const Eigen::Vector3f& eulerDeg);

Mesh createUnitCube();

void collectMeshes(const ModelNode* node,
                   const Eigen::Matrix4f& parentJointWorld,
                   std::vector<Eigen::Vector3f>& outVerts,
                   std::vector<Triangle>& outFaces);

void exportOBJ(const std::string& filename,
               const std::vector<Eigen::Vector3f>& vertices,
               const std::vector<Triangle>& faces);

void printHierarchy(const ModelNode* node, int depth = 0);

static std::tuple<float, float, float, float> scalars_at_time(float time);
std::unique_ptr<ModelNode> buildAnimalModelAtTime(float time);
std::unique_ptr<ModelNode> buildAnimalModelAtPoint(float point);

#endif
