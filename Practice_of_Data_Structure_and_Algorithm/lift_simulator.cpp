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

// 数据结构与算法实习之电梯模拟器
// 文件名：lift_simulator.cpp
// 作者：------
// 日期：2012年10月16日
// 版本：1.5.0
// 编译环境：Microsoft Visual C++ 6.0
// 操作系统：Microsoft Windows XP
// 输入方式：键盘读入数据
// 输出方式：win32控制台界面输出

// 基本功能：
// 1.输出各个乘客的总等候时间、总乘梯时间，每个乘客的人均等候时间、人均乘梯时间，所有电梯的载客量和每辆电梯单位时间内的载客量
// 2.使用win32控制台界面输出电梯运动状态、位置、电梯内乘客数，以及每层楼等待的乘客数
// 扩展功能：
// 1.使用了四种电梯调度算法，能计算各种算法的人均等候时间、载客流量等数据，并依据载客流量对算法优劣进行排序
// 2.输出每辆电梯内的乘客按的按钮、等待的乘客按的按钮、每秒钟每层楼新出现的乘客人数、上电梯的乘客人数、离开电梯的乘客人数
// 3.在给定的人流输入和系统参数下，得出了电梯数量对性能指标的影响（见实习报告）
// 4.在给定的人流输入和系统参数下，得出了高层梯、低层梯对系统性能指标的影响（见实习报告）

#include<iostream>
#include<iomanip>
#include<cmath>
#include<ctime>
#include<vector>
#include<list>
#include<windows.h>
#pragma comment(lib,"winmm.lib")

#define MIN_MAXLOAD 5
#define MAX_MAXLOAD 20 // 每辆电梯的最大负载的范围从5到20
#define MIN_FLOOR_NUM 2
#define MAX_FLOOR_NUM 40 // 楼层总数的范围从2到40
#define MIN_LIFT_NUM 1
#define MAX_LIFT_NUM 13 // 电梯数量的范围从1到13
#define MIN_LAMBDA 0.1
#define MAX_LAMBDA 20 // 泊松参数的范围从0.1到20
#define MAX_NEW_PASSENGER 100 // 每秒新出现的乘客最多为100人
#define MAX_PASSTIME 400 // 快速模拟和演示的最长时间为400秒（这里的秒指电梯模拟器的虚拟时间而非现实时间）
#define NORMAL 0 // 调颜色，下同
#define INFO 1
#define BLANK 2
#define DOWN 3
#define UP 4
#define OPEN 5
#define BUTTON 6
#define TABLE 7
#define SCREEN_WIDTH 168 // 屏幕宽度
#define SCREEN_HEIGHT 57 // 屏幕高度
#define WINDOW_WIDTH 167 // 窗口宽度
#define WINDOW_HEIGHT 43 // 窗口高度
#define SECONDS_PER_HOUR 3600.0

using namespace std;

double maximum(double,double); 
// 计算两个数的最大值，两个参数为x和y，返回x和y的最大值
double minimum(double,double); 
// 计算两个数的最小值，两个参数为x和y，返回x和y的最小值
double average(double,double); 
// 计算两个数的平均值，两个参数为x和y，返回x和y的平均值
double poisson(double,int); 
// 计算泊松分布的概率值，第一个参数为λ，第二个参数为k，返回exp(-λ)*(λ^k)/(k!)
void play(int); 
// 播放音乐，参数为0到17的整数，参数为0时不播放，参数非0时播放某一频率的音
void calcpk(double,double[]); 
// 计算每一秒钟新增的乘客的人数的概率值，第一个参数为λ，将计算后的结果保存在第二个参数的数组中
int createpassenger(int,int,double[]); 
// 根据概率值产生新乘客，第一个参数为楼层总数，第二个参数为此前的乘客总人数，第三个参数为之前计算的概率值，返回值为之后的乘客总人数
int recreatepassenger(int,int[]); 
// 在不同算法的快速模拟和各个算法的演示中重新产生之前产生的乘客流，第一个参数为此前的乘客总人数，第二个参数为每层楼新增的乘客人数，返回值为之后的乘客总人数
void calc(int);
// 计算总等待时间、客流量等各个数据，参数为当前的乘客总人数
void print(int,int,int,int,int,int[],int[],int[]);
// 输出函数，参数依次为输出状态、采用的算法、楼层总数、电梯总数、当前乘客总人数、每层楼新增的乘客人数、每层楼离开的乘客人数、每层楼上电梯的乘客人数
void goonlift(int,int,int,int[],list<int>::iterator);
// 令乘客上电梯，参数依次为电梯序号、所在楼层、乘客序号、每层楼上电梯的乘客人数、每层楼等待的乘客序列的迭代器
void operate(int,int,int,int,int[],int[]);
// 调度电梯，参数依次为采用的算法、楼层总数、电梯总数、电梯最大负载、每层楼上电梯的乘客人数、每层楼离开的乘客人数
void setcolor(int);
// 改变背景颜色和字体颜色，参数为颜色方案
void clearscreen(int,int,int,double);
// 在每次演示前清屏，参数依次为楼层总数、电梯总数、电梯最大负载、泊松参数λ

class PASSENGER
{
public:
	int from,to,createtime,onlifttime,leavetime;
	// from表示乘客在哪层楼出现，to表示乘客的目的楼层，createtime表示乘客出现的时间，onlifttime表示乘客进电梯的时间，leavetime表示乘客离开电梯的时间
	PASSENGER(const int f,const int t,const int c)
	{
		from=f;
		to=t;
		createtime=c;
		onlifttime=leavetime=0;
	}
	void init()
	{
		onlifttime=leavetime=0;
	}
};

