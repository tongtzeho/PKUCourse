// 我真诚地保证：
    
// 我自己独立地完成了整个程序从分析、设计到编码的所有工作。
// 如果在上述过程中，我遇到了什么困难而求教于人，那么，我将在程序实习报告中
// 详细地列举我所遇到的问题，以及别人给我的提示。

// 我的程序里中凡是引用到其他程序或文档之处，
// 例如教材、课堂笔记、网上的源代码以及其他参考书上的代码段,
// 我都已经在程序的注释里很清楚地注明了引用的出处。

// 我从未抄袭过别人的程序，也没有盗用别人的程序，
// 不管是修改式的抄袭还是原封不动的抄袭。

// 我编写这个程序，从来没有想过要去破坏或妨碍其他计算机系统的正常运转。

// 数据结构与算法实习之连连看（自动消解）
// 文件名：Lianliankan.cpp
// 作者：唐子豪
// 日期：2012年11月18日
// 编译环境：Microsoft Visual C++ 6.0
// 操作系统：Microsoft Windows XP
// 输入方式：键盘读入数据或文件读入数据皆可
// 输出方式：Graphics图形界面输出

#include<iostream>
#include<fstream>
#include<iomanip>
#include<cstring>
#include<ctime>
#include<vector>
#include<algorithm>
#include<graphics.h> // 需要先安装提交包内的EasyX库才能使用graphics.h
#include<windows.h>
#pragma comment(lib,"winmm.lib")

#define MAX_SIZE 22 // 数字矩阵行、列的最大值
#define MAX_WIDTH 20 // 列的最大值（不含最左和最右空出的列）
#define MAX_HEIGHT 12 // 行的最大值（不含最上和最下空出的行）
#define PATTERN_NUM 30 // 不同的图案的个数
#define SCREEN_WIDTH 120 // 屏幕宽度
#define SCREEN_HEIGHT 50 // 屏幕高度
#define WINDOW_WIDTH 119 // 窗口宽度
#define WINDOW_HEIGHT 30 // 窗口高度
#define MAX_MEM_SIZE 150
#define SLEEPTIME 240
#define MAX_SEARCH_TIME 13000

using namespace std;

// POS表示某一点，其中x代表行号，y代表列号
class POS
{
public:
	int x,y;
	POS(const int X=-1,const int Y=-1)
	{
		x=X;
		y=Y;
	}
};

// WAY表示点p1和点p2的一种消解方式，当t2为真时表示有两个拐角，分别用turn1和turn2表示，当t2为假而t1为真时表示有一个拐角，用turn1表示
class WAY
{
public:
	POS p1,p2,turn1,turn2;
	bool t1,t2;
	WAY(){};
	WAY(const int X1,const int Y1,const int X2,const int Y2,const bool T1=0,const bool T2=0)
	{
		p1.x=X1;
		p1.y=Y1;
		p2.x=X2;
		p2.y=Y2;
		t1=T1;
		t2=T2;
	}
	int distance() // 计算连接p1和p2的最短折线长度
	{
		if(p1.x>p2.x)
		{
			if(p1.y>p2.y)return p1.x+p1.y-p2.x-p2.y;
			else return p1.x+p2.y-p2.x-p1.y;
		}
		else
		{
			if(p1.y>p2.y)return p2.x+p1.y-p1.x-p2.y;
			else return p2.x+p2.y-p1.x-p1.y;
		}
	}
};

// MEMDATA记录递归时已经搜索过的数据
class MEMDATA
{
public:
	int pattern[MAX_SIZE][MAX_SIZE];
	int patternnum[PATTERN_NUM+1];
	int maxstep;
	MEMDATA(const int p[][MAX_SIZE],const int pn[],const int M)
	{
		int i,j;
		for(i=0;i<MAX_SIZE;i++)
			for(j=0;j<MAX_SIZE;j++)
				pattern[i][j]=p[i][j];
		for(i=1;i<=PATTERN_NUM;i++)
			patternnum[i]=pn[i];
		maxstep=M;
	}
};

static HMIDIOUT handle; // 音频输出句柄
static int pattern[MAX_SIZE][MAX_SIZE],weight[MAX_SIZE][MAX_SIZE]; // pattern为每个点的图案，weight为每个点的权值
static vector<WAY> vway,eway,answay; // vway记录当前的消解方法，eway记录从某点出发的消解路径，answay为使剩下图案最少的消解方法

// 以下为准备工作的函数
void init(); // 初始化界面、图案
bool input(int&,int[],int&,int&); // 选择输入模式、输入图案
int datacreate(int[],int,int); // 当选择自动生成图案时，自动生成一种图案方案

