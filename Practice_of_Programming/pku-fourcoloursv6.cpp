#include<iostream>
#include<algorithm>
#include<vector>
#include<ctime>
#define MAXNUM 83
#define SEARCHRANGEMAX1 5
#define SEARCHRANGEMAX2 5
#define SEARCHRANGEMAX3 4
#define TIMELIMIT 850
using namespace std;

int nationnum,seanum,hillnum,totnum,player;
int area[MAXNUM]={0},map[MAXNUM]={0},linknum[MAXNUM]={0},link[MAXNUM][MAXNUM]={0},nearhill[MAXNUM]={0},nearsea[MAXNUM]={0};
int value[MAXNUM]={0},value2[MAXNUM]={0},value3[MAXNUM]={0};
bool canfill[4][MAXNUM]={0},islink[MAXNUM][MAXNUM]={0};
vector<int> linknation[MAXNUM],linksea[MAXNUM],linkhill[MAXNUM],neighbor[MAXNUM];

void init();
void take(const int,const int);
void remove(const int);
inline int min(const int,const int);
bool cmp(const int,const int);
bool cmp2(const int,const int);
bool cmp3(const int,const int);
int calcvalue(int,int);
inline int calcpriorty(const int,const int,const int,const int);
inline int nextcolor(const int,const int);

inline int min(const int x,const int y)
{
	if(x<y)return x;
	return y;
}

inline int nextcolor(const int color,const int n)
{
	return (color+4+n)%4;
}

void init()
{
	int i,j,temp;
	cin>>nationnum>>seanum>>hillnum>>player;
	totnum=nationnum+seanum+hillnum;
	for(i=1;i<=totnum;i++)
		map[i]=-1;
	for(i=1;i<=totnum;i++)
	{
		cin>>area[i]>>linknum[i];
		for(j=0;j<linknum[i];j++)
		{
			cin>>temp;
			link[i][j]=temp;
			islink[i][temp]=1;
			if(temp<=nationnum)
			{
				linknation[i].push_back(temp);
				if(i>nationnum+seanum)nearhill[temp]+=area[i];
			}
			else if(temp<=nationnum+seanum)
			{
				linksea[i].push_back(temp);
				if(i>nationnum&&i<=nationnum+seanum)nearsea[temp]+=area[i];
			}
			else linkhill[i].push_back(temp);
		}
	}
	for(i=0;i<4;i++)
		for(j=1;j<=nationnum;j++)
			canfill[i][j]=1;
}

void take(const int color,const int n)
{
	map[n]=color;
	int i,temp;
	for(i=0;i<linknation[n].size();i++)
	{
		temp=linknation[n][i];
		canfill[color][temp]=0;
	}
	for(i=0;i<linkhill[n].size();i++)
	{
		temp=linkhill[n][i];
		neighbor[temp].push_back(n);
	}
}

void remove(const int n)
{
	int i,j,temp,temp2,color=map[n];
	map[n]=-1;
	canfill[color][n]=1;
	for(i=0;i<linknation[n].size();i++)
	{
		temp=linknation[n][i];
		canfill[color][temp]=1;
		for(j=0;j<linknation[temp].size();j++)
		{
			temp2=linknation[temp][j];
			if(map[temp2]==color)
			{
				canfill[color][temp]=0;
				break;
			}
		}
	}
	for(i=0;i<linkhill[n].size();i++)
	{
		temp=linkhill[n][i];
		neighbor[temp].pop_back();
	}
}

