/*
 * 项目：miniSQL
 * 日期：2012.12.30
 * 组长：-------
 * 组员：-------
 * 编译环境：Microsoft Visual C++ 6.0
 * 功能：
 * 1. 基本功能：SELECT语句（支持DISTINCT、WHERE、INNER JOIN、ORDER BY、IN语句和SELECT嵌套）、INSERT语句、DELETE语句、UPDATE语句
 * 2. 扩展功能：CREATE语句、DROP语句
 * 3. 对大规模文件的读写和外部排序
 * 4. 对语句语法错误、表达式语法错误等判错
 * 程序流程：
 * 1. 初始化窗口，并输入SQL命令文件的文件名以及存储数据库的文件夹
 * 2. 逐条读入SQL命令并进行处理
 *  (1)SELECT语句
 *    ①先找到FROM关键字，确定要找的表格，如果是多个表则连接起来
 *    ②再看是否有JOIN关键字，如果有，则将后面的表格连接起来（如果JOIN关键字后面跟着ON关键字，则连接时考虑ON关键字的后面的条件）
 *    ③然后找WHERE关键字，对WHERE的表达式进行计算（通过将中缀表达式转化成后缀表达式进行计算），只选取符合条件的行
 *    ④接着看是否有ORDER BY关键字，如果有，则根据要求进行外部排序（先对一定大小范围内的数据进行快速排序，并将有序的文件输出到硬盘上，
 *      再重新读入进行多路归并排序）
 *    ⑤最后选取查询的列，如果有DISTINCT关键字则剔除重复的行
 *  (2)INSERT语句
 *    ①先找到INTO关键字，确定要增加数据的表格
 *    ②然后确定增加的数据所在的列
 *    ③最后确定增加的数据，并增加在对应的表格上（修改文件）
 *  (3)DELETE语句
 *    ①先找到FROM关键字，确定要删除数据的表格
 *    ②再看是否有WHERE关键字，如果没有，则删除全部行，否则进行下一步
 *    ③计算WHERE的表达式，将符合条件的行删除（操作时实际上是将不符合条件的行保留）
 *  (4)UPDATE语句
 *    ①先找到UPDATE关键字，确定要更新数据的表格
 *    ②再找SET关键字，确定要更新数据的列和对应的数据
 *    ③再找WHERE关键字，将符合条件的行的数据更新
 *  (5)CREATE语句
 *    ①先找到TABLE关键字，确定要增加的表格的名称
 *    ②确定该表格的各列的标题名称及数据类型
 *  (6)DROP语句
 *    ①找到TABLE关键字，确定要删除的表格的名称，在硬盘中删除该表格
 * 测试方法：
 * 目录下有DB1、DB2、DB3三个存储数据库的文件夹和sql_command_1.txt、sql_command_2.txt、sql_command_3.txt三个SQL命令文件，
 * 三个文件夹和三个SQL命令文件分别一一对应，在编译器下运行本程序后，输入sql_command_1.txt和DB1\即可（2和3同理）
 */

#include<iostream>
#include<fstream>
#include<sstream>
#include<cstring>
#include<string>
#include<vector>
#include<algorithm>
#include<windows.h> /* 在Linux或Unix下编译时请删除此行 */

#define MAXLENGTH 2048 /* char数组的最大长度 */
#define MAXROW 5000 /* 储存在内存中的数据的最大行数 */
#define MAXPRINTLINE 9000 /* 输出到屏幕中的最大行数 */

/* 各SQL关键字的代号 */
#define SELECT 0
#define FROM 1
#define WHERE 2
#define INNER 3
#define JOIN 4
#define ON 5
#define DISTINCT 6
#define INSERT 7
#define INTO 8
#define VALUES 9
#define DELETE 10
#define UPDATE 11
#define SET 12
#define ORDER 13
#define BY 14
#define CREATE 15
#define DROP 16

/* 排序方式的代号 */
#define ASC 0
#define DESC 1

/* 表达式中数据类型的代号 */
#define CONST_STRING 1
#define CONST_INT 2
#define ARGUMENT 3
#define OPERATION 4
#define LEFT_BRAC 5
#define RIGHT_BRAC 6
#define RESULT_SET 7

/* 运算的代号 */
#define LESS 0
#define GREATER 10
#define EQL 20
#define LESS_EQL 30
#define GREATER_EQL 40
#define LESS_GREATER 50
#define AND 60
#define OR 70
#define LEFT 80
#define RIGHT 90
#define PLUS 100
#define MINUS 110
#define MULTI 120
#define DIV 130
#define MOD 140
#define IN 150
#define NOT_IN 160

/* 错误类型的代号 */
#define TABLE_NOT_FOUND -1
#define TITLE_NOT_FOUND -2
#define EXPRESSION_ERROR -3
#define RESULTSET_ERROR -4
#define GRAMMAR_MISTAKE -5
#define COMMAND_NOT_FOUND -6

using namespace std;

/* 类的声明 */
class KEYWORD_DATA;
class COMPARE_DATA;
class TABLE;
class EXPRESSION_DATA;

/* 函数声明 */
int atoi(string &st);
inline char upper_case(char ch);
string upper_case(const string &st);
void string2char(const string &str,char *buf);
inline bool not_charornum(char ch);
string noblank_string(string &str);
bool strncasecmp(const char *s1,const char *s2,int n);
bool strncasecmp(const string &s1,const string &s2,int n);
char *get_file(const char *filename);
inline int priority(int ope);
void sql_error(const int error_no,string msg1,string msg2);
void grammar_check(vector<KEYWORD_DATA> &vkw);
void find_keywords(const string &buf,vector<KEYWORD_DATA> &vkw);
void parse_select(string buf,const char *resultset,int *setsize);
void parse_insert(string buf);
void parse_delete(string buf);
void parse_update(string buf);
void parse_create(string buf);
void parse_drop(string buf);
void init_window(char *command_filename); /* 在Linux或Unix下编译时请删除此函数 */

char db_dir[MAXLENGTH]; /* 储存数据库的文件夹 */

/* 关键字 */
class KEYWORD_DATA
{
public:
	int type,beginpos,endpos; /* type为代号，beginpos和endpos分别表示该关键字在字符串的起始位置和结束位置 */

	KEYWORD_DATA(){};

	KEYWORD_DATA(const int t,const int b,const int e)
	{
		type=t;
		beginpos=b;
		endpos=e;
	}
};

/* 用于快速排序时的比较函数的比较数据 */
class COMPARE_DATA
{
public:
	int r,c;
	string **s;
	int **n;
	bool *isint,*rule;

	void init(const int R,const int C,const bool init_rule=1)
	{
		int i;
		r=R;
		c=C;
		if(init_rule)
		{
			rule=new bool[c];
			isint=new bool[c];
			for(i=0;i<c;i++)
			{
				rule[i]=ASC;
				isint[i]=1;
			}
		}
		s=new string*[r];
		for(i=0;i<r;i++)
			s[i]=new string[c];
		n=new int*[r];
		for(i=0;i<r;i++)
			n[i]=new int[c];
	}

	void destroy(const bool destroy_rule=0)
	{
		int i;
		if(destroy_rule)
		{
			delete []isint;
			delete []rule;
		}
		for(i=0;i<r;i++)
		{
			delete []s[i];
			delete []n[i];
		}
		delete []s;
		delete []n;
	}
}compare_data;

/* 表达式中的数据 */
class EXPRESSION_DATA
{
	friend class TABLE;
	int type,value; /* type是类型代号，value和st是不同类型下的数据值 */
	string st;

	EXPRESSION_DATA(){};

	EXPRESSION_DATA(const int typ,const int val)
	{
		type=typ;
		value=val;
	}

	EXPRESSION_DATA(const string s)
	{
		type=CONST_STRING;
		st=s;
	}

	EXPRESSION_DATA(const int typ,const string buf)
	{
		static int setnum=0;
		int i,j=-1,k;
		bool needselect=0;
		stringstream ss;
		char filename[MAXLENGTH];
		ofstream outfile;
		type=typ;
		setnum++;
		ss<<"~$tmpre"<<setnum;
		ss>>filename;
		st=filename;
		for(i=0;i<buf.length();i++)
			if(!strncasecmp(buf.substr(i,6),"SELECT",6))
			{
				needselect=1;
				break;
			}
		if(needselect)
		{
			parse_select(buf,filename,&value);
			return;
		}
		outfile.open(get_file(filename));
		value=0;
		for(i=0;i<buf.length();i++)
		{
			if(buf[i]==39)
			{
				value++;
				for(j=i+1;j<buf.length();j++)
					if(buf[j]==39)break;
				k=i+1;
				i=j+1;
				j--;
				outfile<<noblank_string(buf.substr(k,j-k+1))<<endl;
			}
			else if(buf[i]=='-'||(buf[i]>='0'&&buf[i]<='9'))
			{
				value++;
				k=i;
				i++;
				while(buf[i]>='0'&&buf[i]<='9')
					i++;
				j=i-1;
				outfile<<noblank_string(buf.substr(k,j-k+1))<<endl;
			}
		}
		outfile.close();
	}