class FLOOR
{
public:
	list<int> lstp;
	// lstp记录在该层楼等待的乘客的序号
	void init()
	{
		lstp.clear();
	}
};

class LIFT
{
public:
	bool stop[MAX_FLOOR_NUM+1];
	// stop表示电梯是否应该在某层楼停（也可理解为电梯内的乘客是否按了某层楼的按钮）
	int flor,target,formerfloor;
	// flor表示电梯当前所在楼层，target为电梯的目的楼层，formerfloor表示电梯在上一秒所在的楼层
	list<int> lstp;
	// lstp记录在该电梯内的乘客的序号
	LIFT()
	{
		target=formerfloor=0;
		int i;
		for(i=1;i<=MAX_FLOOR_NUM;i++)
			stop[i]=0;
	}
	void init()
	{
		lstp.clear();
		target=formerfloor=0;
		int i;
		for(i=1;i<=MAX_FLOOR_NUM;i++)
			stop[i]=0;
	}
};

HANDLE hOut; // 输出句柄
COORD pos={0,0}; // 光标位置
HMIDIOUT handle; // 音频输出句柄
unsigned long result=midiOutOpen(&handle,0,0,0,CALLBACK_NULL); // 获取音频输出句柄
vector<PASSENGER> passenger; // 乘客序列
FLOOR flor[MAX_FLOOR_NUM+1]; // 楼层
LIFT lift[MAX_LIFT_NUM+1]; // 电梯
int passtime=0,passengerwaiting=0,passengerin=0,passengerout=0,totalwaitingtime=0,totalduration=0;
// passtime:当前时间 passengerwaiting:正在等候的乘客人数 passengerin:正在乘梯的乘客人数 passengerout:已离开电梯的乘客人数
double averagewaitingtime,averageduration;
// totalwaitingtime:所有乘客等待电梯总时间 averagewaitingtime:人均等待电梯时间 totalduration:所有乘客乘梯总时间 averageduration:人均乘梯时间

double maximum(const double x,const double y)
{
	if(x>=y)return x;
	return y;
}

double minimum(const double x,const double y)
{
	if(x<=y)return x;
	return y;
}

double average(const double x,const double y)
{
	return (x+y)/2.0;
}

double poisson(const double x,const int y)
{
	double r=exp(-x);
	int i;
	for(i=1;i<=y;i++)
	{
		r*=x;
		r/=i;
	}
	return r;
}

void calcpk(const double lambda,double pk[])
{
	double sum=0;
	int i;
	for(i=0;i<=MAX_NEW_PASSENGER;i++)
	{
		pk[i]=poisson(lambda,i);
		sum+=pk[i];
	}
	for(i=0;i<=MAX_NEW_PASSENGER;i++)
	{
		pk[i]/=sum; // 令各个概率值相加为1
		if(i>0)pk[i]+=pk[i-1]; // 为方便随机处理，将概率值累加起来
	}
}

int createpassenger(const int floornum,int passengernum,double pk[])
{
	int i,j;
	double pr=rand()%30000/30000.0;
	for(i=0;i<=MAX_NEW_PASSENGER;i++)
	{
		if(pk[i]>pr) // 当随机数落在某一区间时，就增加该区间对应的人数
		{
			for(j=1;j<=i;j++)
			{
				int f=rand()%floornum+1,t=rand()%floornum+1;
				while(f==t) // 避免目的地和“出生”楼层一样
					t=rand()%floornum+1;
				passenger.push_back(PASSENGER(f,t,passtime));
				passengernum++;
			}
			return passengernum;
		}
	}
	return passengernum;
}

int recreatepassenger(int passengernum,int add[])
{
	int i;
	for(i=passengernum; ;i++)
	{
		if(i>=passenger.size())return passengernum;
		if(passenger[i].createtime==passtime)
		{
			passenger[i].init();
			flor[passenger[i].from].lstp.push_back(passengernum);
			add[passenger[i].from]++;
			passengernum++;
		}
		else return passengernum;
	}
	return passengernum;
}

void calc(const int passengernum)
{
	passengerwaiting=passengerin=passengerout=totalwaitingtime=totalduration=0;
	int i;
	for(i=0;i<passengernum;i++)
	{
		if(!passenger[i].onlifttime)passengerwaiting++;
		else if(!passenger[i].leavetime)
		{
			passengerin++;
			totalwaitingtime+=passenger[i].onlifttime-passenger[i].createtime;
		}
		else
		{
			passengerout++;
			totalduration+=passenger[i].leavetime-passenger[i].onlifttime;
			totalwaitingtime+=passenger[i].onlifttime-passenger[i].createtime;
		}
	}
	averagewaitingtime=double(totalwaitingtime)/(passengerin+passengerout);
	if(passengerin+passengerout==0)averagewaitingtime=0;
	averageduration=double(totalduration)/passengerout;
	if(passengerout==0)averageduration=0;
}

void play(int c)
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

