#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <list>
#include <set>
#include <map>

#define INPUT_STR_MAX_LENGTH 1024

// 指令的类型
#define UNDEFINED -2 // 未定义的指令
#define NOP -1 // nop entrypc
#define FUNC_OPERATION 0 // add sub mul div mod cmp(..) load
#define FUNC_TITLE 1 // enter
#define FUNC_RET 2 // ret
#define FUNC_STORE 3 // move store
#define FUNC_WRITE 4 // write wrl
#define FUNC_BRANCH_BEGIN 5 // blbc blbs
#define FUNC_BRANCH_END 6 // br
#define FUNC_PARAM 7 // param
#define FUNC_CALL 8 // call

#define MAX_ADDR 32768

#define OPT_NULL 0 // 无优化（默认）
#define OPT_SCP 1 // -opt=scp
#define OPT_DSE 2 // -opt=dse

#define BACKEND_C 0 // -backend=c（默认）
#define BACKEND_3ADDR 1 // -backend=3addr
#define BACKEND_CFG 2 // -backend=cfg
#define BACKEND_REP 3 // -backend=rep

using namespace std;

map<int, string> global; // 全局变量（地址：名称）

class FUNC // 函数
{
public:
	string name; // 函数名（除main函数外均命名为"func_*"）
	int size; // 局部变量占用空间
	int paramnum; // 参数个数
	map<int, string> params; // 参数（地址：名称）
	map<int, string> var; // 局部变量（地址：名称）
	set<string> varname; // 参数及局部变量的符号表（-opt=scp不包括数组，-opt=dse包括数组）
	map<int, set<int> > block, pred; // 基本块及其关系（例如2->3,4，block：[2:{3,4}]，pred：[3:{2}],[4:{2}]）
	map<int, set<int> > gen, kill, in, out; // 到达定值的gen,kill,in,out集合，分别记录指令序号
	map<int, set<string> > genvar;
	map<int, set<string> > def, use, invar, outvar; // 活跃变量的def,use,in,out集合，分别记录变量名
	vector<int> assignment; // 赋值语句的序号
	int constantspropagated, statementseliminatedinscr, statementseliminatedoutscr; // 常量传播的数量、在SCR内部消除的语句的数量和在SCR外部消除的语句的数量
	set<int> scrblock, visited; // scrblock记录哪些基本块在SCR内
	int getblock(int instrno) // 输入一条指令的序号，返回这条语句所在的基本块
	{
		map<int, set<int> >::iterator iter = block.find(instrno);
		if (iter != block.end())
		{
			return iter->first;
		}
		pair<map<int, set<int> >::iterator, bool> insertresult = block.insert(map<int, set<int> >::value_type(instrno, set<int>()));
		insertresult.first--;
		int result = insertresult.first->first;
		insertresult.first++;
		block.erase(insertresult.first);
		return result;
	}
	void buildpred() // 由block生成pred
	{
		map<int, set<int> >::iterator iter;
		set<int>::iterator iterset;
		for (iter = block.begin(); iter != block.end(); iter++)
			for (iterset = iter->second.begin(); iterset != iter->second.end(); iterset++)
			{
				pred[*iterset].insert(iter->first);
			}
	}
	void dfs(int n) // 通过深度优先搜索得到SCR
	{
		scrblock.insert(n);
		set<int>::iterator iter;
		for (iter = pred[n].begin(); iter != pred[n].end(); iter++)
		{
			if (visited.find(*iter) == visited.end())
			{
				visited.insert(*iter);
				dfs(*iter);
				visited.erase(*iter);
			}
		}
	}
};

vector<FUNC> func; // 函数集

class INSTR // 三地址指令
{
public:
	string full; // 输入的未经任何修改的三地址指令
	int instrno; // 序号，即"instr *:"
	string params[3]; // 指令的三个参数
	int func; // 指令所在的函数（func向量的下标）
	int type; // 指令类型（见#define）
	string result; // OPERATION型指令得到的结果
	string resultdiv8; // result除以8的结果（维护数组下标）
	bool isaddr; // result是否一个地址
	string arrayname; // result的地址保存的数组的名称
	string index; // 数组下标
	string output; // STORE、CALL、WRITE等类型指令的输出结果
	string key; // 输出if或while（仅对BRANCH_BEGIN型指令有效）
	int rightbrace; // 输出该指令前要输出多少个"}"
	bool beginwithelse; // 输出该指令前是否要输出"else"
	bool unlive; // 被赋值的变量是否非活跃变量（仅对赋值语句有效）
	INSTR() // 初始化
	{
		full = "";
		instrno = -1;
		params[0] = params[1] = params[2];
		func = UNDEFINED;
		type = FUNC_OPERATION;
		result = resultdiv8 = "";
		isaddr = false;
		arrayname = index = output = key = "";
		rightbrace = 0;
		beginwithelse = false;
		unlive = false;
	}
};

vector<INSTR> instr; // 指令集
bool warningdiv0 = false; // -opt=scp时是否出现除零错误

int split(char *str, string *args) // 通过空格分隔字符串
{
	stringstream ss;
	ss << str;
	string temp;
	int i = 0;
	while (ss >> temp)
	{
		args[i] = temp;
		i++;
	}
	return i;
}

string tostring(int n) // 跟JAVA的tostring类似
{
	stringstream ss;
	ss << n;
	string result;
	ss >> result;
	return result;
}

inline bool isopcode(string str) // 返回是否运算
{
	return str == "add" || str == "sub" || str == "mul" || str == "div" || str == "mod" || str == "cmpeq" || str == "cmple" || str == "cmplt";
}

int getvar(string str, string &name) // 根据"#"识别并返回变量名（保存在name中）和变量地址
{
	int i;
	for (i = 1; i < str.length()-1; i++)
	{
		if (str[i] == '#' || str.substr(i, 6) == "_base#" || str.substr(i, 8) == "_offset#")
		{
			name = str.substr(0, i);
			break;
		}
	}
	for ( ; i < str.length()-1; i++)
	{
		if (str[i] == '#')
		{
			break;
		}
	}
	return atoi(str.substr(i+1, str.length()-i-1).c_str());
}

bool contains(string longstr, string shortstr) // longstr中是否包含shortstr
{
	int i;
	for (i = 0; i < longstr.length(); i++)
	{
		if (longstr.substr(i, shortstr.length()) == shortstr)
		{
			return true;
		}
	}
	return false;
}

bool allnumber(string str) // str是否都由数字组成
{
	int i;
	for (i = 0; i < str.length(); i++)
	{
		if (str[i] < '0' || str[i] > '9') return false;
	}
	return true;
}

string tooperator(string str)
{
	if (str == "add") return "+";
	if (str == "sub") return "-";
	if (str == "mul") return "*";
	if (str == "div") return "/";
	if (str == "mod") return "%";
	if (str == "cmpeq") return " == ";
	if (str == "cmple") return " <= ";
	if (str == "cmplt") return " < ";
	return "+";
}

void printindent(int n) // 输出缩进
{
	int i;
	for (i = 0; i < n; i++)
	{
		cout << "    ";
	}
}

string inverse(string str) // 处理blbs指令时把比较大小表达式调转
{
	int i;
	string result = str;
	for (i = 0; i < str.length(); i++)
	{
		if (str.substr(i, 2) == "==")
		{
			result[i] = '!';
			break;
		}
		else if (str.substr(i, 2) == "<=")
		{
			result = str.substr(0, i)+">"+str.substr(i+2, str.length()-i-2);
			break;
		}
		else if (str[i] == '<')
		{
			result = str.substr(0, i)+">="+str.substr(i+1, str.length()-i-1);
			break;
		}
	}
	return result;
}