	/* 与num数据做ope运算 */
	void calc(EXPRESSION_DATA &num,EXPRESSION_DATA &ope)
	{
		if(num.type==RESULT_SET&&(ope.value==IN||ope.value==NOT_IN))
		{
			ifstream infile;
			char filename[MAXLENGTH],buf[MAXLENGTH];
			int i;
			bool found=0;
			string2char(num.st,filename);
			infile.open(get_file(filename));
			for(i=0;i<num.value;i++)
			{
				infile.getline(buf,MAXLENGTH);
				if(type==CONST_INT&&value==atoi(buf))
				{
					value=(ope.value==IN);
					found=1;
					break;
				}
				else if(type==CONST_STRING&&!strncasecmp(st,buf,st.length()))
				{
					value=(ope.value==IN);
					found=1;
					break;
				}
			}
			if(!found)value=(ope.value!=IN);
			type=CONST_INT;
			infile.close();
			return;
		}
		if(num.type==RESULT_SET)
		{
			if(num.value!=1)sql_error(RESULTSET_ERROR,"结果集中有多个元素但只需访问一个","");
			ifstream infile;
			char filename[MAXLENGTH],buf[MAXLENGTH];
			string2char(num.st,filename);
			infile.open(get_file(filename));
			infile.getline(buf,MAXLENGTH);
			infile.close();
			num.type=type;
			if(type==CONST_INT)num.value=atoi(buf);
			else num.st=buf;
		}
		if(type==CONST_INT)
		{
			if(ope.value==LESS)value=value<num.value;
			else if(ope.value==GREATER)value=value>num.value;
			else if(ope.value==EQL)value=value==num.value;
			else if(ope.value==LESS_EQL)value=value<=num.value;
			else if(ope.value==GREATER_EQL)value=value>=num.value;
			else if(ope.value==LESS_GREATER)value=value!=num.value;
			else if(ope.value==AND)value=(!!value)&(!!num.value);
			else if(ope.value==OR)value=(!!value)|(!!num.value);
			else if(ope.value==PLUS)value+=num.value;
			else if(ope.value==MINUS)value-=num.value;
			else if(ope.value==MULTI)value*=num.value;
			else if(ope.value==DIV)value/=num.value;
			else if(ope.value==MOD)value%=num.value;
		}
		else if(type==CONST_STRING)
		{
			if(ope.value==LESS)value=upper_case(st)<upper_case(num.st);
			else if(ope.value==GREATER)value=upper_case(st)>upper_case(num.st);
			else if(ope.value==EQL)value=upper_case(st)==upper_case(num.st);
			else if(ope.value==LESS_EQL)value=upper_case(st)<=upper_case(num.st);
			else if(ope.value==GREATER_EQL)value=upper_case(st)>=upper_case(num.st);
			else if(ope.value==LESS_GREATER)value=upper_case(st)!=upper_case(num.st);
			else if(ope.value==PLUS)st+=num.st;
			if(ope.value!=PLUS)type=CONST_INT;
		}
	}
};

/* 表格类型 */
class TABLE
{
	int r,c,ts; /* r和c分别表示行数和列数，ts表示该表格包含的表格数量（即连接了多少个表格）*/
	bool *isint; /* isint表示某一列的数据类型是否为整型 */
	char *tablename; /* 表格名称 */
	string tn0,tablenamest,*title,*data; /* title记录每列的标题 */

	/* 分析中缀表达式buf，并转化成的后缀表达式储存在ved中 */
	void parse_infixexpre(const string &buf,vector<EXPRESSION_DATA> &ved)
	{
		int i,j,k,leftbrac=0,rightbrac=0;
		bool negon=1;
		vector<EXPRESSION_DATA> sed;
		string tn;
		if(ts==1)tn=tn0+".";
		i=0;
		while(i<buf.length())
		{
			if(buf[i]==' '||buf[i]=='\n')
			{
				i++;
			}
			else if(buf[i]==39)
			{
				j=i+1;
				while(buf[j]!=39)
					j++;
				i++;
				while(buf[i]==' ')
					i++;
				k=j-1;
				while(buf[k]==' ')
					k--;
				ved.push_back(EXPRESSION_DATA(noblank_string(buf.substr(i,k-i+1))));
				i=j+1;
				negon=0;
			}
			else if((buf[i]=='-'&&buf[i+1]>='0'&&buf[i+1]<='9'&&negon)||(buf[i]>='0'&&buf[i]<='9'))
			{
				j=i+1;
				while(buf[j]>='0'&&buf[j]<='9')
					j++;
				ved.push_back(EXPRESSION_DATA(CONST_INT,atoi(buf.substr(i,j-i))));
				i=j;
				negon=0;
			}
			else if(buf[i]=='(')
			{
				leftbrac++;
				sed.push_back(EXPRESSION_DATA(LEFT_BRAC,LEFT));
				i++;
				negon=1;
			}
			else if(buf[i]==')')
			{
				rightbrac++;
				if(rightbrac<=leftbrac)
				{
					while(!sed.empty()&&sed.back().type!=LEFT_BRAC)
					{
						ved.push_back(sed.back());
						sed.pop_back();
					}
					if(!sed.empty()&&sed.back().type==LEFT_BRAC)sed.pop_back();
				}
				i++;
				negon=0;
			}
			else if(!negon&&(buf[i]=='<'||buf[i]=='>'||buf[i]=='='||buf[i]=='+'||buf[i]=='-'||buf[i]=='*'||buf[i]=='/'||buf[i]=='%'||(!strncasecmp(buf.substr(i,3),"AND",3)&&not_charornum(buf[i+3]))||(!strncasecmp(buf.substr(i,2),"OR",2)&&not_charornum(buf[i+2]))))
			{
				EXPRESSION_DATA tmped;
				tmped.type=OPERATION;
				if(buf.substr(i,2)=="<=")
				{
					tmped.value=LESS_EQL;
					i+=2;
				}
				else if(buf.substr(i,2)==">=")
				{
					tmped.value=GREATER_EQL;
					i+=2;
				}
				else if(buf.substr(i,2)=="<>")
				{
					tmped.value=LESS_GREATER;
					i+=2;
				}
				else if(buf[i]=='<')
				{
					tmped.value=LESS;
					i++;
				}
				else if(buf[i]=='>')
				{
					tmped.value=GREATER;
					i++;
				}
				else if(buf[i]=='=')
				{
					tmped.value=EQL;
					i++;
				}
				else if(buf[i]=='+')
				{
					tmped.value=PLUS;
					i++;
				}
				else if(buf[i]=='-')
				{
					tmped.value=MINUS;
					i++;
				}
				else if(buf[i]=='*')
				{
					tmped.value=MULTI;
					i++;
				}
				else if(buf[i]=='/')
				{
					tmped.value=DIV;
					i++;
					}
				else if(buf[i]=='%')
				{
					tmped.value=MOD;
					i++;
				}
				else if(!strncasecmp(buf.substr(i,3),"AND",3))
				{
					tmped.value=AND;
					i+=3;
				}
				else if(!strncasecmp(buf.substr(i,2),"OR",2))
				{
					tmped.value=OR;
					i+=2;
				}
				while(!sed.empty()&&sed.back().type!=LEFT_BRAC&&priority(sed.back().value)>=priority(tmped.value))
				{
					ved.push_back(sed.back());
					sed.pop_back();
				}
				sed.push_back(tmped);
				negon=1;
			}
			else if(!strncasecmp(buf.substr(i,2),"IN",2)||!strncasecmp(buf.substr(i,3),"NOT",3))
			{
				int lefttmp=1,righttmp=0,qmnum=0;
				if(!strncasecmp(buf.substr(i,2),"IN",2))
				{
					sed.push_back(EXPRESSION_DATA(OPERATION,IN));
					i+=2;
				}
				else
				{
					sed.push_back(EXPRESSION_DATA(OPERATION,NOT_IN));
					i+=3;
					while(strncasecmp(buf.substr(i,2),"IN",2))
						i++;
					i+=2;
				}
				while(i<buf.length()&&buf[i]!='(')
					i++;
				if(i==buf.length())sql_error(EXPRESSION_ERROR," 缺少左括号 ","");
				j=i+1;
				i++;
				while(i<buf.length()&&lefttmp>righttmp)
				{
					qmnum+=(buf[i]==39);
					if(qmnum%2==0)
					{
						lefttmp+=buf[i]=='(';
						righttmp+=buf[i]==')';
					}
					i++;
				}
				if(lefttmp>righttmp)sql_error(EXPRESSION_ERROR," 缺少右括号 ","");
				k=i-2;
				ved.push_back(EXPRESSION_DATA(RESULT_SET,buf.substr(j,k-j+1)));
				negon=0;
			}
			else if(!strncasecmp(buf.substr(i,6),"SELECT",6)&&not_charornum(buf[i+6]))
			{
				int lefttmp=1,righttmp=0,qmnum=0;
				j=i;
				while(i<buf.length()&&lefttmp>righttmp)
				{
					qmnum+=(buf[i]==39);
					if(qmnum%2==0)
					{
						lefttmp+=buf[i]=='(';
						righttmp+=buf[i]==')';
					}
					i++;
				}
				if(lefttmp>righttmp)sql_error(EXPRESSION_ERROR," 缺少右括号 ","");
				k=i-2;
				ved.push_back(EXPRESSION_DATA(RESULT_SET,buf.substr(j,k-j+1)));
				negon=0;
			}
			else
			{
				j=i+1;
				k=i;
				while(buf[j]!='('&&buf[j]!=')'&&buf[j]!='<'&&buf[j]!='>'&&buf[j]!='='&&buf[j]!='+'&&buf[j]!='-'&&buf[j]!='*'&&buf[j]!='/'&&buf[j]!='%'&&(!not_charornum(buf[j-1])||strncasecmp(buf.substr(j,3),"AND",3))&&(!not_charornum(buf[j-1])||strncasecmp(buf.substr(j,2),"OR",2))&&(!not_charornum(buf[j-1])||strncasecmp(buf.substr(j,2),"IN",2))&&(!not_charornum(buf[j-1])||strncasecmp(buf.substr(j,3),"NOT",3))&&j<buf.length())
				{
					if(buf[j]!=' '&&buf[j]!='\n')k=j;
					j++;
				}
				string tmpst=buf.substr(i,k-i+1);
				for(k=0;k<c;k++)
				{
					if(!strncasecmp(title[k],tmpst,tmpst.length()))break;
					else if(ts==1&&!strncasecmp(tn+title[k],tmpst,tmpst.length()))break;
				}
				if(k==c)sql_error(TITLE_NOT_FOUND,tmpst,buf);
				ved.push_back(EXPRESSION_DATA(ARGUMENT,k));
				i=j;
				negon=0;
			}
		}
		while(!sed.empty())
		{
			if(sed.back().type==OPERATION)ved.push_back(sed.back());
			sed.pop_back();
		}
	}

