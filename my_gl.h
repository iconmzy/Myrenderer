#ifndef MYGL_H
#define MYGL_H
#include "tgaimage.h"
#include "geometry.h"

extern Matrix ModelView;
extern Matrix ViewPort;
extern Matrix Projection;



const float depth = 1000.f;

void viewport(int x, int y, int w, int h);

void viewtrans(Vec3f eye, Vec3f center, Vec3f up);


void projection(float coeff);


struct IShader {
	virtual ~IShader();

	//= 0;���﷨��ǣ������Ǵ��麯��������override
	// ������ɫ����ɵĹ���
	//�任����
	//׼�����ݸ�Ƭ����ɫ����
	virtual Vec4f vertex(int iface, int nthvert) = 0;
	// Ƭ����ɫ����ɵĹ���
	//������ǰ���ص���ɫ
	//�Ƿ�Ҫ������ǰ����
	virtual bool fragment(Vec3f bar, TGAColor& color) = 0;
};

void triangle_box(Vec4f* pts, IShader& shader, TGAImage& image, TGAImage& zbuffer);

void triangle(Vec4f* pts, IShader& shader, TGAImage& image, float* zbuffer);

#endif