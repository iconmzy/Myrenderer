#ifndef MODEL_H
#define MODEL_H
#include <vector>
#include <string>
#include "geometry.h"
#include "tgaimage.h"
#include <unordered_map>
class Model {
private:
	enum TextureType { DIFFUSE, NORMAL, SPECULAR}; // 用枚举的形式方便扩展texture类型
	std::vector<Vec3f> verts_;
	std::vector<std::vector<Vec3i>> faces_; //Vec3i means vertex/uv/normal
	std::vector<Vec3f> norms_;
	std::vector<Vec2f> uv_;


	std::unordered_map<TextureType, TGAImage> textures_; // 纹理集合,替代下面的单项
	//TGAImage diffusemap_;
	//TGAImage normalmap_;
	//TGAImage specularmap_;
	//void load_texture(std::string filename, const char* suffix, TGAImage& img);
	void load_texture(std::string filename, TextureType suffix);
public:
	Model(const char* filename);
	~Model();
	int nverts();
	int nfaces();
	Vec2f uv(int iface, int nthvert);
	Vec3f vert(int iface, int nthvert);

	//返回模型原始顶点法线（从.obj文件直接读取）
	Vec3f normal(int iface, int nthvert);


	//从法线贴图中读取逐像素法线（RGB编码）
	Vec3f normal(Vec2f uv);

	//
	float specular(Vec2f uvf);
	std::vector<int> face(int idx);
	TGAColor diffuse(Vec2f uvf);
};

#endif