// 以下为计算部分的函数
void searchpos(int,int,int,int); // 从某一点出发，寻找能消解的点对
bool cmp(const WAY &w1,const WAY &w2); // 比较两种消解方式的权值
bool calculate(int,int,int,time_t); // 找能消解的点对并选择最佳方案消解

// 以下为输出的函数
void logoutput(int,int); // 将消解过程输出到log.txt中
void play(unsigned char); // 发出声音
void fillrectangle(int,int,int,int,COLORREF); // 画填充矩形
void drawline(int,int,int,int,COLORREF); // 画直线
void drawcircle(int,int,COLORREF); // 画填充圆
void turntoempty(int,int,int,int,COLORREF); // 将某块的颜色消掉
void drawdefaultimage(int,int,int); // 输出默认图案（当找不到图片文件时）
void graphicsoutput(int,int); // 图形界面输出（该函数在main函数中调用，再调用以上各函数）

void init()
{
	HANDLE hOut=GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTitle("连连看 Lianliankan");
	COORD size={SCREEN_WIDTH,SCREEN_HEIGHT};
	SetConsoleScreenBufferSize(hOut,size);
	SMALL_RECT rc={0,0,WINDOW_WIDTH,WINDOW_HEIGHT};
	SetConsoleWindowInfo(hOut,true,&rc);
	unsigned long result=midiOutOpen(&handle,0,0,0,CALLBACK_NULL);
	int i,j;
	for(i=0;i<MAX_SIZE;i++)
		for(j=0;j<MAX_SIZE;j++)
			pattern[i][j]=0;
	srand(unsigned(time(0)));
}

bool input(int &patternednum,int patternnum[],int &r,int &c)
{
	int i,j,mode;
	cout<<"请选择模式（输入1、2或3）：\n1：从键盘读入数据；  2：从文件读入数据；  3：自动生成数据\n";
	cin>>mode;
	if(mode<1||mode>3)
	{
		cout<<"出错！\n";
		return 0;
	}
	cout<<endl;
	if(mode==1||mode==2)
	{
		cout<<"数据样例：\n\n";
		cout<<"┌———————————————————┐\n";
		cout<<"︱ 8 12                                 ︱\n";
		cout<<"︱ 15 10  9 10  2  5 25 22 16  6  8 11  ︱\n";
		cout<<"︱ 23 21 21 20 21 25 14 23 23 15 16 19  ︱\n";
		cout<<"︱  1  9 26 20  3 15 22  8  7  1 13 11  ︱\n";
		cout<<"︱ 14  9  1 21  4  2 17  5 12 13 12  4  ︱\n";
		cout<<"︱ 19 25 26 22 19  8 17 11 25  6  6  4  ︱\n";
		cout<<"︱ 13 10 20  1  7 17 13  8  3 16  7  3  ︱\n";
		cout<<"︱ 17  7 22 19  5  4 26  2 23  2 20 10  ︱\n";
		cout<<"︱  9  5 14 12  3  6 15 14 12 16 11 26  ︱\n";
		cout<<"└———————————————————┘\n\n";
		cout<<"请确保输入的数据符合样例的规范！\n\n";
	}
	if(mode==1) // 从键盘读入行、列和各个点的图案
	{
		cout<<"输入行数r（2≤r≤12）和列数c（2≤c≤20）\n";
		cin>>r>>c;
		if(r<=1||c<=1||r>MAX_HEIGHT||c>MAX_WIDTH)
		{
			cout<<"输入数据出错！\n";
			return 0;
		}
		cout<<"请依次输入图案的数字矩阵（用1到30的整数表示图案，用0表示空，不能全为空）\n";
		for(i=1;i<=r;i++)
			for(j=1;j<=c;j++)
			{
				cin>>pattern[i][j];
				if(pattern[i][j]!=0)patternednum++;
				if(pattern[i][j]>PATTERN_NUM||pattern[i][j]<0)
				{
					cout<<"输入数据出错！\n";
					return 0;
				}
				else patternnum[pattern[i][j]]++;
			}
	}
	else if(mode==2) // 从指定的文件读入行、列和各个点的图案
	{
		cout<<"输入文件的路径和文件名\n";
		char filename[300];
		cin.get();
		cin.getline(filename,300,'\n');
		ifstream infile;
		infile.open(filename);
		if(!infile)
		{
			cout<<"文件不存在！出错！\n";
			return 0;
		}
		infile>>r>>c;
		if(r<=1||c<=1||r>MAX_HEIGHT||c>MAX_WIDTH)
		{
			cout<<"输入数据出错！\n";
			return 0;
		}
		for(i=1;i<=r;i++)
			for(j=1;j<=c;j++)
			{
				infile>>pattern[i][j];
				if(pattern[i][j]!=0)patternednum++;
				if(pattern[i][j]>PATTERN_NUM||pattern[i][j]<0)
				{
					cout<<"输入数据出错！\n";
					system("PAUSE");
					return 0;
				}
				else patternnum[pattern[i][j]]++;
			}
		infile.close();
	}
	else // 从键盘输入行、列数，自动生成图案
	{
		cout<<"输入行数r（2≤r≤12）和列数c（2≤c≤20）\n";
		cin>>r>>c;
		if(r<=1||c<=1||r>MAX_HEIGHT||c>MAX_WIDTH)
		{
			cout<<"输入数据出错！\n";
			return 0;
		}
		cout<<"\n正在生成数据，请稍候……\n";
		patternednum=datacreate(patternnum,r,c);
	}
	if(patternednum==0)
	{
		cout<<"\n全为空！出错！\n";
		return 0;
	}
	return 1;
}