int calcvalue(const int color,const int n)
{
	int r,i,j,empty,temp,tempa,tempb,tempt;
	int seaa,seab,seac,sead,sea0,sea1,sea2,sea3,s0,s1,s2,s3,sfa,sfb,hilla,hillb,hillc,hilld,hill0,hill1,hill2,hill3,h0,h1,h2,h3;
	vector<int> vn,vt;
	take(color,n);
	const int color1=nextcolor(color,1);
	const int color2=nextcolor(color,2);
	const int color3=nextcolor(color,3);
	r=400*area[n];
	for(i=nationnum+1;i<=nationnum+seanum;i++)
	{
		empty=seaa=seab=seac=sead=sea0=sea1=sea2=sea3=s0=s1=s2=s3=sfa=sfb=0;
		vn.clear();
		for(j=0;j<linknation[i].size();j++)
		{
			temp=linknation[i][j];
			if(map[temp]==color||map[temp]==color2)sfa++;
			else if(map[temp]==color1||map[temp]==color3)sfb++;
			else
			{
				empty++;
				bool seadup=0;
				if((canfill[color][temp]||canfill[color2][temp])&&(!canfill[color1][temp])&&(!canfill[color3][temp]))seaa++;
				else if((canfill[color1][temp]||canfill[color3][temp])&&(!canfill[color][temp])&&(!canfill[color2][temp]))seab++;
				else if((canfill[color1][temp]||canfill[color3][temp])&&(canfill[color][temp]||canfill[color2][temp]))seac++;
				else
				{
					seadup=1;
					sead++;
				}
				if(!seadup)vn.push_back(temp);
				if(canfill[color][temp]&&(!canfill[color1][temp])&&(!canfill[color2][temp])&&(!canfill[color3][temp]))sea0++;
				else if(canfill[color1][temp]&&(!canfill[color][temp])&&(!canfill[color2][temp])&&(!canfill[color3][temp]))sea1++;
				else if(canfill[color2][temp]&&(!canfill[color][temp])&&(!canfill[color1][temp])&&(!canfill[color3][temp]))sea2++;
				else if(canfill[color3][temp]&&(!canfill[color][temp])&&(!canfill[color1][temp])&&(!canfill[color2][temp]))sea3++;
				if(canfill[color][temp])s0++;
				if(canfill[color1][temp])s1++;
				if(canfill[color2][temp])s2++;
				if(canfill[color3][temp])s3++;
			}
		}
		if(empty-sead==0)
		{
			if(islink[i][n])
			{
			    if(sfa-sfb==1)r+=12000*area[i];
			    else if(sfa-sfb==0)r+=4000*area[i];
			    else if(sfa-sfb==2)r+=10000*area[i];
			}
			else
			{
				if(sfa>sfb)r+=4500*area[i];
				else if(sfa<sfb)r-=4500*area[i];
				else r-=2500*area[i];
			}
		}
		else if(empty-sead==1)
		{
			if(seaa==1)
			{
				if(sfa-sfb==0)r+=9000*area[i];
				else if(sfa-sfb==-1)r+=2500*area[i];
				else if(sfa-sfb==1)r+=7500*area[i];
			}
			else if(seab==1||canfill[color1][vn.back()])
			{
				if(sfa-sfb==2)r+=9000*area[i];
				else if(sfa-sfb==1)r+=3500*area[i];
				else if(sfa-sfb==3)r+=7000*area[i];
			}
			else if(!canfill[color1][vn.back()]&&canfill[color2][vn.back()])
			{
				if(sfa-sfb==0)r+=9000*area[i];
				else if(sfa-sfb==-1)r+=2500*area[i];
				else if(sfa-sfb==1)r+=7500*area[i];
			}
			else
			{
				if(sfa-sfb==2)r+=9000*area[i];
				else if(sfa-sfb==1)r+=3500*area[i];
				else if(sfa-sfb==0)r+=500*area[i];
				else if(sfa-sfb==3)r+=7000*area[i];
			}
		}
		else if(empty-sead==2)
		{
			if(seaa==2||(seaa==1&&!canfill[color1][vn[0]]&&!canfill[color1][vn[1]]&&((canfill[color2][vn[0]]&&!canfill[color3][vn[1]])||(canfill[color2][vn[1]]&&!canfill[color3][vn[0]]))))
			{
				if(sfa-sfb==-1)r+=8000*area[i];
				else if(sfa-sfb==-2)r+=2000*area[i];
				else if(sfa-sfb==0)r+=6500*area[i];
			}
			else if(seab==2||(seab==1&&((canfill[color1][vn[0]]&&!canfill[color2][vn[1]])||(canfill[color1][vn[1]]&&!canfill[color2][vn[0]]))))
			{
				if(sfa-sfb==3)r+=8000*area[i];
				else if(sfa-sfb==2)r+=3000*area[i];
				else if(sfa-sfb==1)r+=1000*area[i];
				else if(sfa-sfb==4)r+=6000*area[i];
			}
			else if((seaa==1||(canfill[color2][vn[0]]&&canfill[color2][vn[1]]))&&(seab==1||canfill[color1][vn[0]]||canfill[color1][vn[1]]))
			{
				if(sfa-sfb==1)r+=8000*area[i];
				else if(sfa-sfb==0)r+=2500*area[i];
				else if(sfa-sfb==2)r+=6500*area[i];
			}
			else
			{
				tempa=sfa+seaa;
				tempb=sfb+seab+seac%2;
				if(tempa-tempb==1)r+=8000*area[i];
				else if(tempa-tempb==0)r+=2500*area[i];
				else if(tempa-tempb==2)r+=6500*area[i];
				else if(tempa-tempb==3)r+=3000*area[i];
				else if(tempa-tempb==-1)r+=500*area[i];
			}
		}
		else
		{
			tempa=sfa+seaa;
			tempb=sfb+seab+seac%2;
			if(tempa-tempb==1)r+=7000*area[i];
			else if(tempa-tempb==0)r+=3000*area[i];
			else if(tempa-tempb==2)r+=6000*area[i];
			else if(tempa-tempb==3)r+=2500*area[i];
			else if(tempa-tempb==4)r+=500*area[i];
			else if(tempa-tempb==-1)r+=1000*area[i];
		}
	}
	for(i=nationnum+seanum+1;i<=totnum;i++)
	{
		empty=hilla=hillb=hillc=hilld=hill0=hill1=hill2=hill3=h0=h1=h2=h3=0;
		vn.clear();
		vt.clear();
		for(j=0;j<linknation[i].size();j++)
		{
			temp=linknation[i][j];
			if(map[temp]==-1)
			{
				empty++;
				bool hilldup=0;
				if((canfill[color][temp]||canfill[color2][temp])&&(!canfill[color1][temp])&&(!canfill[color3][temp]))hilla++;
				else if((canfill[color1][temp]||canfill[color3][temp])&&(!canfill[color][temp])&&(!canfill[color2][temp]))hillb++;
				else if((canfill[color1][temp]||canfill[color3][temp])&&(canfill[color][temp]||canfill[color2][temp]))hillc++;
				else
				{
					hilldup=1;
					hilld++;
				}
				if(!hilldup)vn.push_back(temp);
				if(canfill[color][temp]&&(!canfill[color1][temp])&&(!canfill[color2][temp])&&(!canfill[color3][temp]))hill0++;
				else if(canfill[color1][temp]&&(!canfill[color][temp])&&(!canfill[color2][temp])&&(!canfill[color3][temp]))hill1++;
				else if(canfill[color2][temp]&&(!canfill[color][temp])&&(!canfill[color1][temp])&&(!canfill[color3][temp]))hill2++;
				else if(canfill[color3][temp]&&(!canfill[color][temp])&&(!canfill[color1][temp])&&(!canfill[color2][temp]))hill3++;
				if(canfill[color][temp])h0++;
				else if(canfill[color1][temp])h1++;
				else if(canfill[color2][temp])h2++;
				else if(canfill[color3][temp])h3++;
				tempt=canfill[color1][temp]+2*canfill[color2][temp]+4*canfill[color3][temp]+8*canfill[color][temp];
				if(tempt>0)vt.push_back(tempt);
			}
		}
		if(empty-hilld==0)
		{
			if(islink[i][n])
			{
				int size=neighbor[i].size();
				if(size==1)r+=16000*area[i];
				else if(size>=2&&(map[neighbor[i][size-2]]==color||map[neighbor[i][size-2]]==color2))r+=11000*area[i];
				else r+=15000*area[i];
			}
			else if(!neighbor[i].empty())
			{
				temp=neighbor[i].back();
				if(map[temp]==color||map[temp]==color2)r+=5000*area[i];
				else r-=5000*area[i];
			}
		}
		else if(empty-hilld==1)
		{
			bool hillbelongus=0;
			if(islink[i][n])
			{
				int size=neighbor[i].size();
				if(size>=2&&(map[neighbor[i][size-2]]==color||map[neighbor[i][size-2]]==color2))hillbelongus=1;
			}
			else if(!neighbor[i].empty())
			{
				temp=neighbor[i].back();
				if(map[temp]==color||map[temp]==color2)hillbelongus=1;
			}
			if(vt[0]==1||vt[0]==4)r-=9500*area[i];
			else if(vt[0]==2||vt[0]==8)
			{
				if(hillbelongus)r+=7000*area[i];
				else r+=10000*area[i];
			}
			else if(vt[0]==5)r-=10000*area[i];
			else if(vt[0]==10)
			{
				if(hillbelongus)r+=6500*area[i];
				else r+=10500*area[i];
			}
			else if(vt[0]==3||vt[0]==9||vt[0]==12)r-=8000*area[i];
			else if(vt[0]==6)r+=6500*area[i];
			else if(vt[0]==7||vt[0]==15||vt[0]==13)r-=9000*area[i];
			else if(vt[0]==11)r-=7500*area[i];
			else if(vt[0]==14)r+=7000*area[i];
		}
		else if(empty-hilld==2)
		{
			sort(vt.begin(),vt.end());
			if(vt[0]==15)
			{
				r+=0;
			}
			else if(vt[0]==14)
			{
				if(vt[1]==15)r+=3000*area[i];
				else if(vt[1]==14)r+=4500*area[i];
			}
			else if(vt[0]==13)
			{
				if(vt[1]==15)r-=4500*area[i];
				else if(vt[1]==14)r-=2500*area[i];
				else if(vt[1]==13)r-=6500*area[i];
			}
			else if(vt[0]==12)
			{
				if(vt[1]==15)r-=5500*area[i];
				else if(vt[1]==14)r+=3000*area[i];
				else if(vt[1]==13)r-=6000*area[i];
				else if(vt[1]==12)r+=0;
			}
			else if(vt[0]==11)
			{
				if(vt[1]==15)r+=3500*area[i];
				else if(vt[1]==14)r+=4000*area[i];
				else if(vt[1]==13)r-=4000*area[i];
				else if(vt[1]==12)r-=4000*area[i];
				else if(vt[1]==11)r+=5000*area[i];
			}
			else if(vt[0]==10)
			{
				if(vt[1]==15)r+=4500*area[i];
				else if(vt[1]==14)r+=6000*area[i];
				else if(vt[1]==13)r+=3000*area[i];
				else if(vt[1]==12)r+=4000*area[i];
				else if(vt[1]==11)r+=6000*area[i];
				else if(vt[1]==10)r+=8000*area[i];
			}
			else if(vt[0]==9)
			{
				if(vt[1]==15)r+=3500*area[i];
				else if(vt[1]==14)r+=4000*area[i];
				else if(vt[1]==13)r-=4500*area[i];
				else if(vt[1]==12)r-=4500*area[i];
				else if(vt[1]==11)r+=4000*area[i];
				else if(vt[1]==10)r+=5000*area[i];
				else if(vt[1]==9)r+=0;
			}
			else if(vt[0]==8)
			{
				if(vt[1]==15)r+=4500*area[i];
				else if(vt[1]==14)r+=5000*area[i];
				else if(vt[1]==13)r+=2500*area[i];
				else if(vt[1]==12)r+=3000*area[i];
				else if(vt[1]==11)r+=5000*area[i];
				else if(vt[1]==10)r+=7500*area[i];
				else if(vt[1]==9)r+=2500*area[i];
				else if(vt[1]==8)r+=7000*area[i];
			}
			else if(vt[0]==7)
			{
				if(vt[1]==15)r-=4000*area[i];
				else if(vt[1]==14)r-=3500*area[i];
				else if(vt[1]==13)r-=5500*area[i];
				else if(vt[1]==12)r-=5500*area[i];
				else if(vt[1]==11)r+=3000*area[i];
				else if(vt[1]==10)r+=3500*area[i];
				else if(vt[1]==9)r+=3000*area[i];
				else if(vt[1]==8)r+=3500*area[i];
				else if(vt[1]==7)r-=5500*area[i];
			}
			else if(vt[0]==6)
			{
				if(vt[1]==15)r+=3000*area[i];
				else if(vt[1]==14)r+=3500*area[i];
				else if(vt[1]==13)r+=2500*area[i];
				else if(vt[1]==12)r+=3000*area[i];
				else if(vt[1]==11)r+=4500*area[i];
				else if(vt[1]==10)r+=5000*area[i];
				else if(vt[1]==9)r+=3500*area[i];
				else if(vt[1]==8)r+=4000*area[i];
				else if(vt[1]==7)r-=4000*area[i];
				else if(vt[1]==6)r+=0;
			}
			else if(vt[0]==5)
			{
				if(vt[1]==15)r-=5500*area[i];
				else if(vt[1]==14)r-=4000*area[i];
				else if(vt[1]==13)r-=7000*area[i];
				else if(vt[1]==12)r-=6500*area[i];
				else if(vt[1]==11)r-=4500*area[i];
				else if(vt[1]==10)r+=0;
				else if(vt[1]==9)r-=5000*area[i];
				else if(vt[1]==8)r-=1000*area[i];
				else if(vt[1]==7)r-=7000*area[i];
				else if(vt[1]==6)r-=4500*area[i];
				else if(vt[1]==5)r-=9000*area[i];
			}
			else if(vt[0]==4)
			{
				if(vt[1]==15)r-=4000*area[i];
				else if(vt[1]==14)r-=3500*area[i];
				else if(vt[1]==13)r-=5500*area[i];
				else if(vt[1]==12)r-=4000*area[i];
				else if(vt[1]==11)r-=4500*area[i];
				else if(vt[1]==10)r+=500*area[i];
				else if(vt[1]==9)r-=4500*area[i];
				else if(vt[1]==8)r+=0;
				else if(vt[1]==7)r-=5500*area[i];
				else if(vt[1]==6)r-=4000*area[i];
				else if(vt[1]==5)r-=8500*area[i];
				else if(vt[1]==4)r-=8000*area[i];
			}
			else if(vt[0]==3)
			{
				if(vt[1]==15)r-=4000*area[i];
				else if(vt[1]==14)r-=3500*area[i];
				else if(vt[1]==13)r-=6000*area[i];
				else if(vt[1]==12)r-=5000*area[i];
				else if(vt[1]==11)r+=3000*area[i];
				else if(vt[1]==10)r+=3500*area[i];
				else if(vt[1]==9)r+=3500*area[i];
				else if(vt[1]==8)r+=4000*area[i];
				else if(vt[1]==7)r-=4500*area[i];
				else if(vt[1]==6)r-=4000*area[i];
				else if(vt[1]==5)r-=5000*area[i];
				else if(vt[1]==4)r-=5000*area[i];
				else if(vt[1]==3)r+=0;
			}
			else if(vt[0]==2)
			{
				if(vt[1]==15)r+=4500*area[i];
				else if(vt[1]==14)r+=5000*area[i];
				else if(vt[1]==13)r+=3000*area[i];
				else if(vt[1]==12)r+=3500*area[i];
				else if(vt[1]==11)r+=5500*area[i];
				else if(vt[1]==10)r+=7500*area[i];
				else if(vt[1]==9)r+=4500*area[i];
				else if(vt[1]==8)r+=7500*area[i];
				else if(vt[1]==7)r+=2500*area[i];
				else if(vt[1]==6)r+=3000*area[i];
				else if(vt[1]==5)r-=1000*area[i];
				else if(vt[1]==4)r+=0;
				else if(vt[1]==3)r+=3000*area[i];
				else if(vt[1]==2)r+=7000*area[i];
			}
			else if(vt[0]==1)
			{
				if(vt[1]==15)r-=5500*area[i];
				else if(vt[1]==14)r-=4000*area[i];
				else if(vt[1]==13)r-=6500*area[i];
				else if(vt[1]==12)r-=5500*area[i];
				else if(vt[1]==11)r-=3500*area[i];
				else if(vt[1]==10)r+=500*area[i];
				else if(vt[1]==9)r-=4000*area[i];
				else if(vt[1]==8)r+=0;
				else if(vt[1]==7)r-=6000*area[i];
				else if(vt[1]==6)r-=4500*area[i];
				else if(vt[1]==5)r-=8500*area[i];
				else if(vt[1]==4)r-=8500*area[i];
				else if(vt[1]==3)r-=4000*area[i];
				else if(vt[1]==2)r+=0;
				else if(vt[1]==1)r-=8000*area[i];
			}
		}
		else
		{
			r+=(1000*hilla-2500*hillb-500*(hillc%2))*area[i];
		}
	}
	for(i=1;i<nationnum;i++)
	{
		if(map[i]==-1)
		{
			int ss=area[i]*2.5+nearhill[i]*1.2+nearsea[i];
			temp=canfill[color1][i]+2*canfill[color2][i]+4*canfill[color3][i]+8*canfill[color][i];
			if(temp==0)r-=800*ss;
			else if(temp==1)r-=1200*ss;
			else if(temp==2)r+=1000*ss;
			else if(temp==3)r-=300*ss;
			else if(temp==4)r-=1200*ss;
			else if(temp==5)r-=1500*ss;
			else if(temp==6)r+=100*ss;
			else if(temp==7)r-=500*ss;
			else if(temp==8)r+=1000*ss;
			else if(temp==9)r-=300*ss;
			else if(temp==10)r+=1300*ss;
			else if(temp==11)r-=100*ss;
			else if(temp==12)r-=300*ss;
			else if(temp==13)r-=700*ss;
			else if(temp==14)r+=300*ss;
			else if(temp==15)r-=50*ss;
		}
		else
		{
			if(map[i]==color||map[i]==color2)r+=3200*area[i];
			else r-=3200*area[i];
		}
	}
	remove(n);
	return r;
}

