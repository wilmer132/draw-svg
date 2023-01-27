#include "software_renderer.h"
#include "texture.h"

#include <cmath>
#include <vector>
#include <iostream>
#include <algorithm>

#include "triangulation.h"

using namespace std;

namespace CS248 {


// Implements SoftwareRenderer //

// fill a sample location with color
void SoftwareRendererImp::fill_sample(int sx, int sy, int sb, const Color &color) {
  // Task 2: implement this function

	// check bounds
	if (sx < 0 || sx >= width) return;
	if (sy < 0 || sy >= height) return;

  int const sample_r_i = 4 * (sx + sy * width) + (4 * width * height * sb);
  int const sample_g_i = 4 * (sx + sy * width) + (4 * width * height * sb) + 1;
  int const sample_b_i = 4 * (sx + sy * width) + (4 * width * height * sb) + 2;
  int const sample_a_i = 4 * (sx + sy * width) + (4 * width * height * sb) + 3;

	Color pixel_color;
	float inv255 = 1.0 / 255.0;
	pixel_color.r = sample_buffer[sample_r_i] * inv255;
	pixel_color.g = sample_buffer[sample_g_i] * inv255;
	pixel_color.b = sample_buffer[sample_b_i] * inv255;
	pixel_color.a = sample_buffer[sample_a_i] * inv255;

  pixel_color = alpha_blending(pixel_color, color);

	sample_buffer[sample_r_i] = (uint8_t)(pixel_color.r * 255);
	sample_buffer[sample_g_i] = (uint8_t)(pixel_color.g * 255);
	sample_buffer[sample_b_i] = (uint8_t)(pixel_color.b * 255);
	sample_buffer[sample_a_i] = (uint8_t)(pixel_color.a * 255);
}

// fill samples in the entire pixel specified by pixel coordinates
void SoftwareRendererImp::fill_pixel(int x, int y, const Color &color) {
 	// check bounds
	if (x < 0 || x >= width) return;
  if (y < 0 || y >= height) return;

  for (int db = 0; db < sample_rate * sample_rate; db++) {
    fill_sample(x, y, db, color);
  }
}

void SoftwareRendererImp::draw_svg( SVG& svg ) {

  // clear sample_buffer before drawing
  memset(this->sample_buffer, 255, 
    4 * width * height * sample_rate * sample_rate);

  // set top level transformation
  transformation = canvas_to_screen;

  // canvas outline
  Vector2D a = transform(Vector2D(0, 0)); a.x--; a.y--;
  Vector2D b = transform(Vector2D(svg.width, 0)); b.x++; b.y--;
  Vector2D c = transform(Vector2D(0, svg.height)); c.x--; c.y++;
  Vector2D d = transform(Vector2D(svg.width, svg.height)); d.x++; d.y++;

  svg_bbox_top_left = Vector2D(a.x+1, a.y+1);
  svg_bbox_bottom_right = Vector2D(d.x-1, d.y-1);

  // draw all elements
  for (size_t i = 0; i < svg.elements.size(); ++i) {
    draw_element(svg.elements[i]);
  }

  // draw canvas outline
  rasterize_line(a.x, a.y, b.x, b.y, Color::Black);
  rasterize_line(a.x, a.y, c.x, c.y, Color::Black);
  rasterize_line(d.x, d.y, b.x, b.y, Color::Black);
  rasterize_line(d.x, d.y, c.x, c.y, Color::Black);

  // resolve and send to pixel buffer
  resolve();

}

void SoftwareRendererImp::set_sample_rate( size_t sample_rate ) {

  // Task 2: 
  // You may want to modify this for supersampling support

  // if sample rate equal, no need to reset
  if (this->sample_rate == sample_rate) return;

  // otherwise, free current sample_buffer
  delete[] this->sample_buffer;

  // set up new sample_buffer
  this->sample_buffer = new unsigned char[4 * width * height *
    sample_rate * sample_rate];
  // set all values to 255 (i.e. white rbg color)
  memset(this->sample_buffer, 255,
    4 * width * height * sample_rate * sample_rate);

  this->sample_rate = sample_rate;
}

void SoftwareRendererImp::set_pixel_buffer( unsigned char* pixel_buffer,
                                             size_t width, size_t height ) {

  // Task 2: 
  // You may want to modify this for supersampling support
  this->pixel_buffer = pixel_buffer;
  // if sample_buffer not NULL (not first run) free previous sample_buffer
  if (this->sample_buffer) {
    delete[] this->sample_buffer;
  }
  // set up new sample_buffer
  this->sample_buffer = new unsigned char[4 * width * height *
    this->sample_rate * this->sample_rate];
  // set all values to 255 (i.e. white rbg color)
  memset(this->sample_buffer, 255,
    4 * width * height * this->sample_rate * this->sample_rate);
  
  this->width = width;
  this->height = height;

}

void SoftwareRendererImp::draw_element( SVGElement* element ) {

	// Task 3 (part 1):
	// Modify this to implement the transformation stack

  Matrix3x3 original = transformation;
  Matrix3x3 result = original * element->transform;
  transformation = result;

	switch (element->type) {
	case POINT:
		draw_point(static_cast<Point&>(*element));
		break;
	case LINE:
		draw_line(static_cast<Line&>(*element));
		break;
	case POLYLINE:
		draw_polyline(static_cast<Polyline&>(*element));
		break;
	case RECT:
		draw_rect(static_cast<Rect&>(*element));
		break;
	case POLYGON:
		draw_polygon(static_cast<Polygon&>(*element));
		break;
	case ELLIPSE:
		draw_ellipse(static_cast<Ellipse&>(*element));
		break;
	case IMAGE:
		draw_image(static_cast<Image&>(*element));
		break;
	case GROUP:
		draw_group(static_cast<Group&>(*element));
		break;
	default:
		break;
	}

  transformation = original;
}


// Primitive Drawing //

void SoftwareRendererImp::draw_point( Point& point ) {

  Vector2D p = transform(point.position);
  rasterize_point( p.x, p.y, point.style.fillColor );

}

void SoftwareRendererImp::draw_line( Line& line ) { 

  Vector2D p0 = transform(line.from);
  Vector2D p1 = transform(line.to);
  rasterize_line( p0.x, p0.y, p1.x, p1.y, line.style.strokeColor );

}

void SoftwareRendererImp::draw_polyline( Polyline& polyline ) {

  Color c = polyline.style.strokeColor;

  if( c.a != 0 ) {
    int nPoints = polyline.points.size();
    for( int i = 0; i < nPoints - 1; i++ ) {
      Vector2D p0 = transform(polyline.points[(i+0) % nPoints]);
      Vector2D p1 = transform(polyline.points[(i+1) % nPoints]);
      rasterize_line( p0.x, p0.y, p1.x, p1.y, c );
    }
  }
}

void SoftwareRendererImp::draw_rect( Rect& rect ) {

  Color c;
  
  // draw as two triangles
  float x = rect.position.x;
  float y = rect.position.y;
  float w = rect.dimension.x;
  float h = rect.dimension.y;

  Vector2D p0 = transform(Vector2D(   x   ,   y   ));
  Vector2D p1 = transform(Vector2D( x + w ,   y   ));
  Vector2D p2 = transform(Vector2D(   x   , y + h ));
  Vector2D p3 = transform(Vector2D( x + w , y + h ));
  
  // draw fill
  c = rect.style.fillColor;
  if (c.a != 0 ) {
    rasterize_triangle( p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, c );
    rasterize_triangle( p2.x, p2.y, p1.x, p1.y, p3.x, p3.y, c );
  }

  // draw outline
  c = rect.style.strokeColor;
  if( c.a != 0 ) {
    rasterize_line( p0.x, p0.y, p1.x, p1.y, c );
    rasterize_line( p1.x, p1.y, p3.x, p3.y, c );
    rasterize_line( p3.x, p3.y, p2.x, p2.y, c );
    rasterize_line( p2.x, p2.y, p0.x, p0.y, c );
  }

}

void SoftwareRendererImp::draw_polygon( Polygon& polygon ) {

  Color c;

  // draw fill
  c = polygon.style.fillColor;
  if( c.a != 0 ) {

    // triangulate
    vector<Vector2D> triangles;
    triangulate( polygon, triangles );

    // draw as triangles
    for (size_t i = 0; i < triangles.size(); i += 3) {
      Vector2D p0 = transform(triangles[i + 0]);
      Vector2D p1 = transform(triangles[i + 1]);
      Vector2D p2 = transform(triangles[i + 2]);
      rasterize_triangle( p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, c );
    }
  }

  // draw outline
  c = polygon.style.strokeColor;
  if( c.a != 0 ) {
    int nPoints = polygon.points.size();
    for( int i = 0; i < nPoints; i++ ) {
      Vector2D p0 = transform(polygon.points[(i+0) % nPoints]);
      Vector2D p1 = transform(polygon.points[(i+1) % nPoints]);
      rasterize_line( p0.x, p0.y, p1.x, p1.y, c );
    }
  }
}

void SoftwareRendererImp::draw_ellipse( Ellipse& ellipse ) {

  // Advanced Task
  // Implement ellipse rasterization

}

void SoftwareRendererImp::draw_image( Image& image ) {

  // Advanced Task
  // Render image element with rotation

  Vector2D p0 = transform(image.position);
  Vector2D p1 = transform(image.position + image.dimension);

  rasterize_image( p0.x, p0.y, p1.x, p1.y, image.tex );
}

void SoftwareRendererImp::draw_group( Group& group ) {

  for ( size_t i = 0; i < group.elements.size(); ++i ) {
    draw_element(group.elements[i]);
  }

}

// Rasterization //

// The input arguments in the rasterization functions 
// below are all defined in screen space coordinates

void SoftwareRendererImp::rasterize_point( float x, float y, Color color ) {

  // fill in the nearest pixel
  int sx = (int)floor(x);
  int sy = (int)floor(y);

  // check bounds
  if (sx < 0 || sx >= width) return;
  if (sy < 0 || sy >= height) return;

  fill_pixel(sx, sy, color);
}

void SoftwareRendererImp::rasterize_line( float x0, float y0,
                                          float x1, float y1,
                                          Color color) {
  // Task 0: 
  // Implement Bresenham's algorithm (delete the line below and implement your own)
  ref->rasterize_line_helper(x0, y0, x1, y1, width, height, color, this);

  // STUDENT IMPLEMENTATION
  // Determine vertical rasterization
  // if (x0 == x1) {
  //   // Fill vertical line from top to bottom (start at higher spot)
  //   float start_y = (y0 <= y1) ? y0: y1;
  //   float end_y = (y0 <= y1) ? y1: y0;

  //   for (float y = start_y; y <= end_y; y++) {
  //     rasterize_point(x0, y, color);
  //   }
  //   return;
  // }

  // // Determine horizontal rasterization
  // if (y0 == y1) {
  //   // Fill horizontal line from left to right
  //   float start_x = (x0 <= x1) ? x0: x1;
  //   float end_x = (x0 <= x1) ? x1: x0;

  //   for (float x = start_x; x <= end_x; x++) {
  //     rasterize_point(x, y0, color);
  //   }
  //   return;
  // }

  // // For all other lines, follow Bresham's algorithm
  // // Find differences in x, y, and overall slope
  // float dx = x1 - x0;
  // float dy = y1 - y0;
  // float slope = dy / dx;

  // // Use absolute values to generalize slope needs
  // // Helps handle negative, positive, combo, etc.
  // float abs_dx = abs(dx);
  // float abs_dy = abs(dy);

  // // Calculate slope error intervals
  // float slope_error_x = 2 * abs_dy - abs_dx;
  // float slope_error_y = 2 * abs_dx - abs_dy;

  // // Line has a less steep slope (more change along X)
  // if (abs_dy <= abs_dx) {
  //   // Decide which way to follow the line
  //   float start_x = (x0 <= x1) ? x0: x1;
  //   float end_x = (x0 <= x1) ? x1: x0;
  //   float curr_y = (x0 <= x1) ? y0: y1;

  //   // Render starting point
  //   rasterize_point(start_x, curr_y, color);

  //   // Render the rest of the line
  //   for (float x = start_x; x < end_x; x++) {
  //     if (slope_error_x < 0) {
  //       slope_error_x = slope_error_x + 2 * abs_dy;
  //     } else {
  //       // Positive
  //       if (slope > 0) {
  //         curr_y = curr_y + 1;
  //       } 
  //       // Negative
  //       else {
  //         curr_y = curr_y - 1;
  //       }
  //       slope_error_x = slope_error_x + 2 * (abs_dy - abs_dx);
  //     }
  //     rasterize_point(x, curr_y, color);
  //   }
  // } 
  // // Line has a more steep slope (more change along Y)
  // else {
  //   // Decide which way to follow the line
  //   float start_y = (y0 <= y1) ? y0: y1;
  //   float end_y = (y0 <= y1) ? y1: y0;
  //   float curr_x = (y0 <= y1) ? x0: x1;
    
  //   // Render starting point
  //   rasterize_point(curr_x, start_y, color);

  //   // Render the rest of the line
  //   for (float y = start_y; y < end_y; y++) {
  //     if (slope_error_y < 0) {
  //       slope_error_y = slope_error_y + 2 * abs_dx;
  //     } else {
  //       // Positive
  //       if (slope > 0) {
  //         curr_x = curr_x + 1;
  //       } 
  //       // Negative
  //       else {
  //         curr_x = curr_x - 1;
  //       }
  //       slope_error_y = slope_error_y + 2 * (abs_dx - abs_dy);
  //     }
  //     rasterize_point(curr_x, y, color);
  //   }
  // }
}

void SoftwareRendererImp::rasterize_triangle( float x0, float y0,
                                              float x1, float y1,
                                              float x2, float y2,
                                              Color color ) {
  // Task 1: 
  // Implement triangle rasterization
  // Determine box edges to iterate through
  float start_x = min({x0, x1, x2});
  float start_y = min({y0, y1, y2});
  float end_x = max({x0, x1, x2});
  float end_y = max({y0, y1, y2});

  // Set up legs connecting all of the edges
  Vector2D leg0(x1 - x0, y1 - y0);
  Vector2D leg1(x2 - x1, y2 - y1);
  Vector2D leg2(x0 - x2, y0 - y2);

  // Vectors for checking direction of arrows (clockwise or counter-clockwise)
  Vector2D source1(x1 - x0, y1 - y0);
  Vector2D source2(x2 - x0, y2 - y0);

  // Find normals that point inwards from each edge
  Vector2D legN0(-leg0.y, leg0.x);
  Vector2D legN1(-leg1.y, leg1.x);
  Vector2D legN2(-leg2.y, leg2.x);

  // Triangle points are passed in counter-clockwise
  if (cross(source1, source2) > 0) {
    // Find normals that point outwards from each edge
    legN0 = Vector2D(leg0.y, -leg0.x);
    legN1 = Vector2D(leg1.y, -leg1.x);
    legN2 = Vector2D(leg2.y, -leg2.x);
  }

  float const db = 1.0 / sample_rate;
  float const offset = db / 2.0;
  // Iterate through all points in designated area
  for (float y = start_y; y <= end_y; y++) {
    for (float x = start_x; x <= end_x; x++) {
      // retrieve values inside of loc(x, y)
      for (float by = 0; by < sample_rate; by++) {
        for (float bx = 0; bx < sample_rate; bx++) {
          float x_adj = floor(x) + db * bx + offset;
          float y_adj = floor(y) + db * by + offset;

          // Find vectors from start of each edge towards the point
          Vector2D pt_vec0 = Vector2D(x_adj - x0, y_adj - y0);
          Vector2D pt_vec1 = Vector2D(x_adj - x1, y_adj - y1);
          Vector2D pt_vec2 = Vector2D(x_adj - x2, y_adj - y2);
          
          // Check if point is inside triangle
          // Convention: CCW, Inside when dot between N and inside edge is <= 0
          if (dot(pt_vec0, legN0) <= 0 && 
              dot(pt_vec1, legN1) <= 0 && 
              dot(pt_vec2, legN2) <= 0) {
            fill_sample((int)floor(x), (int)floor(y), bx + by * sample_rate, color);
          }
        }
      }
    }
  }
  
  // Advanced Task
  // Implementing Triangle Edge Rules

}

void SoftwareRendererImp::rasterize_image( float x0, float y0,
                                           float x1, float y1,
                                           Texture& tex ) {
  // Task 4: 
  // Implement image rasterization
  //CS248::Sampler2DImp sampler(NEAREST);
  CS248::Sampler2DImp sampler(BILINEAR);
  //sampler.generate_mips(tex, 0);

  // Loop over all pixels in the valid range
  // Make sure to account for rounding, split pixels up as needed
  // Select the appropriate approximated texture
  float const db = 1 / sample_rate;
  float const offset = sample_rate / 2;
  for (float y = y0; y < y1; y++) {
    for (float x = x0; x < x1; x++) {
      // retrieve values inside of loc(x, y)
      for (float by = 0; by < sample_rate; by++) {
        for (float bx = 0; bx < sample_rate; bx++) {
          float x_adj = floor(x) + db * bx + offset;
          float y_adj = floor(y) + db * by + offset;
          float u = (x_adj - x0) / (x1 - x0);
          float v = (y_adj - y0) / (y1 - y0);
          Color color = sampler.sample_bilinear(tex, u, v);
          fill_sample((int)floor(x_adj), (int)floor(y_adj), bx + by * sample_rate, color);
        }
      }
    }
  }
}

// resolve samples to pixel buffer
void SoftwareRendererImp::resolve( void ) {

  // Task 2: 
  // Implement supersampling
  // You may also need to modify other functions marked with "Task 2".

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      int sample_r_sum = 0;
      int sample_g_sum = 0;
      int sample_b_sum = 0;
      int sample_a_sum = 0;
      for (int b = 0; b < sample_rate * sample_rate; b++) {
        int const sample_r_i = 4 * (x + y * width) + (4 * width * height * b);
        int const sample_g_i = 4 * (x + y * width) + (4 * width * height * b) + 1;
        int const sample_b_i = 4 * (x + y * width) + (4 * width * height * b) + 2;
        int const sample_a_i = 4 * (x + y * width) + (4 * width * height * b) + 3;

        sample_r_sum += sample_buffer[sample_r_i];
        sample_g_sum += sample_buffer[sample_g_i];
        sample_b_sum += sample_buffer[sample_b_i];
        sample_a_sum += sample_buffer[sample_a_i];
      }
      uint8_t avg_r = (uint8_t)(sample_r_sum / (sample_rate * sample_rate));
      uint8_t avg_g = (uint8_t)(sample_g_sum / (sample_rate * sample_rate));
      uint8_t avg_b = (uint8_t)(sample_b_sum / (sample_rate * sample_rate));
      uint8_t avg_a = (uint8_t)(sample_a_sum / (sample_rate * sample_rate));

      // Add average of rbga sums to pixel_buffer
      pixel_buffer[4 * (x + y * width)] = avg_r;
      pixel_buffer[4 * (x + y * width) + 1] = avg_g;
      pixel_buffer[4 * (x + y * width) + 2] = avg_b;
      pixel_buffer[4 * (x + y * width) + 3] = avg_a;
    }
  }
  return;

}

Color SoftwareRendererImp::alpha_blending(Color pixel_color, Color color)
{
  // Task 5
  // Implement alpha compositing
  float element_alpha = color.a;
  float element_red = color.r * element_alpha;
  float element_green = color.g * element_alpha;
  float element_blue = color.b * element_alpha;

  float canvas_alpha = pixel_color.a;
  float canvas_red = pixel_color.r;
  float canvas_green = pixel_color.g;
  float canvas_blue = pixel_color.b;

  float alpha_red = (1 - element_alpha) * canvas_red +  element_red;
  float alpha_green = (1 - element_alpha) * canvas_green + element_green;
  float alpha_blue = (1 - element_alpha) * canvas_blue + element_blue;
  float alpha_a = 1 - (1 - element_alpha) * (1 - canvas_alpha);

  return Color(alpha_red, alpha_green, alpha_blue, alpha_a);
}


} // namespace CS248