int datacreate(int patternnum[],int r,int c)
{
	int i,j,k,a,b,s=r*c/2,temp[PATTERN_NUM+1]={0};
	bool patternused[PATTERN_NUM+1];
	for(i=1;i<=PATTERN_NUM;i++)
		patternused[i]=0;
	for(i=1;i<=PATTERN_NUM&&s;i++)
	{
		k=rand()%PATTERN_NUM+1;
		while(patternused[k])
			k=k%PATTERN_NUM+1;
		patternused[k]=1;
		patternnum[k]=rand()%s+1;
		while(patternnum[k]>(r*c/PATTERN_NUM+1)&&i!=PATTERN_NUM)
			patternnum[k]=rand()%s+1;
		s-=patternnum[k];
		patternnum[k]*=2;
		temp[k]=patternnum[k];
	}
	if(r*c%2) // 当行、列都为奇数时，“挖空”一点
	{
		a=rand()%r+1;
		b=rand()%c+1;
	}
	else a=b=0;
	for(i=1;i<=r;i++)
		for(j=1;j<=c;j++)
		{
			if(i==a&&j==b)
			{
				pattern[i][j]=0;
				continue;
			}
			k=rand()%PATTERN_NUM+1;
			while(!temp[k])
				k=rand()%PATTERN_NUM+1;
			temp[k]--;
			pattern[i][j]=k;
		}
	for(i=1;i<=r;i++)
		for(j=1;j<c;j++)
			if(pattern[i][j]==pattern[i][j+1]) // 尽量避免左右相邻两点的图案一样
			{
				k=rand()%r+1;
				s=rand()%c+1;
				swap(pattern[i][j+1],pattern[k][s]);
			}
	s=rand()%(r*c)+1;
	for(i=1;i<=s;i++) // 再随机打乱一些点
	{
		j=rand()%r+1;
		a=rand()%r+1;
		k=rand()%c+1;
		b=rand()%c+1;
		swap(pattern[j][k],pattern[a][b]);
	}
	return r*c-r*c%2;
}