	/* 计算后缀表达式ved，将结果保存在sed中 */
	void calculate(vector<EXPRESSION_DATA> &ved,EXPRESSION_DATA *sed)
	{
		int i,tmp,size=0;
		for(i=0;i<ved.size();i++)
		{
			if(ved[i].type==CONST_INT||ved[i].type==CONST_STRING||ved[i].type==RESULT_SET)
			{
				sed[size]=ved[i];
				size++;
			}
			else if(ved[i].type==ARGUMENT)
			{
				tmp=ved[i].value;
				if(isint[tmp])
				{
					sed[size].type=CONST_INT;
					sed[size].value=atoi(data[tmp]);
				}
				else
				{
					sed[size].type=CONST_STRING;
					sed[size].st=data[tmp];
				}
				size++;
			}
			else
			{
				if(size<2)
				{
					delete []sed;
					sql_error(EXPRESSION_ERROR," 缺少常量/变量 ","");
				}
				sed[size-2].calc(sed[size-1],ved[i]);
				size--;
			}
		}
		if(size!=1)
		{
			delete []sed;
			sql_error(EXPRESSION_ERROR," 缺少常量/变量 ","");
		}
	}

public:
	TABLE(){};

	/* 读取硬盘中文件名为fn的表格 */
	TABLE(string &fn)
	{
		int len=fn.length(),i;
		char *filename=new char[len+1],buf[MAXLENGTH];
		ifstream infile;
		tablename=new char[len+1];
		for(i=0;i<len;i++)
			tablename[i]=filename[i]=fn[i];
		tablename[len]=filename[len]='\0';
		tn0=tablenamest=string(tablename);
		infile.open(get_file(filename));
		if(infile==NULL)sql_error(TABLE_NOT_FOUND,filename,fn);
		infile>>r>>c;
		infile.get();
		isint=new bool[c];
		title=new string[c];
		data=new string[c];
		for(i=0;i<c;i++)
		{
			infile.getline(buf,MAXLENGTH);
			string tmp=string(buf);
			if(tmp.substr(0,5)=="(int)")
			{
				isint[i]=1;
				title[i]=noblank_string(tmp.substr(5,tmp.length()-5));
			}
			else
			{
				isint[i]=0;
				title[i]=noblank_string(tmp);
			}
		}
		ts=1;
		infile.close();
		delete []filename;
	}

	/* 快速排序中的比较函数 */
	friend bool compare(const int n1,const int n2)
	{
		int i;
		for(i=0;i<compare_data.c;i++)
		{
			if(compare_data.isint[i])
			{
				if(compare_data.n[n1][i]!=compare_data.n[n2][i])
				{
					return compare_data.rule[i]^(compare_data.n[n1][i]<compare_data.n[n2][i]);
				}
			}
			else
			{
				if(compare_data.s[n1][i]!=compare_data.s[n2][i])
				{
					return compare_data.rule[i]^(upper_case(compare_data.s[n1][i])<upper_case(compare_data.s[n2][i]));
				}
			}
		}
		return 0;
	}

	/* 处理WHERE语句，只保留结果和ans一样的行（SELECT和DELETE语句），
	 * 或者将结果和ans一样的行的数据更新为upd中的数据（UPDATE语句）
	 */
	void parse_where(const string &buf,int ans,const string *upd=NULL)
	{
		int i,j,tmp,newr=0,leftbrac=0,rightbrac=0;
		vector<EXPRESSION_DATA> ved;
		EXPRESSION_DATA *sed;
		string tn;
		char st[MAXLENGTH];
		ifstream infile;
		ofstream outfile;
		parse_infixexpre(buf,ved);
		sed=new EXPRESSION_DATA[ved.size()];
		infile.open(get_file(tablename));
		infile>>tmp>>tmp;
		infile.get();
		for(i=0;i<c;i++)
			infile.getline(st,MAXLENGTH);
		outfile.open(get_file("~$temp"));
		for(i=0;i<r;i++)
		{
			for(j=0;j<c;j++)
			{
				infile.getline(st,MAXLENGTH);
				data[j]=noblank_string(string(st));
			}
			calculate(ved,sed);
			if(upd==NULL)
			{
				if(sed->value==ans)
				{
					newr++;
					for(j=0;j<c;j++)
						outfile<<data[j]<<endl;
				}
			}
			else
			{
				newr++;
				if(sed->value==ans)
				{
					for(j=0;j<c;j++)
					{
						if(upd[j].length()!=0)outfile<<upd[j]<<endl;
						else outfile<<data[j]<<endl;
					}
				}
				else
				{
					for(j=0;j<c;j++)
						outfile<<data[j]<<endl;
				}
			}
		}
		delete []sed;
		infile.close();
		outfile.close();
		infile.open(get_file("~$temp"));
		outfile.open(get_file(tablename));
		outfile<<newr<<endl<<c<<endl;
		for(i=0;i<c;i++)
		{
			if(isint[i])outfile<<"(int)";
			outfile<<title[i]<<endl;
		}
		for(i=0;i<newr;i++)
			for(j=0;j<c;j++)
			{
				infile.getline(st,MAXLENGTH);
				outfile<<st<<endl;
			}
		infile.close();
		outfile.close();
		r=newr;
		remove(get_file("~$temp"));
		for(i=0;i<ved.size();i++)
			if(ved[i].type==RESULT_SET)
			{
				string2char(ved[i].st,st);
				remove(get_file(st));
			}
	}

	/* 外部排序：分成若干个最大行数不超过MAXROW的文件分别进行快速排序，再进行多路归并排序 */
	void extern_sort(const string &buf)
	{
		int i,j,k,tmp,expre_num=1,beginpos,endpos,order[MAXROW],*pagelen,maxvedsize=0;
		vector<EXPRESSION_DATA> *ved;
		EXPRESSION_DATA *sed;
		char st[MAXLENGTH];
		ifstream infile,*arrinfs,*arrinfd;
		ofstream outfile;
		string **str;
		for(i=0;i<buf.length();i++)
			expre_num+=buf[i]==',';
		ved=new vector<EXPRESSION_DATA>[expre_num];
		compare_data.init(MAXROW,expre_num);
		beginpos=0;
		k=0;
		for(i=0;i<buf.length();i++)
		{
			if(buf[i]==','||i==buf.length()-1)
			{
				if(buf[i]==',')endpos=i-1;
				else endpos=i;
				for(j=endpos;j>beginpos;j--)
				{
					if(!strncasecmp(buf.substr(j,3),"ASC",3)&&not_charornum(buf[j-1])&&not_charornum(buf[j+3]))
					{
						endpos=j-1;
						break;
					}
					else if(!strncasecmp(buf.substr(j,4),"DESC",4)&&not_charornum(buf[j-1])&&not_charornum(buf[j+4]))
					{
						endpos=j-1;
						compare_data.rule[k]=DESC;
						break;
					}
				}
				parse_infixexpre(buf.substr(beginpos,endpos-beginpos+1),ved[k]);
				if(ved[k].size()>maxvedsize)maxvedsize=ved[k].size();
				k++;
				beginpos=i+1;
			}
		}
		str=new string*[MAXROW];
		for(i=0;i<MAXROW;i++)
			str[i]=new string[c];
		infile.open(get_file(tablename));
		infile>>tmp>>tmp;
		infile.get();
		for(i=0;i<c;i++)
			infile.getline(st,MAXLENGTH);
		pagelen=new int[(r-1)/MAXROW+1];
		sed=new EXPRESSION_DATA[maxvedsize];
		for(i=0;i<r;i++)
		{
			for(j=0;j<c;j++)
			{
				infile.getline(st,MAXLENGTH);
				str[i%MAXROW][j]=data[j]=noblank_string(string(st));
			}
			for(j=0;j<expre_num;j++)
			{
				calculate(ved[j],sed);
				if(sed->type==CONST_STRING)
				{
					compare_data.isint[j]=0;
					compare_data.s[i%MAXROW][j]=sed->st;
				}
				else
				{
					compare_data.n[i%MAXROW][j]=sed->value;
				}
			}
			if(i%MAXROW==MAXROW-1||i==r-1)
			{
				for(j=0;j<=i%MAXROW;j++)
					order[j]=j;
				sort(order,order+i%MAXROW+1,compare);
				stringstream ss1,ss2;
				ss1<<"~$tmpsd"<<i/MAXROW;
				ss1>>st;
				outfile.open(get_file(st));
				for(j=0;j<=i%MAXROW;j++)
					for(k=0;k<expre_num;k++)
					{
						if(compare_data.isint[k])outfile<<compare_data.n[order[j]][k]<<endl;
						else outfile<<compare_data.s[order[j]][k]<<endl;
					}
				outfile.close();
				ss2<<"~$tmpss"<<i/MAXROW;
				ss2>>st;
				outfile.open(get_file(st));
				for(j=0;j<=i%MAXROW;j++)
					for(k=0;k<c;k++)
						outfile<<str[order[j]][k]<<endl;
				outfile.close();
				pagelen[i/MAXROW]=i%MAXROW+1;
			}
		}
		infile.close();
		compare_data.destroy();
		compare_data.init((r-1)/MAXROW+1,expre_num,0);
		for(i=0;i<c;i++)
			delete []str[i];
		delete []str;
		delete []sed;
		arrinfs=new ifstream[(r-1)/MAXROW+1];
		arrinfd=new ifstream[(r-1)/MAXROW+1];
		for(i=0;i<=(r-1)/MAXROW;i++)
		{
			stringstream ss1,ss2;
			ss1<<"~$tmpsd"<<i;
			ss1>>st;
			arrinfd[i].open(get_file(st));
			ss2<<"~$tmpss"<<i;
			ss2>>st;
			arrinfs[i].open(get_file(st));
		}
		outfile.open(get_file("~$tmpsr"));
		outfile<<r<<endl<<c<<endl;
		for(i=0;i<c;i++)
		{
			if(isint[i])outfile<<"(int)";
			outfile<<title[i]<<endl;
		}
		for(i=0;i<=(r-1)/MAXROW;i++)
		{
			pagelen[i]--;
			for(j=0;j<expre_num;j++)
			{
				if(compare_data.isint[j])
				{
					arrinfd[i]>>compare_data.n[i][j];
					arrinfd[i].get();
				}
				else
				{
					arrinfd[i].getline(st,MAXLENGTH);
					compare_data.s[i][j]=noblank_string(string(st));
				}
			}
			if(!pagelen[i])
			{
				arrinfd[i].close();
				stringstream ss;
				ss<<"~$tmpsd"<<i;
				ss>>st;
				remove(get_file(st));
			}
		}
		for(k=0;k<r;k++)
		{
			bool first=0;
			int pos;
			for(i=0;i<=(r-1)/MAXROW;i++)
			{
				if(pagelen[i]>=0)
				{
					if(!first)
					{
						first=1;
						pos=i;
					}
					else if(compare(i,pos))pos=i;
				}
			}
			if(first)
			{
				if(pagelen[pos]>0)
				{
					for(j=0;j<expre_num;j++)
					{
						if(compare_data.isint[j])
						{
							arrinfd[pos]>>compare_data.n[pos][j];
							arrinfd[pos].get();
						}
						else
						{
							arrinfd[pos].getline(st,MAXLENGTH);
							compare_data.s[pos][j]=noblank_string(string(st));
						}
					}
				}
				for(i=0;i<c;i++)
				{
					arrinfs[pos].getline(st,MAXLENGTH);
					outfile<<noblank_string(string(st))<<endl;
				}
				pagelen[pos]--;
				if(pagelen[pos]==0)
				{
					arrinfd[pos].close();
					stringstream ss;
					ss<<"~$tmpsd"<<pos;
					ss>>st;
					remove(get_file(st));
				}
				else if(pagelen[pos]==-1)
				{
					arrinfs[pos].close();
					stringstream ss;
					ss<<"~$tmpss"<<pos;
					ss>>st;
					remove(get_file(st));
				}
			}
		}
		outfile.close();
		compare_data.destroy(1);
		delete []pagelen;
		delete []ved;
		delete []arrinfs;
		delete []arrinfd;
		remove(get_file(tablename));
		strcpy(st,get_file("~$tmpsr"));
		rename(st,get_file(tablename));
	}

