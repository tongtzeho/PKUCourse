/***************************************************************************
文件名：计算概论大作业-神奇翻转.cpp
作者：北京大学信息科学技术学院2011级3班 唐子豪
日期：2011年12月4日
编译环境：Microsoft Visual C++ 6.0 (在dev-cpp下也能编译通过)
版本：2.4.0

基本功能：
1.方块阵有红、蓝两种颜色，在游戏开始时，某些方格为红色，某些方格为蓝色
2.用户通过鼠标点击方块即可翻转方块，不需要使用键盘
3.当用户通过不断翻转，把方块阵的方块都变成蓝色时，游戏将提示用户胜利
4.当用户要求系统给出提示时，游戏可以提供给用户一种正确的翻转方式
补充功能：
1.方块阵的行数和列数范围都是2到20，可以相等，也可以不相等
2.游戏提供了四种游戏难度供用户选择，用户可以根据需要选择不同的难度，也可选择随机难度
3.游戏提供了两种提示方式，除了提供翻转方式以外，还可以提示用户至少还需翻转多少次方块
4.在游戏过程中可随时重新开局或退出游戏
5.实时显示游戏时间
6.有自动提示模式和自动翻转模式，但必须在游戏界面中用鼠标点击某些特定位置才能启用
7.在每次运行游戏时，能自动最大化窗口（不是全屏）
8.在每次翻转时有极短时间的停滞而显示出翻转的效果

历史版本及修改功能简述：
1.0 用键盘读入坐标；有两种提示方式；能在游戏结束后给出游戏时间；行数和列数上限为18
1.1 在每次翻转时能有极短时间的停滞而显示出翻转的效果；每次运行游戏时能自动最大化窗口
1.2 提供三种难度及随机难度
1.3 把此前版本的命令行指令用其他函数代替
2.0 用鼠标点击方块
2.1 优化初始化方块的算法；行数和列数的上限改为20；提供四种难度及随机难度；实时显示游戏时间；在翻转时不闪屏
2.2 增加自动提示模式；优化不同难度下初始化方块的算法
2.3 优化翻转方块时的算法；增加自动翻转模式；修复2.1和2.2中提示的字样有时不消失的bug；修复个别变量未初始化的bug
2.4 继续优化不同难度下初始化方块的算法

初次制作游戏，如果程序或源代码有任何错漏或不当之处，欢迎指出
***************************************************************************/

#include<windows.h>
#include<iostream>
#include<ctime>
#include<string>
#include<iomanip>
using namespace std;
HANDLE hIn,hOut;//用于获取命令行输入、输出流的句柄
INPUT_RECORD mouseRec;//用于储存鼠标的位置、是否点击等信息
DWORD res;//用于储存从鼠标处读入信息的个数
COORD pos={0,0};//用于记录鼠标指针的位置

/*其他全局变量定义：
n和width分别表示方块阵的行数和列数
若把红色的方块用1、蓝色的方块用0来表示，把每行的方块的状态从左到右连起来可以组成一个二进制数，
line1[25]用于记录每一行的这个二进制数的十进制表示形式，取值为0到2^width-1，称之为方块状态
block[25][25]用于记录每个方块的状态，取值为0和1
ver[626]用于每次初始化时记录要翻转哪些方块，取值为0和1
steptotal,hint1total,hint2total,timetotal分别记录已经翻转了多少次、用了多少次提示一、用了多少次提示二、游戏时间
leaststep表示至少还需翻转多少次能把全部方块变成蓝色
step[25][25]用于记录需要翻转哪些方块，取值为0和1
laststepx和laststepy分别表示上一次翻转的方块所在的行和列
hintx和hinty分别表示提示的方块所在的行和列
cheat1和cheat2分别表示是否启用自动提示模式和自动翻转模式
difficulty记录用户选择的难度
coediffmem[25][25]用于储存不同行数和列数的方块阵对应的“难度系数”   */
int line1[25]={0},block[25][25]={0},step[25][25]={0},ver[626]={0};
int steptotal=0,hint1total=0,hint2total=0,laststepx=0,laststepy=0,n=0,width=0,leaststep=0,cheat1=0,cheat2=0,hintx=0,hinty=0;
int difficulty=0,coediffmem[25][25]={0};
time_t timetotal;

/*函数功能：清屏——用于将整个屏幕的背景颜色变成白色，将所有字符清除，并把输出光标置于缓冲区左上方
在welcome,gamestart,win,gameend,main函数中被调用*/
void clearscreen();

/*函数功能：设置颜色——用于设定每次输出时的背景颜色和字体颜色
i代表一种背景颜色和字体颜色的方案
在clearscreen,welcome,gamestart,output,outputword,press,hint,win函数中被调用*/
void setcolor(int i);

/*函数功能：游戏的欢迎界面
在main函数中被调用*/
void welcome();

/*函数功能：开始游戏——选择方块阵的行数与列数，选择游戏难度，并初始化变量
返回值固定为1
在main函数中被调用*/
bool gamestart();

/*函数功能：返回整数a,b的最大值
在calcdifficultyinf,calcdifficultysup,output函数中被调用*/
inline int maxnum(int a,int b);

/*函数功能：返回在某种难度和难度系数下初始状态的leaststep值的下限
coediff代表难度系数
在main函数中被调用*/
inline int calcdifficultyinf(int coediff);

/*函数功能：返回在某种难度和难度系数下初始状态的leaststep值的上限
coediff代表难度系数
在main函数中被调用*/
inline int calcdifficultysup(int coediff);

/*函数功能：随机初始化方块的颜色，由方块阵的颜色状态得到每行方块状态的值
返回值代表初始化过程中翻转的方块数量
inf,sup分别代表leaststep值的下限和上限
w=2^width，等于line1[ ]数组每个元素的上限值减一
在main函数中被调用*/
int randomblock(int inf,int sup,int w);

/*函数功能：计算将某种状态下的方块阵全部变成蓝色，需要翻转的最少次数，并给出一种翻转方案储存在step[ ][ ]数组中
k=2^width，等于line1[ ]数组每个元素的上限值减一
在main函数中被调用*/
void calculate(int k);

/*函数功能：在调用randomblock函数并leaststep符合难度要求后，将每行的方块状态转化成block[ ][ ]的值
在main函数中被调用*/
void turn();

/*函数功能：主输出函数——按不同的指令(由t的值确定)输出方块阵，并根据需要调用outputword和press函数
w=2^width，等于line1[ ]数组每个元素的上限值减一
返回值代表是继续游戏、重新开局，还是退出游戏
在invert,win,main函数中被调用*/
int output(int t,int w);

