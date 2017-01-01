#include<iostream>
#include<iomanip>

#define DRAGON 0
#define NINJA 1
#define ICEMAN 2
#define LION 3
#define WOLF 4

#define SWORD 0
#define BOMB 1
#define ARROW 2

#define RED 0
#define BLUE 1

using namespace std;

const int bornorder[2][5]={{ICEMAN,LION,WOLF,NINJA,DRAGON},{LION,DRAGON,NINJA,ICEMAN,WOLF}};
const char *(color[2])={"red","blue"};
const char *(warcraftname[5])={"dragon","ninja","iceman","lion","wolf"};
const char *(weaponname[3])={"sword","bomb","arrow"};

class CLOCK;
class WARCRAFT;
class HEADQUARTER;
class MAP;
class WORLD;

class CLOCK
{
	friend class WORLD;
	int hour,minute,timelimit;
	void init(const int T)
	{
		hour=minute=0;
		timelimit=T;
	}
	bool operator+=(const int addminute)
	{
		minute+=addminute;
		hour+=minute/60;
		minute%=60;
		return hour*60+minute<=timelimit;
	}
	friend ostream & operator<<(ostream &os,const CLOCK &clock)
	{
		os<<setw(3)<<setfill('0')<<clock.hour<<":"<<setw(2)<<clock.minute<<" ";
		return os;
	}
};

class WARCRAFT
{
	friend class WORLD;
	friend class MAP;
	bool alive,arrive,belong,bomb;
	int id,kind,hp,atk,pos,sword,arrow,loyalty;
	short pace;
	double morale;
	friend ostream & operator<<(ostream &os,const WARCRAFT &warcraft)
	{
		os<<color[warcraft.belong]<<" "<<warcraftname[warcraft.kind]<<" "<<warcraft.id;
		return os;
	}
	WARCRAFT operator-=(WARCRAFT &warcraft)
	{
		hp-=(warcraft.atk+warcraft.sword);
		warcraft.sword*=0.8;
		return *this;
	}
	WARCRAFT operator/=(WARCRAFT &warcraft)
	{
		hp-=(warcraft.atk/2+warcraft.sword);
		warcraft.sword*=0.8;
		return *this;
	}
	WARCRAFT operator++(int)
	{
		pace++;
		if(!belong)pos++;
		else pos--;
		if(kind==ICEMAN&&pace%2==0)
		{
			atk+=20;
			hp-=9;
			if(hp<=0)hp=1;
		}
		return *this;
	}
	void getweapon(const int sw,const bool bo,const int ar)
	{
		sword=sw;
		arrow=ar;
		bomb=bo;
	}
	void moveoutput()
	{
		cout<<(*this)<<" marched to city "<<pos<<" with "<<hp<<" elements and force "<<atk<<endl;
	}
	void reachheadquarter()
	{
		cout<<(*this)<<" reached "<<color[!belong]<<" headquarter with "<<hp<<" elements and force "<<atk<<endl;
		alive=0;
		arrive=1;
	}
	void report()
	{
		cout<<(*this)<<" has ";
		bool hasweapon=0;
		if(arrow!=0)
		{
			cout<<"arrow("<<arrow<<")";
			hasweapon=1;
		}
		if(bomb!=0)
		{
			if(hasweapon)cout<<",";
			else hasweapon=1;
			cout<<"bomb";
		}
		if(sword!=0)
		{
			if(hasweapon)cout<<",";
			else hasweapon=1;
			cout<<"sword("<<sword<<")";
		}
		if(!hasweapon)cout<<"no weapon";
		cout<<endl;
	}
};

class HEADQUARTER
{
	friend class WORLD;
	bool belong;
	int bornsum,insum,hp,order;
	void init(const int M,const bool bl)
	{
		hp=M;
		belong=bl;
		bornsum=insum=order=0;
	}
	friend ostream & operator<<(ostream &os,const HEADQUARTER &headquarter)
	{
		os<<headquarter.hp<<" elements in "<<color[headquarter.belong]<<" headquarter"<<endl;
		return os;
	}
};