void print(const int t,const int opt,const int floornum,const int liftnum,const int passengernum,int add[],int leave[],int onlift[])
{
	int i,j,sleeptime;
	list<int>::iterator iter;
	time_t tmptime=clock();
	if(t==1) // 输出每层楼新出现的乘客人数，不输出上电梯的人数和离开的人数
	{
		for(i=2;i<=floornum+1;i++)
		{
			pos.X=29+10*liftnum;
			pos.Y=i;
			setcolor(NORMAL);
			SetConsoleCursorPosition(hOut,pos);
			cout<<"    ";
			pos.X=23;
			SetConsoleCursorPosition(hOut,pos);
			cout<<"    ";
			pos.X=5;
			SetConsoleCursorPosition(hOut,pos);
			if(add[floornum+2-i])
			{
				if(add[floornum+2-i]<100)cout<<setw(3)<<add[floornum+2-i];
				else cout<<setw(4)<<add[floornum+2-i];
			}
			else cout<<"    ";
		}
	}
	else // 输出每层楼上电梯的乘客人数和离开的乘客人数，不输出新出现的乘客人数
	{
		for(i=2;i<=floornum+1;i++)
		{
			pos.X=5;
			pos.Y=i;
			setcolor(NORMAL);
			SetConsoleCursorPosition(hOut,pos);
			cout<<"    ";
			pos.X=29+10*liftnum;
			SetConsoleCursorPosition(hOut,pos);
			if(leave[floornum+2-i])
			{
				if(leave[floornum+2-i]<100)cout<<setw(3)<<leave[floornum+2-i];
				else cout<<setw(4)<<leave[floornum+2-i];
			}
			else cout<<"    ";
			pos.X=23;
			SetConsoleCursorPosition(hOut,pos);
			if(onlift[floornum+2-i])
			{
				if(onlift[floornum+2-i]<100)cout<<setw(3)<<onlift[floornum+2-i];
				else cout<<setw(4)<<onlift[floornum+2-i];
			}
			else cout<<"    ";
			pos.X=17;
			SetConsoleCursorPosition(hOut,pos);
			bool tup=0,tdown=0;
			for(iter=flor[floornum+2-i].lstp.begin();iter!=flor[floornum+2-i].lstp.end();iter++)
			{
				if(passenger[*iter].to>floornum+2-i)tup=1;
				else if(passenger[*iter].to<floornum+2-i)tdown=1;
				if(tup&&tdown||(tup&&i==floornum+1)||(tdown&&i==2))break;
			}
			if(tup)cout<<"上";
			else cout<<"  ";
			if(tdown)cout<<"下";
			else cout<<"  ";
		}
	}
	for(i=2;i<=floornum+1;i++) // 输出每层楼正在等候的乘客人数
	{
		pos.X=11;
		pos.Y=i;
		SetConsoleCursorPosition(hOut,pos);
		if(flor[floornum+2-i].lstp.size()<100)cout<<setw(3)<<flor[floornum+2-i].lstp.size();
		else cout<<setw(4)<<flor[floornum+2-i].lstp.size();
	}
	for(i=1;i<=liftnum;i++) // 输出每辆电梯的运动状态、运载的乘客人数等
	{
		if(t==1&&!lift[i].formerfloor)
		{
			setcolor(NORMAL);
			pos.X=19+10*i;
			pos.Y=floornum+2-lift[i].flor;
			SetConsoleCursorPosition(hOut,pos);
			cout<<"【"<<setw(2)<<lift[i].lstp.size()<<"】  ";
		}
		else if(t==2)
		{
			setcolor(NORMAL);
			pos.X=19+10*i;
			pos.Y=floornum+2-lift[i].formerfloor;
			SetConsoleCursorPosition(hOut,pos);
			cout<<"        ";
			pos.Y=floornum+2-lift[i].flor;
			SetConsoleCursorPosition(hOut,pos);
			if(lift[i].flor==lift[i].formerfloor)
			{
				setcolor(OPEN);
				cout<<"【"<<setw(2)<<lift[i].lstp.size()<<"】開";
				setcolor(BUTTON);
				for(j=1;j<=floornum;j++)
				{
					if(lift[i].flor==j)continue;
					if(lift[i].stop[j])
					{
						pos.Y=floornum+2-j;
						SetConsoleCursorPosition(hOut,pos);
						cout<<setw(5)<<j;
					}
				}
			}
			else if(lift[i].flor<lift[i].formerfloor)
			{
				setcolor(DOWN);
				cout<<"【"<<setw(2)<<lift[i].lstp.size()<<"】↓";
			}
			else
			{
				setcolor(UP);
				cout<<"【"<<setw(2)<<lift[i].lstp.size()<<"】↑";
			}
			setcolor(NORMAL);
		}
	}
	pos.X=0;
	pos.Y=floornum+3;
	SetConsoleCursorPosition(hOut,pos); // 输出当前时间等数据
	setcolor(INFO);
	cout<<"   当前算法："<<setw(7)<<opt<<"   ";
	cout<<"     当前时间："<<setw(9)<<passtime<<" s  ";
	cout<<"乘客总人数："<<setw(9)<<passengernum<<"  ";
	cout<<"    等候总人数："<<setw(9)<<passengerwaiting<<"  ";
	cout<<"  乘梯总人数："<<setw(6)<<passengerin<<"　    ";
	cout<<"  离开总人数："<<setw(6)<<passengerout<<"  "<<endl;
	cout<<" 总等候时间："<<setw(7)<<totalwaitingtime<<" s  ";
	cout<<"人均等候时间："<<setw(9)<<averagewaitingtime<<" s  ";
	cout<<"总乘梯时间："<<setw(9)<<totalduration<<" s  ";
	cout<<"人均乘梯时间："<<setw(9)<<averageduration<<" s  ";
	cout<<"每辆电梯载客流量："<<setw(9)<<SECONDS_PER_HOUR*passengerout/liftnum/passtime<<" 人/小时                  ";
	setcolor(NORMAL);
	//play(rand()%17);
	sleeptime=100*t-clock()+tmptime;
	if(sleeptime>0)Sleep(sleeptime); // 略停顿一下
}