/*函数功能：在游戏过程中输出界面右上方的文字
在output函数中被调用*/
void outputword();

/*函数功能：鼠标点击——读入游戏当中鼠标点击的位置并进行相应处理
在output函数中被调用，返回值将作为output函数的返回值*/
int press(int w);

/*函数功能：提示一——根据step[ ][ ]和leaststep给出提供一种翻转方式，并调用output函数在方块阵中将该方块表示出来
k是随机生成的数值，用于随机选择一个需要翻转的方块
在press函数中被调用*/
void hint(int k);

/*函数功能：翻转方块——将第i行第j列以及其上下左右的相邻方块的颜色改变
w=2^width，等于line1[ ]数组每个元素的上限值减一
在press函数中被调用*/
void invert(int i,int j,int w);

/*函数功能：判断是否胜利，如果胜利将输出提示文字
返回值代表是未胜利、胜利后再来一盘，还是胜利后退出游戏
在main函数中被调用*/
int win();

/*函数功能：游戏的结束界面
在main函数中被调用*/
void gameend();

//------------------------------------------------------------------------------------------------------------

void clearscreen()
{
    setcolor(2);//输出的背景颜色设定为白色，字体颜色设定为黑色
	CONSOLE_SCREEN_BUFFER_INFO bInfo;
    GetConsoleScreenBufferInfo(hOut,&bInfo);
	COORD home={0,0};
	WORD att=bInfo.wAttributes;
	unsigned long size = bInfo.dwSize.X * bInfo.dwSize.Y;
	FillConsoleOutputAttribute(hOut, att, size, home, NULL);
	FillConsoleOutputCharacter(hOut, ' ', size, home, NULL);//在整个屏幕填充' '字符
	SetConsoleCursorPosition(hOut,home);//将光标移动到左上角
}//结束clearscreen函数

//------------------------------------------------------------------------------------------------------------

void setcolor(int i)
{
	if(i==0) 
	{
		//常规状态下蓝色方块的输出设定，背景颜色为蓝色，字体颜色为灰色
		SetConsoleTextAttribute(hOut,BACKGROUND_INTENSITY|BACKGROUND_BLUE|FOREGROUND_INTENSITY);
	}
	else if(i==1) 
	{
		//常规状态下蓝色方块的输出设定，背景颜色为红色，字体颜色为灰色
		SetConsoleTextAttribute(hOut,BACKGROUND_INTENSITY|BACKGROUND_RED|FOREGROUND_INTENSITY);
	}
	else if(i==2) 
	{
		//清屏、输出文字时的输出设定，背景颜色为白色，字体颜色为黑色
		SetConsoleTextAttribute(hOut,BACKGROUND_INTENSITY|BACKGROUND_GREEN|BACKGROUND_RED|BACKGROUND_BLUE);
	}
	else if(i==3) 
	{
		//将界面下方、右方变成黑色背景时的输出设定，背景颜色为黑色，字体颜色为白色
		SetConsoleTextAttribute(hOut,FOREGROUND_INTENSITY|FOREGROUND_INTENSITY|FOREGROUND_GREEN|FOREGROUND_RED|FOREGROUND_BLUE);
	}
	else if(i==4) 
	{
		//提示翻转蓝色方块时该蓝色方块的输出设定，背景颜色为蓝色，字体颜色为白色
		SetConsoleTextAttribute(hOut,BACKGROUND_INTENSITY|BACKGROUND_BLUE|FOREGROUND_INTENSITY|FOREGROUND_INTENSITY|FOREGROUND_GREEN|FOREGROUND_RED|FOREGROUND_BLUE);
	}
	else if(i==5) 
	{
		//提示翻转红色方块时该蓝色方块的输出设定，背景颜色为红色，字体颜色为白色
		SetConsoleTextAttribute(hOut,BACKGROUND_INTENSITY|BACKGROUND_RED|FOREGROUND_INTENSITY|FOREGROUND_INTENSITY|FOREGROUND_GREEN|FOREGROUND_RED|FOREGROUND_BLUE);
	}
	else if(i==6) 
	{
		//在翻转蓝色方块时的输出设定，该状态每次只会出现极短时间，背景颜色为深红，字体颜色为灰色
		SetConsoleTextAttribute(hOut,BACKGROUND_RED|FOREGROUND_INTENSITY|FOREGROUND_GREEN|FOREGROUND_RED|FOREGROUND_BLUE);//翻转状态红色
	}
	else if(i==7) 
	{
		//在翻转红色方块时的输出设定，该状态每次只会出现极短时间，背景颜色为深蓝，字体颜色为灰色
		SetConsoleTextAttribute(hOut,BACKGROUND_BLUE|FOREGROUND_INTENSITY|FOREGROUND_GREEN|FOREGROUND_RED|FOREGROUND_BLUE);//翻转状态蓝色
	}
	else if(i==8) 
	{
		//紫色方框的输出设定，背景颜色为紫色，字体颜色为白色
		SetConsoleTextAttribute(hOut,BACKGROUND_INTENSITY|BACKGROUND_RED|BACKGROUND_BLUE|FOREGROUND_INTENSITY|FOREGROUND_GREEN|FOREGROUND_RED|FOREGROUND_BLUE);
	}
}//结束setcolor函数

//------------------------------------------------------------------------------------------------------------

