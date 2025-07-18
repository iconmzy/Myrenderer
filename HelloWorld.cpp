#include "tgaimage.h"

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);

int drawpoint(int argc, char** argv) {
	TGAImage image(100, 100, TGAImage::RGB);
	image.set(52, 41, red);
	/*
	image.flip_vertically 使图像水平翻转
	许多数学和图形学计算（如光线追踪、3D 渲染）默认使用 左下角原点 的坐标系（与笛卡尔坐标系一致）。翻转图像可以保持代码逻辑与数学习惯一致。
	*/


	image.flip_vertically();
	image.write_tga_file("output.tga");
	return 0;
}