	/* DISTINCT处理：用外部排序的方法剔除重复的行 */
	void distinct()
	{
		int i,j,k,tmp,newr;
		bool b;
		vector<int> vdelrow;
		TABLE tmptd;
		string buf="  ",addtitle="KEYid";
		char st[MAXLENGTH];
		ifstream infile;
		ofstream outfile;
		while(1)
		{
			b=1;
			for(i=0;i<c;i++)
				if(!strncasecmp(addtitle,title[i],addtitle.length()))
				{
					b=0;
					break;
				}
			if(b)break;
			else addtitle+="o";
		}
		infile.open(get_file(tablename));
		outfile.open(get_file("~$tmpds"));
		infile>>tmp>>tmp;
		infile.get();
		outfile<<r<<endl<<c+1<<endl<<"(int)"<<addtitle<<endl;
		for(i=0;i<c;i++)
		{
			infile.getline(st,MAXLENGTH);
			outfile<<st<<endl;
		}
		for(i=0;i<r;i++)
		{
			outfile<<i<<endl;
			for(j=0;j<c;j++)
			{
				infile.getline(st,MAXLENGTH);
				outfile<<st<<endl;
			}
		}
		infile.close();
		outfile.close();
		tmptd=TABLE(string("~$tmpds"));
		for(i=0;i<c;i++)
		{
			if(i)buf+=" , ";
			buf+=title[i];
		}
		buf+=" , "+addtitle+"  ";
		tmptd.extern_sort(buf);
		b=0;
		infile.open(get_file("~$tmpds"));
		infile>>tmp>>tmp;
		infile.get();
		for(i=0;i<=c+1;i++)
			infile.getline(st,MAXLENGTH);
		for(i=0;i<c;i++)
		{
			infile.getline(st,MAXLENGTH);
			data[i]=noblank_string(string(st));
		}
		for(i=1;i<r;i++)
		{
			infile.getline(st,MAXLENGTH);
			tmptd.data[c]=st;
			for(j=0;j<c;j++)
			{
				infile.getline(st,MAXLENGTH);
				tmptd.data[j]=noblank_string(string(st));
			}
			b=1;
			for(j=0;j<c;j++)
				if(strncasecmp(data[j],tmptd.data[j],data[j].length()))
				{
					b=0;
					break;
				}
			if(b)
			{
				vdelrow.push_back(atoi(tmptd.data[c]));
			}
			for(j=0;j<c;j++)
				data[j]=tmptd.data[j];
		}
		infile.close();
		sort(vdelrow.begin(),vdelrow.end());
		newr=r-vdelrow.size();
		outfile.open(get_file("~$tmpds"));
		infile.open(get_file(tablename));
		infile>>tmp>>tmp;
		infile.get();
		outfile<<newr<<endl<<c<<endl;
		for(i=0;i<c;i++)
		{
			infile.getline(st,MAXLENGTH);
			outfile<<st<<endl;
		}
		k=0;
		for(i=0;i<r;i++)
		{
			b=k<vdelrow.size()&&i==vdelrow[k];
			k+=b;
			for(j=0;j<c;j++)
			{
				infile.getline(st,MAXLENGTH);
				if(!b)outfile<<st<<endl;
			}
		}
		infile.close();
		outfile.close();
		remove(get_file(tablename));
		strcpy(st,get_file("~$tmpds"));
		rename(st,get_file(tablename));
		r=newr;
	}

	/* 选取buf中包括的列 */
	void select_colomn(const string &buf)
	{
		int i,j,k,tmp,strbeginpos=-1,strendpos=0;
		char st[MAXLENGTH];
		vector<int> vc;
		vector<string> vt;
		string *mt=title;
		bool *mb=isint;
		ifstream infile;
		ofstream outfile;
		for(i=0;i<buf.length();i++)
		{
			if(buf[i]=='*')return;
			else if(buf[i]!=' '&&buf[i]!=','&&buf[i]!='\n')
			{
				if(strbeginpos==-1)strbeginpos=strendpos=i;
				strendpos=i;
			}
			else if(buf[i]==','||i==buf.length()-1)
			{
				if(strbeginpos!=-1)
				{
					string oldname,newname;
					bool changename=0;
					for(j=strbeginpos;j<strendpos-3;j++)
						if(!strncasecmp(buf.substr(j,4)," AS ",4))
						{
							changename=1;
							break;
						}
					if(!changename)
					{
						oldname=newname=buf.substr(strbeginpos,strendpos-strbeginpos+1);
					}
					else
					{
						for(k=j; ;k--)
							if(buf[k]!=' '&&buf[k]!='\n')break;
						oldname=buf.substr(strbeginpos,k-strbeginpos+1);
						for(k=j+3; ;k++)
							if(buf[k]!=' '&&buf[k]!='\n')break;
						newname=buf.substr(k,strendpos-k+1);
					}
					vt.push_back(newname);
					for(j=0;j<c;j++)
					{
						if(!strncasecmp(title[j],oldname,oldname.length()))break;
						else if(ts==1&&!strncasecmp(tn0+"."+title[j],oldname,oldname.length()))break;
					}
					if(j==c)sql_error(TITLE_NOT_FOUND,oldname,buf);
					vc.push_back(j);
					if(changename)mt[j]=newname;
				}
				strbeginpos=strendpos=-1;
			}
		}
		infile.open(get_file(tablename));
		outfile.open(get_file("~$temp"));
		infile>>tmp>>tmp;
		infile.get();
		outfile<<r<<endl<<vt.size()<<endl;
		for(i=0;i<c;i++)
			infile.getline(st,MAXLENGTH);
		for(i=0;i<vt.size();i++)
		{
			if(isint[vc[i]])outfile<<"(int)";
			outfile<<vt[i]<<endl;
		}
		for(i=0;i<r;i++)
		{
			for(j=0;j<c;j++)
			{
				infile.getline(st,MAXLENGTH);
				data[j]=noblank_string(string(st));
			}
			for(j=0;j<vt.size();j++)
				outfile<<data[vc[j]]<<endl;
		}
		infile.close();
		outfile.close();
		remove(get_file(tablename));
		strcpy(st,get_file("~$temp"));
		rename(st,get_file(tablename));
		c=vt.size();
		isint=new bool[c];
		title=new string[c];
		for(i=0;i<c;i++)
		{
			isint[i]=mb[vc[i]];
			title[i]=mt[vc[i]];
		}
		delete []mb;
		delete []mt;
		delete []data;
		data=new string[c];
	}