string getdef(string str) // 返回赋值语句的被赋值变量
{
	int begin, end;
	for (begin = 0; begin < str.length(); begin++)
	{
		if (str[begin] == '_' || (str[begin] >= 'a' && str[begin] <= 'z') || (str[begin] >= 'A' && str[begin] <= 'Z'))
		{
			break;
		}
	}
	for (end = begin+1; end < str.length(); end++)
	{
		if (!(str[end] == '_' || (str[end] >= 'a' && str[end] <= 'z') || (str[end] >= 'A' && str[end] <= 'Z') || (str[end] >= '0' && str[end] <= '9')))
		{
			break;
		}
	}
	return str.substr(begin, end);
}

set<string> getuse(string str) // 返回赋值语句中使用到的变量集
{
	int i;
	string curuse = "";
	set<string> result;
	for (i = 0; i < str.length(); i++)
	{
		if (str[i] == '(' || str[i] == '[' || str[i] == '=')
		{
			i++;
			break;
		}
	}
	for ( ; i < str.length(); i++)
	{
		if (str[i] == '_' || (str[i] >= 'a' && str[i] <= 'z') || (str[i] >= 'A' && str[i] <= 'Z'))
		{
			curuse += str[i];
		}
		else if (curuse != "")
		{
			result.insert(curuse);
			curuse = "";
		}
	}
	return result;
}

bool defarray(string str) // 赋值语句是否在给数组某个元素赋值
{
	int i;
	bool result = false;
	for (i = 0; i < str.length(); i++)
	{
		if (str[i] == '[')
		{
			result = true;
		}
		else if (str[i] == '=')
		{
			break;
		}
	}
	return result;
}

string calculate(string str) // 返回str表达式的运算结果。如果有除零则warningdiv0为true
{
	string result = str;
	bool brace = true;
	int lb, rb;
	while (brace) // 有括号则一直计算
	{
		brace = false;
		int i;
		for (i = 0; i < result.length(); i++) // 找匹配的括号对
		{
			if (result[i] == '(')
			{
				brace = true;
				lb = i;
			}
			else if (result[i] == ')')
			{
				if (brace)
				{
					rb = i;
					break;
				}
			}
			else if ((result[i] >= 'A' && result[i] <= 'Z') || (result[i] >= 'a' && result[i] <= 'z') || result[i] == '_')
			{
				brace = false;
			}
		}
		if (brace)
		{
			string num1 = "", op = "", num2 = "";
			for (i = lb+1; i < rb; i++)
			{
				if (result[i] >= '0' && result[i] <= '9')
				{
					if (op == "")
					{
						num1 += result[i];
					}
					else
					{
						num2 += result[i];
					}
				}
				else if (result[i] == '+' || result[i] == '-' || result[i] == '*' || result[i] == '/' || result[i] == '%')
				{
					op += result[i];
				}
				else if (result.substr(i, 3) == " < " || result.substr(i, 3) == " > ")
				{
					op += result[i+1];
					i += 2;
				}
				else if (result.substr(i, 4) == " <= " || result.substr(i, 4) == " >= " || result.substr(i, 4) == " == " || result.substr(i, 4) == " != ")
				{
					op += result.substr(i+1, 2);
					i += 3;
				}
			}
			string tempresult;
			if (op == "+") tempresult = tostring(atoi(num1.c_str())+atoi(num2.c_str()));
			else if (op == "-") tempresult = tostring(atoi(num1.c_str())-atoi(num2.c_str()));
			else if (op == "*") tempresult = tostring(atoi(num1.c_str())*atoi(num2.c_str()));
			else if (op == "/") 
			{
				if (num2 == "0") // 除零退出，下同
				{
					warningdiv0 = true;
					break;
				}
				tempresult = tostring(atoi(num1.c_str())/atoi(num2.c_str()));
			}
			else if (op == "%")
			{
				if (num2 == "0")
				{
					warningdiv0 = true;
					break;
				}
				tempresult = tostring(atoi(num1.c_str())%atoi(num2.c_str()));
			}
			else if (op == "<") tempresult = tostring(atoi(num1.c_str())<atoi(num2.c_str()));
			else if (op == ">") tempresult = tostring(atoi(num1.c_str())>atoi(num2.c_str()));
			else if (op == "==") tempresult = tostring(atoi(num1.c_str())==atoi(num2.c_str()));
			else if (op == "!=") tempresult = tostring(atoi(num1.c_str())!=atoi(num2.c_str()));
			else if (op == "<=") tempresult = tostring(atoi(num1.c_str())<=atoi(num2.c_str()));
			else if (op == ">=") tempresult = tostring(atoi(num1.c_str())>=atoi(num2.c_str()));
			else
			{
				break;
			}
			result = result.substr(0, lb)+tempresult+result.substr(rb+1, result.length()-rb-1); // 用新的计算结果取代旧的表达式
		}
	}
	return result;
}

void getconstant(string output, bool &init, bool &sameconstant, int &value) // 调用calculate计算output的值。如果value未初始化（init为false），则定义value，如果value已经初始化，则sameconstant取决于该计算结果是否等于value
{
	int begin, end;
	int i;
	for (i = 0; i < output.length(); i++) // 获取要计算的部分
	{
		if (output[i] == '=')
		{
			begin = i;
		}
		else if (output[i] == ';')
		{
			end = i;
		}
	}
	for (i = begin+1; i < end; i++) // 如果要计算的部分有变量（即非定值），则不可能得到定值，sameconstant为false
	{
		if ((output[i] >= 'a' && output[i] <= 'z') || (output[i] >= 'A' && output[i] <= 'Z') || output[i] == '_')
		{
			sameconstant = false;
			return;
		}
	}
	string calculateresult = calculate(output.substr(begin+2, end-begin-2).c_str()).c_str(); // 计算定值表达式运算的结果
	int result = atoi(calculateresult.c_str());
	if (!init)
	{
		init = true;
		if (calculateresult[0] == '(') // 出现除零（例如calculateresult == "(2/0)"）
		{
			sameconstant =false;
		}
		else
		{
			value = result;
		}
	}
	else if (calculateresult[0] == '(' || result != value) // 定值一样且没有发生除零
	{
		sameconstant = false;
	}
}

string varreplace(string str, string var, string value) // 常量传播：用value替换var
{
	int i;
	string curuse = "";
	string result = str;
	for (i = 0; i < str.length(); i++) // 从赋值号或第一个"("（Write语句或函数调用）或第一个"["（对数组元素的赋值）开始替换
	{
		if (str[i] == '(' || str[i] == '[' || str[i] == '=')
		{
			i++;
			break;
		}
	}
	for ( ; i < str.length(); i++) // 找所有可替换的变量
	{
		if (str[i] == '_' || (str[i] >= 'a' && str[i] <= 'z') || (str[i] >= 'A' && str[i] <= 'Z'))
		{
			curuse += str[i];
		}
		else if (curuse!= "")
		{
			if (curuse == var)
			{
				result = result.substr(0, i-curuse.length())+value+result.substr(i, result.length()-i);
				i -= curuse.length();
			}
			curuse = "";
		}
	}
	return result;
}