void goonlift(const int liftid,const int floorid,const int passengerid,int onlift[],list<int>::iterator iter)
{
	passenger[passengerid].onlifttime=passtime;
	lift[liftid].lstp.push_back(passengerid); // 将该乘客加入电梯的乘客序列中
	lift[liftid].stop[passenger[passengerid].to]=1;
	onlift[floorid]++;
	flor[floorid].lstp.erase(iter); // 将该乘客从该楼层的等待序列中删除
}

void operate(const int t,const int floornum,const int liftnum,const int maxload,int onlift[],int leave[])
{
	// 对于所有算法：认为在1秒内，电梯要么往上，要么往下，要么开门
	// 对于所有算法：当电梯所在的楼层有乘客要离开，必须开门放人
	// 算法1：当等候的乘客的目的地与电梯内的乘客的目的地有重复时，或者等候者的目的地更远时，才允许停下载人的电梯并开门让乘客进入
	// 算法2：只要乘客的目的地方向（上/下）与电梯的运动方向一致，即开门让乘客进入
	// 算法3：只要有乘客在等候，无论运动方向，都开门让乘客进入
	// 算法4：在开门的选择上和算法2一致，不同之处在于无论电梯是否载人都上到最顶层或下到最底层才反向
	int i,j,k,target[MAX_FLOOR_NUM+1]={0},fartarget;
	bool opendoor[MAX_LIFT_NUM+1]={0},move[MAX_LIFT_NUM+1]={0}; // opendoor标记电梯是否开门，move标记电梯是否移动
	list<int>::iterator iter,iter2;
	for(i=1;i<=floornum;i++)
		onlift[i]=leave[i]=0;
	for(i=1;i<=liftnum;i++)
		lift[i].formerfloor=lift[i].flor;
	if(t==4&&passtime==1) // 算法4：直上直下
	{
		for(i=1;i<=liftnum;i++)
			if(i%2)lift[i].target=floornum;
			else lift[i].target=1;
	}
	for(i=1;i<=liftnum;i++) // 当电梯内有乘客在该层楼需要离开时，即开门
	{
		j=lift[i].flor;
		if(lift[i].stop[j])
		{
			opendoor[i]=1;
			lift[i].stop[j]=0;
			for(iter=lift[i].lstp.begin();iter!=lift[i].lstp.end(); )
			{
				k=*iter;
				iter2=iter;
				iter++;
				if(passenger[k].to==j)
				{
					passenger[k].leavetime=passtime;
					lift[i].lstp.erase(iter2);
					leave[j]++;
				}
			}
			if(t!=4) // 非算法4：电梯暂时没有目的楼层
			{
				if(lift[i].lstp.empty())lift[i].target=0;
				else if(lift[i].target==j)
				{
					lift[i].target=0;
					for(iter=lift[i].lstp.begin();iter!=lift[i].lstp.end();iter++)
					{
						k=*iter;
						if(!lift[i].target)lift[i].target=passenger[k].to;
						else if(abs(passenger[k].to-j)>abs(lift[i].target-j))lift[i].target=passenger[k].to;
					}
				}
			}
		}
	}
	for(i=1;i<=liftnum;i++) // 当电梯为空且有人在电梯所在楼层等候时，开门让乘客进入
	{
		if(t==4&&lift[i].lstp.empty()&&!flor[lift[i].flor].lstp.empty()) // 算法4：让同向的乘客进入
		{
			j=lift[i].flor;
			for(iter=flor[j].lstp.begin();iter!=flor[j].lstp.end(); )
			{
				k=*iter;
				iter2=iter;
				iter++;
				if((passenger[k].to-j)*(lift[i].target-j)>0)
				{
					goonlift(i,j,k,onlift,iter2);
				}
				if(lift[i].lstp.size()>=maxload)break;
			}
		}
		else if(lift[i].lstp.empty()&&!flor[lift[i].flor].lstp.empty()) // 非算法4：让同一方向更多的乘客进入
		{
			int upnum=0,up2=0,downnum=0,down2=0;
			j=lift[i].flor;
			for(iter=flor[j].lstp.begin();iter!=flor[j].lstp.end();iter++)
			{
				k=*iter;
				if(passenger[k].to>j)
				{
					upnum++;
					up2+=passenger[k].to-j;
				}
				else
				{
					downnum++;
					down2+=j-passenger[k].to;
				}
			}
			if(upnum>downnum||(upnum==downnum&&up2>down2))
			{
				fartarget=0;
				for(iter=flor[j].lstp.begin();iter!=flor[j].lstp.end(); )
				{
					k=*iter;
					iter2=iter;
					iter++;
					if(passenger[k].to>j&&lift[i].lstp.size()<maxload)
					{
						goonlift(i,j,k,onlift,iter2);
						if(passenger[k].to>fartarget)fartarget=passenger[k].to;
					}
					if(lift[i].lstp.size()==maxload)break;
				}
				if(fartarget>lift[i].target||!lift[i].target)lift[i].target=fartarget;
			}
			else
			{
				fartarget=MAX_FLOOR_NUM;
				for(iter=flor[j].lstp.begin();iter!=flor[j].lstp.end(); )
				{
					k=*iter;
					iter2=iter;
					iter++;
					if(passenger[k].to<j&&lift[i].lstp.size()<maxload)
					{
						goonlift(i,j,k,onlift,iter2);
						if(passenger[k].to<fartarget)fartarget=passenger[k].to;
					}
					if(lift[i].lstp.size()>=maxload)break;
				}
				if(fartarget<lift[i].target||!lift[i].target)lift[i].target=fartarget;
			}
			if(t==3&&lift[i].lstp.size()<maxload&&!flor[j].lstp.empty()) // 算法3：允许反向的乘客也进入
			{
				for(iter=flor[j].lstp.begin();iter!=flor[j].lstp.end(); )
				{
					k=*iter;
					iter2=iter;
					iter++;
					goonlift(i,j,k,onlift,iter2);
					if(lift[i].lstp.size()>=maxload)break;
				}
			}
			opendoor[i]=1;
		}
	}
	for(i=1;i<=liftnum;i++) // 当空电梯之前没有开门时，运动去其他楼层
	{
		if(t==4&&!opendoor[i]&&lift[i].lstp.empty()) // 算法4：直上直下
		{
			if(lift[i].target>lift[i].flor)
			{
				lift[i].flor++;
				move[i]=1;
				if(lift[i].flor==floornum)lift[i].target=1;
			}
			else
			{
				lift[i].flor--;
				move[i]=1;
				if(lift[i].flor==1)lift[i].target=floornum;
			}
		}
		else if(!opendoor[i]&&lift[i].lstp.empty()) // 非算法4：根据其他楼层等候的乘客情况选择往上或往下
		{
			k=0;
			for(j=1;j<=floornum;j++)
			{
				if(!flor[j].lstp.empty()&&target[j]<2)
				{
					if(!k)k=j;
					else if(abs(lift[i].flor-j)<abs(lift[i].flor-k))k=j;
				}
			}
			lift[i].target=k;
			target[k]++;
			if(lift[i].target>lift[i].flor||(!lift[i].target&&floornum/2+1>lift[i].flor))
			{
				lift[i].flor++;
				move[i]=1;
			}
			else if((lift[i].target&&lift[i].target<lift[i].flor)||(!lift[i].target&&floornum/2+1<lift[i].flor))
			{
				lift[i].flor--;
				move[i]=1;
			}
			else
			{
				move[i]=0;
			}
		}
	}
	for(i=1;i<=liftnum;i++) // 非空电梯选择开门或者移动
	{
		j=lift[i].flor;
		if(!move[i]&&!lift[i].lstp.empty())
		{
			if(!flor[j].lstp.empty()&&lift[i].lstp.size()<maxload)
			{
				for(iter=flor[j].lstp.begin();iter!=flor[j].lstp.end(); )
				{
					k=*iter;
					iter2=iter;
					iter++;
					if(t==1)
					{
						if(lift[i].stop[passenger[k].to]&&lift[i].lstp.size()<maxload)
						{
							goonlift(i,j,k,onlift,iter2);
							opendoor[i]=1;
						}
					}
					else
					{
						if((passenger[k].to-j)*(lift[i].target-j)>0&&lift[i].lstp.size()<maxload)
						{
							goonlift(i,j,k,onlift,iter2);
							opendoor[i]=1;
							if((passenger[k].to-lift[i].target)*(lift[i].target-j)>0)lift[i].target=passenger[k].to;
						}
					}
					if(lift[i].lstp.size()>=maxload)break;
				}
				if(t==1&&lift[i].lstp.size()<maxload&&opendoor[i])
				{
					for(iter=flor[j].lstp.begin();iter!=flor[j].lstp.end(); )
					{
						k=*iter;
						iter2=iter;
						iter++;
						if(((passenger[k].to-lift[i].target)*(lift[i].target-j)>0)&&lift[i].lstp.size()<maxload)
						{
							goonlift(i,j,k,onlift,iter2);
							opendoor[i]=1;
							lift[i].target=passenger[k].to;
						}
						if(lift[i].lstp.size()>=maxload)break;
					}
				}
				else if(t==3&&lift[i].lstp.size()<maxload&&opendoor[i]&&!flor[j].lstp.empty())
				{
					for(iter=flor[j].lstp.begin();iter!=flor[j].lstp.end(); )
					{
						k=*iter;
						iter2=iter;
						iter++;
						goonlift(i,j,k,onlift,iter2);
						opendoor[i]=1;
						if((passenger[k].to-lift[i].target)*(lift[i].target-j)>0)lift[i].target=passenger[k].to;
						if(lift[i].lstp.size()>=maxload)break;
					}
				}
			}
		}
		if(!move[i]&&!opendoor[i]&&!lift[i].lstp.empty())
		{
			if(lift[i].target==j)
			{
				if(t==4)
				{
					if(lift[i].target==1)lift[i].target=floornum;
					else lift[i].target=1;
				}
				else
				{
					for(iter=lift[i].lstp.begin();iter!=lift[i].lstp.end();iter++)
					{
						k=*iter;
						if(abs(passenger[k].to-j)>abs(lift[i].target-j))lift[i].target=passenger[k].to;
					}
				}
			}
			if(lift[i].target>j)
			{
				lift[i].flor++;
				move[i]=1;
				if(lift[i].flor==floornum)lift[i].target=1;
			}
			else if(lift[i].target<j)
			{
				lift[i].flor--;
				move[i]=1;
				if(lift[i].flor==1)lift[i].target=floornum;
			}
			else
			{
				move[i]=0;
			}
		}
	}
}