void welcome()
{
	//获取输入、输出流句柄
	hIn=GetStdHandle(STD_INPUT_HANDLE);
	hOut=GetStdHandle(STD_OUTPUT_HANDLE);
	
	//改变窗口标题
	SetConsoleTitle("神奇翻转");
	
	//设置窗口尺寸
	COORD size={123,43};
	SetConsoleScreenBufferSize(hOut,size);
	SMALL_RECT rc={0,0,122,42};
    SetConsoleWindowInfo(hOut,true,&rc);
	
	//设置不显示闪烁光标
	CONSOLE_CURSOR_INFO ConsoleCursorInfo;
    GetConsoleCursorInfo(hOut,&ConsoleCursorInfo);
    ConsoleCursorInfo.bVisible=false;
    SetConsoleCursorInfo(hOut,&ConsoleCursorInfo);
	
	//运行游戏时自动将窗口最大化
	HWND hwnd=GetForegroundWindow();
	ShowWindow(hwnd,SW_MAXIMIZE);
	clearscreen();
	cout<<"                                              欢迎进入神奇翻转游戏\n";
	cout<<"                                                                            制作者：北大信科2011级 唐子豪\n";
	cout<<"\n\n\n";
	cout<<"游戏规则：\n\n";
	cout<<"1.请先选择方块阵的行数与列数\n\n";
	cout<<"2.方块阵中，每块方块的颜色为红色或蓝色，每翻转一块方块，将改变这块方块以及其上、下、左、右侧相邻方块共五块方块的颜色\n\n";
	cout<<"3.当您把全部方块的颜色变成蓝色时，游戏结束，您获得胜利\n\n";
	cout<<"4.游戏中有两种提示方式，提示一向您提供一种正确的翻转方式，提示二告诉您至少还需翻转多少方块，在每局游戏中每次翻转都允许使用提示，但在每次翻转中只限使用一次\n\n";
	cout<<"5.游戏使用鼠标进行操作，请使用鼠标点击方块或紫色的方框\n\n";
	cout<<"6.为确保游戏顺利运行，请在游戏过程中一直使窗口最大化（但不要全屏），选择8×16的字体大小，并请勿打开快速编辑模式\n\n";
	cout<<"\n\n\n\n";
	
	//在下方中部输出“开始游戏”的供鼠标点击的紫色方框
	COORD po={50,26};
	setcolor(8);
	SetConsoleCursorPosition(hOut,po);
	cout<<"            ";
	po.Y++;
	SetConsoleCursorPosition(hOut,po);
	cout<<"  开始游戏  ";
	po.Y++;
	SetConsoleCursorPosition(hOut,po);
	cout<<"            ";
	setcolor(2);
	cout<<"\n\n";
	
	//等待鼠标点击“开始游戏”
    while(1)
	{
		ReadConsoleInput(hIn,&mouseRec,1,&res);
		if(mouseRec.EventType==MOUSE_EVENT)
		{
			pos=mouseRec.Event.MouseEvent.dwMousePosition;
			if(mouseRec.Event.MouseEvent.dwButtonState==FROM_LEFT_1ST_BUTTON_PRESSED)
				if(pos.Y<=28&&pos.Y>=26&&pos.X>=50&&pos.X<=61)break;//当单击该方框时结束等待的循环
		}
	} 
}//结束welcome函数

//------------------------------------------------------------------------------------------------------------

bool gamestart()
{
	clearscreen();
	cout<<"请选择行数：\n\n";
	int i,j;

	//依次输出2,3,4,...,20的紫色方框
	for(i=1;i<=3;i++)
	{
		for(j=2;j<=20;j++)
		{
			setcolor(2);
			cout<<' ';
			setcolor(8);
			if(i%2==1&&j<10)cout<<"   ";
			else if(i%2==1)cout<<"    ";
			else cout<<' '<<j<<' ';
		}
		setcolor(2);
		cout<<endl;
	}

	//等待选择行数
	while(1)
	{
		ReadConsoleInput(hIn,&mouseRec,1,&res);
		if(mouseRec.EventType==MOUSE_EVENT)
		{
			pos=mouseRec.Event.MouseEvent.dwMousePosition;
			if(mouseRec.Event.MouseEvent.dwButtonState==FROM_LEFT_1ST_BUTTON_PRESSED&&pos.Y>=2&&pos.Y<=4)
			{
				if(pos.X<32&&pos.X%4!=0)
				{
					n=pos.X/4+2;
					break;
				}
				if(pos.X>32&&pos.X<87&&pos.X%5!=2)
				{
					n=(pos.X+2)/5+3;
					break;
				}
			}
		}
	}

	//依次输出2,3,4,...,20的紫色方框
	cout<<"\n请选择列数\n\n";
	for(i=1;i<=3;i++)
	{
		for(j=2;j<=20;j++)
		{
			setcolor(2);
			cout<<' ';
			setcolor(8);
			if(i%2==1&&j<10)cout<<"   ";
			else if(i%2==1)cout<<"    ";
			else cout<<' '<<j<<' ';
		}
		setcolor(2);
		cout<<endl;
	}

	//等待选择列数
	while(1)
	{
		ReadConsoleInput(hIn,&mouseRec,1,&res);
		if(mouseRec.EventType==MOUSE_EVENT)
		{
			pos=mouseRec.Event.MouseEvent.dwMousePosition;
			if(mouseRec.Event.MouseEvent.dwButtonState==FROM_LEFT_1ST_BUTTON_PRESSED&&pos.Y>=8&&pos.Y<=10)
			{
				if(pos.X<32&&pos.X%4!=0)
				{
					width=pos.X/4+2;
					break;
				}
				if(pos.X>32&&pos.X<87&&pos.X%5!=2)
				{
					width=(pos.X+2)/5+3;
					break;
				}
			}
		}
	}
	cout<<"\n您选择了"<<n<<"行"<<width<<"列的方块阵\n";
	cout<<"\n请选择游戏难度：\n\n";
	string diff[5]={"新手","业余","专家","大师","随机"};

	//依次输出“新手”、“业余”、“专家”、“大师”、“随机”的紫色方框
	for(i=1;i<=3;i++)
	{
		for(j=0;j<=4;j++)
		{
			setcolor(2);
			cout<<"  ";
			setcolor(8);
			i%2==0?cout<<"  "<<diff[j]<<"  ":cout<<"        ";
		}
		cout<<endl;
	}

	//等待选择难度
	while(1)
	{
		ReadConsoleInput(hIn,&mouseRec,1,&res);
		if(mouseRec.EventType==MOUSE_EVENT)
		{
			pos=mouseRec.Event.MouseEvent.dwMousePosition;
			if(mouseRec.Event.MouseEvent.dwButtonState==FROM_LEFT_1ST_BUTTON_PRESSED&&pos.Y>=16&&pos.Y<=18)
			{
				if(pos.X<50&&pos.X/2%5!=0)
				{
					//点击“新手”、“业余”、“专家”、“大师”、“随机”的结果将分别对应difficulty取值0,1,2,3,4
					difficulty=pos.X/10;
					//将laststepx,laststepy的值初始化为选择难度时的点击位置
					laststepx=(pos.Y+1)/2;
					laststepy=(pos.X+1)/4;
					break;
				}
			}
		}
	}
	setcolor(2);
	cout<<"\n\n您选择了"<<diff[difficulty]<<"难度\n\n";

	//全局变量初始化
	for(i=0;i<=n+1;i++)
		line1[i]=0;
	steptotal=hint1total=hint2total=cheat1=cheat2=hintx=hinty=0;
	cout<<"\n\n数据生成中，请稍候..";
	return 1;
}//结束startgame函数

//------------------------------------------------------------------------------------------------------------

inline int maxnum(int a,int b)
{
	//返回a,b的最大值
	if(a>b)return a;
	return b;
}

//------------------------------------------------------------------------------------------------------------