	/* 将buf2中的数据插入的buf1的列中，或插入到所有列中（当selectall为真时）*/
	void insert(const bool selectall,const string &buf1,const string &buf2)
	{
		int i,j,k,tmp,strbeginpos=-1,strendpos;
		char st[MAXLENGTH];
		TABLE tmptd;
		vector<int> vc;
		vector<string> vs;
		ifstream infile;
		ofstream outfile;
		if(selectall)
		{
			for(i=0;i<c;i++)
				vc.push_back(i);
		}
		else
		{
			i=0;
			while(i<buf1.length())
			{
				if(buf1[i]!='('&&buf1[i]!=')'&&buf1[i]!=','&&buf1[i]!=' '&&buf1[i]!='\n')
				{
					if(strbeginpos==-1)strbeginpos=i;
					strendpos=i;
				}
				else if(buf1[i]==','||buf1[i]==')')
				{
					for(j=0;j<c;j++)
						if(!strncasecmp(title[j],buf1.substr(strbeginpos,strendpos-strbeginpos+1),title[j].length()))break;
					if(j==c)sql_error(TITLE_NOT_FOUND,buf1.substr(strbeginpos,strendpos-strbeginpos+1),buf1);
					vc.push_back(j);
					strbeginpos=-1;
					if(buf1[i]==')')break;
				}
				i++;
			}
		}
		for(i=0;i<buf2.length();i++)
		{
			if(buf2[i]==')')break;
			else if(buf2[i]==39)
			{
				j=i+1;
				while(j<buf2.length()&&buf2[j]!=39)
					j++;
				if(j==buf2.length())sql_error(GRAMMAR_MISTAKE,"INSERT","缺少右括号");
				i++;
				while(buf2[i]==' ')
					i++;
				k=j-1;
				while(buf2[k]==' ')
					k--;
				vs.push_back(buf2.substr(i,k-i+1));
				i=j+1;
			}
			else if(buf2[i]=='-'||(buf2[i]>='0'&&buf2[i]<='9'))
			{
				j=i+1;
				while(j<buf2.length()&&buf2[j]>='0'&&buf2[j]<='9')
					j++;
				if(j==buf2.length())sql_error(GRAMMAR_MISTAKE,"INSERT","缺少右括号");
				vs.push_back(buf2.substr(i,j-i));
				i=j;
			}
		}
		if(vc.size()!=vs.size())sql_error(GRAMMAR_MISTAKE,"INSERT","数据不匹配");
		copy("~$temp",tmptd);
		for(i=0;i<c;i++)
			data[i]="";
		for(i=0;i<vc.size();i++)
			data[vc[i]]=vs[i];
		r++;
		infile.open(get_file("~$temp"));
		outfile.open(get_file(tablename));
		outfile<<r<<endl<<c<<endl;
		infile>>tmp>>tmp;
		infile.get();
		for(i=0;i<c;i++)
		{
			infile.getline(st,MAXLENGTH);
			outfile<<st<<endl;
		}
		for(i=0;i<r-1;i++)
			for(j=0;j<c;j++)
			{
				infile.getline(st,MAXLENGTH);
				outfile<<st<<endl;
			}
		infile.close();
		remove(get_file("~$temp"));
		for(i=0;i<c;i++)
			outfile<<data[i]<<endl;
		outfile.close();
	}

	/* 满足buf2条件的行更新数据 */
	void update(const string &buf1,const string &buf2)
	{
		int i,j,strbeginpos=-2,strendpos;
		string *update_data=NULL;
		vector<int> vc;
		vector<string> vs;
		i=0;
		while(i<buf1.length())
		{
			if(buf1[i]!=' '&&buf1[i]!='\n'&&buf1[i]!='='&&buf1[i]!=',')
			{
				if(strbeginpos<0)strbeginpos=i;
				strendpos=i;
			}
			else if(buf1[i]=='=')
			{
				for(j=0;j<c;j++)
					if(!strncasecmp(title[j],buf1.substr(strbeginpos,strendpos-strbeginpos+1),title[j].length()))break;
				if(j==c)sql_error(TITLE_NOT_FOUND,buf1.substr(strbeginpos,strendpos-strbeginpos+1),buf1);
				vc.push_back(j);
				i++;
				while(buf1[i]==' '||buf1[i]=='\n')
					i++;
				if(buf1[i]==39)
				{
					strbeginpos=i+1;
					i++;
					while(buf1[i]!=39)
						i++;
					strendpos=i-1;
					vs.push_back(noblank_string(buf1.substr(strbeginpos,strendpos-strbeginpos+1)));
				}
				else if(buf1[i]=='-'||(buf1[i]>='0'&&buf1[i]<='9'))
				{
					strbeginpos=i;
					i++;
					while(buf1[i]>='0'&&buf1[i]<='9')
						i++;
					strendpos=i-1;
					vs.push_back(buf1.substr(strbeginpos,strendpos-strbeginpos+1));
				}
				while(i<buf1.length())
				{
					if(buf1[i]==',')break;
					i++;
				}
				strbeginpos=-1;
			}
			i++;
		}
		if(strbeginpos!=-1)sql_error(GRAMMAR_MISTAKE,"UPDATE","未找到更新的列和数据");
		update_data=new string[c];
		for(i=0;i<c;i++)
			update_data[i]="";
		for(i=0;i<vc.size();i++)
			update_data[vc[i]]=vs[i];
		parse_where(buf2,1,update_data);
		delete []update_data;
	}

	/* 将当前表格复制到td中，并在硬盘中以newfn命名 */
	void copy(const char *newfn,TABLE &td,const bool usenewtitle=0)
	{
		int len=strlen(newfn),i,j,tmp;
		char buf[MAXLENGTH];
		ofstream outfile;
		ifstream infile;
		infile.open(get_file(tablename));
		outfile.open(get_file(newfn));
		outfile<<r<<endl;
		infile>>tmp;
		td.r=r;
		outfile<<c<<endl;
		infile>>tmp;
		td.c=c;
		td.ts=ts;
		td.tn0=tn0;
		td.tablename=new char[len+1];
		strcpy(td.tablename,newfn);
		td.tablename[len]='\0';
		td.tablenamest=string(newfn);
		td.isint=new bool[c];
		td.title=new string[c];
		td.data=new string[c];
		infile.get();
		for(i=0;i<c;i++)
		{
			td.isint[i]=isint[i];
			if(usenewtitle)td.title[i]=tablenamest+"."+noblank_string(title[i]);
			else td.title[i]=noblank_string(title[i]);
			if(isint[i])outfile<<"(int)";
			outfile<<td.title[i]<<endl;
			infile.getline(buf,MAXLENGTH);
		}
		for(i=0;i<r;i++)
			for(j=0;j<c;j++)
			{
				infile.getline(buf,MAXLENGTH);
				outfile<<noblank_string(string(buf))<<endl;
			}
		infile.close();
		outfile.close();
	}

	/* 根据on条件连接表格td */
	void join(const TABLE &td,const string &on="")
	{
		int i,j,k,l,tmp,oldc=c,newr=0,leftr;
		char buf[MAXLENGTH];
		bool *mb=isint,parseon=on.length()>0;
		string *mt=title,*md=data,**tmpdata;
		ifstream infile1,infile2;
		ofstream outfile;
		vector<EXPRESSION_DATA> ved;
		EXPRESSION_DATA *sed;
		c+=td.c;
		isint=new bool[c];
		data=new string[c];
		title=new string[c];
		for(i=0;i<oldc;i++)
		{
			isint[i]=mb[i];
			title[i]=mt[i];
		}
		for(i=0;i<td.c;i++)
		{
			isint[oldc+i]=td.isint[i];
			title[oldc+i]=td.tablenamest+"."+noblank_string(td.title[i]);
		}
		if(parseon)
		{
			parse_infixexpre(on,ved);
			sed=new EXPRESSION_DATA[ved.size()];
		}
		outfile.open(get_file("~$tmpj"));
		outfile<<r*td.r<<endl<<c<<endl;
		infile1.open(get_file(tablename));
		infile1>>tmp>>tmp;
		infile1.get();
		for(i=0;i<oldc;i++)
		{
			infile1.getline(buf,MAXLENGTH);
			outfile<<noblank_string(string(buf))<<endl;
		}
		infile1.close();
		for(i=0;i<td.c;i++)
		{
			if(td.isint[i])outfile<<"(int)";
			outfile<<td.tablenamest<<"."<<td.title[i]<<endl;
		}
		infile2.open(get_file(td.tablename));
		for(j=0;j<td.c+2;j++)
			infile2.getline(buf,MAXLENGTH);
		tmpdata=new string*[MAXROW];
		for(i=0;i<MAXROW;i++)
			tmpdata[i]=new string[td.c];
		leftr=td.r;
		for(i=0;i<=(td.r-1)/MAXROW;i++)
		{
			int readr=0;
			for(j=0;j<MAXROW;j++)
			{
				for(k=0;k<td.c;k++)
				{
					infile2.getline(buf,MAXLENGTH);
					tmpdata[j][k]=noblank_string(string(buf));
				}
				leftr--;
				readr++;
				if(leftr<=0)break;
			}
			infile1.open(get_file(tablename));
			for(j=0;j<oldc+2;j++)
				infile1.getline(buf,MAXLENGTH);
			for(j=0;j<r;j++)
			{
				for(k=0;k<oldc;k++)
				{
					infile1.getline(buf,MAXLENGTH);
					data[k]=noblank_string(string(buf));
				}
				for(k=0;k<readr;k++)
				{
					for(l=0;l<td.c;l++)
						data[oldc+l]=tmpdata[k][l];
					if(parseon)calculate(ved,sed);
					if(!parseon||sed->value==1)
					{
						newr++;
						for(l=0;l<c;l++)
							outfile<<data[l]<<endl;
					}
				}
			}
			infile1.close();
		}
		infile2.close();
		outfile.close();
		for(i=0;i<MAXROW;i++)
			delete []tmpdata[i];
		delete []tmpdata;
		delete []mb;
		delete []mt;
		delete []md;
		ts++;
		if(parseon&&newr!=r*td.r)
		{
			delete []sed;
			r=newr;
			infile1.open(get_file("~$tmpj"));
			outfile.open(get_file("~$tmpjo"));
			infile1.getline(buf,MAXLENGTH);
			outfile<<newr<<endl;
			while(infile1.getline(buf,MAXLENGTH))
				outfile<<buf<<endl;
			infile1.close();
			outfile.close();
			remove(get_file("~$tmpj"));
			remove(get_file(tablename));
			strcpy(buf,get_file("~$tmpjo"));
			rename(buf,get_file(tablename));
		}
		else
		{
			r*=td.r;
			remove(get_file(tablename));
			strcpy(buf,get_file("~$tmpj"));
			rename(buf,get_file(tablename));
		}
	}

