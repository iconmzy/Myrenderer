

//homework extend texture 

#ifndef __MODEL_H__
#define __MODEL_H__

#include <vector>
#include "geometry.h"
#include "tgaimage.h"

class Model {
private:

public:

	std::vector<Vec3f> verts_; // ������еĶ�������
	std::vector<std::vector<Vec3i> > faces_; // vertex/normal/texcoord������
	std::vector<Vec2f> uv_;//������е���������
	std::vector<Vec3f> norms_;//����
	TGAImage diffusemap_;//��Ŷ�ȡ����ͼ
	void load_texture(std::string filename, const char* suffix, TGAImage& img);
	Model(const char* filename);
	~Model();
	int nverts(); //��������
	int nfaces(); // ����
	Vec3f vert(int i); //��ȡ����Ϊi�Ķ���
	Vec2i uv(int iface, int nvert);//��ȡ����Ϊiface�����е�nvert����uv
	TGAColor diffuse(Vec2i uv); //����������ͼ��ȡ��uv��Ӧ����ɫ
	std::vector<int> face(int idx); //����������ȡ�����ж�������
};

#endif //__MODEL_H__