inline int calcdifficultyinf(int coediff)
{
	if(difficulty==3)
	{
		if(coediff==7)return int(n*width*0.6);
		if(coediff==6)return int(n*width*0.55);
		if(coediff==5)return int(n*width*0.5);
		if(coediff==4)return int(n*width*0.45);
		if(coediff==3)return int(n*width*0.4);
		if(coediff==2)return int(n*width*0.375);
		if(coediff==1)return int(n*width*0.35);
	}
	if(difficulty==2)
	{
		if(coediff>4)return int(n*width*0.35);
		else return int(n*width*0.25);
	}
	if(difficulty==1)
	{
		//此处引入maxnum(1, )的作用是避免初始化的方块阵全部都是蓝色，在clacdifficultysup函数中同理
		if(coediff>4)return maxnum(int(n*width*0.175),1);
		else return maxnum(int(n*width*0.125),1);
	}
	return maxnum(int(n*width*0.025),1);
}//结束calcdifficultyinf函数

//------------------------------------------------------------------------------------------------------------

inline int calcdifficultysup(int coediff)
{
	if(difficulty==4)return n*width;
	if(difficulty==3)return int(n*width*0.85);
	if(difficulty==2)
	{
		if(coediff>4)return int(n*width*0.8);
		else return int(n*width*0.7);
	}
	if(difficulty==1)
	{
		if(coediff>4)return int(n*width*0.55);
		else return int(n*width*0.45);
	}
	if(difficulty==0)
	{
		if(coediff>4)return int(n*width*0.25);
		else return maxnum(int(n*width*0.2),1);
	}
	return n*width;
}//结束calcdifficultysup函数

//------------------------------------------------------------------------------------------------------------

int randomblock(int inf,int sup,int w)
{
	int i,j,k,m,d,a[25]={0};//k和d都用来代表随机数；a[ ]和line1[ ]作用类似，是把每一行的某种0和1状态连起来后用十进制数表示，称之为翻转状态
	//生成在inf到sup之间的一个随机数k
	srand((unsigned)time(0));
	k=rand()%(sup-inf+1)+inf;

	//ver[ ]初始化
	m=n*width;
	for(i=1;i<=m;i++)
		ver[i]=0;

	//生成k个不同在1到n*width之间的随机数，并将ver数组中下标为这些随机数的元素标记为1，
	for(i=1;i<=k;i++)
	{
		d=rand()%m+1;
		while(ver[d]!=0)
			d!=m?d++:d=1;//如果生成随机数重复的话，就往后找，若找到m就跳回1继续找，直到不重复
		ver[d]=1;
	}

	/*先把ver[ ]的每个元素分成n行width列，这样ver[ ]每个元素即对应一个方块
	再把ver[ ]当中值为1的方块及其上下左右相邻方块的颜色改变（一开始都是蓝色）
	再把每行的翻转状态储存在数组a中
	再由数组a各行的翻转状态，通过左移、右移和异或运算得到每行方块状态的值*/
	for(i=1;i<=n;i++)
	{
		d=1;
		for(j=1;j<=width;j++)
		{
			//将0和1的状态连起来并转化为十进制数
			a[i]+=ver[(i-1)*width+j]*d;
			d*=2;
		}
	}
	for(i=1;i<=n;i++)
		line1[i]=(a[i-1]^a[i]^(a[i]<<1)^(a[i]>>1)^a[i+1])%w;//line1[ ]数值不能超过w
	return k;
}//结束randomblock函数

//------------------------------------------------------------------------------------------------------------

void calculate(int k)
{
	int c[25]={0},i,j,sum,d;//c[ ]类似randomblock中的a[ ]，是记录每行的翻转状态的十进制数；sum代表每种方案翻转了多少次方块；d为临时变量
	leaststep=10000000;
	/*这里采取了枚举法，算法复杂度为O(n*k)=O(n*(2^width))
	将第一行的翻转状态用循环来枚举，下面每一行的翻转状态可由前面两行的翻转状态以及方块状态确定
	当第n行翻转状态确定后，如果方块阵全部变成蓝色，则是一种正确的翻转方案*/
	for(c[1]=0;c[1]<k;c[1]++)
	{
		for(i=2;i<=n;i++)
			c[i]=(line1[i-1]^c[i-1]^c[i-2]^(c[i-1]>>1)^(c[i-1]<<1))%k;//计算每一行的翻转状态
		if((c[n-1]^c[n]^(c[n]>>1)^(c[n]<<1)^line1[n])%k==0)//如果翻转后最后一行全为蓝色
		{
			//计算此种正确的方案翻转了多少次方块
			sum=0;
			for(i=1;i<=n;i++)
				for(j=1;j<=width;j++)
					sum+=(1^(c[i]>>(j-1))+1)%2;

			if(sum<leaststep)
			{
				leaststep=sum;
				for(i=1;i<=n;i++)
				{
					d=c[i];
					//将该可行方案的翻转状态记录在step[ ][ ]中
					for(j=1;j<=width;j++)
					{
						step[i][j]=0;
						step[i][j]=d%2;
						d/=2;
					}
				}
			}
		}//结束if(最后一行全为蓝色)
	}//结束枚举循环
}//结束calculate函数

//------------------------------------------------------------------------------------------------------------

void turn()
{
	int i,j,d;//d为临时变量
	for(i=1;i<=n;i++)
	{
		d=line1[i];
		for(j=1;j<=width;j++)
		{
			//将方块状态记录在block[ ][ ]中
			block[i][j]=0;
			block[i][j]=d%2;
			d/=2;
		}
	}
	cout<<"\n\n数据生成完毕";
	Sleep(150);
}//结束turn函数

//------------------------------------------------------------------------------------------------------------

