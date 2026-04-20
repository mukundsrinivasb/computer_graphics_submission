# comp8610 Group Project
## Mukund Notes
### The Model
The hierarchical structure was traversed recursively. At each node, the parent's coordinate space was preserved using the push matrix operation. Joint translations and rotations were applied sequentially to the ModelView matrix. 
The `collectMeshes()` method outlined in `model.hpp` traverses each node , in this case a Unit Cube centered at (0,0,0) and stores them with the necessary scaling and translations required to preserve the intended geometry of the model.

These were rasterized to the screen using the `renderNode()` methode defined in `render.hpp`. Each part of the model was translated with respect to its parent , and then rasterized on screen.

```
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
```
The floor was drawn using the drawFloor() method. In our case , its just a 1X1 meter grids repeated for a 100 meters in the X-Y Plane and X-Z plane

```
void drawFloor(){
  //Defining the grids on the floor
  glColor3f(0.3f,0.8f,0.3f);
  glLineWidth(1.0f);
  glBegin(GL_LINES);
  //Draw a 100X100 grid
  float floorHeight = -1.50f;
  for(float i=-50.0f;i<=50.0f;i+=1.0f){
    //Lines in the X-Z plane
    glVertex3f(i,floorHeight,-50.0f);
    glVertex3f(i,floorHeight,50.0f);
    //Lines in the X-Y plane
    glVertex3f(-50.0f,floorHeight,i);
    glVertex3f(50.0f,floorHeight,i);
  }
  glEnd();
}
```

### Z-buffer
The graphics pipeline utilizes a Z-buffer to store the depth value of each rendered pixel. This is implemented in `main.cpp` using OpenGLs native support for z-buffer through `glEnable(GL_DEPTH_TEST);`

### Shading
The lighting and projection was handled using OpenGLs native support in the `main.cpp`

```
    //Setting up lighting
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    GLfloat light_direction[] = {5.0f,10.0f,5.0f,0.0f};
    glLightfv(GL_LIGHT0,GL_POSITION,light_direction);
    // Set up the Lens
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, 1.0f, 0.1f, 100.0f);
```