inline int calcpriorty(const int val0,const int val1,const int val2,const int size)
{
	if(size==0)return 400000;
	else if(size==1)return 100000-val0;
	else if(size==2)return 10000-0.8*val0-0.2*val1;
	else return -0.75*val0-0.15*val1-0.1*val2;
}

bool cmp(const int x,const int y)
{
	if(value[x]!=value[y])return value[x]>value[y];
	else return area[x]>area[y];
}

bool cmp2(const int x,const int y)
{
	if(value2[x]!=value2[y])return value2[x]>value2[y];
	else return area[x]>area[y];
}

bool cmp3(const int x,const int y)
{
	if(value3[x]!=value3[y])return value3[x]>value3[y];
	else return area[x]>area[y];
}

int main()
{
	int curcolor,i,j,k,l,m,p,q,laststep,temp,tempcolor,searchrange1,searchrange2,searchrange3,priorty[MAXNUM]={0},vput0;
	time_t starttime;
	bool timeout,stopped[4]={0};
	vector<int> vput,vput2,vput3,vput4;
	init();
	if(player==1)curcolor=3;
	else player=curcolor=0;
	while(1)
	{
		cin>>laststep;
		if(laststep==0&&player==0)stopped[curcolor]=1;
		player=timeout=0;
		starttime=clock();
		if(laststep>0)take(curcolor,laststep);
		curcolor=nextcolor(curcolor,1);
		vput.clear();
		for(i=1;i<=nationnum;i++)
			if(canfill[curcolor][i]&&map[i]==-1)vput.push_back(i);
		if(vput.size()==0)
		{
			cout<<0<<endl;
			stopped[curcolor]=1;
		}
		else if(vput.size()==1)
		{
			cout<<vput[0]<<endl<<flush;
			take(curcolor,vput[0]);
		}
		else
		{
			for(i=0;i<vput.size();i++)
				value[vput[i]]=calcvalue(curcolor,vput[i]);
			sort(vput.begin(),vput.end(),cmp);
			vput0=vput[0];
			searchrange1=min(SEARCHRANGEMAX1,vput.size());
			vector<int> tempvalue;
			for(i=0;i<searchrange1;i++)
				tempvalue.push_back(value[vput[i]]);
			for(i=0;i<searchrange1;i++)
			{
				if(clock()-starttime>=TIMELIMIT)
				{
					timeout=1;
					break;
				}
				take(curcolor,vput[i]);
				priorty[vput[i]]=4.5*tempvalue[i];
				tempcolor=curcolor;
				m=0;
				do
				{
					m++;
				    vput2.clear();
					tempcolor=nextcolor(tempcolor,1);
				    for(j=1;j<=nationnum;j++)
					    if(canfill[tempcolor][j]&&map[j]==-1&&!stopped[tempcolor])vput2.push_back(j);
					if(!vput2.empty())break;
					if(m%2)priorty[vput[i]]+=4*calcpriorty(0,0,0,0);
					else priorty[vput[i]]-=4*calcpriorty(0,0,0,0);
				}
				while(tempcolor!=curcolor&&vput2.empty());
				if(!vput2.empty())
				{
				    for(j=0;j<vput2.size();j++)
					    value[vput2[j]]=calcvalue(tempcolor,vput2[j]);
				    sort(vput2.begin(),vput2.end(),cmp);
					searchrange2=min(SEARCHRANGEMAX2,vput2.size());
					for(j=0;j<searchrange2;j++)
					{
						if(clock()-starttime>=TIMELIMIT)
						{
							timeout=1;
							break;
						}
						take(tempcolor,vput2[j]);
						p=0;
						int tempcolor2=tempcolor;
						do
						{
							p++;
						    vput3.clear();
							tempcolor2=nextcolor(tempcolor2,1);
						    for(k=1;k<=nationnum;k++)
							    if(canfill[tempcolor2][k]&&map[k]==-1&&!stopped[tempcolor2])vput3.push_back(k);
							if(!vput3.empty())break;
							if(p%2)value[vput2[j]]+=0.9*calcpriorty(0,0,0,0);
							else value[vput2[j]]-=0.9*calcpriorty(0,0,0,0);
						}
						while(tempcolor!=tempcolor2&&vput3.empty());
						if(!vput3.empty())
						{
							for(k=0;k<vput3.size();k++)
								value2[vput3[k]]=calcvalue(tempcolor2,vput3[k]);
							sort(vput3.begin(),vput3.end(),cmp2);
							searchrange3=min(SEARCHRANGEMAX3,vput3.size());
							for(k=0;k<searchrange3;k++)
							{
								if(clock()-starttime>=TIMELIMIT)
								{
									timeout=1;
									break;
								}
								take(tempcolor2,vput3[k]);
								q=0;
								int tempcolor3=tempcolor2;
								do
								{
									q++;
								    vput4.clear();
									tempcolor3=nextcolor(tempcolor3,1);
								    for(l=1;l<=nationnum;l++)
									    if(canfill[tempcolor3][l]&&map[l]==-1&&!stopped[tempcolor3])vput4.push_back(l);
									if(!vput4.empty())break;
									if(q%2)value2[vput3[k]]+=0.85*calcpriorty(0,0,0,0);
									else value2[vput3[k]]-=0.85*calcpriorty(0,0,0,0);
								}
								while(tempcolor2!=tempcolor3&&vput4.empty());
								if(!vput4.empty())
								{
									for(l=0;l<vput4.size();l++)
										value3[vput4[l]]=calcvalue(tempcolor3,vput4[l]);
									sort(vput4.begin(),vput4.end(),cmp3);
									if(vput4.size()==1)value2[vput3[k]]+=0.85*(2*(q%2)-1)*calcpriorty(value3[vput4[0]],0,0,1);
									else if(vput4.size()==2)value2[vput3[k]]+=0.85*(2*(q%2)-1)*calcpriorty(value3[vput4[0]],value3[vput4[1]],0,2);
									else if(vput4.size()>=3)value2[vput3[k]]+=0.85*(2*(q%2)-1)*calcpriorty(value3[vput4[0]],value3[vput4[1]],value3[vput4[2]],3);
								}
								remove(vput3[k]);
							}
							if(vput3.size()==1)value[vput2[j]]+=0.9*(2*(p%2)-1)*calcpriorty(value2[vput3[0]],0,0,1);
							else if(vput3.size()==2)value[vput2[j]]+=0.9*(2*(p%2)-1)*calcpriorty(value2[vput3[0]],value2[vput3[1]],0,2);
							else if(vput3.size()>=3)value[vput2[j]]+=0.9*(2*(p%2)-1)*calcpriorty(value2[vput3[0]],value2[vput3[1]],value2[vput3[2]],3);
						}
						remove(vput2[j]);
					}
					vector<int> tempvput2;
					for(j=0;j<searchrange2;j++)
						tempvput2.push_back(vput2[j]);
					sort(tempvput2.begin(),tempvput2.end(),cmp);
					if(tempvput2.size()==1)priorty[vput[i]]+=4*(2*(m%2)-1)*calcpriorty(value[tempvput2[0]],0,0,1);
					else if(tempvput2.size()==2)priorty[vput[i]]+=4*(2*(m%2)-1)*calcpriorty(value[tempvput2[0]],value[tempvput2[1]],0,2);
					else if(tempvput2.size()>=3)priorty[vput[i]]+=4*(2*(m%2)-1)*calcpriorty(value[tempvput2[0]],value[tempvput2[1]],value[tempvput2[2]],3);
				}
				remove(vput[i]);
			}
			tempvalue.clear();
			if(timeout)temp=vput0;
			else
			{
			    temp=vput[0];
			    for(i=0;i<searchrange1;i++)
				    if(priorty[vput[i]]>priorty[temp])temp=vput[i];
			}
			cout<<temp<<endl<<flush;
			take(curcolor,temp);
		}
		curcolor=nextcolor(curcolor,1);
	}
	return 0;
}
