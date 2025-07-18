#include "tgaimage.h"
#include <iostream>;
using namespace std;


// 定义常用颜色常量（RGBA格式）
// 格式: TGAColor(Red, Green, Blue, Alpha)
// - RGB分量范围: 0~255 （0=无强度，255=最大强度）
// - Alpha范围: 0=完全透明，255=完全不透明
const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);
const TGAColor pink = TGAColor(255, 192, 203, 255);




void drawLineBresenham(int X0, int Y0, int X1, int Y1, TGAImage& image, TGAColor color) {
	//当斜率k＞1，每个x步进1，y会步进1+，画出来就是离散的。
	bool angle45Plus = false;
	if (abs(X0 - X1) < abs(Y0 - Y1)) {
		swap(X0, Y0);
		swap(Y1, X1);
		angle45Plus = true;
	}
	//let X begin as small one
	if (X0 > X1) {
		swap(X0, X1);
		swap(Y0, Y1);
	}


/*
*	一步一步算，会有浮点运算
	for (int currentX = X0; currentX < X1; currentX++){
		float stepRate = (currentX - X0) / dX;
		//用比例来算步进就无所谓k的正负了
		float currentY = Y0 + stepRate*dY;
		if (angle45Plus) {
			//之前调转了斜率，真正要画的点其实是
			image.set(currentY, currentX, pink);
		}
		else {
			image.set(currentX, currentY, pink);
		}
	}
*/

	//LineBresenham算法
	//经过前面的斜率倒置，已经把|k|限制在0-1的范围
	/*
		定义判别式 pi = 2dy - dx 的推导
		直线斜率 k = dy/dx ，表示 x 每增加 1 时，y 的理论增量。
		当前点假设是(xi, yi)，下一个候选点
		正右方：(xi+1, yi)
		右上方：(xi+1, yi+1)，计算这两个候选点和理想y值k⋅(xi+1)的距离

		正右方误差d1 = ​dy/dx (不变和增量之间的差)

		右上方误差d2 = 1- dy/dx (往上走一格和理论y之间的垂直差值)

		显然  dx (d1-d2) = 2dy - dx
		非常直观的，若pi  < 0 ,那么往正右边渲染误差更小，反之右上方更小

		那么设误差e是下一步(x+1) 处理想 y 值与当前 y 的差
		e = k(X+1)+b−Y
		e的两边乘以 2dx 会得到
		p =2dx⋅e=2dy(X+1)−2dx⋅Y
		
		（1）如果下一步去到正右方的点，新的误差​
		e′=k(x+2)+b−y，对应的p的更新为

		p′ = 2dx⋅e ′=2dy(x+2)−2dx⋅y = p+2dy
		（2）如果下一步去到右上方的点，新的误差
		e′=k(x+2)+b−（y+1），对应的p的更新为

		p′ = 2dx⋅e ′=2dy(x+2)−2dx⋅（y+1） = p+2dy-2dx
		非常地amazing	
	*/

	/*
	如果完全按照上面推导的设计代码，直观的形式为
		int dX = X1 - X0;
	int dY = Y1 - Y0;
	int p = 2 * dY - dX;
	int currentY = Y0;
	int y_step = (Y1 > Y0) ? 1 : -1;
	for (int currentX = X0; currentX < X1; ++currentX) {
		if (angle45Plus) {
			image.set(currentY, currentX, pink);
		}else {
			image.set(currentX, currentY, pink);
		}

		if (p >= 0){
			currentY += y_step;
			p += 2 * (dY - dX);
		}else {
			p += 2 + dY;
		}
	}
	*/

	//稍微简化一点的写法，还可以避免负数计算
	int dX = X1 - X0;
	int dY = Y1 - Y0;
	//dy这个分量不一定需要减去
	int derror2 = 2 * abs(dY);
	int error2 = 0;
	int currentY = Y0;
	int y_step = (Y1 > Y0) ? 1 : -1;
	for (int currentX = X0; currentX < X1; ++currentX) {
		if (angle45Plus) {
			image.set(currentY, currentX, color);
		}
		else {
			image.set(currentX, currentY, color);
		}
		//无论选择正右边还是右上点都存在2+dY的分量
		error2 += derror2;
		//这里等价于p_i >= 0，仅在需要调整y（选择右上点）时减去2dX
		if (error2 > dX) {
			currentY += y_step;
			error2 -= 2 * dX;
		}
	}

}

