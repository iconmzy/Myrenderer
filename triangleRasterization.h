#include "tgaimage.h"


void triangle(Model* model,int iface,Vec3f* pts, float *zbuffer,TGAImage& image, TGAColor color);


//void triangle(Model* model, int iface, Vec3f* tri, float* zbuffer, TGAImage& image, float intensity);

Vec3f barycentric_3d(Vec2i* pts, Vec3f P);
//Vec3f barycentric_2d(Vec2i* pts, Vec2i P);

void triangle_without_zbuffer(Vec2i* tri, TGAImage& image, TGAColor color);