void searchpos(const int x,const int y,const int r,const int c)
{
	bool repeat[MAX_SIZE][MAX_SIZE]={0};
	int i,j,tx,ty,x0,y0,w[4]={0};
	const int dxdy[4][2]={{1,0},{-1,0},{0,1},{0,-1}};
	vector<POS> v1,v2,turn;
	repeat[x][y]=1;
	weight[x][y]=0;
	for(i=0;i<4;i++) // 寻找从(x,y)点能直接（即不拐弯）到达的点，并记录在v1中
	{
		tx=x+dxdy[i][0];
		ty=y+dxdy[i][1];
		while(tx>=0&&ty>=0&&tx<=r+1&&ty<=c+1)
		{
			w[i]++;
			repeat[tx][ty]=1;
			if(pattern[tx][ty]!=0)
			{
				if(pattern[tx][ty]==pattern[x][y])
				{
					if(tx>x||(tx==x&&ty>y))eway.push_back(WAY(x,y,tx,ty));
				}
				break;
			}
			else v1.push_back(POS(tx,ty));
			tx+=dxdy[i][0];
			ty+=dxdy[i][1];
		}
		weight[x][y]+=w[i]*w[i];
	}
	for(i=0;i<v1.size();i++) // 寻找从(x,y)点拐一次弯到达的点，并记录在v2中
	{
		x0=v1[i].x;
		y0=v1[i].y;
		for(j=0;j<4;j++)
		{
			tx=x0+dxdy[j][0];
			ty=y0+dxdy[j][1];
			while(tx>=0&&ty>=0&&tx<=r+1&&ty<=c+1&&!repeat[tx][ty])
			{
				repeat[tx][ty]=1;
				if(pattern[tx][ty]!=0)
				{
					if(pattern[tx][ty]==pattern[x][y])
					{
						if(tx>x||(tx==x&&ty>y))
						{
							eway.push_back(WAY(x,y,tx,ty,1));
							eway.back().turn1=v1[i];
						}
					}
					break;
				}
				else
				{
					v2.push_back(POS(tx,ty));
					turn.push_back(v1[i]);
				}
				tx+=dxdy[j][0];
				ty+=dxdy[j][1];
			}
		}
	}
	for(i=0;i<v2.size();i++) // 寻找从(x,y)点拐两次弯到达的点
	{
		x0=v2[i].x;
		y0=v2[i].y;
		for(j=0;j<4;j++)
		{
			tx=x0+dxdy[j][0];
			ty=y0+dxdy[j][1];
			while(tx>0&&ty>0&&tx<=r&&ty<=c&&!repeat[tx][ty])
			{
				repeat[tx][ty]=1;
				if(pattern[tx][ty]!=0)
				{
					if(pattern[tx][ty]==pattern[x][y])
					{
						if(tx>x||(tx==x&&ty>y))
						{
							eway.push_back(WAY(x,y,tx,ty,1,1));
							eway.back().turn2=v2[i];
							eway.back().turn1=turn[i];
						}
					}
					break;
				}
				tx+=dxdy[j][0];
				ty+=dxdy[j][1];
			}
		}
	}
}

bool cmp(const WAY &w1,const WAY &w2)
{
	return weight[w1.p1.x][w1.p1.y]+weight[w1.p2.x][w1.p2.y]<weight[w2.p1.x][w2.p1.y]+weight[w2.p2.x][w2.p2.y];
}

bool calculate(const int left,const int r,const int c,const time_t starttime)
{
	if(left==0)return 1;
	else if(left==1)return 0;
	static vector<MEMDATA> mem1,mem2;
	int i,j,k,x1,y1,x2,y2,mempattern[MAX_SIZE][MAX_SIZE],memweight[MAX_SIZE][MAX_SIZE],patternnum[PATTERN_NUM+1]={0},possiblewaynum=vway.size();
	bool same=1;
	for(i=0;i<mem1.size();i++) // 当前图案在之前已经搜索过并证实不行时，返回
	{
		same=1;
		for(j=1;j<=r&&same;j++)
			for(k=1;k<=c&&same;k++)
				if(pattern[j][k]!=mem1[i].pattern[j][k])same=0;
		if(same)return 0;
	}
	for(i=0;i<mem2.size();i++) // 与当前图案类似的图案在之前已经搜索过并证实不行时，返回
	{
		same=1;
		for(j=1;j<=r&&same;j++)
			for(k=1;k<=c&&same;k++)
				if(pattern[j][k]!=mem2[i].pattern[j][k]&&pattern[j][k]>0&&mem2[i].patternnum[pattern[j][k]]>0)same=0;
		if(same)return 0;
	}
	eway.clear();
	vector<WAY> tmpway;
	for(i=1;i<=r;i++)
		for(j=1;j<=c;j++)
			if(pattern[i][j]!=0) // 寻找所有能消解的点对
			{
				searchpos(i,j,r,c);
				patternnum[pattern[i][j]]++;
			}
	for(i=1;i<=PATTERN_NUM;i++)
		possiblewaynum+=patternnum[i]/2;
	tmpway=eway;
	sort(tmpway.begin(),tmpway.end(),cmp); // 按权值排序（某点的权值与该点到上、下、左、右四个方向的最近点的距离正相关）
	for(i=0;i<=r+1;i++)
		for(j=0;j<=c+1;j++)
		{
			mempattern[i][j]=pattern[i][j];
			memweight[i][j]=weight[i][j];
			if(j<c&&i<r&&pattern[i][j]&&pattern[i][j+1]&&patternnum[pattern[i][j]]==2&&patternnum[pattern[i][j+1]]==2&&pattern[i][j]==pattern[i+1][j+1]&&pattern[i][j+1]==pattern[i+1][j])possiblewaynum-=2;
		}
	for(i=0;i<tmpway.size();i++) // 按权值从小到大，进行消解试验
	{
		if(possiblewaynum==answay.size()||clock()-starttime>MAX_SEARCH_TIME)return 0;
		vway.push_back(tmpway[i]);
		if(vway.size()>answay.size())answay=vway;
		x1=tmpway[i].p1.x;
		x2=tmpway[i].p2.x;
		y1=tmpway[i].p1.y;
		y2=tmpway[i].p2.y;
		pattern[x1][y1]=pattern[x2][y2]=0;
		if(calculate(left-2,r,c,starttime))return 1; // 当消解成功时，返回1
		else
		{
			for(j=0;j<=r+1;j++)
				for(k=0;k<=c+1;k++)
				{
					pattern[j][k]=mempattern[j][k];
					weight[j][k]=memweight[j][k];
				}
			vway.pop_back();
		}
	}
	if(mem1.size()<MAX_MEM_SIZE)mem1.push_back(MEMDATA(pattern,patternnum,answay.size()));
	if(mem2.size()<MAX_MEM_SIZE)mem2.push_back(MEMDATA(pattern,patternnum,answay.size()));
	return 0;
}