int output(int t,int w)
{
	string titl[25]={"Ａ","Ｂ","Ｃ","Ｄ","Ｅ","Ｆ","Ｇ","Ｈ","Ｉ","Ｊ","Ｋ","Ｌ","Ｍ","Ｎ","Ｏ","Ｐ","Ｑ","Ｒ","Ｓ","Ｔ","Ｕ","Ｖ","Ｗ","Ｘ"};
	COORD po;
	int i,j;
	//x[ ],y[ ]表示在游戏中点击某块方块时，需要重新输出的方块的行、列坐标
	int x[6]={0,laststepx,laststepx-1,laststepx+1,laststepx,laststepx},y[6]={0,laststepy,laststepy,laststepy,laststepy-1,laststepy+1};

	po.X=0;
	po.Y=2*n+2;
	SetConsoleCursorPosition(hOut,po);
	setcolor(2);
	cout<<"            ";
	po.Y=0;
	SetConsoleCursorPosition(hOut,po);
	if(t>=0)//当开始游戏(t=0)或胜利(t=2)时会进入此分支，会把全部方块重新输出
	{
	    po.X=0;
	    po.Y=2*n+2;
	    SetConsoleCursorPosition(hOut,po);
	    setcolor(2);
	    cout<<"                             ";
	    
		//在缓冲区第一行输出Ａ,Ｂ,Ｃ,...
		po.Y=0;
	    SetConsoleCursorPosition(hOut,po);
		cout<<"   ";
	    for(i=0;i<width;i++)
		    cout<<' '<<titl[i]<<' ';
	    cout<<endl;

		//输出每行方块阵，每行方块阵占缓冲区的两行，在每行方块阵的第一行的左边和右边输出行数
	    for(i=1;i<=n;i++)
		{
		    cout<<setw(2)<<i<<' ';
		    for(j=1;j<=width;j++)
			{
			    setcolor(block[i][j]);
			    cout<<setw(2)<<i<<char(j+64)<<' ';
			}
		    setcolor(2);
		    cout<<' '<<setw(2)<<i<<endl<<"___";
		    for(j=1;j<=width;j++)
			{
			    setcolor(block[i][j]);
			    cout<<"   .";
			}
		    setcolor(2);
		    cout<<"____"<<endl;
		}

		//在方块阵下端再次输出Ａ,Ｂ,Ｃ,...
	    cout<<"   ";
	    for(i=0;i<width;i++)
		    cout<<' '<<titl[i]<<' ';
	    cout<<endl;

		//将界面右方、下方的多余区域变成黑色背景
		setcolor(3);
		CONSOLE_SCREEN_BUFFER_INFO bInfo;
        GetConsoleScreenBufferInfo(hOut,&bInfo);
		WORD att=bInfo.wAttributes;
		po.X=0;
		po.Y=maxnum(15,2*n+3);
		FillConsoleOutputAttribute(hOut,att,123*(42-maxnum(15,2*n+3)+1),po,NULL);
		po.X=width*4+43;
		for(po.Y=0;po.Y<=42;po.Y++)
		{
			FillConsoleOutputAttribute(hOut,att,80-width*4,po,NULL);
		}

	}//结束if(t>=0)分支

	else//当游戏过程中鼠标点击方块时会先后进入此分支两次(t=-1,t=-2)，只会把x[ ],y[ ]中的方块重新输出
	{
		for(i=1;i<=5;i++)
		{
			if(x[i]>0&&x[i]<=n&&y[i]>0&&y[i]<=width)//当x[ ],y[ ]中的方块在方块阵范围以内时
			{
				po.X=4*y[i]-1;
	            po.Y=2*x[i]-1;
	            SetConsoleCursorPosition(hOut,po);
	            if(t==-1)//翻转瞬间的效果，原来蓝色的方块变成深红，原来红色的方块变成深蓝
				{
					setcolor(block[x[i]][y[i]]+6);
					cout<<"    ";
					po.Y++;
					SetConsoleCursorPosition(hOut,po);
					cout<<"    ";
				}
				else if(t==-2)//翻转之后
				{
					setcolor(block[x[i]][y[i]]);
					cout<<setw(2)<<x[i]<<char(y[i]+64)<<' ';
					po.Y++;
					SetConsoleCursorPosition(hOut,po);
					cout<<"   .";
				}
			}
		}
		if(t==-2&&hintx>0&&hinty>0)//如果使用了提示一，那么在某块方块处会有“翻转这块”的字样，需要消去该字样
		{
			po.X=4*hinty-1;
			po.Y=2*hintx-1;
			SetConsoleCursorPosition(hOut,po);
			setcolor(block[hintx][hinty]);
			cout<<setw(2)<<hintx<<char(hinty+64)<<' ';
			po.Y++;
			SetConsoleCursorPosition(hOut,po);
			cout<<"   .";
			hintx=hinty=0;
		}
	}//结束else的分支

	po.X=0;
	po.Y=2*n+2;
	setcolor(2);
	SetConsoleCursorPosition(hOut,po);
	if(t!=0&&t!=-2)return 0;//当t=2时，右上方的文字输出由win函数确定；当t=-1时，为翻转瞬间的效果，不需要输出右上方的文字
	outputword();//输出右上方的文字
	return press(w);//调用press函数，并把press的返回值作为output的返回值
}//结束output函数

//------------------------------------------------------------------------------------------------------------

void outputword()
{
	COORD po;
	//依次在右上方输出上一步翻转的方块位置、翻转次数、使用提示一次数、使用提示二次数、时间
	po.X=width*4+8;
	po.Y=0;
	SetConsoleCursorPosition(hOut,po);
	if(steptotal>0)cout<<"|上一步翻转了第"<<laststepx<<"行、第"<<char(laststepy+64)<<"列的方块";
	po.Y=1;
	SetConsoleCursorPosition(hOut,po);
	cout<<"|目前已翻转了"<<steptotal<<"次方块";
	po.Y=2;
	SetConsoleCursorPosition(hOut,po);
	cout<<"|目前已使用了"<<hint1total<<"次提示一";
	po.Y=3;
	SetConsoleCursorPosition(hOut,po);
	cout<<"|目前已使用了"<<hint2total<<"次提示二";
	po.Y=4;
	SetConsoleCursorPosition(hOut,po);
	cout<<"|";
	if(cheat1==0&&cheat2==0)cout<<'*';//没有启动自动提示/自动翻转模式则输出*，否则输出!
	else cout<<'!';
	cout<<"时间:";
	po.Y=5;
	SetConsoleCursorPosition(hOut,po);
	cout<<"|---------------------------";
	
	//输出“提示一”、“提示二”、“重新开局”、“退出游戏”的紫色方框
	po.Y=6;
	SetConsoleCursorPosition(hOut,po);
	cout<<'|';
	if(cheat1+cheat2==0)//当启动了自动提示/自动翻转模式时，不输出“提示一”、“提示二”，下同
	{
		setcolor(8);
	    cout<<"            ";
	    setcolor(2);
	    cout<<"  ";
	    setcolor(8);
	    cout<<"            ";
	}
	po.Y=7;
	SetConsoleCursorPosition(hOut,po);
	setcolor(2);
	cout<<'|';
	if(cheat1+cheat2==0)
	{
	    setcolor(8);
	    cout<<"  提 示 一  ";
	    setcolor(2);
	    cout<<"  ";
	    setcolor(8);
	    cout<<"  提 示 二  ";
	    setcolor(2);
	}
	po.Y=8;
	SetConsoleCursorPosition(hOut,po);
	cout<<'|';
	if(cheat1+cheat2==0)
	{
	    setcolor(8);
	    cout<<"            ";
	    setcolor(2);
	    cout<<"  ";
	    setcolor(8);
	    cout<<"            ";
	    setcolor(2);
	}
	po.Y=9;
	SetConsoleCursorPosition(hOut,po);
	cout<<"|---------------------------";
	po.Y=10;
	SetConsoleCursorPosition(hOut,po);
	cout<<'|';
	setcolor(8);
	cout<<"            ";
	setcolor(2);
	cout<<"  ";
	setcolor(8);
	cout<<"            ";
	po.Y=11;
	SetConsoleCursorPosition(hOut,po);
	setcolor(2);
	cout<<'|';
	setcolor(8);
	cout<<"  重新开局  ";
	setcolor(2);
	cout<<"  ";
	setcolor(8);
	cout<<"  退出游戏  ";
	setcolor(2);
	po.Y=12;
	SetConsoleCursorPosition(hOut,po);
	cout<<'|';
	setcolor(8);
	cout<<"            ";
	setcolor(2);
	cout<<"  ";
	setcolor(8);
	cout<<"            ";
	setcolor(2);
	po.Y=13;
	SetConsoleCursorPosition(hOut,po);
	cout<<"|---------------------------";
	po.X=0;
	po.Y=2*n+2;
	SetConsoleCursorPosition(hOut,po);
}//结束outputword函数