void setcolor(const int i)
{
	if(i==NORMAL)SetConsoleTextAttribute(hOut,FOREGROUND_INTENSITY|FOREGROUND_GREEN|FOREGROUND_RED|FOREGROUND_BLUE);// 黑底白字
	else if(i==INFO)SetConsoleTextAttribute(hOut,BACKGROUND_INTENSITY|BACKGROUND_GREEN|BACKGROUND_RED|BACKGROUND_BLUE);// 白底黑字
	else if(i==BLANK)SetConsoleTextAttribute(hOut,BACKGROUND_GREEN|BACKGROUND_RED);// 棕底黑字
	else if(i==DOWN)SetConsoleTextAttribute(hOut,BACKGROUND_INTENSITY|BACKGROUND_BLUE|FOREGROUND_INTENSITY|FOREGROUND_GREEN|FOREGROUND_RED|FOREGROUND_BLUE);// 蓝底白字
	else if(i==UP)SetConsoleTextAttribute(hOut,BACKGROUND_INTENSITY|BACKGROUND_RED|FOREGROUND_INTENSITY|FOREGROUND_GREEN|FOREGROUND_RED|FOREGROUND_BLUE);// 红底白字
	else if(i==OPEN)SetConsoleTextAttribute(hOut,BACKGROUND_GREEN|FOREGROUND_INTENSITY|FOREGROUND_GREEN|FOREGROUND_RED|FOREGROUND_BLUE);// 绿底白字
	else if(i==BUTTON)SetConsoleTextAttribute(hOut,FOREGROUND_INTENSITY|FOREGROUND_GREEN|FOREGROUND_RED);// 黑底黄字
	else if(i==TABLE)SetConsoleTextAttribute(hOut,BACKGROUND_BLUE|BACKGROUND_RED|FOREGROUND_INTENSITY|FOREGROUND_GREEN|FOREGROUND_RED|FOREGROUND_BLUE);// 紫底白字
}

