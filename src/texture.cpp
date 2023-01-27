#include "texture.h"
#include "color.h"

#include <assert.h>
#include <iostream>
#include <algorithm>

using namespace std;

namespace CS248 {

inline void uint8_to_float( float dst[4], unsigned char* src ) {
  uint8_t* src_uint8 = (uint8_t *)src;
  dst[0] = src_uint8[0] / 255.f;
  dst[1] = src_uint8[1] / 255.f;
  dst[2] = src_uint8[2] / 255.f;
  dst[3] = src_uint8[3] / 255.f;
}

inline void float_to_uint8( unsigned char* dst, float src[4] ) {
  uint8_t* dst_uint8 = (uint8_t *)dst;
  dst_uint8[0] = (uint8_t) ( 255.f * max( 0.0f, min( 1.0f, src[0])));
  dst_uint8[1] = (uint8_t) ( 255.f * max( 0.0f, min( 1.0f, src[1])));
  dst_uint8[2] = (uint8_t) ( 255.f * max( 0.0f, min( 1.0f, src[2])));
  dst_uint8[3] = (uint8_t) ( 255.f * max( 0.0f, min( 1.0f, src[3])));
}

void Sampler2DImp::generate_mips(Texture& tex, int startLevel) {

  // NOTE: 
  // This starter code allocates the mip levels and generates a level 
  // map by filling each level with placeholder data in the form of a 
  // color that differs from its neighbours'. You should instead fill
  // with the correct data!

  // Advanced Task
  // Implement mipmap for trilinear filtering

  // check start level
  if ( startLevel >= tex.mipmap.size() ) {
    std::cerr << "Invalid start level"; 
  }

  // allocate sublevels
  int baseWidth  = tex.mipmap[startLevel].width;
  int baseHeight = tex.mipmap[startLevel].height;
  int numSubLevels = (int)(log2f( (float)max(baseWidth, baseHeight)));

  numSubLevels = min(numSubLevels, kMaxMipLevels - startLevel - 1);
  tex.mipmap.resize(startLevel + numSubLevels + 1);

  int width  = baseWidth;
  int height = baseHeight;
  for (int i = 1; i <= numSubLevels; i++) {

    MipLevel& level = tex.mipmap[startLevel + i];

    // handle odd size texture by rounding down
    width  = max( 1, width  / 2); assert(width  > 0);
    height = max( 1, height / 2); assert(height > 0);

    level.width = width;
    level.height = height;
    level.texels = vector<unsigned char>(4 * width * height);

  }

  // fill all 0 sub levels with interchanging colors
  Color colors[3] = { Color(1,0,0,1), Color(0,1,0,1), Color(0,0,1,1) };
  for(size_t i = 1; i < tex.mipmap.size(); ++i) {

    Color c = colors[i % 3];
    MipLevel& mip = tex.mipmap[i];

    for(size_t i = 0; i < 4 * mip.width * mip.height; i += 4) {
      float_to_uint8( &mip.texels[i], &c.r );
    }
  }

}

Color Sampler2DImp::sample_nearest(Texture& tex, 
                                   float u, float v, 
                                   int level) {
  
  // Task 4: Implement nearest neighbour interpolation
  if (level < 0 || level > 14) return Color(1, 0, 1, 1);
  
  size_t level_width = tex.mipmap[level].width;
  size_t level_height = tex.mipmap[level].height;

  // Find adjusted vector for loc in texture map
  int u_near = u * level_width;
  int v_near = v * level_height;
  
  // Access color values inside of map that correlate to nearest pixel
  uint8_t sample_r = tex.mipmap[level].texels[4 * (u_near + v_near * level_width)];
  uint8_t sample_g = tex.mipmap[level].texels[4 * (u_near + v_near * level_width) + 1];
  uint8_t sample_b = tex.mipmap[level].texels[4 * (u_near + v_near * level_width) + 2];
  uint8_t sample_a = tex.mipmap[level].texels[4 * (u_near + v_near * level_width) + 3];
  return Color(sample_r, sample_g, sample_b, sample_a);
}

Color Sampler2DImp::sample_bilinear(Texture& tex, 
                                    float u, float v, 
                                    int level) {
  
  // Task 4: Implement bilinear filtering
  if (level < 0 || level > 14) return Color(1, 0, 1, 1);
  
  size_t level_width = tex.mipmap[level].width;
  size_t level_height = tex.mipmap[level].height;

  // Find adjusted vector for loc in texture map
  float u_exact = u * level_width;
  float v_exact = v * level_height;

  // These points will represent the four surrounding our exact point
  float pixel_left_top_x, pixel_left_top_y,
        pixel_left_bot_x, pixel_left_bot_y,
        pixel_right_top_x, pixel_right_top_y, 
        pixel_right_bot_x, pixel_right_bot_y;

  // Truncate to find dividor between the pixels we care about
  int u_trunc = (int) u_exact;
  int v_trunc = (int) v_exact;
  
  // Use difference to divide if we should grab pixels
  // from left/right or up/down
  float u_offset = u_exact - u_trunc;
  float v_offset = v_exact - v_trunc;

  if (u_offset >= 0.5) {
    pixel_left_top_x = u_trunc + 0.5;
    pixel_left_bot_x = u_trunc + 0.5;
    pixel_right_top_x = u_trunc + 1.5;
    pixel_right_bot_x = u_trunc + 1.5;
  } else {
    pixel_left_top_x = u_trunc - 0.5;
    pixel_left_bot_x = u_trunc - 0.5;
    pixel_right_top_x = u_trunc + 0.5;
    pixel_right_bot_x = u_trunc + 0.5;
  }

  if (v_offset >= 0.5) {
    pixel_left_top_y = v_trunc + 0.5;
    pixel_right_top_y = v_trunc + 0.5;
    pixel_left_bot_y = v_trunc + 1.5;
    pixel_right_bot_y = v_trunc + 1.5;
  } else {
    pixel_left_top_y = v_trunc - 0.5;
    pixel_right_top_y = v_trunc - 0.5;
    pixel_left_bot_y = v_trunc + 0.5;
    pixel_right_bot_y = v_trunc + 0.5;
  }

  // Find first interpolation (between bottom two points in x)
  // Values must be pulled from one-dimensional vector that contains all texture
  float left_mid_dist = u_exact - pixel_left_bot_x;
  float mid_right_dist = pixel_right_bot_x - u_exact;
  float bottom_interp_red = left_mid_dist * tex.mipmap[level].texels[4 * ((int)pixel_left_bot_x + (int)pixel_left_bot_y * level_width)]
                          + mid_right_dist * tex.mipmap[level].texels[4 * ((int)pixel_right_bot_x + (int)pixel_right_bot_y * level_width)];
  float bottom_interp_green = left_mid_dist * tex.mipmap[level].texels[4 * ((int)pixel_left_bot_x + (int)pixel_left_bot_y * level_width) + 1]
                          + mid_right_dist * tex.mipmap[level].texels[4 * ((int)pixel_right_bot_x + (int)pixel_right_bot_y * level_width) + 1];
  float bottom_interp_blue = left_mid_dist * tex.mipmap[level].texels[4 * ((int)pixel_left_bot_x + (int)pixel_left_bot_y * level_width) + 2]
                          + mid_right_dist * tex.mipmap[level].texels[4 * ((int)pixel_right_bot_x + (int)pixel_right_bot_y * level_width) + 2];
  float bottom_interp_a = left_mid_dist * tex.mipmap[level].texels[4 * ((int)pixel_left_bot_x + (int)pixel_left_bot_y * level_width) + 3]
                          + mid_right_dist * tex.mipmap[level].texels[4 * ((int)pixel_right_bot_x + (int)pixel_right_bot_y * level_width) + 3];

  // Find second interpolation (between top two points in x)
    // Values must be pulled from one-dimensional vector that contains all texture
  float top_interp_red = left_mid_dist * tex.mipmap[level].texels[4 * ((int)pixel_left_top_x + (int)pixel_left_top_y * level_width)]
                          + mid_right_dist * tex.mipmap[level].texels[4 * ((int)pixel_right_top_x + (int)pixel_right_top_y * level_width)];
  float top_interp_green = left_mid_dist * tex.mipmap[level].texels[4 * ((int)pixel_left_top_x + (int)pixel_left_top_y * level_width) + 1]
                          + mid_right_dist * tex.mipmap[level].texels[4 * ((int)pixel_right_top_x + (int)pixel_right_top_y * level_width) + 1];
  float top_interp_blue = left_mid_dist * tex.mipmap[level].texels[4 * ((int)pixel_left_top_x + (int)pixel_left_top_y * level_width) + 2]
                          + mid_right_dist * tex.mipmap[level].texels[4 * ((int)pixel_right_top_x + (int)pixel_right_top_y * level_width) + 2];
  float top_interp_a = left_mid_dist * tex.mipmap[level].texels[4 * ((int)pixel_left_top_x + (int)pixel_left_top_y * level_width) + 3]
                          + mid_right_dist * tex.mipmap[level].texels[4 * ((int)pixel_right_top_x + (int)pixel_right_top_y * level_width) + 3];

  // Find third interpolation (between the two averaged values in x)
  float top_mid_dist = v_exact - pixel_left_top_y;
  float mid_bot_dist = pixel_left_bot_y - v_exact;

  float red = top_mid_dist * top_interp_red + mid_bot_dist * bottom_interp_red;
  float green = top_mid_dist * top_interp_green + mid_bot_dist * bottom_interp_green;
  float blue = top_mid_dist * top_interp_blue + mid_bot_dist * bottom_interp_blue;
  float a = top_mid_dist * top_interp_a + mid_bot_dist * bottom_interp_a;

  // Divide by 255 to place value within valid color range
  return Color(red / 255, green / 255, blue / 255, a / 255);
}

Color Sampler2DImp::sample_trilinear(Texture& tex, 
                                     float u, float v, 
                                     float u_scale, float v_scale) {

  // Advanced Task
  // Implement trilinear filtering

  // return magenta for invalid level
  return Color(1,0,1,1);

}

} // namespace CS248