	/* 输出当前表格到屏幕中和文件中 */
	void print(const string &command)
	{
		static int printtime=0;
		int i,j,k,rtmp,ctmp,*maxlen=new int[c];
		char *buf=new char[MAXLENGTH];
		stringstream ss;
		string st;
		ifstream infile;
		ofstream outfile;
		infile.open(get_file(tablename));
		infile>>rtmp>>ctmp;
		infile.get();
		for(i=0;i<c;i++)
		{
			infile.getline(buf,MAXLENGTH);
			if(!strncmp(buf,"(int)",5))buf+=5;
			st=noblank_string(string(buf));
			maxlen[i]=st.length();
		}
		for(i=0;i<r;i++)
		{
			for(j=0;j<c;j++)
			{
				infile.getline(buf,MAXLENGTH);
				st=noblank_string(string(buf));
				if(st.length()>maxlen[j])maxlen[j]=st.length();
			}
		}
		infile.close();
		printtime++;
		ss<<"Output"<<printtime<<".txt";
		ss>>buf;
		outfile.open(buf);
		infile.open(get_file(tablename));
		infile>>rtmp>>ctmp;
		outfile<<"SQL命令 : "<<endl<<command<<";"<<endl<<endl;
		cout<<endl<<"表格大小 : "<<rtmp<<"行×"<<ctmp<<"列"<<endl<<endl;
		outfile<<"表格大小 : "<<rtmp<<"行×"<<ctmp<<"列"<<endl<<endl;
		infile.get();
		for(i=0;i<ctmp;i++)
		{
			if(i)
			{
				cout<<"|";
				outfile<<"|";
			}
			infile.getline(buf,MAXLENGTH);
			if(!strncmp(buf,"(int)",5))buf+=5;
			st=noblank_string(string(buf));
			cout<<" "<<st;
			outfile<<" "<<st;
			for(j=0;j<=maxlen[i]-st.length();j++)
			{
				cout<<" ";
				outfile<<" ";
			}
		}
		cout<<endl;
		outfile<<endl;
		for(i=0;i<ctmp;i++)
		{
			if(i)
			{
				cout<<"|";
				outfile<<"|";
			}
			for(j=0;j<=maxlen[i]+1;j++)
			{
				cout<<"-";
				outfile<<"-";
			}
		}
		cout<<endl;
		outfile<<endl;
		for(i=0;i<rtmp;i++)
		{
			if(i==MAXPRINTLINE)cout<<endl<<"输出数据太大，剩余部分只输出到Output"<<printtime<<".txt中，不输出到屏幕中"<<endl;
			for(j=0;j<ctmp;j++)
			{
				if(j)
				{
					if(i<MAXPRINTLINE)cout<<"|";
					outfile<<"|";
				}
				infile.getline(buf,MAXLENGTH);
				st=noblank_string(string(buf));
				if(i<MAXPRINTLINE)cout<<" "<<st;
				outfile<<" "<<st;
				for(k=0;k<=maxlen[j]-st.length();k++)
				{
					if(i<MAXPRINTLINE)cout<<" ";
					outfile<<" ";
				}
			}
			if(i<MAXPRINTLINE)cout<<endl;
			outfile<<endl;
		}
		cout<<endl<<"表格输出完毕，同时保存在Output"<<printtime<<".txt中"<<endl<<endl;
		infile.close();
		outfile.close();
		delete []maxlen;
	}

	/* 将表格以结果集的形式输出到硬盘中 */
	void write_set(const char *setname,int *setsize)
	{
		int i,tmp;
		char buf[MAXLENGTH];
		ifstream infile;
		ofstream outfile;
		if(c!=1)sql_error(RESULTSET_ERROR,"结果集中出现多列","");
		infile.open(get_file(tablename));
		infile>>tmp>>tmp;
		infile.get();
		infile.getline(buf,MAXLENGTH);
		outfile.open(get_file(setname));
		*setsize=r;
		for(i=0;i<r;i++)
		{
			infile.getline(buf,MAXLENGTH);
			outfile<<noblank_string(string(buf))<<endl;
		}
		infile.close();
		outfile.close();
	}
};

/* 将st转换为整型 */
int atoi(string &st)
{
	int i,len=st.length(),ans=0;
	for(i=0;i<len;i++)
		ans=ans*10+st[i]-'0';
	return ans;
}

/* 返回大写字母 */
inline char upper_case(char ch)
{
	if(ch>='a'&&ch<='z')return ch-'a'+'A';
	return ch;
}

/* 将st的各个字母都变成大写字母 */
string upper_case(const string &st)
{
	int i=st.length();
	string str=st;
	for(i--;i>=0;i--)
		str[i]=upper_case(st[i]);
	return str;
}

/* 将string类型的字符串转换为char*类型的字符串 */
void string2char(const string &str,char *buf)
{
	int i,len=str.length();
	for(i=0;i<len;i++)
		buf[i]=str[i];
	buf[len]='\0';
}

/* 判断某个字符是否字母/数字/下划线 */
inline bool not_charornum(char ch)
{
	return !((ch>='a'&&ch<'z')||(ch>='A'&&ch<'Z')||(ch>='0'&&ch<='9')||ch=='_');
}

/* 删除字符串头和尾的空格和换行符 */
string noblank_string(string &str)
{
	int i,j;
	for(i=0;i<str.length();i++)
		if(str[i]!=' '&&str[i]!='\n')break;
	for(j=str.length()-1;j>=i;j--)
		if(str[j]!=' '&&str[j]!='\n')break;
	return str.substr(i,j-i+1);
}

/* 忽略大小写比较s1和s2的前n个字符，下同 */
bool strncasecmp(const char *s1,const char *s2,int n)
{
	int i;
	for(i=0;i<n;i++)
		if(s1[i]=='\0'||s2[i]=='\0'||upper_case(s1[i])!=upper_case(s2[i]))return 1;
	return 0;
}

bool strncasecmp(const string &s1,const string &s2,int n)
{
	int i;
	for(i=0;i<n;i++)
		if(s1[i]=='\0'||s2[i]=='\0'||upper_case(s1[i])!=upper_case(s2[i]))return 1;
	return 0;
}

/* 在文件filename之前加上文件夹db_dir名 */
char *get_file(const char *filename)
{
	static char filepathname[MAXLENGTH];
	int len1=strlen(db_dir),len2=strlen(filename),i;
	for(i=0;i<len1;i++)
		filepathname[i]=db_dir[i];
	for(i=0;i<len2;i++)
		filepathname[i+len1]=filename[i];
	filepathname[len1+len2]='\0';
	return filepathname;
}

/* 返回某个运算的优先级 */
inline int priority(int ope)
{
	if(ope==PLUS||ope==MINUS)return 4;
	else if(ope==MULTI||ope==DIV||ope==MOD)return 5;
	else if(ope==AND)return 2;
	else if(ope==OR)return 1;
	else return 3;
}

/* 报错函数 */
void sql_error(const int error_no,string msg1,string msg2)
{
	int i;
	ofstream outfile;
	outfile.open("Err.log",ios::app);
	for(i=0;i<msg1.length();i++)
		if(msg1[i]=='\n')msg1[i]=' ';
	for(i=0;i<msg2.length();i++)
		if(msg2[i]=='\n')msg2[i]=' ';
	if(error_no==TABLE_NOT_FOUND)
	{
		cerr<<endl<<"出错 :"<<endl<<"在语句 \""<<msg2<<"\" 中未找到表格 \""<<msg1<<"\"，可能是数据库路径名错误"<<endl;
		outfile<<"出错 : 在语句 \""<<msg2<<"\" 中未找到表格 \""<<msg1<<"\"，可能是数据库路径名错误"<<endl;
	}
	else if(error_no==TITLE_NOT_FOUND)
	{
		cerr<<endl<<"出错 :"<<endl<<"在语句 \""<<msg2<<"\" 中未找到标题为 \""<<msg1<<"\" 的列"<<endl;
		outfile<<"出错 : 在语句 \""<<msg2<<"\" 中未找到标题为 \""<<msg1<<"\" 的列"<<endl;
	}
	else if(error_no==EXPRESSION_ERROR)
	{
		cerr<<endl<<"出错 :"<<endl<<"WHERE/ON/ORDER BY表达式语法错误 -"<<msg1<<endl;
		outfile<<"出错 : WHERE/ON/ORDER BY表达式语法错误 -"<<msg1<<endl;
	}
	else if(error_no==RESULTSET_ERROR)
	{
		cerr<<endl<<"出错 :"<<endl<<msg1<<endl;
		outfile<<"出错 : "<<msg1<<endl;
	}
	else if(error_no==GRAMMAR_MISTAKE)
	{
		cerr<<endl<<"出错 :"<<endl<<"在语句 \""<<msg1<<"\" 中出现语法错误 - "<<msg2<<endl;
		outfile<<"出错 : 在语句 \""<<msg1<<"\" 中出现语法错误 - "<<msg2<<endl;
	}
	else if(error_no==COMMAND_NOT_FOUND)
	{
		cerr<<endl<<"出错 :"<<endl<<"未找到SQL命令文件 \""<<msg1<<"\""<<endl;
		outfile<<"出错 : 未找到SQL命令文件 \""<<msg1<<"\""<<endl;
	}
	outfile.close();
	cout<<"\n程序异常中断\n";
	system("PAUSE");
	exit(0);
}

