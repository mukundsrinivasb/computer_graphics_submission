/*
 * Task 3: Cubism Animal via Hierarchical Modelling
 *
 * Build a blocky animal (or robot) from transformed unit cubes arranged
 * in a parent-child tree. Export the result as a Wavefront OBJ file.
 *
 * Build:  mkdir build && cd build && cmake .. && make && ./task3_cubism_animal
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cmath>
#include <memory>
#include <Eigen/Dense>

using namespace std;
using namespace Eigen;

// ---- Data Structures ----

struct Triangle {
    int v0, v1, v2;   // 0-based vertex indices (0-based means 0-indexed)
};

struct Mesh {
    vector<Vector3f> vertices;
    vector<Triangle> faces;
};

struct ModelNode {
    string name;
    Vector3f position;
    Vector3f rotation;
    Vector3f scale;

    vector<unique_ptr<ModelNode>> children;

    ModelNode(const string& name,
              const Vector3f& pos = Vector3f::Zero(),
              const Vector3f& rot = Vector3f::Zero(),
              const Vector3f& scl = Vector3f::Ones())
        : name(name), position(pos), rotation(rot), scale(scl) {}

    ModelNode* addChild(const string& name,
                        const Vector3f& pos,
                        const Vector3f& scl,
                        const Vector3f& rot = Vector3f::Zero()) {
        children.push_back(make_unique<ModelNode>(name, pos, rot, scl));
        return children.back().get();
    }
};

// ---- Transformation Matrices ----

// TODO
Matrix4f translationMatrix(const Vector3f& t) {
    Matrix4f M = Matrix4f::Identity();
    M(0, 3) = t.x();
    M(1, 3) = t.y();
    M(2, 3) = t.z();
    return M;
}

// TODO
Matrix4f scalingMatrix(const Vector3f& s) {
    Matrix4f M = Matrix4f::Identity();
    M(0, 0) = s.x();
    M(1, 1) = s.y();
    M(2, 2) = s.z();
    return M;
}

// TODO (remember to convert degrees to radians)
Matrix4f rotationX(float degrees) {
    float r = degrees * M_PI / 180.0f;
    Matrix4f M = Matrix4f::Identity();

    float c = cos(r); 
    float s = sin(r);

    M(1, 1) = c;
    M(1, 2) = -s;
    M(2, 1) = s;
    M(2, 2) = c;

    return M;
}

// TODO
Matrix4f rotationY(float degrees) {
    float r = degrees * M_PI / 180.0f;
    Matrix4f M = Matrix4f::Identity();

    float c = cos(r); 
    float s = sin(r);

    M(0, 0) = c;
    M(0, 2) = s;
    M(2, 0) = -s;
    M(2, 2) = c;

    return M;
}

// TODO
Matrix4f rotationZ(float degrees) {
    float r = degrees * M_PI / 180.0f;
    Matrix4f M = Matrix4f::Identity();

    float c = cos(r); 
    float s = sin(r);

    M(0, 0) = c;
    M(0, 1) = -s;
    M(1, 0) = s;
    M(1, 1) = c;

    return M;
}

Matrix4f rotationMatrix(const Vector3f& eulerDeg) {
    return rotationZ(eulerDeg.z()) * rotationY(eulerDeg.y()) * rotationX(eulerDeg.x());
}

// ---- Unit Cube ----

// TODO: unit cube centered at origin, 8 vertices, 12 triangles (CCW winding)
Mesh createUnitCube() {
    Mesh mesh;
    mesh.vertices = {
        // TODO
        Vector3f(0.5, 0.5,  0.5), Vector3f(-0.5, 0.5,  0.5), Vector3f(0.5, -0.5,  0.5), Vector3f(-0.5, -0.5,  0.5), 
        Vector3f(0.5, 0.5, -0.5), Vector3f(-0.5, 0.5, -0.5), Vector3f(0.5, -0.5, -0.5), Vector3f(-0.5, -0.5, -0.5)
    };

    mesh.faces = {
        // TODO
        (Triangle {0, 3, 2}), (Triangle {0, 1, 3}), 
        (Triangle {1, 7, 3}), (Triangle {1, 5, 7}), 
        (Triangle {5, 6, 7}), (Triangle {5, 4, 6}), 
        (Triangle {4, 2, 6}), (Triangle {4, 0, 2}), 
        (Triangle {4, 1, 0}), (Triangle {4, 5, 1}), 
        (Triangle {2, 7, 6}), (Triangle {2, 3, 7})
    };
    return mesh;
}

// ---- Hierarchical Mesh Collection ----

// TODO: traverse the tree, transform each node's cube, collect into outVerts/outFaces.
// Important: pass jointWorld (Translation*Rotation only, no scale) to children, not meshWorld.
void collectMeshes(const ModelNode* node,
                   const Matrix4f& parentJointWorld,
                   vector<Vector3f>& outVerts,
                   vector<Triangle>& outFaces) {
    // TODO
    Matrix4f rotation = rotationMatrix(node->rotation);
    Matrix4f translation = translationMatrix(node->position);
    Matrix4f scale = scalingMatrix(node->scale);

    Matrix4f jointWorld = parentJointWorld * translation * rotation;
    const Matrix4f& passJointWorld = jointWorld;

    for (int i = 0; i < node->children.size(); i++){
        const ModelNode* child = node->children[i].get();
        collectMeshes(child, passJointWorld, outVerts, outFaces);
    }

    Matrix4f transformation = jointWorld * scale;
    Mesh cube = createUnitCube();

    int index = outVerts.size();

    // have to convert to a 4x1 vector to do the multiplication and back again 
    for (Vector3f v : cube.vertices) {
        Vector4f v4 = {v.x(), v.y(), v.z(), 1};
        v4 = transformation * v4;
        for (int i = 0; i < 3; i++){
            v[i] = v4[i];
        }
        outVerts.push_back(v);
    }

    for (Triangle t : cube.faces){
        t.v0 += index;
        t.v1 += index;
        t.v2 += index;

        outFaces.push_back(t);
    }

}

// ---- OBJ Export ----

// TODO: write vertices and faces to .obj (OBJ indices are 1-based!) (1-based means 1-indexed)
void exportOBJ(const string& filename,
               const vector<Vector3f>& vertices,
               const vector<Triangle>& faces) {
    ofstream ofs(filename);
    if (!ofs.is_open()) {
        cerr << "Error: cannot open " << filename << " for writing." << endl;
        return;
    }

    // TODO
    ofs << "# Vertices" << endl << endl;

    for (Vector3f v : vertices) {
        ofs << "v " << ++v.x() << " " << ++v.y() << " " << ++v.z() << endl;
    }

    ofs << endl << "# Faces" << endl << endl;

    for (Triangle f : faces) {
        ofs << "f " << ++f.v0 << " " << ++f.v1 << " " << ++f.v2 << endl;
    }

    ofs.close();
    cout << "Exported: " << filename << endl;
}

// ---- Print Tree ----

void printHierarchy(const ModelNode* node, int depth = 0) {
    string indent(depth * 2, ' ');
    cout << indent << "|- " << node->name
         << "  pos(" << node->position.transpose() << ")"
         << "  rot(" << node->rotation.transpose() << ")"
         << "  scale(" << node->scale.transpose() << ")" << endl;
    for (const auto& child : node->children) {
        printHierarchy(child.get(), depth + 1);
    }
}

// ---- Build Your Animal ----

// TODO: build your animal here (at least 6-8 parts)
unique_ptr<ModelNode> buildAnimalModel() {
    auto body = make_unique<ModelNode>(
        "Body",
        Vector3f(0, 0, 0),
        Vector3f(0, 0, 0),
        Vector3f(1.2f, 1.0f, 2.0f));

    // TODO: use addChild() to attach parts
    // e.g. body->addChild("Head", Vector3f(0, 0.5f, 1.3f), Vector3f(0.8f, 0.8f, 0.8f));
    
    auto head = body->addChild("Head", Vector3f(0, 0.8f, 1.2f), Vector3f(1.2f, 1, 1), Vector3f(-10, 0, 0));
    auto snout = head->addChild("Snout", Vector3f(0, -0.25f, 0.68f), Vector3f(1.2, 0.5f, 0.36f), Vector3f(0, 0, 0));

    auto earL = head->addChild("Left ear", Vector3f(0.65f, 0.15f, 0.2f), Vector3f(0.2f, 0.66f, 0.4f), Vector3f(0, 0, 20));
    auto earR = head->addChild("Right ear", Vector3f(-0.65f, 0.15f, 0.2f), Vector3f(0.2f, 0.66f, 0.4f), Vector3f(0, 0, -20));
    
    auto tail = body->addChild("Tail", Vector3f(0, 0.5f, -1.4f), Vector3f(0.3f, 0.3f, 1), Vector3f(20, 0, 0));
    
    auto legFL = body->addChild("Front left leg", Vector3f(0.4f, -0.85f, 0.6f), Vector3f(0.4f, 1, 0.4f), Vector3f(25, 0, 0));
    auto legFR = body->addChild("Front right leg", Vector3f(-0.4f, -0.85f, 0.95f), Vector3f(0.4f, 1, 0.4f), Vector3f(-25, 0, 0));
    auto legBL = body->addChild("Back left leg", Vector3f(0.4f, -0.85f, -0.6f), Vector3f(0.4f, 1, 0.4f), Vector3f(-25, 0, 0));
    auto legBR = body->addChild("Back right leg", Vector3f(-0.4f, -0.85f, -0.95f), Vector3f(0.4f, 1, 0.4f), Vector3f(25, 0, 0));

    return body;
}

// ---- Main ----

int main() {
    cout << "========================================" << endl;
    cout << " Task 3: Cubism Animal" << endl;
    cout << " Hierarchical Modelling" << endl;
    cout << "========================================" << endl << endl;

    auto animal = buildAnimalModel();

    cout << "--- Model Hierarchy ---" << endl;
    printHierarchy(animal.get());
    cout << endl;

    vector<Vector3f> allVertices;
    vector<Triangle> allFaces;
    collectMeshes(animal.get(), Matrix4f::Identity(), allVertices, allFaces);

    exportOBJ("../model/cubism_animal.obj", allVertices, allFaces);

    int V = (int)allVertices.size();
    int F = (int)allFaces.size();
    int E = F * 3 / 2;

    cout << endl;
    cout << "--- Model Statistics ---" << endl;
    cout << "Total vertices  (V): " << V << endl;
    cout << "Total edges     (E): " << E << endl;
    cout << "Total faces     (F): " << F << endl;
    cout << "Euler: V - E + F = " << (V - E + F) << endl;

    cout << endl << "Done. Open model/cubism_animal.obj in MeshLab to visualize." << endl;
    return 0;
}