void clearscreen(const int floornum,const int liftnum,const int maxload,const double lambda)
{
	system("cls");
	pos.X=pos.Y=0;
	SetConsoleCursorPosition(hOut,pos);
	setcolor(NORMAL);
	cout<<" Ｆ";
	setcolor(BLANK);
	cout<<"︱";
	setcolor(NORMAL);
	cout<<"新增";
	setcolor(BLANK);
	cout<<"︱";
	setcolor(NORMAL);
	cout<<"等候";
	setcolor(BLANK);
	cout<<"︱";
	setcolor(NORMAL);
	cout<<"按钮";
	setcolor(BLANK);
	cout<<"︱";
	setcolor(NORMAL);
	cout<<"上梯";
	setcolor(BLANK);
	cout<<"︱";
	int i,j;
	for(i=1;i<=liftnum;i++)
	{
		setcolor(NORMAL);
		cout<<" 电梯"<<setw(2)<<i<<" ";
		setcolor(BLANK);
		cout<<"︱";
	}
	setcolor(NORMAL);
	cout<<"离开"<<endl;
	setcolor(BLANK);
	for(i=1;i<=33+10*liftnum;i++)
		cout<<"-";
	setcolor(NORMAL);
	cout<<endl;
	for(i=floornum;i>=1;i--)
	{
		cout<<" "<<setw(2)<<i;
		setcolor(BLANK);
		cout<<"︱";
		setcolor(NORMAL);
		cout<<"    ";
		setcolor(BLANK);
		cout<<"︱";
		setcolor(NORMAL);
		cout<<"    ";
		setcolor(BLANK);
		cout<<"︱";
		setcolor(NORMAL);
		cout<<"    ";
		setcolor(BLANK);
		cout<<"︱";
		setcolor(NORMAL);
		cout<<"    ";
		setcolor(BLANK);
		cout<<"︱";
		for(j=1;j<=liftnum;j++)
		{
			setcolor(NORMAL);
			cout<<"        ";
			setcolor(BLANK);
			cout<<"︱";
		}
		setcolor(NORMAL);
		cout<<endl;
	}
	setcolor(BLANK);
	for(i=1;i<=33+10*liftnum;i++)
		cout<<"-";
	setcolor(NORMAL);
	cout<<endl;
	setcolor(INFO);
	for(i=1;i<=SCREEN_WIDTH*2;i++)
		cout<<" ";
	cout<<"     总层数："<<setw(7)<<floornum<<"  ";
	cout<<"      电梯数量："<<setw(9)<<liftnum<<"  ";
	cout<<"    最大负载："<<setw(9)<<maxload<<"  ";
	cout<<"    泊松参数λ："<<setw(9)<<lambda<<"  ";
	cout<<"  最终乘客总数："<<setw(6)<<passenger.size()<<"  ";
	cout<<"    总模拟时间："<<setw(6)<<MAX_PASSTIME<<" s                   ";
	setcolor(NORMAL);
}