class MAP
{
	friend class WORLD;
	bool belong,raiseflag;
	int hp,id;
	short winner;
	WARCRAFT *(warcraft[2]);
	void init(const int cityid)
	{
		belong=(cityid+1)%2;
		hp=winner=raiseflag=0;
		id=cityid;
		warcraft[RED]=warcraft[BLUE]=NULL;
	}
	void win(const CLOCK &clock,const bool winnerbelong)
	{
		if(warcraft[winnerbelong]->kind==DRAGON)
		{
			warcraft[winnerbelong]->morale+=0.2;
			if(winnerbelong==belong&&warcraft[winnerbelong]->morale>0.8)cout<<clock<<(*warcraft[belong])<<" yelled in city "<<id<<endl;
		}
		else if(warcraft[winnerbelong]->kind==WOLF)
		{
			if(warcraft[winnerbelong]->sword==0)warcraft[winnerbelong]->sword=warcraft[!winnerbelong]->sword;
			if(warcraft[winnerbelong]->bomb==0)warcraft[winnerbelong]->bomb=warcraft[!winnerbelong]->bomb;
			if(warcraft[winnerbelong]->arrow==0)warcraft[winnerbelong]->arrow=warcraft[!winnerbelong]->arrow;
		}
		cout<<clock<<(*warcraft[winnerbelong])<<" earned "<<hp<<" elements for his headquarter"<<endl;
		if(winnerbelong)
		{
			if(winner==1)
			{
				winner=2;
				if(!raiseflag||!belong)
				{
					raiseflag=belong=1;
					cout<<clock<<"blue flag raised in city "<<id<<endl;
				}
			}
			else if(winner<=0)winner=1;
		}
		else
		{
			if(winner==-1)
			{
				winner=-2;
				if(!raiseflag||belong)
				{
					raiseflag=1;
					belong=0;
					cout<<clock<<"red flag raised in city "<<id<<endl;
				}
			}
			else if(winner>=0)winner=-1;
		}
	}
	void draw(const CLOCK &clock,const int K)
	{
		int i;
		for(i=RED;i<=BLUE;i++)
		{
			if(warcraft[i]->kind==LION)warcraft[i]->loyalty-=K;
			else if(warcraft[i]->kind==DRAGON)warcraft[i]->morale-=0.2;
		}
		if(warcraft[belong]->kind==DRAGON&&warcraft[belong]->morale>0.8)cout<<clock<<(*warcraft[belong])<<" yelled in city "<<id<<endl;
		winner=0;
	}
	int fight(const bool t,const CLOCK &clock,const int K)
	{
		if(t)cout<<clock<<(*warcraft[belong])<<" attacked "<<(*warcraft[!belong])<<" in city "<<id<<" with "<<(warcraft[belong]->hp)<<" elements and force "<<(warcraft[belong]->atk)<<endl;
		int temphp[2]={warcraft[RED]->hp,warcraft[BLUE]->hp};
		bool loser;
		*warcraft[!belong]-=*warcraft[belong];
		if(warcraft[!belong]->hp<=0)
		{
			loser=!belong;
			if(t)
			{
				cout<<clock<<(*warcraft[!belong])<<" was killed in city "<<id<<endl;
				win(clock,belong);
				if(warcraft[loser]->kind==LION)warcraft[!loser]->hp+=temphp[loser];
			}
			return loser;
		}
		if(warcraft[!belong]->kind!=NINJA)
		{
		    if(t)cout<<clock<<(*warcraft[!belong])<<" fought back against "<<(*warcraft[belong])<<" in city "<<id<<endl;
		    *warcraft[belong]/=*warcraft[!belong];
			if(warcraft[belong]->hp<=0)
			{
				loser=belong;
				if(t)
				{
					cout<<clock<<(*warcraft[belong])<<" was killed in city "<<id<<endl;
					win(clock,!belong);
					if(warcraft[loser]->kind==LION)warcraft[!loser]->hp+=temphp[loser];
				}
				return loser;
			}
		}
		if(t)draw(clock,K);
		return -1;
	}
};