//------------------------------------------------------------------------------------------------------------

int press(int w)
{
    COORD po;
	time_t starttime,endtime,temp;//starttime表示每次翻转初的时间，endtime表示点击时的时间
	//h表示是否可使用提示(0/1)，cheat表示点击*的次数，changecheat表示是否可以启动自动提示/自动翻转模式，repeat表示重复点击同一方块的次数
	int i,j,h=1,cheat=0,changecheat=1,repeat=0;
	starttime=clock();
	if(cheat1==1||cheat2==1)//当启动了自动提示/自动翻转模式时，先找到提示翻转的方块位置
	{
		h=changecheat=0;
		srand((unsigned)time(0));
		i=rand()%leaststep+1;
		hint(i);
		if(cheat1==1)hint1total++;
		if(cheat2==1)hint2total++;
	}
	while(1)//当鼠标点击方块或点击“重新开局”、“退出游戏”或自动翻转模式开始翻转时，退出循环
	{
		//模拟一直在按着键盘的A键，这样即使不移动鼠标，ReadConsoleInput也能一直执行
		keybd_event('A',0,0,0);
		ReadConsoleInput(hIn,&mouseRec,1,&res);

		//计算时间并实时显示时间
		endtime=clock();
		temp=timetotal+endtime-starttime;
		i=int(temp)/1000/60;
		j=int(temp)/1000%60;
		po.X=width*4+15;
		po.Y=4;
		SetConsoleCursorPosition(hOut,po);
		setcolor(2);
		cout<<' '<<i<<"分钟"<<j<<"秒";

		//当启动了自动翻转模式时，进入循环后一小段时间即翻转
		if(cheat2==1&&temp-timetotal>60)
		{
			endtime=clock();
			timetotal+=endtime-starttime;
			invert(hintx,hinty,w);
			return 0;
		}

		if(mouseRec.EventType==MOUSE_EVENT)//当鼠标有移动或有点击时
		{
			keybd_event('A',0,KEYEVENTF_KEYUP,0);//模拟松开键盘的A键
			pos=mouseRec.Event.MouseEvent.dwMousePosition;
			if(mouseRec.Event.MouseEvent.dwButtonState==FROM_LEFT_1ST_BUTTON_PRESSED)//当点击鼠标左键时
			{
				if(pos.X==width*4+9&&pos.Y==4&&changecheat==1)//当可以启动自动提示/自动翻转模式而且点击了“时间”左方的*时
				{
					cheat++;
					if(cheat==6)//点击该位置恰好达6次时把*变成?
					{
						po.X=width*4+9;
		                po.Y=4;
		                SetConsoleCursorPosition(hOut,po);
						cout<<'?';
					}
				}
				else if(cheat!=6)//当cheat不等于6并点击了其他位置时
				{
					changecheat=0;
				}

				//没有启动自动翻转模式且点击了方块时
				if(pos.X>=3&&pos.X<=4*width+2&&pos.Y>=1&&pos.Y<=2*n&&mouseRec.Event.MouseEvent.dwEventFlags!=DOUBLE_CLICK&&cheat2==0)
				{
					i=(pos.Y+1)/2;
					j=(pos.X+1)/4;
					//如果希望连续两次翻转同一方块，必须点击该方块三次才能翻转，这是为了避免误操作
					if(i==laststepx&&j==laststepy)repeat++;
					if(i!=laststepx||j!=laststepy||(i==laststepx&&j==laststepy&&repeat>2))
					{
						endtime=clock();
						timetotal+=endtime-starttime;
						invert(i,j,w);
					    return 0;
					}
				}

				//当可使用提示并点击了“提示一”时，给出提示一
				if(pos.X>=4*width+9&&pos.X<=4*width+20&&pos.Y>=6&&pos.Y<=8&&h==1)
				{
					srand((unsigned)time(0));
					i=rand()%leaststep+1;
					hint1total++;
					if(cheat==6&&changecheat==1)//当*刚变成?时，点击“提示一”启动自动提示模式，并把?改为!
					{
						cheat1=1;
						changecheat=0;
						po.X=width*4+9;
		                po.Y=4;
		                SetConsoleCursorPosition(hOut,po);
						cout<<'!';
					}
					hint(i);
					h=0;
				}

				//当可使用提示并点击了“提示二”时，给出提示二
				if(pos.X>=4*width+23&&pos.X<=4*width+34&&pos.Y>=6&&pos.Y<=8&&h==1)
				{
					hint2total++;
					if(cheat==6&&changecheat==1)//当*刚变成?时，点击“提示二”启动自动翻转模式，并把?改为!
					{
						cheat2=1;
						changecheat=0;
						po.X=width*4+9;
		                po.Y=4;
		                SetConsoleCursorPosition(hOut,po);
						cout<<'!';
					}

					//在右上方输出“提示二”对应的文字
					setcolor(2);
					po.X=4*width+9;
					po.Y=6;
					SetConsoleCursorPosition(hOut,po);
					cout<<"提示二：                      ";
					po.Y=7;
					SetConsoleCursorPosition(hOut,po);
					cout<<"至少还需翻转"<<leaststep<<"次方块        ";
					po.Y=8;
					SetConsoleCursorPosition(hOut,po);
					cout<<"本步翻转中您不可再使用提示  ";
					po.X=0;
	                po.Y=2*n+2;
	                SetConsoleCursorPosition(hOut,po);
					h=0;

					//如果刚启动了自动翻转模式，则马上计算要提示的方块并开始自动翻转
					if(cheat2==1)
					{
		                srand((unsigned)time(0));
		                i=rand()%leaststep+1;
		                hint(i);
					}
				}

				if(pos.X>=4*width+9&&pos.X<=4*width+20&&pos.Y>=10&&pos.Y<=12)//点击“重新开局”时
				{
					return 1;
				}
				
				if(pos.X>=4*width+23&&pos.X<=4*width+34&&pos.Y>=10&&pos.Y<=12)//点击“退出游戏”时
				{
					return 2;
				}
			}//结束判断点击鼠标左键
		}//结束判断鼠标移动或点击
	}//结束等待鼠标操作或等待自动翻转
}//结束press函数

