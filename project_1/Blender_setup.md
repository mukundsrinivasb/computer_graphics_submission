# comp8610_group_project
## Mukund Notes
### The Model
The hierarchical structure was traversed recursively. At each node, the parent's coordinate space was preserved using the push matrix operation. Joint translations and rotations were applied sequentially to the ModelView matrix.
local scaling transformations required isolation to prevent geometric distortion in child nodes. This was achieved by introducing a secondary push/pop matrix scope exclusively for the scaling operation prior to drawing the mesh. This ensured that while child limbs inherited the parent's position and rotation, they did not inherit its structural stretching.

### Z-buffer
By enabling depth testing, the graphics pipeline utilizes a Z-buffer to store the depth value of each rendered pixel. During the rasterization phase, incoming fragments are subjected to a depth comparison against the existing buffer values; fragments that are physically occluded by previously drawn geometry are discarded. The depth buffer is cleared alongside the color buffer at the beginning of each frame.

### Shading
to retain the localized color definitions of the animal's distinct body parts while under the influence of the global light source, the color material property was enabled. This instructed the lighting model to factor the current color state into its ambient and diffuse material reflectance calculations.