class WORLD
{
	int M,N,R,K,T,hp0[5],atk0[5];
	bool gameover;
	CLOCK clock;
	HEADQUARTER headquarter[2];
	MAP *city;
	WARCRAFT *(warcraft[2]),tempwarcraft[2];
	void born(const bool belong)
	{
		if(headquarter[belong].hp>=hp0[bornorder[belong][headquarter[belong].order]])
		{
			headquarter[belong].hp-=hp0[bornorder[belong][headquarter[belong].order]];
			headquarter[belong].bornsum++;
			warcraft[belong][headquarter[belong].bornsum].alive=1;
			warcraft[belong][headquarter[belong].bornsum].pace=warcraft[belong][headquarter[belong].bornsum].arrive=0;
			warcraft[belong][headquarter[belong].bornsum].belong=belong;
			warcraft[belong][headquarter[belong].bornsum].id=headquarter[belong].bornsum;
			warcraft[belong][headquarter[belong].bornsum].kind=bornorder[belong][headquarter[belong].order];
			warcraft[belong][headquarter[belong].bornsum].hp=hp0[bornorder[belong][headquarter[belong].order]];
			warcraft[belong][headquarter[belong].bornsum].atk=atk0[bornorder[belong][headquarter[belong].order]];
			warcraft[belong][headquarter[belong].bornsum].getweapon(0,0,0);
			if(belong)warcraft[belong][headquarter[belong].bornsum].pos=N+1;
			else warcraft[belong][headquarter[belong].bornsum].pos=0;
			city[warcraft[belong][headquarter[belong].bornsum].pos].warcraft[belong]=&warcraft[belong][headquarter[belong].bornsum];
			cout<<clock<<warcraft[belong][headquarter[belong].bornsum]<<" born"<<endl;
			if(warcraft[belong][headquarter[belong].bornsum].kind==DRAGON||warcraft[belong][headquarter[belong].bornsum].kind==ICEMAN)
			{
				if(warcraft[belong][headquarter[belong].bornsum].kind==DRAGON)
				{
					warcraft[belong][headquarter[belong].bornsum].morale=double(headquarter[belong].hp)/warcraft[belong][headquarter[belong].bornsum].hp;
					cout<<"Its morale is "<<setiosflags(ios::fixed)<<setprecision(2)<<warcraft[belong][headquarter[belong].bornsum].morale<<endl;
				}
				if(warcraft[belong][headquarter[belong].bornsum].id%3==SWORD)warcraft[belong][headquarter[belong].bornsum].getweapon(int(warcraft[belong][headquarter[belong].bornsum].atk*0.2),0,0);
				else if(warcraft[belong][headquarter[belong].bornsum].id%3==BOMB)warcraft[belong][headquarter[belong].bornsum].getweapon(0,1,0);
				else warcraft[belong][headquarter[belong].bornsum].getweapon(0,0,3);
			}
			else if(warcraft[belong][headquarter[belong].bornsum].kind==NINJA)
			{
				if(warcraft[belong][headquarter[belong].bornsum].id%3==SWORD)warcraft[belong][headquarter[belong].bornsum].getweapon(int(warcraft[belong][headquarter[belong].bornsum].atk*0.2),1,0);
				else if(warcraft[belong][headquarter[belong].bornsum].id%3==BOMB)warcraft[belong][headquarter[belong].bornsum].getweapon(0,1,3);
				else warcraft[belong][headquarter[belong].bornsum].getweapon(int(warcraft[belong][headquarter[belong].bornsum].atk*0.2),0,3);
			}
			else if(warcraft[belong][headquarter[belong].bornsum].kind==LION)
			{
				warcraft[belong][headquarter[belong].bornsum].loyalty=headquarter[belong].hp;
				cout<<"Its loyalty is "<<warcraft[belong][headquarter[belong].bornsum].loyalty<<endl;
			}
			headquarter[belong].order++;
			headquarter[belong].order%=5;
		}
	}
	void lionranaway(const int pos,const bool belong)
	{
		if((city[pos].warcraft[belong])->kind==LION&&(city[pos].warcraft[belong])->alive==1&&(city[pos].warcraft[belong])->loyalty<=0)
		{
			(city[pos].warcraft[belong])->alive=0;
			cout<<clock<<color[belong]<<" lion "<<((city[pos].warcraft[belong])->id)<<" ran away"<<endl;
			city[pos].warcraft[belong]=NULL;
		}
	}
	void move()
	{
		int i,j;
		for(i=RED;i<=BLUE;i++)
			for(j=1;j<=headquarter[i].bornsum;j++)
				if(warcraft[i][j].alive)
				{
					city[warcraft[i][j].pos].warcraft[i]=NULL;
					(warcraft[i][j])++;
					city[warcraft[i][j].pos].warcraft[i]=&warcraft[i][j];
				}
		for(i=0;i<=N+1;i++)
			for(j=RED;j<=BLUE;j++)
				if(city[i].warcraft[j]!=NULL)
				{
					cout<<clock;
					if(i==0||i==N+1)
					{
						(city[i].warcraft[j])->reachheadquarter();
						city[i].warcraft[j]=NULL;
            			headquarter[j].insum++;
						if(headquarter[j].insum==2)
						{
						    cout<<clock<<color[!j]<<" headquarter was taken"<<endl;
						    gameover=1;
						}
					}
					else (city[i].warcraft[j])->moveoutput();
				}
	}
	void gethp(const int c)
	{
		int i;
		for(i=RED;i<=BLUE;i++)
			if(city[c].warcraft[i]==NULL&&city[c].warcraft[!i]!=NULL)
			{
				headquarter[!i].hp+=city[c].hp;
				cout<<clock<<(*city[c].warcraft[!i])<<" earned "<<city[c].hp<<" elements for his headquarter"<<endl;
				city[c].hp=0;
			}
	}
	void shot()
	{
		int i,j;
		const int nextpos[2]={1,-1};
		for(i=1;i<=N;i++)
			for(j=RED;j<=BLUE;j++)
				if(i+nextpos[j]>=1&&i+nextpos[j]<=N&&city[i].warcraft[j]!=NULL&&city[i+nextpos[j]].warcraft[1-j]!=NULL&&(city[i].warcraft[j])->arrow>0)
				{
					(city[i].warcraft[j])->arrow--;
					(city[i+nextpos[j]].warcraft[1-j])->hp-=R;
					if((city[i+nextpos[j]].warcraft[1-j])->hp>0)cout<<clock<<(*city[i].warcraft[j])<<" shot"<<endl;
					else cout<<clock<<(*city[i].warcraft[j])<<" shot and killed "<<(*city[i+nextpos[j]].warcraft[1-j])<<endl;
				}
	}
	void usebomb()
	{
		int i,temp;
		for(i=1;i<=N;i++)
			if(city[i].warcraft[RED]!=NULL&&city[i].warcraft[BLUE]!=NULL&&(city[i].warcraft[RED])->hp>0&&(city[i].warcraft[BLUE])->hp>0)
			{
				tempwarcraft[RED]=*city[i].warcraft[RED];
				tempwarcraft[BLUE]=*city[i].warcraft[BLUE];
				temp=city[i].fight(0,clock,K);
				*city[i].warcraft[RED]=tempwarcraft[RED];
				*city[i].warcraft[BLUE]=tempwarcraft[BLUE];
				if(temp==RED&&(city[i].warcraft[RED])->bomb)
				{
					(city[i].warcraft[RED])->hp=(city[i].warcraft[BLUE])->hp=(city[i].warcraft[RED])->bomb=0;
					cout<<clock<<(*city[i].warcraft[RED])<<" used a bomb and killed "<<(*city[i].warcraft[BLUE])<<endl;
				}
				else if(temp==BLUE&&(city[i].warcraft[BLUE])->bomb)
				{
					(city[i].warcraft[RED])->hp=(city[i].warcraft[BLUE])->hp=(city[i].warcraft[BLUE])->bomb=0;
					cout<<clock<<(*city[i].warcraft[BLUE])<<" used a bomb and killed "<<(*city[i].warcraft[RED])<<endl;
				}
			}
	}
	void bonus(const bool belong,const int citypos,int &bonushp)
	{
		bonushp+=city[citypos].hp;
		city[citypos].hp=0;
		if(headquarter[belong].hp>8)
		{
			headquarter[belong].hp-=8;
			(city[citypos].warcraft[belong])->hp+=8;
		}
		city[citypos].warcraft[!belong]->alive=0;
		city[citypos].warcraft[!belong]=NULL;
	}
	void battle()
	{
		int i,temp,bonusred=0,bonusblue=0,*redwinlist=new int[N+1],redwinnersum=0;
		for(i=1;i<=N;i++)
			if(city[i].warcraft[RED]!=NULL&&city[i].warcraft[BLUE]!=NULL)
			{
				if((city[i].warcraft[RED])->hp<=0&&(city[i].warcraft[BLUE])->hp<=0)
				{
					city[i].warcraft[RED]->alive=city[i].warcraft[BLUE]->alive=0;
					city[i].warcraft[RED]=city[i].warcraft[BLUE]=NULL;
				}
				else if((city[i].warcraft[BLUE])->hp<=0)
				{
					city[i].win(clock,RED);
					redwinnersum++;
					redwinlist[redwinnersum]=i;
				}
				else if((city[i].warcraft[RED])->hp<=0)
				{
					city[i].win(clock,BLUE);
					bonus(BLUE,i,bonusblue);
				}
				else
				{
					temp=city[i].fight(1,clock,K);
					if(temp==BLUE)
					{
						redwinnersum++;
						redwinlist[redwinnersum]=i;
					}
					else if(temp==RED)
					{
						bonus(BLUE,i,bonusblue);
					}
				}
			}
			else if(city[i].warcraft[RED]!=NULL&&city[i].warcraft[RED]->hp<=0)
			{
				city[i].warcraft[RED]->alive=0;
				city[i].warcraft[RED]=NULL;
			}
			else if(city[i].warcraft[BLUE]!=NULL&&city[i].warcraft[BLUE]->hp<=0)
			{
				city[i].warcraft[BLUE]->alive=0;
				city[i].warcraft[BLUE]=NULL;
			}
		for(i=redwinnersum;i>=1;i--)
			bonus(RED,redwinlist[i],bonusred);
		headquarter[RED].hp+=bonusred;
		headquarter[BLUE].hp+=bonusblue;
		delete []redwinlist;
	}
public:
	WORLD(const int casenum)
	{
		cin>>M>>N>>R>>K>>T;
		int i;
		for(i=0;i<5;i++)
			cin>>hp0[i];
		for(i=0;i<5;i++)
			cin>>atk0[i];
		cout<<"Case "<<casenum<<":"<<endl;
		gameover=0;
		clock.init(T);
		headquarter[RED].init(M,RED);
		headquarter[BLUE].init(M,BLUE);
		city=new MAP[N+2];
		for(i=0;i<=N+1;i++)
			city[i].init(i);
		for(i=RED;i<=BLUE;i++)
			warcraft[i]=new WARCRAFT[T/60+3];
	}
	void run()
	{
		int i,j;
		while(1)
		{
			born(RED);
			born(BLUE);
			if(!(clock+=5))break;
			for(i=0;i<=N+1;i++)
				for(j=RED;j<=BLUE;j++)
					if(city[i].warcraft[j]!=NULL)lionranaway(i,j);
			if(!(clock+=5))break;
			move();
			if(gameover||!(clock+=10))break;
			for(i=1;i<=N;i++)
				city[i].hp+=10;
			if(!(clock+=10))break;
			for(i=1;i<=N;i++)
			    gethp(i);
			if(!(clock+=5))break;
			shot();
			if(!(clock+=3))break;
			usebomb();
			if(!(clock+=2))break;
			battle();
			if(!(clock+=10))break;
			for(i=RED;i<=BLUE;i++)
				cout<<clock<<headquarter[i];
			if(!(clock+=5))break;
			for(i=headquarter[RED].bornsum;i>=1;i--)
				if(warcraft[RED][i].alive||warcraft[RED][i].arrive)
				{
					cout<<clock;
					warcraft[RED][i].report();
				}
			for(i=1;i<=headquarter[BLUE].bornsum;i++)
				if(warcraft[BLUE][i].alive||warcraft[BLUE][i].arrive)
				{
					cout<<clock;
				    warcraft[BLUE][i].report();
				}
			if(!(clock+=5))break;
		}
		delete []city;
		for(i=RED;i<=BLUE;i++)
			delete []warcraft[i];
	}
};

int main()
{
	int casesum;
	cin>>casesum;
	for(int i=1;i<=casesum;i++)
	{
		WORLD worldofwarcraft(i);
		worldofwarcraft.run();
	}
	return 0;
}