int main(int argc, char *argv[]) // 程序入口
{
	char inputstr[INPUT_STR_MAX_LENGTH]; // 每行输入的东西
	int total = 0; // 指令数量
	int i, j, k;
	int mode[2] = {OPT_NULL, BACKEND_C}; // -opt=.. -backend=..
	if (argc >= 2)
	{
		if (!strcmp(argv[1], "-opt=scp")) mode[0] = OPT_SCP;
		else if (!strcmp(argv[1], "-opt=dse")) mode[0] = OPT_DSE;
		else if (!strcmp(argv[1], "-backend=c")) mode[1] = BACKEND_C;
		else if (!strcmp(argv[1], "-backend=3addr")) mode[1] = BACKEND_3ADDR;
		else if (!strcmp(argv[1], "-backend=cfg")) mode[1] = BACKEND_CFG;
		else if (!strcmp(argv[1], "-backend=rep")) mode[1] = BACKEND_REP;
	}
	if (argc >= 3)
	{
		if (!strcmp(argv[2], "-backend=c")) mode[1] = BACKEND_C;
		else if (!strcmp(argv[2], "-backend=3addr")) mode[1] = BACKEND_3ADDR;
		else if (!strcmp(argv[2], "-backend=cfg")) mode[1] = BACKEND_CFG;
		else if (!strcmp(argv[2], "-backend=rep")) mode[1] = BACKEND_REP;		
	}
	vector<string> callparams; // 函数参数栈
	instr.push_back(INSTR()); // 第一条指令从1开始而不是从0开始
	while (cin.getline(inputstr, INPUT_STR_MAX_LENGTH))
	{
		string args[5];
		int argnum = split(inputstr, args);
		total++;
		if (instr.size() <= total)
		{
			instr.push_back(INSTR());
		}
		instr[total].instrno = atoi(args[1].c_str());
		instr[total].full = inputstr;
		instr[total].params[0] = instr[total].params[1] = instr[total].params[2] = "";
		for (i = 0; i < argnum-2; i++)
		{
			instr[total].params[i] = args[i+2];
		}
		instr[total].type = 0;
		for (i = 1; i <= 2; i++)
		{
			if (contains(instr[total].params[i], "#") && !contains(instr[total].params[i], "_offset#") && instr[total].params[2] != "GP") // 记录函数局部变量或函数参数
			{
				string varname;
				int addr;
				addr = getvar(instr[total].params[i], varname);
				if (addr > 0)
				{
					func.back().params.insert(map<int, string>::value_type(addr, varname));
				}
				else
				{
					func.back().var.insert(map<int, string>::value_type(addr, varname));
				}
			}
		}
		if (instr[total].params[0] == "nop" || instr[total].params[0] == "entrypc") // nop指令或entrypc指令不处理
		{
			instr[total].func = NOP;
		}
		else if (instr[total].params[0] == "enter") // enter指令：函数入口
		{
			func.push_back(FUNC());
			if (instr[total-1].params[0] == "entrypc") // 如果上一条指令是entrypc，则该函数是main函数
			{
				func.back().name = "main";
			}
			else
			{
				func.back().name = "func_" + tostring(func.size());
			}
			func.back().size = atoi(instr[total].params[1].c_str());
			instr[total].func = func.size()-1;
			instr[total].type = FUNC_TITLE;
			func.back().block.insert(map<int, set<int> >::value_type(total, set<int>()));
		}
		else if (instr[total].params[0] == "ret") // ret指令：函数出口
		{
			instr[total].func = func.size()-1;
			instr[total].type = FUNC_RET;
			func.back().paramnum = atoi(instr[total].params[1].c_str())/8; // 函数参数个数
		}
		else if (isopcode(instr[total].params[0])) // 运算（add、sub、mul、……）指令
		{
			instr[total].func = func.size()-1;
			instr[total].type = FUNC_OPERATION;			
			if (instr[total].params[0] == "add" && instr[total].params[2] == "GP") // 全局变量
			{
				string varname;
				int addr = getvar(instr[total].params[1], varname);
				global.insert(map<int, string>::value_type(addr, varname));
				instr[total].result = varname;
				instr[total].isaddr = true;
				instr[total].arrayname = varname;
				instr[total].index = "";
			}
			else if (instr[total].params[0] == "add" && instr[total].params[2] == "FP") // 数组或结构体局部变量
			{
				string varname;
				getvar(instr[total].params[1], varname);
				instr[total].result = varname;
				instr[total].isaddr = contains(instr[total].params[1], "_base#");
				if (instr[total].isaddr)
				{
					instr[total].arrayname = varname;
					instr[total].index = "";
				}
			}
			else if (instr[total].params[0] == "add" && contains(instr[total].params[2], "_offset#")) // 结构体的成员变量也通过地址偏移量表示，最终转换成数组的形式
			{
				string fieldname;
				int addr = getvar(instr[total].params[2], fieldname);
				int reg1 = atoi(instr[total].params[1].substr(1, instr[total].params[1].length()-1).c_str());
				instr[total].isaddr = true;
				instr[total].arrayname = instr[reg1].arrayname;
				if (instr[reg1].index == "" || instr[reg1].index == "0")
				{
					instr[total].index = tostring(addr/8);
				}
				else if (instr[reg1].index != "" && addr != 0)
				{
					instr[total].index = instr[reg1].index+"+"+tostring(addr/8);
				}
				else
				{
					instr[total].index = instr[reg1].index;
				}
			}
			else // 普通运算
			{
				string operand[2] = {"", ""}; // 变量名或数
				int reg[2] = {-1, -1}; // 取前面的哪条表达式的结果
				for (i = 1; i <= 2; i++)
				{
					if (contains(instr[total].params[i], "#")) // 局部非数组非结构体变量，如”i#-8“
					{
						string varname;
						getvar(instr[total].params[i], varname);
						operand[i-1] = varname;
					}
					else if (allnumber(instr[total].params[i])) // 数
					{
						operand[i-1] = instr[total].params[i];
					}
					else if (instr[total].params[i][0] == '(') // 取前面的指令的计算结果
					{
						reg[i-1] = atoi(instr[total].params[i].substr(1, instr[total].params[i].length()-1).c_str());
					}
					else
					{
						instr[total].func = UNDEFINED;
					}
				}
				if (instr[total].func != UNDEFINED)
				{
					if (reg[0] == -1 && reg[1] == -1) // 变量/数和变量/数的计算
					{
						instr[total].isaddr = false;
						instr[total].result = "("+operand[0]+tooperator(instr[total].params[0])+operand[1]+")";						
					}
					else if (reg[0] != -1 && reg[1] == -1) // 前面的结果和变量/数的计算
					{
						if (instr[reg[0]].isaddr)
						{
							instr[total].func = UNDEFINED;
						}
						else
						{
							instr[total].isaddr = false;
							instr[total].result = "("+instr[reg[0]].result+tooperator(instr[total].params[0])+operand[1]+")";	
						}
					}
					else if (reg[0] == -1 && reg[1] != -1) // 变量/数和前面的结果的计算
					{
						if (instr[reg[1]].isaddr)
						{
							instr[total].func = UNDEFINED;
						}
						else
						{
							instr[total].isaddr = false;
							instr[total].result = "("+operand[0]+tooperator(instr[total].params[0])+instr[reg[1]].result+")";
						}
					}
					else if (reg[0] != -1 && reg[1] != -1) // 前面的结果和前面的结果的计算
					{
						if (instr[reg[1]].isaddr)
						{
							instr[total].func = UNDEFINED;
						}
						else
						{
							if (instr[reg[0]].isaddr)
							{
								instr[total].isaddr = true;
								instr[total].arrayname = instr[reg[0]].arrayname;
								if (instr[reg[0]].index == "")
								{
									instr[total].index = instr[reg[1]].resultdiv8;
								}
								else
								{
									if (instr[total].params[0] != "add" && instr[total].params[0] != "sub")
									{
										instr[total].func = UNDEFINED;
									}
									else
									{
										instr[total].index = "("+instr[reg[0]].index+tooperator(instr[total].params[0])+instr[reg[1]].resultdiv8+")";
									}							
								}
							}
							else
							{
								instr[total].isaddr = false;
								instr[total].result = "("+instr[reg[0]].result+tooperator(instr[total].params[0])+instr[reg[1]].result+")";	
							}								
						}					
					}
					else
					{
						instr[total].func = UNDEFINED;
					}
					if (!instr[total].isaddr && (reg[0] == -1 || !instr[reg[0]].isaddr) && (reg[1] == -1 || !instr[reg[1]].isaddr)) // 可能的数组下标计算
					{
						if (instr[total].params[0] == "add" && reg[0] != -1 && reg[1] != -1 && instr[reg[0]].resultdiv8 != "" && instr[reg[1]].resultdiv8 != "")
						{
							instr[total].resultdiv8 = instr[reg[0]].resultdiv8+"+"+instr[reg[1]].resultdiv8;
						}
						else if (instr[total].params[0] == "sub" && reg[0] != -1 && reg[1] != -1 && instr[reg[0]].resultdiv8 != "" && instr[reg[1]].resultdiv8 != "")
						{
							instr[total].resultdiv8 = instr[reg[0]].resultdiv8+"-"+instr[reg[1]].resultdiv8;
						}
						else if (instr[total].params[0] == "mul" && allnumber(instr[total].params[2]) && atoi(instr[total].params[2].c_str()) > 0 && atoi(instr[total].params[2].c_str())%8 == 0)
						{
							if (reg[0] == -1)
							{
								instr[total].resultdiv8 = operand[0];
							}
							else
							{
								instr[total].resultdiv8 = instr[reg[0]].result;
							}
							if (atoi(instr[total].params[2].c_str()) != 8)
							{
								instr[total].resultdiv8 += "*"+tostring(atoi(instr[total].params[2].c_str())/8);
							}
						}
					}
				}
			}
		}
		else if (instr[total].params[0] == "move") // 赋值语句（对局部非数组非结构体变量）
		{
			instr[total].func = func.size()-1;
			instr[total].type = FUNC_STORE;
			string var2;
			getvar(instr[total].params[2], var2);
			if (contains(instr[total].params[1], "#")) // 变量<-变量，如i = j;
			{
				string var1;
				getvar(instr[total].params[1], var1);
				instr[total].output = var2+" = "+var1+";";
				instr[total].result = var2;
				instr[total].isaddr = false;
			}
			else if (allnumber(instr[total].params[1])) // 变量<-数，如i = 8;
			{
				instr[total].output = var2+" = "+instr[total].params[1]+";";
				instr[total].result = var2;
				instr[total].isaddr = false;
			}
			else if (instr[total].params[1][0] == '(') // 变量<-前面的结果，如i = (a+b);
			{
				int reg1 = atoi(instr[total].params[1].substr(1, instr[total].params[1].length()-1).c_str());
				if (instr[reg1].isaddr)
				{
					instr[total].func = UNDEFINED;
				}
				else
				{
					instr[total].output = var2+" = "+instr[reg1].result+";";
					instr[total].result = var2;
					instr[total].isaddr = false;
				}
			}
			else
			{
				instr[total].func = UNDEFINED;
			}
		}
		else if (instr[total].params[0] == "store") // 赋值语句（对全局变量或局部数组某个元素或局部结构体某个成员变量）
		{
			instr[total].func = func.size()-1;
			instr[total].type = FUNC_STORE;
			int reg2 = atoi(instr[total].params[2].substr(1, instr[total].params[2].length()-1).c_str());
			string val1 = "";
			if (contains(instr[total].params[1], "#")) // 变量<-变量
			{
				getvar(instr[total].params[1], val1);
			}
			else if (allnumber(instr[total].params[1])) // 变量<-数
			{
				val1 = instr[total].params[1];
			}
			else if (instr[total].params[1][0] == '(') // 变量<-前面的结果
			{
				int reg1 = atoi(instr[total].params[1].substr(1, instr[total].params[1].length()-1).c_str());
				if (instr[reg1].isaddr)
				{
					instr[total].func = UNDEFINED;
				}
				else
				{
					val1 = instr[reg1].result;
				}
			}
			else
			{
				instr[total].func = UNDEFINED;
			}
			string val2 = "";
			if (!instr[reg2].isaddr)
			{
				instr[total].func = UNDEFINED;
			}
			else
			{
				if (instr[reg2].index == "") // 赋值给非数组非结构体的全局变量
				{
					val2 = instr[reg2].arrayname;
				}
				else // 赋值给数组或结构体
				{
					val2 = instr[reg2].arrayname+"["+instr[reg2].index+"]";
				}
			}
			if (instr[total].func != UNDEFINED)
			{
				instr[total].output = val2+" = "+val1+";";
				instr[total].result = val2;
				instr[total].isaddr = false;
			}
		}
		else if (instr[total].params[0] == "load") // Load指令：作为运算指令处理
		{
			instr[total].func = func.size()-1;
			instr[total].type = FUNC_OPERATION;
			instr[total].isaddr = false;
			int reg = atoi(instr[total].params[1].substr(1, instr[total].params[1].length()-1).c_str());
			if (!instr[reg].isaddr) // 取前面的结果
			{
				instr[total].result = instr[reg].result;
			}
			else if (instr[reg].index == "") // 非数组非结构体的全局变量，取变量名
			{
				instr[total].result = instr[reg].arrayname;
			}
			else // 数组或结构体，取变量名[下标]
			{
				instr[total].result = instr[reg].arrayname+"["+instr[reg].index+"]";
			}
		}
		else if (instr[total].params[0] == "param") // Param指令：函数参数入栈
		{
			instr[total].func = func.size()-1;
			instr[total].type = FUNC_PARAM;
			string val;
			if (contains(instr[total].params[1], "#")) // 局部非数组非结构体变量
			{
				getvar(instr[total].params[1], val);
			}
			else if (allnumber(instr[total].params[1])) // 数
			{
				val = instr[total].params[1];
			}
			else if (instr[total].params[1][0] == '(') // 前面的结果
			{
				int reg = atoi(instr[total].params[1].substr(1, instr[total].params[1].length()-1).c_str());
				if (instr[reg].isaddr)
				{
					instr[total].func = UNDEFINED;
				}
				else
				{
					val = instr[reg].result;
				}
			}
			else
			{
				instr[total].func = UNDEFINED;
			}
			if (instr[total].func != UNDEFINED)
			{
				callparams.push_back(val); // 入栈
			}			
		}
		else if (instr[total].params[0] == "call") // Call指令：函数参数出栈并调用函数
		{
			instr[total].func = func.size()-1;
			instr[total].type = FUNC_CALL;
			int reg = atoi(instr[total].params[1].substr(1, instr[total].params[1].length()-1).c_str()); // 跳转到的指令的序号
			instr[total].output = func[instr[reg].func].name+"(";
			for (i = 0; i < callparams.size(); i++)
			{
				if (i == 0)
				{
					instr[total].output += callparams[i];
				}
				else
				{
					instr[total].output += ", "+callparams[i];
				}
			}
			instr[total].output += ");";
			callparams.clear();
			func.back().block.insert(map<int, set<int> >::value_type(total+1, set<int>())); // 下一条指令是一个基本块的入口
			j = func.back().getblock(total);
			set<int>::iterator iterset;
			for (iterset = func.back().block[j].begin(); iterset != func.back().block[j].end(); iterset++) // 把该基本块的后续结点改为下一指令的基本块的后续结点
			{
				if (*iterset != total+1)
				{
					func.back().block[total+1].insert(*iterset);
				}
			}
			func.back().block[j].clear();
			func.back().block[j].insert(total+1); // 该基本块能到达下一指令的基本块
		}
		else if (instr[total].params[0] == "blbc" || instr[total].params[0] == "blbs") // Branch-Begin型指令：if或while的开始
		{
			instr[total].func = func.size()-1;
			instr[total].type = FUNC_BRANCH_BEGIN;
			int reg1 = atoi(instr[total].params[1].substr(1, instr[total].params[1].length()-1).c_str()); // 条件
			int reg2 = atoi(instr[total].params[2].substr(1, instr[total].params[2].length()-1).c_str()); // 不符合条件跳转到的指令的序号
			while (instr.size() <= reg2)
			{
				instr.push_back(INSTR());
			}
			instr[reg2].rightbrace++; // 缩进++
			instr[total].key = "if"; // 如果后续有br语句跳转到该语句之前并执行该语句，则key改为while
			if (instr[total].params[0] == "blbc")
			{
				instr[total].output = instr[reg1].result;
			}
			else // 处理blbs指令把条件调转
			{
				instr[total].output = inverse(instr[reg1].result);
			}
			func.back().block.insert(map<int, set<int> >::value_type(total+1, set<int>())); // 下一条指令是一个基本块的入口
			func.back().block.insert(map<int, set<int> >::value_type(reg2, set<int>())); // 不符合条件跳转到的指令是一个基本块的入口
			j = func.back().getblock(total);
			func.back().block[j].insert(total+1); // 该基本块能到达下一指令的基本块（暂时按if语句处理）
			func.back().block[j].insert(reg2); // 该基本块能到达不符合条件跳转到的指令的基本块（暂时按if语句处理）
			func.back().block[total+1].insert(reg2); // 下一指令的基本块能到达不符合条件跳转到的指令的基本块（暂时按if语句处理）
		}
		else if (instr[total].params[0] == "br") // br指令：往已执行过的指令跳转则if语句改为while语句；往未执行过的指令跳转则if语句改为if-else语句
		{
			instr[total].func = func.size()-1;
			instr[total].type = FUNC_BRANCH_END;
			int reg = atoi(instr[total].params[1].substr(1, instr[total].params[1].length()-1).c_str()); // 跳转到的指令的序号
			if (reg < total) // 往已执行过的指令跳转
			{
				for (i = reg+1; i < total; i++)
				{
					if (instr[i].key == "if")
					{
						instr[i].key = "while"; // if改为while
						break;
					}
				}
			}
			else // 往未执行过的指令跳转
			{
				while (instr.size() <= reg)
				{
					instr.push_back(INSTR());
				}
				instr[reg].rightbrace++;
				instr[total+1].beginwithelse = true; // 输出下一条指令前要输出else
			}
			func.back().block.insert(map<int, set<int> >::value_type(reg, set<int>())); // 跳转到的指令是一个基本块的入口
			map<int, set<int> >::iterator iter = func.back().block.end();
			iter--;
			while (iter != func[instr[total].func].block.begin())
			{
				if (iter->first <= total)
				{
					break;
				}
				iter--;
			}
			if (reg < total)
			{
				iter->second.insert(reg); // while循环末所在的基本块能到达到while循环的判断条件的基本块
				iter = func.back().block.find(reg);
				iter++;
				map<int, set<int> >::iterator itertemp = iter;
				while (itertemp->second.find(total+1) == itertemp->second.end())
				{
					itertemp++;
				}
				itertemp->second.erase(total+1); // while循环末所在的基本块不能到达while循环结束后的第一个基本块
				iter--;
				iter--;
				set<int>::iterator iterset;
				for (iterset = iter->second.begin(); iterset != iter->second.end(); iterset++)
				{
					if (reg != *iterset)
					{
						func.back().block[reg].insert(*iterset); // 把原本while循环的判断条件所在的基本块（现在是while循环的判断条件的前面一条指令所在的基本块）的后续结点改为while循环的判断条件所在的基本块的后续结点
					}
				}
				iter->second.clear();
				iter->second.insert(reg); // while循环的判断条件的前面一条指令所在的基本块能到达while循环的判断条件所在的基本块
			}
			else
			{
				iter->second.erase(total+1); // if部分末尾所在的基本块不能到达else部分所在的基本块
				func.back().block[total+1].insert(reg); // else部分所在的基本块能到达if-else语句结束后的第一个基本块
				iter->second.insert(reg); // if部分末尾所在的基本块能到达if-else语句结束后的第一个基本块
			}
		}
		else if (instr[total].params[0] == "wrl") // Wrl指令：输出一个空行
		{
			instr[total].func = func.size()-1;
			instr[total].type = FUNC_WRITE;
			instr[total].output = "WriteLine();";
		}
		else if (instr[total].params[0] == "write") // Write指令：输出一个数
		{
			instr[total].func = func.size()-1;
			instr[total].type = FUNC_WRITE;
			string val;
			if (contains(instr[total].params[1], "#")) // 输出局部非数组非结构体变量
			{
				getvar(instr[total].params[1], val);
			}
			else if (allnumber(instr[total].params[1])) // 输出常数
			{
				val = instr[total].params[1];
			}
			else if (instr[total].params[1][0] == '(') // 输出前面的结果
			{
				int reg = atoi(instr[total].params[1].substr(1, instr[total].params[1].length()-1).c_str());
				if (instr[reg].isaddr)
				{
					instr[total].func = UNDEFINED;
				}
				else
				{
					val = instr[reg].result;
				}
			}
			else
			{
				instr[total].func = UNDEFINED;
			}
			if (instr[total].func != UNDEFINED)
			{
				instr[total].output = "WriteLong("+val+");";
			}
		}
		else
		{
			instr[total].func = UNDEFINED;
		}
	}
	if (mode[0] == OPT_SCP) // -opt=scp：到达定值计算和常量传播
	{
		for (i = 1; i <= total; i++)
		{
			if (instr[i].func >= 0)
			{
				if (instr[i].type == FUNC_TITLE) // 函数入口：构建函数局部变量的符号表（未排除数组或结构体）
				{
					map<int, string>::iterator iter;
					for (iter = func[instr[i].func].params.begin(); iter != func[instr[i].func].params.end(); iter++)
					{
						func[instr[i].func].varname.insert(iter->second);
					}
					for (iter = func[instr[i].func].var.begin(); iter != func[instr[i].func].var.end(); iter++)
					{
						func[instr[i].func].varname.insert(iter->second);
					}
				}
				else if (instr[i].type == FUNC_STORE) // 赋值语句
				{
					if (!defarray(instr[i].output))
					{
						if (func[instr[i].func].varname.find(getdef(instr[i].output)) != func[instr[i].func].varname.end())
						{
							func[instr[i].func].assignment.push_back(i); // 构建函数中赋值语句（FUNC_STORE）的集合
						}
					}
					else
					{
						func[instr[i].func].varname.erase(getdef(instr[i].output)); // 符号表剔除数组和结构体
					}
				}
				else if (instr[i].type == FUNC_RET) // 函数出口：构建函数中各个基本块的gen,kill,in,out集合
				{
					FUNC *f = &func[instr[i].func];
					f->constantspropagated = 0;
					f->buildpred(); // 由后续结点构造前驱结点
					map<int, set<int> >::iterator iter;
					for (iter = f->block.begin(); iter != f->block.end(); iter++) // 初始化
					{
						f->gen.insert(map<int, set<int> >::value_type(iter->first, set<int>()));
						f->kill.insert(map<int, set<int> >::value_type(iter->first, set<int>()));
						f->in.insert(map<int, set<int> >::value_type(iter->first, set<int>()));
						f->out.insert(map<int, set<int> >::value_type(iter->first, set<int>()));
					}
					for (j = f->assignment.size()-1; j >= 0; j--) // 计算每个基本块的gen集合（龙书P387）
					{
						if (f->varname.find(getdef(instr[f->assignment[j]].output)) != f->varname.end() && f->genvar[f->getblock(f->assignment[j])].find(getdef(instr[f->assignment[j]].output)) == f->genvar[f->getblock(f->assignment[j])].end())
						{
							f->gen[f->getblock(f->assignment[j])].insert(f->assignment[j]);
							f->genvar[f->getblock(f->assignment[j])].insert(getdef(instr[f->assignment[j]].output));
						}
					}
					for (j = 0; j < f->assignment.size(); j++) // 计算每个基本块的kill集合（龙书P387）
					{
						if (f->varname.find(getdef(instr[f->assignment[j]].output)) != f->varname.end())
						{
							for (k = 0; k < f->assignment.size(); k++)
							{
								if (j != k && getdef(instr[f->assignment[j]].output) == getdef(instr[f->assignment[k]].output))
								{
									f->kill[f->getblock(f->assignment[j])].insert(f->assignment[k]);
								}
							}
						}
					}
					bool unchange = false;
					while (!unchange) // 计算每个基本块的in集合和out集合（龙书P389）
					{
						unchange = true;
						for (iter = f->block.begin(); iter != f->block.end(); iter++)
						{
							int block = iter->first;
							set<int>::iterator iterset, jterset;
							for (iterset = f->pred[block].begin(); iterset != f->pred[block].end(); iterset++)
								for (jterset = f->out[*iterset].begin(); jterset != f->out[*iterset].end(); jterset++) // 计算in集合
								{
									f->in[block].insert(*jterset);
								}
							set<int> tempset = f->in[block];
							//计算out集合
							for (iterset = f->kill[block].begin(); iterset != f->kill[block].end(); iterset++)
							{
								tempset.erase(*iterset);
							}
							for (iterset = f->gen[block].begin(); iterset != f->gen[block].end(); iterset++)
							{
								tempset.insert(*iterset);
							}
							if (tempset != f->out[block]) // out值是否改变
							{
								unchange = false;
							}
							f->out[block] = tempset;
						}
					}									
				}
			}
		}
		set<int> curin; // 基本块内每条语句的in集合
		for (i = 1; i <= total; i++)
		{
			if (instr[i].func >= 0 && instr[i].type != FUNC_RET)
			{
				FUNC *f = &func[instr[i].func];
				if (f->block.find(i) != f->block.end()) // 进入新的基本块，把curin改为新的基本块的in集合
				{
					curin.clear();
					curin = f->in[i];
				}
				if ((instr[i].type == FUNC_OPERATION || instr[i].type == FUNC_PARAM || instr[i].type == FUNC_WRITE) && instr[i].params[2] != "FP" && instr[i].params[2] != "GP") // 运算指令、参数入栈指令、输出指令可能用到了定值（不考虑数组、结构体及全局变量）
				{
					for (j = 1; j <= 2; j++)
					{
						string varname = "";
						if (contains(instr[i].params[j], "#") && getvar(instr[i].params[j], varname) && f->varname.find(varname) != f->varname.end())
						{
							int value = 0;
							bool init = false;
							bool sameconstant = true;
							set<int>::iterator iter;
							for (iter = curin.begin(); iter != curin.end(); iter++) // 判断某个局部变量是否相等的定值
							{
								if (sameconstant && varname == getdef(instr[*iter].output))
								{
									getconstant(instr[*iter].output, init, sameconstant, value);
								}
								if (!sameconstant)
								{
									break;
								}
							}
							if (init && sameconstant) // 如果某个局部变量是相等的定值，则进行常量替换
							{
								instr[i].params[j] = tostring(value); // 三地址码的常量替换
								f->constantspropagated++;
								if (instr[i].type == FUNC_WRITE && j == 1) // 如果是Write指令还要修改C代码的输出
								{
									set<string> varuse = getuse(instr[i].output);
									set<string>::iterator iter;
									for (iter = varuse.begin(); iter != varuse.end(); iter++)
									{
										if (f->varname.find(*iter) != f->varname.end())
										{
											int value = 0;
											bool init = false;
											bool sameconstant = true;
											set<int>::iterator jter;
											for (jter = curin.begin(); jter != curin.end(); jter++)
											{
												if (sameconstant && *iter == getdef(instr[*jter].output))
												{
													getconstant(instr[*jter].output, init, sameconstant, value);
												}
												if (!sameconstant)
												{
													break;
												}
											}
											if (init && sameconstant)
											{
												instr[i].output = varreplace(instr[i].output, *iter, tostring(value)); // C代码的常量替换
											}
										}
									}
									instr[i].output = calculate(instr[i].output); // C代码的常量替换后再计算常量表达式的值
								}
							}
						}
					}
				}
				else if (instr[i].type == FUNC_STORE) // 赋值语句：可能用到定值，并可能改变当前in集合
				{
					string varname = "";
					if (contains(instr[i].params[1], "#") && getvar(instr[i].params[1], varname) && f->varname.find(varname) != f->varname.end()) // 是否用到定值
					{
						int value = 0;
						bool init = false;
						bool sameconstant = true;
						set<int>::iterator iter;
						for (iter = curin.begin(); iter != curin.end(); iter++)
						{
							if (sameconstant && varname == getdef(instr[*iter].output))
							{
								getconstant(instr[*iter].output, init, sameconstant, value);
							}
							if (!sameconstant)
							{
								break;
							}
						}
						if (init && sameconstant)
						{
							instr[i].params[1] = tostring(value); // 三地址码的常量替换
							f->constantspropagated++;
						}
					}
					set<string> varuse = getuse(instr[i].output);
					set<string>::iterator iter;
					for (iter = varuse.begin(); iter != varuse.end(); iter++) // 看输出的C代码是否有使用定值
					{
						if (f->varname.find(*iter) != f->varname.end())
						{
							int value = 0;
							bool init = false;
							bool sameconstant = true;
							set<int>::iterator jter;
							for (jter = curin.begin(); jter != curin.end(); jter++)
							{
								if (sameconstant && *iter == getdef(instr[*jter].output))
								{
									getconstant(instr[*jter].output, init, sameconstant, value);
								}
								if (!sameconstant)
								{
									break;
								}
							}
							if (init && sameconstant)
							{
								instr[i].output = varreplace(instr[i].output, *iter, tostring(value)); // C代码的常量替换
							}
						}
					}
					instr[i].output = calculate(instr[i].output); // C代码的常量替换后再计算常量表达式的值
					// 修改当前in集合（龙书P388）
					varname = getdef(instr[i].output);
					if (f->varname.find(varname) != f->varname.end())
					{
						set<int>::iterator iter, jter;
						for (iter = curin.begin(); iter != curin.end(); )
						{
							jter = iter;
							if (varname == getdef(instr[*iter].output))
							{
								iter++;
								curin.erase(jter);
							}
							else
							{
								iter++;
							}
						}
						curin.insert(i);
					}
				}
				else if (instr[i].type == FUNC_CALL || instr[i].type == FUNC_BRANCH_BEGIN) // 跳转语句：输出的C代码可能用到定值
				{
					set<string> varuse = getuse(instr[i].output);
					set<string>::iterator iter;
					for (iter = varuse.begin(); iter != varuse.end(); iter++) // 看输出的C代码是否有使用定值
					{
						if (f->varname.find(*iter) != f->varname.end())
						{
							int value = 0;
							bool init = false;
							bool sameconstant = true;
							set<int>::iterator jter;
							for (jter = curin.begin(); jter != curin.end(); jter++)
							{
								if (sameconstant && *iter == getdef(instr[*jter].output))
								{
									getconstant(instr[*jter].output, init, sameconstant, value);
								}
								if (!sameconstant)
								{
									break;
								}
							}
							if (init && sameconstant)
							{
								instr[i].output = varreplace(instr[i].output, *iter, tostring(value)); // C代码的常量替换
							}
						}
					}
					instr[i].output = calculate(instr[i].output); // C代码的常量替换后再计算常量表达式的值
					if (instr[i].type == FUNC_BRANCH_BEGIN && instr[i].output[0] != '(') // while (..) 或 if (..) 不能消除"("和")"
					{
						instr[i].output = "("+instr[i].output+")";
					}
				}
			}
		}
		if (warningdiv0) // 出现除零
		{
			cerr << "[Warning] division by zero"<<endl;
		}
	}
	else if (mode[0] == OPT_DSE) // -opt=dse：活跃变量分析和死表达式消除
	{
		for (i = 0; i < func.size(); i++)
		{
			map<int, set<int> >::iterator iter;
			set<int>::iterator jter;
			func[i].buildpred(); // 由后续结点构造前驱结点
			for (iter = func[i].block.begin(); iter != func[i].block.end(); iter++)
				for (jter = iter->second.begin(); jter != iter->second.end(); jter++)
				{
					if (*jter < iter->first)
					{
						func[i].visited.clear();
						func[i].visited.insert(iter->first);
						func[i].visited.insert(*jter);
						func[i].scrblock.insert(*jter);
						func[i].dfs(iter->first); // 深度优先搜索获取SCR（龙书P424）
					}
				}
			func[i].statementseliminatedinscr = func[i].statementseliminatedoutscr = 0;
		}
		for (i = 1; i <= total; i++)
		{
			if (instr[i].func >= 0)
			{
				if (instr[i].type == FUNC_TITLE) // 函数入口：构建函数局部变量的符号表（包括数组或结构体）、初始化def,use,in,out集合
				{
					map<int, string>::iterator iter;
					map<int, set<int> >::iterator jter;
					for (iter = func[instr[i].func].params.begin(); iter != func[instr[i].func].params.end(); iter++)
					{
						func[instr[i].func].varname.insert(iter->second);
					}
					for (iter = func[instr[i].func].var.begin(); iter != func[instr[i].func].var.end(); iter++)
					{
						func[instr[i].func].varname.insert(iter->second);
					}
					for (jter = func[instr[i].func].block.begin(); jter != func[instr[i].func].block.end(); jter++)
					{
						func[instr[i].func].def.insert(map<int, set<string> >::value_type(jter->first, set<string>()));
						func[instr[i].func].use.insert(map<int, set<string> >::value_type(jter->first, set<string>()));
						func[instr[i].func].invar.insert(map<int, set<string> >::value_type(jter->first, set<string>()));
						func[instr[i].func].outvar.insert(map<int, set<string> >::value_type(jter->first, set<string>()));
					}
				}
				else if (instr[i].type == FUNC_STORE || instr[i].type == FUNC_CALL || instr[i].type == FUNC_WRITE || instr[i].type == FUNC_BRANCH_BEGIN) // 根据赋值语句、跳转语句、输出语句构建各个基本块的def和use集合（龙书P390）
				{
					set<string> varuse = getuse(instr[i].output);
					set<string>::iterator iterset;
					int block = func[instr[i].func].getblock(i);
					for (iterset = varuse.begin(); iterset != varuse.end(); iterset++) // 变量的使用
					{
						if (func[instr[i].func].def[block].find(*iterset) == func[instr[i].func].def[block].end())
						{
							func[instr[i].func].use[block].insert(*iterset);
						}
					}
					if (instr[i].type == FUNC_STORE) // 变量的赋值
					{
						string vardef = getdef(instr[i].output);
						if (func[instr[i].func].use[block].find(vardef) == func[instr[i].func].use[block].end())
						{
							func[instr[i].func].def[block].insert(vardef);
						}
					}
				}
				else if (instr[i].type == FUNC_RET) // 函数出口：构建函数中各个基本块的in和out集合（龙书P391）
				{
					FUNC *f = &func[instr[i].func];
					map<int, set<int> >::iterator iter;
					set<string>::iterator iterset;
					bool unchange = false;
					while (!unchange)
					{
						unchange = true;
						iter = f->block.end();
						do
						{
							iter--;
							int block = iter->first;
							set<int>::iterator jter;
							for (jter = f->block[block].begin(); jter != f->block[block].end(); jter++)
								for (iterset = f->invar[*jter].begin(); iterset != f->invar[*jter].end(); iterset++) // 计算out集合
								{
									f->outvar[block].insert(*iterset);
								}
							// 计算in集合
							set<string> tempset = f->outvar[block];
							for (iterset = f->def[block].begin(); iterset != f->def[block].end(); iterset++)
							{
								tempset.erase(*iterset);
							}
							for (iterset = f->use[block].begin(); iterset != f->use[block].end(); iterset++)
							{
								tempset.insert(*iterset);
							}
							if (tempset != f->invar[block]) // in值是否改变
							{
								unchange = false;
							}
							f->invar[block] = tempset;
						}
						while (iter != f->block.begin());
					}
				}
			}
		}
		set<string> curout; // 基本块内每条语句的out集合
		for (i = total; i >= 1; i--) // 后向
		{
			if (instr[i].func >= 0 && instr[i].type != FUNC_TITLE)
			{
				FUNC *f = &func[instr[i].func];
				if (instr[i].type == FUNC_RET || f->block.find(i+1) != f->block.end()) // 基本块改变时，修改当前out集合
				{
					curout.clear();
					curout = f->outvar[f->getblock(i)];
				}
				if (instr[i].type == FUNC_OPERATION || instr[i].type == FUNC_WRITE || instr[i].type == FUNC_PARAM) // 运算指令、输出指令、参数入栈指令可能有对变量的使用
				{
					for (j = 1; j <= 2; j++)
					{
						string varname = "";
						if (contains(instr[i].params[j], "#") && getvar(instr[i].params[j], varname) && f->varname.find(varname) != f->varname.end())
						{
							curout.insert(varname); // 修改当前out集合（龙书P388）
						}
					}
				}
				else if (instr[i].type == FUNC_STORE) // 赋值指令可能有对变量的使用和赋值
				{
					string varname = getdef(instr[i].output);
					if (f->varname.find(varname) != f->varname.end()) // 被赋值的变量不是活跃变量
					{
						if (curout.find(varname) == curout.end())
						{
							instr[i].unlive = true;
							if (f->scrblock.find(f->getblock(i)) != f->scrblock.end())
							{
								f->statementseliminatedinscr++; // 该指令在SCR内
							}
							else
							{
								f->statementseliminatedoutscr++; // 该指令不在SCR内
							}
						}
						curout.erase(varname);
					}
					if (contains(instr[i].params[1], "#") && getvar(instr[i].params[1], varname) && f->varname.find(varname) != f->varname.end())
					{
						curout.insert(varname); // 修改当前out集合（龙书P388）
					}
				}
			}
		}
	}
	if (mode[1] == BACKEND_C) // -backend=c：输出翻译三地址码得到的（经优化后的）C代码
	{
		// 声明
		cout << "#include <stdio.h>" << endl;
		cout << "#define WriteLine() printf(\"\\n\");" << endl;
		cout << "#define WriteLong(x) printf(\" %lld\", (long)x);" << endl;
		cout << "#define ReadLong(a) if (fscanf(stdin, \"%lld\", &a) != 1) a = 0;" << endl;
		cout << "#define long long long" << endl << endl;
		int lastaddr = MAX_ADDR;
		map<int, string>::iterator iter = global.end();
		if (!global.empty()) // 输出全局变量定义
		{
			do
			{
				iter--;
				if (lastaddr-(iter->first) == 8)
				{
					cout << "long " << iter->second << ";" << endl; // 前一个变量的地址和后一个变量的地址差8，则为long，下同
				}
				else
				{
					cout << "long " << iter->second << "[" << (lastaddr-(iter->first))/8 << "];" << endl; // 前一个变量的地址和后一个变量的地址差超过8，则为数组（结构体也视为数组），下同
				}
				lastaddr = iter->first;
			}
			while (iter != global.begin());
			cout<<endl;
		}
		int indent = 0; // 缩进
		for (i = 1; i <= total; i++)
		{
			if (instr[i].func >= 0)
			{
				for (j = 0; j < instr[i].rightbrace; j++) // 输出"}"
				{
					indent--;
					printindent(indent);
					cout << "}" << endl;
				}
				if (instr[i].beginwithelse) // 输出"else"
				{
					printindent(indent);
					cout << "else" << endl;
					printindent(indent);
					cout << "{" << endl;
					indent++;
				}
				if (instr[i].type == FUNC_TITLE) // 函数入口：输出函数名、函数参数和函数局部变量定义
				{
					cout << "void " << func[instr[i].func].name << "("; // 输出函数名
					iter = func[instr[i].func].params.end();
					if (!func[instr[i].func].params.empty())
					{
						iter--;
					}
					for (j = 0; j < func[instr[i].func].paramnum; j++) // 输出函数参数
					{
						if (j == 0)
						{
							cout << "long ";
						}
						else
						{
							cout << ", long ";
						}
						if (!func[instr[i].func].params.empty() && iter != func[instr[i].func].params.end() && (iter->first == 8+8*(func[instr[i].func].paramnum-j)))
						{
							cout << iter->second;
							if (iter != func[instr[i].func].params.begin())
							{
								iter--;
							}
							else
							{
								iter = func[instr[i].func].params.end();
							}
						}
						else
						{
							cout << "arg_" << j+1;
						}
					}
					cout << ")" << endl << "{" << endl;
					indent = 1;
					if (!func[instr[i].func].var.empty()) // 输出函数局部变量定义
					{
						lastaddr = 0;
						iter = func[instr[i].func].var.end();
						do
						{
							iter--;
							if (lastaddr-(iter->first) == 8)
							{
								printindent(indent);
								cout << "long " << iter->second << ";" << endl;
							}
							else
							{
								printindent(indent);
								cout << "long " << iter->second << "[" << (lastaddr-(iter->first))/8 << "];" << endl;
							}
							lastaddr = iter->first;
						}
						while (iter != func[instr[i].func].var.begin());
					}
				}
				else if (instr[i].type == FUNC_RET) // 函数出口：输出"}"
				{
					cout << "}" << endl << endl;
					indent = 0;
				}
				else if (instr[i].type == FUNC_STORE || instr[i].type == FUNC_WRITE || instr[i].type == FUNC_CALL) // 赋值指令、输出指令、调用指令：直接输出
				{
					printindent(indent);
					if (instr[i].unlive) // 注释死表达式
					{
						cout << "//" << instr[i].output << endl; 
					}
					else
					{
						cout << instr[i].output << endl;
					}
				}
				else if (instr[i].type == FUNC_BRANCH_BEGIN) // 条件分支指令：输出关键字（if或while）和条件
				{
					printindent(indent);
					cout << instr[i].key << " " << instr[i].output << endl;
					printindent(indent);
					cout << "{" << endl;
					indent++; 
				}
			}
			else if (instr[i].func == UNDEFINED)
			{
				cout << "Undefined : " << instr[i].full << endl;
			}
		}
	}
	else if (mode[1] == BACKEND_CFG) // -backend=cfg：基本块分析
	{
		for (i = 0; i < func.size(); i++) // 遍历每个函数
		{
			cout << "Function: " << func[i].block.begin()->first << endl;
			map<int, set<int> >::iterator iter;
			cout << "Basic blocks:";
			for (iter = func[i].block.begin(); iter != func[i].block.end(); iter++) // 输出每个基本块
			{
				cout << " " << iter->first;
			}
			cout << endl << "CFG:" << endl;
			for (iter = func[i].block.begin(); iter != func[i].block.end(); iter++)
			{
				cout << iter->first << " ->";
				set<int>::iterator iterset;
				for (iterset = iter->second.begin(); iterset != iter->second.end(); iterset++) // 输出每个基本块的后续结点
				{
					cout << " " << *iterset;
				}
				cout << endl;
			}
		}
	}
	else if (mode[1] == BACKEND_3ADDR) // -backend=3addr：输出（经优化后的）三地址码
	{
		for (i = 1; i <= total; i++)
		{
			if (instr[i].unlive) // 死表达式改为nop指令
			{
				cout << "    instr " << i << ": nop" << endl;
			}
			else
			{
				cout << "    instr " << i << ": " << instr[i].params[0];
				if (instr[i].params[1] != "")
				{
					cout << " " << instr[i].params[1];	
				}
				if (instr[i].params[2] != "")
				{
					cout << " " << instr[i].params[2];
				}
				cout << endl;
			}
		}
	}
	else if (mode[1] == BACKEND_REP) // -backend=rep：优化报告（见Lab2要求）
	{
		if (mode[0] == OPT_SCP)
		{
			for (i = 0; i < func.size(); i++)
			{
				cout << "Function: " << func[i].block.begin()->first << endl;
				cout << "Number of constants propagated: " << func[i].constantspropagated << endl;
			}
		}
		else if (mode[0] == OPT_DSE)
		{
			for (i = 0; i < func.size(); i++)
			{
				cout << "Function: " << func[i].block.begin()->first << endl;
				cout << "Number of statements eliminated in SCR: " << func[i].statementseliminatedinscr << endl;
				cout << "Number of statements eliminated not in SCR: " << func[i].statementseliminatedoutscr << endl;
			}			
		}
	}
	return 0;
}