//------------------------------------------------------------------------------------------------------------

void hint(int k)
{
    int i,j;
	//从上往下，从左往右寻找需要翻转的方块，当找到第k个时退出循环，并把第k个方块的坐标作为提示的方块的坐标
	for(i=1;i<=n;i++)
	{
		for(j=1;j<=width;j++)
		{
			if(step[i][j]==1)k--;
			if(k==0)break;
		}
		if(k==0)break;
	}
	hintx=i;
	hinty=j;

	//输出“提示一”在右上方对应的文字
	setcolor(2);
	COORD po;
	po.X=4*width+9;
	po.Y=6;
	SetConsoleCursorPosition(hOut,po);
	if(cheat1==1)cout<<"您已启动自动提示模式：       ";
	else if(cheat2==1)cout<<"您已启动自动翻转模式：       ";
	else cout<<"提示一：                      ";
	po.Y=7;
	SetConsoleCursorPosition(hOut,po);
	if(cheat2==1)cout<<"至少还需翻转"<<leaststep<<"次方块        ";
	else cout<<"翻转第"<<i<<"行、第"<<char(j+64)<<"列的方块     ";
	po.Y=8;
	SetConsoleCursorPosition(hOut,po);
	if(cheat2==0)cout<<"本步翻转中您不可再使用提示  ";
	else cout<<"您不可再手动翻转方块      ";

	//当没有启动自动翻转模式时，在方块阵中找到提示的方块，并在方块上输出“翻转这块”
	if(cheat2==0)
	{
	    po.X=4*j-1;
	    po.Y=2*i-1;
	    SetConsoleCursorPosition(hOut,po);
	    setcolor(block[i][j]+4);
	    cout<<"翻转";
	    po.Y=2*i;
    	SetConsoleCursorPosition(hOut,po);
	    cout<<"这块";
	}
	setcolor(2);
	po.X=0;
	po.Y=2*n+2;
	SetConsoleCursorPosition(hOut,po);
}//结束hint函数

//------------------------------------------------------------------------------------------------------------

void invert(int i,int j,int w)
{
	int k=0;//k为临时变量
	steptotal++;
	laststepx=i;
	laststepy=j;
	//将要翻转的方块及其上下左右相邻方块的翻转时的瞬间状态输出
	output(-1,w);
	cout<<"正在翻转中..";

	//改变要翻转的方块及其上下左右相邻方块的颜色
	block[i][j]=(block[i][j]+1)%2;
	block[i-1][j]=(block[i-1][j]+1)%2;
	block[i+1][j]=(block[i+1][j]+1)%2;
	block[i][j-1]=(block[i][j-1]+1)%2;
	block[i][j+1]=(block[i][j+1]+1)%2;

	//将block[ ][ ]的值转化为每行的方块状态，只需改变翻转的方块所在行及其上一行、下一行共三行的方块状态即可
	for(i=laststepx-1;i<=laststepx+1;i++)
	{
		k=1;
		line1[i]=0;
		for(j=1;j<=width;j++)
		{
			line1[i]+=block[i][j]*k;
			k*=2;
		}
	}
}//结束invert函数

//------------------------------------------------------------------------------------------------------------

int win()
{
	int i,s=0,second,minute;//s为每行方块状态的和
	for(i=1;i<=n;i++)
		s+=line1[i];
	if(s!=0)return 0;//存在某些行方块状态不为0，即存在红色方块
	
	//如果s=0则执行下面的语句

	second=int(timetotal)/1000%60;
	minute=int(timetotal)/1000/60;
	clearscreen();
	output(2,NULL);//调用output函数，将全部方块输出，但右上方的文字由win输出，不需要outputword来输出
	
	//输出右上方的文字
	COORD po={width*4+8,0};
	SetConsoleCursorPosition(hOut,po);
	cout<<"|Congratulations!!!!您胜利了！！！";
	po.Y=1;
	SetConsoleCursorPosition(hOut,po);
	cout<<"|本局游戏中，您共翻转了"<<steptotal<<"次方块";
	po.Y=2;
	SetConsoleCursorPosition(hOut,po);
	cout<<"|共使用了"<<hint1total<<"次提示一     ";
	po.Y=3;
	SetConsoleCursorPosition(hOut,po);
	cout<<"|共使用了"<<hint2total<<"次提示二     ";
	po.Y=4;
	SetConsoleCursorPosition(hOut,po);
	cout<<"|共耗时"<<minute<<"分钟"<<second<<"秒   ";
		setcolor(2);
	po.Y=5;
	SetConsoleCursorPosition(hOut,po);
	cout<<"|---------------------------";

	//输出“再来一局”、“退出游戏”的紫色方框
	po.Y=6;
	SetConsoleCursorPosition(hOut,po);
	cout<<'|';
	setcolor(8);
	cout<<"            ";
	setcolor(2);
	cout<<"  ";
	setcolor(8);
	cout<<"            ";
	po.Y=7;
	SetConsoleCursorPosition(hOut,po);
	setcolor(2);
	cout<<'|';
	setcolor(8);
	cout<<"  再来一局  ";
	setcolor(2);
	cout<<"  ";
	setcolor(8);
	cout<<"  退出游戏  ";
	setcolor(2);
	po.Y=8;
	SetConsoleCursorPosition(hOut,po);
	cout<<'|';
	setcolor(8);
	cout<<"            ";
	setcolor(2);
	cout<<"  ";
	setcolor(8);
	cout<<"            ";
	setcolor(2);
	po.Y=9;
	SetConsoleCursorPosition(hOut,po);
	cout<<"|---------------------------";

	//等待鼠标点击“再来一局”或“退出游戏”
	while(1)
	{
		ReadConsoleInput(hIn,&mouseRec,1,&res);
		if(mouseRec.EventType==MOUSE_EVENT)
		{
			pos=mouseRec.Event.MouseEvent.dwMousePosition;
			if(mouseRec.Event.MouseEvent.dwButtonState==FROM_LEFT_1ST_BUTTON_PRESSED)
			{
				if(pos.X>=4*width+9&&pos.X<=4*width+20&&pos.Y>=6&&pos.Y<=8)return 1;
				if(pos.X>=4*width+23&&pos.X<=4*width+34&&pos.Y>=6&&pos.Y<=8)return 2;
			}
		}
	}
}//结束win函数

