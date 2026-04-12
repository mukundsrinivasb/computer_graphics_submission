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

### Z-buffer
By enabling depth testing, the graphics pipeline utilizes a Z-buffer to store the depth value of each rendered pixel. During the rasterization phase, incoming fragments are subjected to a depth comparison against the existing buffer values; fragments that are physically occluded by previously drawn geometry are discarded. The depth buffer is cleared alongside the color buffer at the beginning of each frame.

### Shading
to retain the localized color definitions of the animal's distinct body parts while under the influence of the global light source, the color material property was enabled. This instructed the lighting model to factor the current color state into its ambient and diffuse material reflectance calculations.