void logoutput(const int r,const int c)
{
	int i,j,patternednum=0;
	ofstream outfile;
	outfile.open("log.txt");
	outfile<<"r:"<<setw(3)<<r<<endl;
	outfile<<"c:"<<setw(3)<<c<<endl<<endl;
	outfile<<"patterns:"<<endl;
	for(i=1;i<=r;i++)
	{
		for(j=1;j<=c;j++)
		{
			outfile<<setw(4)<<pattern[i][j];
			patternednum+=!!pattern[i][j];
		}
		outfile<<endl;
	}
	outfile<<endl;
	outfile<<"______________________________________________________________________________________"<<endl<<endl<<endl;
	for(i=0;i<vway.size();i++)
	{
		outfile<<"Step"<<setw(4)<<i+1<<" : ";
		outfile<<"erase ("<<setw(2)<<vway[i].p1.x<<","<<setw(2)<<vway[i].p1.y<<") & ("<<setw(2)<<vway[i].p2.x<<","<<setw(2)<<vway[i].p2.y<<")";
		if(vway[i].t1)outfile<<"     turn1: ("<<setw(2)<<vway[i].turn1.x<<","<<setw(2)<<vway[i].turn1.y<<")  ";
		if(vway[i].t2)outfile<<"  turn2: ("<<setw(2)<<vway[i].turn2.x<<","<<setw(2)<<vway[i].turn2.y<<")  ";
		outfile<<endl;
	}
	if(vway.size()*2==patternednum)outfile<<endl<<"Mission Accomplished!!"<<endl;
	else outfile<<endl<<"No Solve!!!!"<<endl;
	outfile.close();
}

void play(const unsigned char c)
{
	if(c==0)return;
	else if(c==1)midiOutShortMsg(handle,0x007a3590);
	else if(c==2)midiOutShortMsg(handle,0x007a3790);
	else if(c==3)midiOutShortMsg(handle,0x007a3990);
	else if(c==4)midiOutShortMsg(handle,0x007a3b90);
	else if(c==5)midiOutShortMsg(handle,0x007a3C90);
	else if(c==6)midiOutShortMsg(handle,0x007a3E90);
	else if(c==7)midiOutShortMsg(handle,0x007a4090);
	else if(c==8)midiOutShortMsg(handle,0x007a4190);
	else if(c==9)midiOutShortMsg(handle,0x007a4390);
	else if(c==10)midiOutShortMsg(handle,0x007a4590);
	else if(c==11)midiOutShortMsg(handle,0x007a4790);
	else if(c==12)midiOutShortMsg(handle,0x007a4890);
	else if(c==13)midiOutShortMsg(handle,0x007a4a90);
	else if(c==14)midiOutShortMsg(handle,0x007a4c90);
	else if(c==15)midiOutShortMsg(handle,0x007a4d90);
	else if(c==16)midiOutShortMsg(handle,0x007a4f90);
}

//以下为图形界面输出相关的函数，若没有安装EasyX库，请将以下函数注释掉

void fillrectangle(int left,int top,int right,int bottom,COLORREF color)
{
	setcolor(color);
	setfillstyle(color);
	rectangle(left,top,right,bottom);
	int i,j;
	for(i=left+15;i<right;i+=50)
		for(j=top+15;j<bottom;j+=50)
			floodfill(i,j,color);
}

