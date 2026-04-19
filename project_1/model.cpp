#include "model.hpp"
#include <iostream>
#include <fstream>
#include <cmath>

using namespace std;
using namespace Eigen;

ModelNode* ModelNode::addChild(const string& name,
                               const Vector3f& pos,
                               const Vector3f& scl,
                               const Vector3f& rot, 
                               const Vector3f& pre_pos) {
    children.push_back(make_unique<ModelNode>(name, pos, rot, scl, pre_pos));
    return children.back().get();
}

void ModelNode::setYRotation(float y) {
        rotation[1] = y;
}

Matrix4f translationMatrix(const Vector3f& t) {
    Matrix4f M = Matrix4f::Identity();
    M(0, 3) = t.x(); M(1, 3) = t.y(); M(2, 3) = t.z();
    return M;
}

Matrix4f scalingMatrix(const Vector3f& s) {
    Matrix4f M = Matrix4f::Identity();
    M(0, 0) = s.x(); M(1, 1) = s.y(); M(2, 2) = s.z();
    return M;
}

Matrix4f rotationX(float degrees) {
    float r = degrees * M_PI / 180.0f;
    Matrix4f M = Matrix4f::Identity();
    float c = cos(r); float s = sin(r);
    M(1, 1) = c; M(1, 2) = -s; M(2, 1) = s; M(2, 2) = c;
    return M;
}

Matrix4f rotationY(float degrees) {
    float r = degrees * M_PI / 180.0f;
    Matrix4f M = Matrix4f::Identity();
    float c = cos(r); float s = sin(r);
    M(0, 0) = c; M(0, 2) = s; M(2, 0) = -s; M(2, 2) = c;
    return M;
}

Matrix4f rotationZ(float degrees) {
    float r = degrees * M_PI / 180.0f;
    Matrix4f M = Matrix4f::Identity();
    float c = cos(r); float s = sin(r);
    M(0, 0) = c; M(0, 1) = -s; M(1, 0) = s; M(1, 1) = c;
    return M;
}

Matrix4f rotationMatrix(const Vector3f& eulerDeg) {
    return rotationZ(eulerDeg.z()) * rotationY(eulerDeg.y()) * rotationX(eulerDeg.x());
}

Mesh createUnitCube() {
    Mesh mesh;
    mesh.vertices = {
        Vector3f(0.5, 0.5,  0.5), Vector3f(-0.5, 0.5,  0.5), Vector3f(0.5, -0.5,  0.5), Vector3f(-0.5, -0.5,  0.5), 
        Vector3f(0.5, 0.5, -0.5), Vector3f(-0.5, 0.5, -0.5), Vector3f(0.5, -0.5, -0.5), Vector3f(-0.5, -0.5, -0.5)
    };
    mesh.faces = {
        (Triangle {0, 3, 2}), (Triangle {0, 1, 3}), 
        (Triangle {1, 7, 3}), (Triangle {1, 5, 7}), 
        (Triangle {5, 6, 7}), (Triangle {5, 4, 6}), 
        (Triangle {4, 2, 6}), (Triangle {4, 0, 2}), 
        (Triangle {4, 1, 0}), (Triangle {4, 5, 1}), 
        (Triangle {2, 7, 6}), (Triangle {2, 3, 7})
    };
    return mesh;
}

void collectMeshes(const ModelNode* node, const Matrix4f& parentJointWorld, vector<Vector3f>& outVerts, vector<Triangle>& outFaces) {
    Matrix4f rotation = rotationMatrix(node->rotation);
    Matrix4f translation = translationMatrix(node->position);
    Matrix4f scale = scalingMatrix(node->scale);
    Matrix4f pre_translation = translationMatrix(node->pre_position);

    Matrix4f jointWorld = parentJointWorld * translation * rotation * pre_translation;
    const Matrix4f& passJointWorld = jointWorld;

    for (int i = 0; i < node->children.size(); i++){
        const ModelNode* child = node->children[i].get();
        collectMeshes(child, passJointWorld, outVerts, outFaces);
    }

    Matrix4f transformation = jointWorld * scale;
    Mesh cube = createUnitCube();
    int index = outVerts.size();

    for (Vector3f v : cube.vertices) {
        Vector4f v4 = {v.x(), v.y(), v.z(), 1};
        v4 = transformation * v4;
        for (int i = 0; i < 3; i++) { v[i] = v4[i]; }
        outVerts.push_back(v);
    }
    for (Triangle t : cube.faces){
        t.v0 += index; t.v1 += index; t.v2 += index;
        outFaces.push_back(t);
    }
}