/* 关键字的语法检查 */
void grammar_check(vector<KEYWORD_DATA> &vkw)
{
	int i,ct,pt;
	bool mistake=0;
	if(vkw[0].type==INSERT&&vkw.size()!=3)sql_error(GRAMMAR_MISTAKE,"INSERT","关键字过多或过少，有可能是缺少分号");
	else if(vkw[0].type==DELETE&&vkw.size()!=2&&vkw.size()!=3)sql_error(GRAMMAR_MISTAKE,"DELETE","关键字过多或过少，有可能是缺少分号");
	else if(vkw[0].type==UPDATE&&vkw.size()!=3)sql_error(GRAMMAR_MISTAKE,"UPDATE","关键字过多或过少，有可能是缺少分号");
	else if(vkw[0].type==CREATE&&vkw.size()!=1)sql_error(GRAMMAR_MISTAKE,"CREATE","关键字过多或过少，有可能是缺少分号");
	else if(vkw[0].type==DROP&&vkw.size()!=1)sql_error(GRAMMAR_MISTAKE,"DROP","关键字过多或过少，有可能是缺少分号");
	for(i=1;i<vkw.size();i++)
	{
		ct=vkw[i].type;
		pt=vkw[i-1].type;
		if(pt==SELECT&&ct!=FROM&&ct!=DISTINCT)mistake=1;
		else if(pt==DISTINCT&&ct!=FROM)mistake=1;
		else if(pt==FROM&&ct!=INNER&&ct!=JOIN&&ct!=WHERE&&ct!=ORDER)mistake=1;
		else if(pt==DELETE&&ct!=FROM)mistake=1;
		else if(pt==INNER&&ct!=JOIN)mistake=1;
		else if(pt==JOIN&&ct!=JOIN&&ct!=ON&&ct!=WHERE&&ct!=ORDER)mistake=1;
		else if(pt==ON&&ct!=INNER&&ct!=JOIN&&ct!=WHERE&&ct!=ORDER)mistake=1;
		else if(pt==WHERE&&ct!=ORDER)mistake=1;
		else if(pt==ORDER&&ct!=BY)mistake=1;
		else if(pt==BY||pt==VALUES||pt==CREATE||pt==DROP)mistake=1;
		else if(pt==INSERT&&ct!=INTO)mistake=1;
		else if(pt==INTO&&ct!=VALUES)mistake=1;
		else if(pt==SET&&ct!=WHERE)mistake=1;
		else if(pt==UPDATE&&ct!=SET)mistake=1;
		if(mistake)
		{
			if(vkw[0].type==SELECT)sql_error(GRAMMAR_MISTAKE,"SELECT","关键字顺序有误");
			else if(vkw[0].type==INSERT)sql_error(GRAMMAR_MISTAKE,"INSERT","关键字顺序有误");
			else if(vkw[0].type==DELETE)sql_error(GRAMMAR_MISTAKE,"DELETE","关键字顺序有误");
			else if(vkw[0].type==UPDATE)sql_error(GRAMMAR_MISTAKE,"UPDATE","关键字顺序有误");
			else if(vkw[0].type==CREATE)sql_error(GRAMMAR_MISTAKE,"CREATE","关键字顺序有误");
			else if(vkw[0].type==DROP)sql_error(GRAMMAR_MISTAKE,"DROP","关键字顺序有误");
		}
	}
}

/* 将buf中的关键字保存在vkw中 */
void find_keywords(const string &buf,vector<KEYWORD_DATA> &vkw)
{
	int i,j,len,leftbracnum=0,rightbracnum=0,qmnum=0;
	bool rightmore=0;
	const string kw[17]={"SELECT","FROM","WHERE","INNER","JOIN","ON","DISTINCT","INSERT","INTO","VALUES","DELETE","UPDATE","SET","ORDER","BY","CREATE","DROP"};
	for(i=0;i<buf.length();i++)
	{
		qmnum+=buf[i]==39;
		if(qmnum%2==0)
		{
			leftbracnum+=buf[i]=='(';
			rightbracnum+=buf[i]==')';
			if(rightbracnum>leftbracnum)rightmore=1;
		}
		for(j=0;j<17;j++)
		{
			len=kw[j].length();
			if(!vkw.empty()&&vkw.back().type==WHERE&&leftbracnum>rightbracnum)break;
			if(i+len<=buf.length()&&!strncasecmp(buf.substr(i,len),kw[j],len)&&(i==0||not_charornum(buf[i-1]))&&not_charornum(buf[i+len]))vkw.push_back(KEYWORD_DATA(j,i,i+len-1));
		}
	}
	grammar_check(vkw);
	if(qmnum%2)sql_error(GRAMMAR_MISTAKE,kw[vkw[0].type],"引号为奇数个");
	else if(rightmore||leftbracnum!=rightbracnum)sql_error(GRAMMAR_MISTAKE,kw[vkw[0].type],"括号不匹配");
}

/* 分析SELECT语句，要调用TABLE类中的copy、join、parse_where、select_colomn、distinct等多个成员函数 */
void parse_select(string buf,const char *resultset=NULL,int *setsize=NULL)
{
	static int selecttimes=-1;
	int i,j,beginpos,endpos,tablesum=1,len=buf.length(),strbeginpos,strendpos;
	bool isdistinct=0,singletable=1,findtable=0;
	TABLE tmptd;
	vector<KEYWORD_DATA> vkw;
	vector<TABLE> vtd;
	stringstream ss;
	char st[MAXLENGTH];
	find_keywords(buf,vkw);
	for(i=0;i<vkw.size();i++)
		if(vkw[i].type==FROM)break;
	beginpos=vkw[i].endpos+1;
	if(i==vkw.size()-1)endpos=len-1;
	else endpos=vkw[i+1].beginpos-1;
	i=beginpos;
	while(i<=endpos)
	{
		while(buf[i]==' '||buf[i]=='\n'||buf[i]=='('||buf[i]==')')
		{
			i++;
			if(i>endpos)break;
		}
		strbeginpos=i;
		if(i>endpos)break;
		findtable=1;
		i++;
		while(i<=endpos)
		{
			if(buf[i]==','||buf[i]==')')break;
			if(buf[i]!=' '&&buf[i]!='\n')strendpos=i;
			i++;
		}
		i++;
		vtd.push_back(TABLE(buf.substr(strbeginpos,strendpos-strbeginpos+1)));
		if(vtd.size()>1)singletable=0;
	}
	if(!findtable)sql_error(GRAMMAR_MISTAKE,"SELECT","未找到表格");
	for(i=0;singletable&&i<vkw.size();i++)
		if(vkw[i].type==JOIN)
		{
			singletable=0;
			break;
		}
	selecttimes++;
	ss<<"~$tmp"<<selecttimes;
	ss>>st;
	vtd[0].copy(st,tmptd,!singletable);
	for(i=1;i<vtd.size();i++)
		tmptd.join(vtd[i]);
	i=0;
	vtd.clear();
	for(j=1;!singletable&&j<vkw.size();j++)
	{
		if(vkw[j].type==SELECT||vkw[j].type==WHERE)break;
		if(vkw[j].type==JOIN)
		{
			beginpos=vkw[j].endpos+1;
			if(j==vkw.size()-1)endpos=len-1;
			else endpos=vkw[j+1].beginpos-1;
			i=beginpos;
			while(i<buf.length()&&(buf[i]==' '||buf[i]=='\n'))
				i++;
			strbeginpos=i;
			i=endpos;
			while(buf[i]==' '||buf[i]=='\n')
				i--;
			strendpos=i;
			vtd.push_back(TABLE(buf.substr(strbeginpos,strendpos-strbeginpos+1)));
			if(j!=vkw.size()-1&&vkw[j+1].type==ON)
			{
				j++;
				beginpos=vkw[j].endpos+1;
				if(j==vkw.size()-1)endpos=len-1;
				else endpos=vkw[j+1].beginpos-1;
				tmptd.join(vtd.back(),buf.substr(beginpos,endpos-beginpos+1));
			}
			else tmptd.join(vtd.back());
		}
	}
	for(i=0;i<vkw.size();i++)
		if(vkw[i].type==WHERE)break;
	if(i<vkw.size())
	{
		beginpos=vkw[i].endpos+1;
		if(i==vkw.size()-1)endpos=len-1;
		else endpos=vkw[i+1].beginpos-1;
		tmptd.parse_where(buf.substr(beginpos,endpos-beginpos+1),1);
	}
	for(i=1;i<vkw.size();i++)
		if(vkw[i].type==BY&&vkw[i-1].type==ORDER)break;
	if(i<vkw.size())
	{
		beginpos=vkw[i].endpos+1;
		if(i==vkw.size()-1)endpos=len-1;
		else endpos=vkw[i+1].beginpos-1;
		tmptd.extern_sort(buf.substr(beginpos,endpos-beginpos+1));
	}
	for(i=0;i<vkw.size();i++)
		if(vkw[i].type==SELECT)break;
	if(vkw[i+1].type==DISTINCT)
	{
		isdistinct=1;
		i++;
	}
	beginpos=vkw[i].endpos+1;
	endpos=vkw[i+1].beginpos-1;
	tmptd.select_colomn(buf.substr(beginpos,endpos-beginpos+1));
	if(isdistinct)tmptd.distinct();
	if(resultset!=NULL)tmptd.write_set(resultset,setsize);
	else tmptd.print(buf);
	remove(get_file(st));
	selecttimes--;
}

/* 分析INSERT语句，要调用TABLE类中的insert成员函数 */
void parse_insert(string buf)
{
	int i,beginpos,endpos,tablesum=1,len=buf.length(),strbeginpos,strendpos;
	bool selectall=1;
	vector<KEYWORD_DATA> vkw;
	TABLE table;
	find_keywords(buf,vkw);
	for(i=0;i<vkw.size();i++)
		if(vkw[i].type==INTO)break;
	beginpos=vkw[i].endpos+1;
	endpos=vkw[i+1].beginpos-1;
	for(i=beginpos;i<=endpos;i++)
		if(buf[i]!=' '&&buf[i]!='\n')break;
	if(i>endpos)sql_error(GRAMMAR_MISTAKE,"INSERT","未找到表格");
	strbeginpos=i;
	for(i=strbeginpos;i<=endpos;i++)
	{
		if(buf[i]=='(')
		{
			selectall=0;
			break;
		}
		if(buf[i]!=' '&&buf[i]!='\n')strendpos=i;
	}
	table=TABLE(buf.substr(strbeginpos,strendpos-strbeginpos+1));
	beginpos=vkw.back().endpos+1;
	table.insert(selectall,buf.substr(i+1,endpos-i),buf.substr(beginpos,len-beginpos));
	table.print(buf);
}

