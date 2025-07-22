#ifndef MYGL_H
#define MYGL_H
#include "tgaimage.h"
#include "geometry.h"

extern Matrix ModelView;
extern Matrix ViewPort;
extern Matrix Projection;

void viewport(int x, int y, int w, int h);

void viewtrans(Vec3f eye, Vec3f center, Vec3f up);


void projection(float coeff);

struct IShader {
	virtual ~IShader();

	//= 0;是语法标记，表明是纯虚函数，必须override
	virtual Vec4f vertex(int iface, int nthvert) = 0;
	virtual bool fragment(Vec3f bar, TGAColor& color) = 0;
};

void triangle_box(Vec4f* pts, IShader& shader, TGAImage& image, TGAImage& zbuffer);

#endif