void exportOBJ(const string& filename, const vector<Vector3f>& vertices, const vector<Triangle>& faces) {
    ofstream ofs(filename);
    if (!ofs.is_open()) {
        cerr << "Error: cannot open " << filename << " for writing." << endl;
        return;
    }
    ofs << "# Vertices\n\n";
    for (Vector3f v : vertices) { ofs << "v " << ++v.x() << " " << ++v.y() << " " << ++v.z() << "\n"; }
    ofs << "\n# Faces\n\n";
    for (Triangle f : faces) { ofs << "f " << ++f.v0 << " " << ++f.v1 << " " << ++f.v2 << "\n"; }
    ofs.close();
    cout << "Exported: " << filename << endl;
}

void printHierarchy(const ModelNode* node, int depth) {
    string indent(depth * 2, ' ');
    cout << indent << "|- " << node->name << "  pos(" << node->position.transpose() << ")"
         << "  rot(" << node->rotation.transpose() << ")"
         << "  scale(" << node->scale.transpose() << ")" << endl;
    for (const auto& child : node->children) { printHierarchy(child.get(), depth + 1); }
}

unique_ptr<ModelNode> buildAnimalModelAtPoint(float point) { // point must be within [0, 1)
    return buildAnimalModelAtTime(2*point);
}

// given the amount of time (in seconds) since the start of the animation,
// give the scalars which will modify the rotations of moving body parts
// at that time
static std::tuple<float, float, float, float> time_to_scalars(float time){
    float legs = sin(M_PI*time); //scalar for leg cycle
    float head = cos(M_PI*time*2); //scalar for head cycle
    float ears = cos(M_PI*(time-0.1)*2); //scalar for ears cycle
    float tail = -cos(M_PI*time*2); //scalar for tail cycle

    return {head, ears, tail, legs};
}

unique_ptr<ModelNode> buildAnimalModelAtTime(float time) {
    auto[h, e, t, l] = time_to_scalars(time);

    auto body = make_unique<ModelNode>("Body", Vector3f(0, 0, 0), Vector3f(0, 0, 0), Vector3f(1.2f, 1.0f, 2.0f));
    
    auto head = body->addChild("Head", Vector3f(0, 0.3, 0.7), Vector3f(1.2f, 1, 1), Vector3f(-5 + (h*5), 0, 0), Vector3f(0, 0.5f, 0.5f)); //animated
    auto snout = head->addChild("Snout", Vector3f(0, -0.25f, 0.68f), Vector3f(1.2, 0.5f, 0.36f), Vector3f(0, 0, 0));

    auto earL = head->addChild("Left ear", Vector3f(0.55f, 0.4f, 0.2f), Vector3f(0.2f, 0.66f, 0.4f), Vector3f(0, 0, 20 - (e*10)), Vector3f(0.1, -0.33f, 0)); //animated
    auto earR = head->addChild("Right ear", Vector3f(-0.55f, 0.4f, 0.2f), Vector3f(0.2f, 0.66f, 0.4f), Vector3f(0, 0, -20 + (e*10)), Vector3f(-0.1, -0.33f, 0)); //animated
    
    auto tail = body->addChild("Tail", Vector3f(0, 0.5f, -0.9f), Vector3f(0.3f, 0.3f, 1), Vector3f(20 + (t*5), 0, 0), Vector3f(0, -0.15, -0.5)); //animated
    
    auto legFL = body->addChild("Front left leg", Vector3f(0.4f, -0.35f, 0.8f), Vector3f(0.4f, 1, 0.4f), Vector3f(25*l, 0, 0), Vector3f(0, -0.5, 0)); //animated
    auto legFR = body->addChild("Front right leg", Vector3f(-0.4f, -0.35f, 0.8f), Vector3f(0.4f, 1, 0.4f), Vector3f(-25*l, 0, 0), Vector3f(0, -0.5, 0)); //animated
    auto legBL = body->addChild("Back left leg", Vector3f(0.4f, -0.35f, -0.8f), Vector3f(0.4f, 1, 0.4f), Vector3f(-25*l, 0, 0), Vector3f(0, -0.5, 0)); //animated
    auto legBR = body->addChild("Back right leg", Vector3f(-0.4f, -0.35f, -0.8f), Vector3f(0.4f, 1, 0.4f), Vector3f(25*l, 0, 0), Vector3f(0, -0.5, 0)); //animated

    return body;
}