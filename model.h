

//homework extend texture 

#ifndef __MODEL_H__
#define __MODEL_H__

#include <vector>
#include "geometry.h"
#include "tgaimage.h"

class Model {
private:

public:

	std::vector<Vec3f> verts_; // 存放所有的顶点坐标
	std::vector<std::vector<Vec3i> > faces_; // vertex/normal/texcoord的索引
	std::vector<Vec2f> uv_;//存放所有的纹理坐标
	std::vector<Vec3f> norms_;//法线
	TGAImage diffusemap_;//存放读取的贴图
	void load_texture(std::string filename, const char* suffix, TGAImage& img);
	Model(const char* filename);
	~Model();
	int nverts(); //顶点总数
	int nfaces(); // 面数
	Vec3f vert(int i); //获取索引为i的顶点
	Vec2i uv(int iface, int nvert);//获取面数为iface，面中第nvert个的uv
	TGAColor diffuse(Vec2i uv); //从漫反射贴图中取出uv对应的颜色
	std::vector<int> face(int idx); //根据面索引取出所有顶点坐标
};

#endif //__MODEL_H__