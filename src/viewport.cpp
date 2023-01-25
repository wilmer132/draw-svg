#include "viewport.h"

#include "CS248.h"

namespace CS248 {

void ViewportImp::set_viewbox( float x, float y, float span ) {

  // Task 3 (part 2): 
  // Set svg to normalized device coordinate transformation. 
  // Your input arguments are defined as SVG canvas coordinates.

  Matrix3x3 m;
  m(0,0) = (1 / (2 * span)); m(0,1) = 0; m(0,2) = ((span - x) / (2 * span));
  m(1,0) = 0; m(1,1) = (1 / (2 * span)); m(1,2) = ((span - y) / (2 * span));
  m(2,0) = 0; m(2,1) = 0; m(2,2) = 1;     

  set_canvas_to_norm(m);  

  this->x = x;
  this->y = y;
  this->span = span; 
}

void ViewportImp::update_viewbox( float dx, float dy, float scale ) { 
  
  this->x -= dx;
  this->y -= dy;
  this->span *= scale;
  set_viewbox( x, y, span );
}

} // namespace CS248