//------------------------------------------------------------------------------------------------------------

void gameend()
{
	clearscreen();
	cout<<"\n\n\n";
	cout<<"                    +-----------------------------------------------------------+\n";
	cout<<"                    |                                                           |\n";
	cout<<"                    |                 非 常 感 谢 您 运 行 本 游 戏             |\n";
	cout<<"                    |                                                           |\n";
	cout<<"                    |  感谢北京大学信息科学技术学院、                           |\n";
	cout<<"                    |  感谢每位在我制作本游戏的过程中向我提供帮助的老师和同学、 |\n";
	cout<<"                    |  特别感谢李戈老师和朱元昊助教！                           |\n";
	cout<<"                    |                                                           |\n";
	cout<<"                    |                                                           |\n";
	cout<<"                    |  初次制作游戏，如有任何错漏或不当之处欢迎指出！           |\n";
	cout<<"                    |                                                           |\n";
	cout<<"                    +-----------------------------------------------------------+\n\n\n\n\n请双击鼠标退出游戏. . .\n\n\n\n";
	while(1)//等待鼠标双击界面
	{
		ReadConsoleInput(hIn,&mouseRec,1,&res);
		if(mouseRec.EventType == MOUSE_EVENT&&mouseRec.Event.MouseEvent.dwEventFlags == DOUBLE_CLICK)return;
	}
}//结束gameend函数

//============================================================================================================

int main()
{
	welcome();//打开欢迎界面
	/*time1与进入游戏时的等待时间有关，其值由width确定
	calctime=2^width
	restartorexit为output函数或win函数的返回值，代表继续游戏、重新开局或退出游戏
	inf,sup分别表示符合难度要求的leaststep的下界和上界
	coediff表示难度系数，由n和width确定，初始值设为7，当方块阵的leaststep可取到的最大值越大时，coediff越高
	difftemp为临时变量，在计算coediff时要用
	rantime表示进入游戏时的等待时间，以及每次翻转中的计算时间*/
	int i,j,time1,calctime,restartorexit,inf,sup,coediff,difftemp;
	time_t rantime,startrantime,temprantime;

	while(gamestart())
	{
		//初始化时间，并计算calctime
		timetotal=rantime=0;
		calctime=1;
		for(i=1;i<=width;i++)
			calctime*=2;

		//把读入的difficulty数据先储存在difftemp中，把difficulty定为3，计算coediff
		difftemp=difficulty;
		difficulty=3;

		//若coediffmem[n][width]不等于0，即储存过之前计算的coediff数据，
		//则coediff初始值为coediffmem[n][width]+1（+1是为了避免误差），否则初始值为7
		coediff=coediffmem[n][width]!=0&&coediffmem[n][width]!=7?coediffmem[n][width]+1:7;
		time1=(width<16)?(width<10?100:200):(width<19?300:400);
		i=j=0;
		startrantime=clock();
		
		/*先按difficuly=3和coediff的值计算inf和sup，
		如果得到的leaststep不小于inf，
		那么就复原原来选定的difficulty的值（把difftemp赋值给difficulty）
		再通过coediff计算inf和sup，然后找出一种可行的方块状态
		如果得到的leaststep一直小于inf，
		那么在计算一定时间后，就让coediff减一，继续计算*/
		do
		{
			inf=calcdifficultyinf(coediff);
		    sup=calcdifficultysup(coediff);
		    do
			{
			    randomblock(inf,sup,calctime);
			    temprantime=clock();
			    rantime=temprantime-startrantime;
			    if(int(rantime)/time1>i)//当经过一段时间仍然没有可行解
				{
				    if((++i)%3==2)cout<<'.';//在“数据生成中，请稍候..”后面输出一个'.'
				    if(coediff>1)coediff--;
				    inf=calcdifficultyinf(coediff);
				    sup=calcdifficultysup(coediff);
				}
			    calculate(calctime);//得出leaststep的值
			}while(leaststep<inf||leaststep>sup);
			difficulty=difftemp;//复原difficulty的值
			j++;
		}while(j==1&&difftemp!=3);//此循环只会之行1次或2次

		coediffmem[n][width]=coediff;//将coediff的结果储存在coediffmem[ ][ ]中，以便下次调用
		turn();//将方块状态储存在block[ ][ ]中
		clearscreen();

		//当游戏过程中没有选择“重新开局”或“退出游戏”，在没有获得胜利前，此循环不终止
		do
		{
			//第一次翻转时要输出全部方块，后面的翻转只需输出部分方块
			if(steptotal==0)restartorexit=output(0,calctime);
			else restartorexit=output(-2,calctime);

			//如果点击了“重新开局”或“退出游戏”，那么退出循环
			if(restartorexit>0)
			{
				break;
			}

			startrantime=clock();

			//如果点击了需要翻转的方块，则不需要通过calculate函数进行leaststep和step[ ][ ]的计算，直接进行leatstep--即可
			if(step[laststepx][laststepy]==1||cheat2==1)
			{
				leaststep--;
				step[laststepx][laststepy]=0;
			}

			/*如果没有点击需要翻转的方块，
			则需要通过calculate函数进行计算，
			但因为n=20,18,16且width=20时，
			对每种方块状态都有且只有一种翻转途径，
			所以这三种特殊情况不需要进行calculate过程，
			否则calctime太大，进行calculate很浪费时间*/
			else if((n==20||n==18||n==16)&&width==20)
			{
				leaststep++;
				step[laststepx][laststepy]=1;
			}

			//对于没有点击需要翻转的方块且非以上三种特殊情况，调用calculate函数计算
			else calculate(calctime);

			/*计算每次翻转中在计算中花了多少时间，
			如果时间短于80ms，则屏幕短暂停滞，使出现翻转时的效果*/
			temprantime=clock();
			rantime=temprantime-startrantime;
			if(int(rantime)<80)Sleep(80-int(rantime));

			//判断是否胜利，如果胜利，则判断是再来一句还是退出游戏
			restartorexit=win();
		}while(restartorexit==0);//restartorexit=0表示继续游戏，restartorexit=1表示重新开局，restartorexit=2表示退出游戏

		if(restartorexit==2)break;//退出游戏
	}//结束while(gamestart())循环
	gameend();//打开结束界面
	return 0;
}