/* 分析DELETE语句，要调用TABLE类的parse_where成员函数 */
void parse_delete(string buf)
{
	int i,beginpos,endpos,tablesum=1,len=buf.length(),strbeginpos,strendpos;
	bool selectall=1;
	vector<KEYWORD_DATA> vkw;
	TABLE table;
	find_keywords(buf,vkw);
	for(i=0;i<vkw.size();i++)
		if(vkw[i].type==FROM)break;
	beginpos=vkw[i].endpos+1;
	if(i==vkw.size()-1)endpos=len-1;
	else endpos=vkw[i+1].beginpos-1;
	i=beginpos;
	for(i=beginpos;i<=endpos;i++)
		if(buf[i]!=' '&&buf[i]!='\n')break;
	if(i>endpos)sql_error(GRAMMAR_MISTAKE,"DELETE","未找到表格");
	strbeginpos=i;
	for(i=strbeginpos;i<=endpos;i++)
		if(buf[i]!=' '&&buf[i]!='\n')strendpos=i;
	table=TABLE(buf.substr(strbeginpos,strendpos-strbeginpos+1));
	for(i=0;i<vkw.size();i++)
		if(vkw[i].type==WHERE)
		{
			selectall=0;
			beginpos=vkw[i].endpos+1;
			break;
		}
	if(selectall)table.parse_where("1",0);
	else table.parse_where(buf.substr(beginpos,len-beginpos),0);
	table.print(buf);
}

/* 分析UPDATE语句，要调用TABLE类的update成员函数 */
void parse_update(string buf)
{
	int i,beginpos1,endpos1,beginpos2,endpos2,tablesum=1,len=buf.length(),strbeginpos,strendpos;
	vector<KEYWORD_DATA> vkw;
	TABLE table;
	find_keywords(buf,vkw);
	for(i=0;i<vkw.size();i++)
		if(vkw[i].type==UPDATE)break;
	beginpos1=vkw[i].endpos+1;
	endpos1=vkw[i+1].beginpos-1;
	i=beginpos1;
	for(i=beginpos1;i<=endpos1;i++)
		if(buf[i]!=' '&&buf[i]!='\n')break;
	if(i>endpos1)sql_error(GRAMMAR_MISTAKE,"UPDATE","未找到表格");
	strbeginpos=i;
	for(i=strbeginpos;i<=endpos1;i++)
		if(buf[i]!=' '&&buf[i]!='\n')strendpos=i;
	table=TABLE(buf.substr(strbeginpos,strendpos-strbeginpos+1));
	for(i=0;i<vkw.size();i++)
		if(vkw[i].type==SET)break;
	beginpos1=vkw[i].endpos+1;
	endpos1=vkw[i+1].beginpos-1;
	for(i=0;i<vkw.size();i++)
		if(vkw[i].type==WHERE)break;
	beginpos2=vkw[i].endpos+1;
	endpos2=len-1;
	table.update(buf.substr(beginpos1,endpos1-beginpos1+1),buf.substr(beginpos2,endpos2-beginpos2+1));
	table.print(buf);
}

/* 分析CREATE语句 */
void parse_create(string buf)
{
	int i,beginpos,endpos,len=buf.length();
	vector<KEYWORD_DATA> vkw;
	ofstream outfile;
	string tablename;
	stringstream ss;
	char st[MAXLENGTH];
	vector<string> title;
	vector<bool> isint;
	find_keywords(buf,vkw);
	for(i=vkw[0].endpos+1;i<len;i++)
		if(!strncasecmp(buf.substr(i,5),"TABLE",5))break;
	if(i==len)sql_error(GRAMMAR_MISTAKE,"CREATE","未找到关键字\"TABLE\"");
	i+=5;
	while(i<len&&(buf[i]==' '||buf[i]=='\n'))
		i++;
	if(i==len)sql_error(GRAMMAR_MISTAKE,"CREATE","未找到表格");
	beginpos=i;
	while(i<len&&buf[i]!='(')
	{
		if(buf[i]!=' '&&buf[i]!='\n')endpos=i;
		i++;
	}
	if(i==len)sql_error(GRAMMAR_MISTAKE,"CREATE","未找到创建的列数据");
	i++;
	tablename=buf.substr(beginpos,endpos-beginpos+1);
	while(i<len&&buf[i]!=')')
	{
		while(i<len&&(buf[i]==' '||buf[i]=='\n'))
			i++;
		if(i==len)sql_error(GRAMMAR_MISTAKE,"CREATE","缺少右括号");
		beginpos=i;
		while(i<len)
		{
			if(not_charornum(buf[i-1])&&(!strncasecmp(buf.substr(i,3),"int",3)||!strncasecmp(buf.substr(i,4),"char",4)))break;
			if(buf[i]!=' '&&buf[i]!='\n')endpos=i;
			i++;
		}
		if(i==len)sql_error(GRAMMAR_MISTAKE,"CREATE","缺少数据类型");
		title.push_back(buf.substr(beginpos,endpos-beginpos+1));
		if(!strncasecmp(buf.substr(i,3),"int",3))isint.push_back(1);
		else isint.push_back(0);
		while(i<len&&buf[i]!=','&&buf[i]!=')')
			i++;
		if(i==len||buf[i]==')')break;
		i++;
	}
	ss<<tablename;
	ss>>st;
	outfile.open(get_file(st));
	outfile<<0<<endl<<isint.size()<<endl;
	for(i=0;i<isint.size();i++)
	{
		if(isint[i])outfile<<"(int)";
		outfile<<title[i]<<endl;
	}
	cout<<endl<<"创建表格"<<tablename<<"成功!"<<endl<<endl;
	outfile.close();
}

/* 分析DROP语句 */
void parse_drop(string buf)
{
	int i,len=buf.length();
	vector<KEYWORD_DATA> vkw;
	ifstream infile;
	char filename[MAXLENGTH];
	stringstream ss;
	find_keywords(buf,vkw);
	for(i=0;i<len;i++)
		if(!strncasecmp(buf.substr(i,5),"TABLE",5))break;
	if(i==len)sql_error(GRAMMAR_MISTAKE,"DROP","未找到关键字\"TABLE\"");
	i+=5;
	ss<<noblank_string(buf.substr(i,len-i));
	ss>>filename;
	infile.open(get_file(filename));
	if(infile==NULL)sql_error(TABLE_NOT_FOUND,filename,buf);
	infile.close();
	remove(get_file(filename));
	cout<<endl<<"删除表格"<<filename<<"成功!"<<endl<<endl;
}

/* 程序开始时初始化窗口，在Linux或Unix下编译时请删除此函数 */
void init_window(char *command_filename)
{
	HANDLE hOut=GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTitle("miniSQL");
	COORD size={200,9999};
	SetConsoleScreenBufferSize(hOut,size);
	SMALL_RECT rc={0,0,152,40};
	SetConsoleWindowInfo(hOut,true,&rc);
	cout<<"欢迎使用miniSQL!"<<endl<<endl<<"请输入SQL命令文件的文件名(如\"sql_command.txt\") :"<<endl;
	do
	{
		cin.getline(command_filename,MAXLENGTH);
	}
	while(strlen(command_filename)==0);
	cout<<endl<<"请输入存储数据库的文件夹(如\"Database\\\") :"<<endl;
	do
	{
		cin.getline(db_dir,MAXLENGTH);
	}
	while(strlen(db_dir)==0);
	system("cls");
}

/* 主函数，先初始化窗口，再读取每条SQL命令并调用对应的分析函数 */
int main()
{
	char command_filename[MAXLENGTH],*buf=new char[MAXLENGTH];
	ifstream infile;
	int i;
	init_window(command_filename);
	infile.open(command_filename);
	if(infile==NULL)
	{
		sql_error(COMMAND_NOT_FOUND,command_filename,"");
		return 0;
	}
	while(infile.getline(buf,MAXLENGTH,';'))
	{
		int len=strlen(buf);
		for(i=0;i<len;i++)
			if(buf[i]!=' '&&buf[i]!='\n')break;
		if(i==len)break;
		if(!strncasecmp(buf+i,"SELECT",6)&&not_charornum(buf[i+6]))
		{
			cout<<"SQL命令 : "<<endl<<buf+i<<";"<<endl;
			parse_select(buf+i);
		}
		else if(!strncasecmp(buf+i,"INSERT",6)&&not_charornum(buf[i+6]))
		{
			cout<<"SQL命令 : "<<endl<<buf+i<<";"<<endl;
			parse_insert(buf+i);
		}
		else if(!strncasecmp(buf+i,"DELETE",6)&&not_charornum(buf[i+6]))
		{
			cout<<"SQL命令 : "<<endl<<buf+i<<";"<<endl;
			parse_delete(buf+i);
		}
		else if(!strncasecmp(buf+i,"UPDATE",6)&&not_charornum(buf[i+6]))
		{
			cout<<"SQL命令 : "<<endl<<buf+i<<";"<<endl;
			parse_update(buf+i);
		}
		else if(!strncasecmp(buf+i,"CREATE",6)&&not_charornum(buf[i+6]))
		{
			cout<<"SQL命令 : "<<endl<<buf+i<<";"<<endl;
			parse_create(buf+i);
		}
		else if(!strncasecmp(buf+i,"DROP",4)&&not_charornum(buf[i+4]))
		{
			cout<<"SQL命令 : "<<endl<<buf+i<<";"<<endl;
			parse_drop(buf+i);
		}
		else sql_error(GRAMMAR_MISTAKE,buf+i,"关键字不匹配");
		system("PAUSE");
		system("cls");
	}
	infile.close();
	cout<<"已读取所有命令，程序将正常退出\n";
	system("PAUSE");
	return 0;
}
