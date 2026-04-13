#include "render.hpp"
// #include <Windows.h>
#include <GLFW/glfw3.h>

using namespace Eigen;
using namespace std;

void drawMesh(const Mesh& mesh){
  glBegin(GL_TRIANGLES);
  glColor3f(0.5f,0.5f,0.5f);
  for(const Triangle& t : mesh.faces){
    const Vector3f& v0 = mesh.vertices[t.v0];
    const Vector3f& v1 = mesh.vertices[t.v1];
    const Vector3f& v2 = mesh.vertices[t.v2];
    
    const Vector3f& edge1 = v1-v0;
    const Vector3f& edge2 = v2-v0;
    const Vector3f& normal = edge1.cross(edge2).normalized();
    
    glNormal3f(normal.x(),normal.y(),normal.z());
    glVertex3f(v0.x(),v0.y(),v0.z());
    glVertex3f(v1.x(),v1.y(),v1.z());
    glVertex3f(v2.x(),v2.y(),v2.z());
  }
  glEnd();
}

void renderNode(const ModelNode* node){
  glPushMatrix();
  //Move to the attachment point
  glTranslatef(node->position.x(),node->position.y(),node->position.z());
  //Rotate as per the model heirarchy
  glRotatef(node->rotation.z(),0.0f,0.0f,1.0f);
  glRotatef(node->rotation.y(),0.0f,1.0f,0.0f);
  glRotatef(node->rotation.x(),1.0f,0.0f,0.0f);
 //Apply translations with respect to the parent node
 //For instance , if the current node is connected to the head , move to head and then apply scaling
  glTranslatef(node->pre_position.x(),node->pre_position.y(),node->pre_position.z());
    glPushMatrix();
      glScalef(node->scale.x(),node->scale.y(),node->scale.z());
      drawMesh(createUnitCube());
    glPopMatrix();
    
    for(const unique_ptr<ModelNode>& child : node->children){
      renderNode(child.get());
    }
  glPopMatrix(); // Restoring parent 
}