int main()
{
	int passengernum=0,liftnum,floornum,maxload,i,j,k,opt,maxtwt,mintwt,maxtd,mintd,maxpass,minpass,order[5]={0,1,2,3,4};
	int totalwt[5],totald[5],passout[5],add[MAX_FLOOR_NUM+1]={0},leave[MAX_FLOOR_NUM+1],onlift[MAX_FLOOR_NUM+1];
	// passengernum:当前出现过的乘客总人数 liftnum:电梯数量 floornum:楼层总数 maxload:最大负载 opt:选择的算法
	// maxtwt:最长总等待时间 mintwt:最短总等待时间 maxtd:最长总乘梯时间 mintd:最短总乘梯时间 maxpass:最大总客流量 minpass:最小总客流量
	// order:用于算法优劣排序 totalwt:各种算法的总等待时间 totald:各种算法的总乘梯时间 passout:各种算法的总客流量
	// add:某一秒各层楼新出现的乘客人数 leave:某一秒各层楼离开电梯的乘客人数 onlift:某一秒各层楼上电梯的乘客人数
	double lambda,avgwt[5],avgd[5],passperhour[5],avgtwt,avgtd,avgpass,avgawt,avgad,avgpph,maxawt,minawt,maxad,minad,maxpph,minpph;
	double pk[MAX_NEW_PASSENGER+1];
	// lambda:泊松参数λ avgwt:各种算法的人均等待时间 avgd:各种算法的人均乘梯时间 passperhour:各种算法的每辆电梯单位时间载客流量 pk:新出现的乘客数的各概率值
	// avgtwt:平均总等待时间 avgtd:平均总乘梯时间 avgpass:平均总客流量 avgawt:平均人均等待时间 avgad:平均人均乘梯时间 avgpph:平均载客流量
	// max_/min_:上一行各数据的最大/最小值
	time_t starttime;
	srand(unsigned(time(0))); // 初始化随机种子
	hOut=GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTitle("电梯模拟器"); // 设定窗口标题
	COORD size={SCREEN_WIDTH,SCREEN_HEIGHT};
	SetConsoleScreenBufferSize(hOut,size);
	SMALL_RECT rc={0,0,WINDOW_WIDTH,WINDOW_HEIGHT};
	SetConsoleWindowInfo(hOut,true,&rc); // 设定窗口大小
	HWND hwnd=GetForegroundWindow();
	ShowWindow(hwnd,SW_MAXIMIZE); // 令窗口最大化
	cout<<setprecision(7);
	cout<<"请依次输入最大负载(5-20)、总电梯数(1-13)、总楼层数(2-40)、泊松过程参数λ(0.1-20)\n";
	cin>>maxload>>liftnum>>floornum>>lambda; // 依次输入最大负载、电梯总数、楼层总数、泊松过程参数λ
	CONSOLE_CURSOR_INFO ConsoleCursorInfo;
    GetConsoleCursorInfo(hOut,&ConsoleCursorInfo);
    ConsoleCursorInfo.bVisible=false;
    SetConsoleCursorInfo(hOut,&ConsoleCursorInfo); // 设定不显示光标
	if(maxload<MIN_MAXLOAD||maxload>MAX_MAXLOAD||liftnum<MIN_LIFT_NUM||liftnum>MAX_LIFT_NUM||floornum<MIN_FLOOR_NUM||floornum>MAX_FLOOR_NUM||lambda<MIN_LAMBDA||lambda>MAX_LAMBDA)
	{
		cout<<"数据不合适，出错啦！一秒后自动退出程序"; // 数据超出范围或输入不规范
		Sleep(1000);
		return 0;
	}
	cout<<endl<<"正在生成数据，请稍候......"<<endl;
	calcpk(lambda,pk); // 计算各概率值
	while(passtime<MAX_PASSTIME)
	{
		passtime++;
		passengernum=createpassenger(floornum,passengernum,pk); // 生成乘客流
	}
	cout<<endl<<"正在快速模拟，请稍候......"<<endl;
	starttime=clock();
	for(i=1;i<=8;i++) // i<=4时为各种算法的快速模拟，i>=5时为各种算法从优到劣的演示
	{
		for(j=1;j<=liftnum;j++)
		{
			lift[j].init();
			lift[j].flor=floornum/2+1;
		}
		for(j=1;j<=floornum;j++)
			flor[j].init();
		passtime=passengernum=passengerwaiting=passengerin=passengerout=totalwaitingtime=totalduration=0;
		if(i<=4)opt=i;
		else opt=order[i-4];
		while(passtime<MAX_PASSTIME)
		{
			if(i<=4&&clock()-starttime>8000) // 此前调试时曾出错，现在不存在此错误
			{
				cout<<endl<<"数据不合适，出错啦！一秒后自动退出程序"<<endl;
				Sleep(1000);
				return 0;
			}
			passtime++;
			for(j=1;j<=floornum;j++)
				add[j]=0;
			passengernum=recreatepassenger(passengernum,add); // 重新生成乘客流
			if(i>=5)
			{
				calc(passengernum);
				print(1,opt,floornum,liftnum,passengernum,add,leave,onlift); // 演示时输出界面
			}
			operate(opt,floornum,liftnum,maxload,onlift,leave); // 调度电梯
			if(i>=5)
			{
				calc(passengernum);
				print(2,opt,floornum,liftnum,passengernum,add,leave,onlift); // 演示时输出界面
			}
		}
		calc(passengernum);
		if(i<=4)
		{
			passout[i]=passengerout;
			passperhour[i]=SECONDS_PER_HOUR*passengerout/liftnum/passtime;
			totalwt[i]=totalwaitingtime;
			totald[i]=totalduration;
			avgwt[i]=averagewaitingtime;
			avgd[i]=averageduration;
		}
		if(i==4) // 计算各参数的最优值、最劣值、平均值，并依据各种算法的载客量对各种算法进行优劣排序
		{
			for(j=1;j<=4;j++)
				for(k=j+1;k<=4;k++) // 对算法优劣进行排序（由于数据规模很小所以采用冒泡法）
				{
					if(passout[order[j]]<passout[order[k]]||(passout[order[j]]==passout[order[k]]&&avgwt[order[j]]+avgd[order[j]]>avgwt[order[k]]+avgd[order[k]]))
					{
						order[j]^=order[k];
						order[k]^=order[j];
						order[j]^=order[k];
					}
				}
			// 计算各数据的平均值、最大值、最小值
			avgpass=average(average(passout[1],passout[2]),average(passout[3],passout[4]));
			avgtwt=average(average(totalwt[1],totalwt[2]),average(totalwt[3],totalwt[4]));
			avgtd=average(average(totald[1],totald[2]),average(totald[3],totald[4]));
			avgawt=average(average(avgwt[1],avgwt[2]),average(avgwt[3],avgwt[4]));
			avgad=average(average(avgd[1],avgd[2]),average(avgd[3],avgd[4]));
			avgpph=average(average(passperhour[1],passperhour[2]),average(passperhour[3],passperhour[4]));
			maxtwt=maximum(maximum(totalwt[1],totalwt[2]),maximum(totalwt[3],totalwt[4]));
			maxtd=maximum(maximum(totald[1],totald[2]),maximum(totald[3],totald[4]));
			maxawt=maximum(maximum(avgwt[1],avgwt[2]),maximum(avgwt[3],avgwt[4]));
			maxad=maximum(maximum(avgd[1],avgd[2]),maximum(avgd[3],avgd[4]));
			maxpph=maximum(maximum(passperhour[1],passperhour[2]),maximum(passperhour[3],passperhour[4]));
			mintwt=minimum(minimum(totalwt[1],totalwt[2]),minimum(totalwt[3],totalwt[4]));
			mintd=minimum(minimum(totald[1],totald[2]),minimum(totald[3],totald[4]));
			minawt=minimum(minimum(avgwt[1],avgwt[2]),minimum(avgwt[3],avgwt[4]));
			minad=minimum(minimum(avgd[1],avgd[2]),minimum(avgd[3],avgd[4]));
			minpph=minimum(minimum(passperhour[1],passperhour[2]),minimum(passperhour[3],passperhour[4]));
		}
		if(i==1)
		{
			maxpass=minpass=passengerout;
		}
		else if(i==2||i==3||i==4)
		{
			if(passengerout>maxpass)
			{
				maxpass=passengerout;
			}
			else if(passengerout<minpass)
			{
				minpass=passengerout;
			}
		}
		if(i>=4)
		{
			if(i!=4)
			{
				setcolor(INFO);
				pos.X=43;
				pos.Y=17;
				SetConsoleCursorPosition(hOut,pos);
				for(j=1;j<=72;j++)
					cout<<" ";
				pos.Y+=2;
				SetConsoleCursorPosition(hOut,pos);
				for(j=1;j<=72;j++)
					cout<<" ";
				pos.Y--;
				if(i==8)
				{
					for(j=3;j>=1;j--)
					{
						SetConsoleCursorPosition(hOut,pos);
						cout<<"         "<<j<<"     秒    后    自    动    退    出    程    序             ";
						Sleep(1000);
					}
					return 0;
				}
				else
				{
					for(j=3;j>=1;j--)
					{
						SetConsoleCursorPosition(hOut,pos);
						cout<<"         "<<j<<"     秒    后    模    拟    下    一    种    算    法       ";
						Sleep(1000);
					}
				}
			}
			setcolor(NORMAL);
			clearscreen(floornum,liftnum,maxload,lambda);
			setcolor(TABLE);
			for(j=1;j<=SCREEN_WIDTH;j++)
				cout<<" ";
			cout<<"             总客流量       每辆电梯载客流量(人/小时)           总等候时间(s)        人均等候时间(s)          总乘梯时间(s)        人均乘梯时间(s)                      ";
			for(j=1;j<=4;j++)
				cout<<"  算法"<<j<<"  "<<setw(12)<<passout[j]<<"                "<<setw(12)<<passperhour[j]<<"              "<<setw(12)<<totalwt[j]<<"           "<<setw(12)<<avgwt[j]<<"           "<<setw(12)<<totald[j]<<"           "<<setw(12)<<avgd[j]<<"                        ";
			cout<<"  平均值 "<<setw(12)<<avgpass<<"                "<<setw(12)<<avgpph<<"              "<<setw(12)<<avgtwt<<"           "<<setw(12)<<avgawt<<"           "<<setw(12)<<avgtd<<"           "<<setw(12)<<avgad<<"                        ";
			cout<<"  最优值 "<<setw(12)<<maxpass<<"                "<<setw(12)<<maxpph<<"              "<<setw(12)<<mintwt<<"           "<<setw(12)<<minawt<<"           "<<setw(12)<<mintd<<"           "<<setw(12)<<minad<<"                        ";
			cout<<"  最劣值 "<<setw(12)<<minpass<<"                "<<setw(12)<<minpph<<"              "<<setw(12)<<maxtwt<<"           "<<setw(12)<<maxawt<<"           "<<setw(12)<<maxtd<<"           "<<setw(12)<<maxad<<"                        ";
			for(j=1;j<=SCREEN_WIDTH;j++)
				cout<<" ";
			setcolor(NORMAL);
		}
	}
	return 0;
}