void drawline(int x1,int y1,int x2,int y2,COLORREF colorref)
{
	setlinestyle(PS_SOLID,NULL,5);
	setcolor(colorref);
	line(50*y1+25,50*x1+25,50*y2+25,50*x2+25);
}

void drawcircle(int x,int y,COLORREF colorref)
{
	setlinestyle(PS_SOLID,NULL,1);
	setcolor(colorref);
	circle(50*y+25,50*x+25,10);
	setfillstyle(colorref);
	floodfill(50*y+20,50*x+20,colorref);
}

void turntoempty(int x1,int y1,int x2,int y2,COLORREF colorref)
{
	int left,top,right,bottom;
	if(y1<y2)
	{
		left=y1*50;
		right=(y2+1)*50;
	}
	else
	{
		left=y2*50;
		right=(y1+1)*50;
	}
	if(x1<x2)
	{
		top=x1*50;
		bottom=(x2+1)*50;
	}
	else
	{
		top=x2*50;
		bottom=(x1+1)*50;
	}
	setlinestyle(PS_SOLID,NULL,1);
	fillrectangle(left,top,right,bottom,colorref);
}

void drawdefaultimage(const int x,const int y,const int p)
{
	COLORREF backgroundcolor;
	COLORREF foregroundcolor;
	if(p==1)backgroundcolor=RGB(0,0,0);
	else if(p==2)backgroundcolor=RGB(0,0,95);
	else if(p==3)backgroundcolor=RGB(0,0,191);
	else if(p==4)backgroundcolor=RGB(0,95,0);
	else if(p==5)backgroundcolor=RGB(0,95,95);
	else if(p==6)backgroundcolor=RGB(0,95,191);
	else if(p==7)backgroundcolor=RGB(0,191,0);
	else if(p==8)backgroundcolor=RGB(0,191,95);
	else if(p==9)backgroundcolor=RGB(0,191,191);
	else if(p==10)backgroundcolor=RGB(255,0,0);
	else if(p==11)backgroundcolor=RGB(255,95,95);
	else if(p==12)backgroundcolor=RGB(255,191,191);
	else if(p==13)backgroundcolor=RGB(95,0,0);
	else if(p==14)backgroundcolor=RGB(95,0,95);
	else if(p==15)backgroundcolor=RGB(95,0,191);
	else if(p==16)backgroundcolor=RGB(95,95,0);
	else if(p==17)backgroundcolor=RGB(95,95,95);
	else if(p==18)backgroundcolor=RGB(95,95,191);
	else if(p==19)backgroundcolor=RGB(95,191,0);
	else if(p==20)backgroundcolor=RGB(95,191,95);
	else if(p==21)backgroundcolor=RGB(95,191,191);
	else if(p==22)backgroundcolor=RGB(127,191,0);
	else if(p==23)backgroundcolor=RGB(127,95,95);
	else if(p==24)backgroundcolor=RGB(127,0,191);
	else if(p==25)backgroundcolor=RGB(191,0,0);
	else if(p==26)backgroundcolor=RGB(191,0,95);
	else if(p==27)backgroundcolor=RGB(191,0,191);
	else if(p==28)backgroundcolor=RGB(191,95,0);
	else if(p==29)backgroundcolor=RGB(191,95,95);
	else if(p==30)backgroundcolor=RGB(191,95,191);
	if(p==1)foregroundcolor=RGB(63,63,0);
	else if(p==2)foregroundcolor=RGB(63,63,191);
	else if(p==3)foregroundcolor=RGB(63,63,255);
	else if(p==4)foregroundcolor=RGB(63,191,0);
	else if(p==5)foregroundcolor=RGB(63,191,191);
	else if(p==6)foregroundcolor=RGB(63,191,255);
	else if(p==7)foregroundcolor=RGB(63,255,0);
	else if(p==8)foregroundcolor=RGB(63,255,191);
	else if(p==9)foregroundcolor=RGB(63,255,255);
	else if(p==10)foregroundcolor=RGB(63,127,0);
	else if(p==11)foregroundcolor=RGB(63,127,191);
	else if(p==12)foregroundcolor=RGB(63,127,255);
	else if(p==13)foregroundcolor=RGB(191,63,0);
	else if(p==14)foregroundcolor=RGB(191,63,191);
	else if(p==15)foregroundcolor=RGB(191,63,255);
	else if(p==16)foregroundcolor=RGB(191,191,0);
	else if(p==17)foregroundcolor=RGB(191,191,191);
	else if(p==18)foregroundcolor=RGB(191,191,255);
	else if(p==19)foregroundcolor=RGB(191,255,0);
	else if(p==20)foregroundcolor=RGB(191,255,191);
	else if(p==21)foregroundcolor=RGB(191,255,255);
	else if(p==22)foregroundcolor=RGB(191,127,0);
	else if(p==23)foregroundcolor=RGB(191,127,191);
	else if(p==24)foregroundcolor=RGB(191,127,255);
	else if(p==25)foregroundcolor=RGB(255,63,0);
	else if(p==26)foregroundcolor=RGB(255,63,191);
	else if(p==27)foregroundcolor=RGB(255,63,255);
	else if(p==28)foregroundcolor=RGB(255,191,0);
	else if(p==29)foregroundcolor=RGB(255,191,191);
	else if(p==30)foregroundcolor=RGB(255,191,255);
	fillrectangle(y*50,x*50,(y+1)*50,(x+1)*50,backgroundcolor);
	RECT rect={y*50,x*50,(y+1)*50,(x+1)*50};
	setfont(36,0,"n");
	setbkmode(TRANSPARENT);
	char output[2]=" ";
	if(p<10)output[0]=p+'0';
	else output[0]=p-10+'A';
	setcolor(foregroundcolor);
	drawtext(output,&rect,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
}

void graphicsoutput(const int r,const int c)
{
	int numpos,i,j,patternednum=0;
	const COLORREF backgroundcolor=RGB(159,255,191);
	const COLORREF linecirclecolor1=RGB(255,0,0);
	const COLORREF linecirclecolor2=RGB(255,0,255);
	const COLORREF linecirclecolor3=RGB(255,255,0);
	const COLORREF linecirclecolor4=RGB(0,0,255);
	Sleep(SLEEPTIME*2);
	initgraph((c+2)*50,(r+2)*50);
	HWND hWnd=GetHWnd();
	SetWindowText(hWnd,"连连看 Lianliankan");
	fillrectangle(0,0,(c+2)*50,(r+2)*50,backgroundcolor);
	char filename[]="llkimage\\00.jpg";
	for(numpos=0;numpos<strlen(filename);numpos++)
		if(filename[numpos]=='0'&&filename[numpos+1]=='0'&&filename[numpos+2]=='.')break;
	IMAGE img;
	for(i=1;i<=r;i++)
		for(j=1;j<=c;j++)
		{
			if(pattern[i][j]==0)continue;
			patternednum++;
			drawdefaultimage(i,j,pattern[i][j]);
			filename[numpos]=char(pattern[i][j]/10+'0');
			filename[numpos+1]=char(pattern[i][j]%10+'0');
			loadimage(&img,filename);
			putimage(50*j,50*i,50,50,&img,0,0);
		}
	fillrectangle(0,0,50,(r+2)*50,backgroundcolor);
	fillrectangle(0,0,(c+2)*50,50,backgroundcolor);
	fillrectangle(0,(r+1)*50,(c+2)*50,(r+2)*50,backgroundcolor);
	fillrectangle((c+1)*50,0,(c+2)*50,(r+2)*50,backgroundcolor);
	play(15);
	Sleep(SLEEPTIME);
	for(i=0;i<vway.size();i++)
	{
		if(vway[i].t2)
		{
			drawline(vway[i].p1.x,vway[i].p1.y,vway[i].turn1.x,vway[i].turn1.y,linecirclecolor1);
			drawline(vway[i].p2.x,vway[i].p2.y,vway[i].turn2.x,vway[i].turn2.y,linecirclecolor1);
			drawline(vway[i].turn2.x,vway[i].turn2.y,vway[i].turn1.x,vway[i].turn1.y,linecirclecolor1);
			drawcircle(vway[i].p1.x,vway[i].p1.y,linecirclecolor1);
			drawcircle(vway[i].p2.x,vway[i].p2.y,linecirclecolor1);
			play(3);
			Sleep(SLEEPTIME);
			drawline(vway[i].p1.x,vway[i].p1.y,vway[i].turn1.x,vway[i].turn1.y,backgroundcolor);
			drawline(vway[i].p2.x,vway[i].p2.y,vway[i].turn2.x,vway[i].turn2.y,backgroundcolor);
			drawline(vway[i].turn2.x,vway[i].turn2.y,vway[i].turn1.x,vway[i].turn1.y,backgroundcolor);
			turntoempty(vway[i].p1.x,vway[i].p1.y,vway[i].turn1.x,vway[i].turn1.y,backgroundcolor);
			turntoempty(vway[i].p2.x,vway[i].p2.y,vway[i].turn2.x,vway[i].turn2.y,backgroundcolor);
			turntoempty(vway[i].turn2.x,vway[i].turn2.y,vway[i].turn1.x,vway[i].turn1.y,backgroundcolor);
		}
		else if(vway[i].t1)
		{
			drawline(vway[i].p1.x,vway[i].p1.y,vway[i].turn1.x,vway[i].turn1.y,linecirclecolor2);
			drawline(vway[i].p2.x,vway[i].p2.y,vway[i].turn1.x,vway[i].turn1.y,linecirclecolor2);
			drawcircle(vway[i].p1.x,vway[i].p1.y,linecirclecolor2);
			drawcircle(vway[i].p2.x,vway[i].p2.y,linecirclecolor2);
			play(5);
			Sleep(SLEEPTIME);
			drawline(vway[i].p1.x,vway[i].p1.y,vway[i].turn1.x,vway[i].turn1.y,backgroundcolor);
			drawline(vway[i].p2.x,vway[i].p2.y,vway[i].turn1.x,vway[i].turn1.y,backgroundcolor);
			turntoempty(vway[i].p1.x,vway[i].p1.y,vway[i].turn1.x,vway[i].turn1.y,backgroundcolor);
			turntoempty(vway[i].p2.x,vway[i].p2.y,vway[i].turn1.x,vway[i].turn1.y,backgroundcolor);
		}
		else
		{
			if(vway[i].distance()==1)
			{
				drawline(vway[i].p2.x,vway[i].p2.y,vway[i].p1.x,vway[i].p1.y,linecirclecolor3);
				drawcircle(vway[i].p1.x,vway[i].p1.y,linecirclecolor3);
				drawcircle(vway[i].p2.x,vway[i].p2.y,linecirclecolor3);
			}
			else
			{
				drawline(vway[i].p2.x,vway[i].p2.y,vway[i].p1.x,vway[i].p1.y,linecirclecolor4);
				drawcircle(vway[i].p1.x,vway[i].p1.y,linecirclecolor4);
				drawcircle(vway[i].p2.x,vway[i].p2.y,linecirclecolor4);
			}
			play(7);
			Sleep(SLEEPTIME);
			drawline(vway[i].p2.x,vway[i].p2.y,vway[i].p1.x,vway[i].p1.y,backgroundcolor);
			turntoempty(vway[i].p2.x,vway[i].p2.y,vway[i].p1.x,vway[i].p1.y,backgroundcolor);
		}
		play(11);
		Sleep(SLEEPTIME);
	}
	RECT rect={0,0,(c+2)*50,(r+2)*50};
	setfont(48,0,"黑体");
	setbkmode(TRANSPARENT);
	Sleep(SLEEPTIME);
	if(vway.size()*2==patternednum)
	{
		for(i=5;i<=17;i++)
		{
			if(i%4==1)setcolor(linecirclecolor1);
			else if(i%4==2)setcolor(linecirclecolor2);
			else if(i%4==3)setcolor(linecirclecolor3);
			else setcolor(linecirclecolor4);
			drawtext("搞掂!!",&rect,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
			if(i<=12)play(i);
			Sleep(SLEEPTIME);
		}
	}
	else
	{
		for(i=12;i>=0;i--)
		{
			if(i%4==1)setcolor(linecirclecolor1);
			else if(i%4==2)setcolor(linecirclecolor2);
			else if(i%4==3)setcolor(linecirclecolor3);
			else setcolor(linecirclecolor4);
			drawtext("无解!!",&rect,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
			if(i>=5)play(i);
			Sleep(SLEEPTIME);
		}
	}
	closegraph();
}

int main()
{
	int i,j,r,c,initpattern[MAX_SIZE][MAX_SIZE],patternednum=0,patternnum[PATTERN_NUM+1]={0};
	init();
	if(!input(patternednum,patternnum,r,c))
	{
		cout<<endl;
		system("PAUSE");
		return 0;
	}
	for(i=1;i<=r;i++)
		for(j=1;j<=c;j++)
			initpattern[i][j]=pattern[i][j];
	cout<<"\n正在计算，请稍候……\n\n";
	if(!calculate(patternednum,r,c,clock()))vway=answay;
	cout<<"计算完毕！正在输出每步记录，请稍候……\n\n";
	for(i=0;i<=r+1;i++)
		for(j=0;j<=c+1;j++)
			pattern[i][j]=initpattern[i][j];
	logoutput(r,c);
	cout<<"输出完毕（记录已保存在log.txt中）！即将进行过程模拟！\n\n";
	graphicsoutput(r,c);
	return 0;
}
