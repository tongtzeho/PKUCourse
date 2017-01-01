#include <iostream>
#include <sstream>
#include <string>
#include <cstdlib>
#include <vector>
#include <map>

#define INPUT_STR_MAX_LENGTH 1024

#define UNDEFINED -2
#define NOP -1

#define FUNC_OPERATION 0
#define FUNC_TITLE 1
#define FUNC_RET 2
#define FUNC_STORE 3
#define FUNC_WRITE 4
#define FUNC_BRANCH_BEGIN 5
#define FUNC_BRANCH_END 6
#define FUNC_PARAM 7
#define FUNC_CALL 8

#define MAX_ADDR 32768

using namespace std;

map<int, string> global;

class FUNC
{
public:
	string name;
	int size;
	int paramnum;
	map<int, string> params;
	map<int, string> var;	
};

vector<FUNC> func;

class INSTR
{
public:
	string full;
	int instrno;
	string params[3];
	int func;
	int type;
	string result;
	string resultdiv8;
	bool isaddr;
	string arrayname;
	string index;
	string output;
	string key;
	int rightbrace;
	bool beginwithelse;
	INSTR()
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
	}
};

vector<INSTR> instr;

int split(char *str, string *args)
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

string tostring(int n)
{
	stringstream ss;
	ss << n;
	string result;
	ss >> result;
	return result;
}

inline bool isopcode(string str)
{
	return str == "add" || str == "sub" || str == "mul" || str == "div" || str == "mod" || str == "cmpeq" || str == "cmple" || str == "cmplt";
}

int getvar(string str, string &name)
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

bool contains(string longstr, string shortstr)
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

bool allnumber(string str)
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

void printindent(int n)
{
	int i;
	for (i = 0; i < n; i++)
	{
		cout << "    ";
	}
}

string inverse(string str)
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

int main()
{
	char inputstr[INPUT_STR_MAX_LENGTH];
	int total = 0;
	int i, j, k;
	vector<string> callparams;
	instr.push_back(INSTR());
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
			if (contains(instr[total].params[i], "#") && !contains(instr[total].params[i], "_offset#") && instr[total].params[2] != "GP")
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
		if (instr[total].params[0] == "nop" || instr[total].params[0] == "entrypc")
		{
			instr[total].func = NOP;
		}
		else if (instr[total].params[0] == "enter")
		{
			func.push_back(FUNC());
			if (instr[total-1].params[0] == "entrypc")
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
		}
		else if (instr[total].params[0] == "ret")
		{
			instr[total].func = func.size()-1;
			instr[total].type = FUNC_RET;
			func.back().paramnum = atoi(instr[total].params[1].c_str())/8;
		}
		else if (isopcode(instr[total].params[0]))
		{
			instr[total].func = func.size()-1;
			instr[total].type = FUNC_OPERATION;			
			if (instr[total].params[0] == "add" && instr[total].params[2] == "GP")
			{
				string varname;
				int addr = getvar(instr[total].params[1], varname);
				global.insert(map<int, string>::value_type(addr, varname));
				instr[total].result = varname;
				instr[total].isaddr = true;
				instr[total].arrayname = varname;
				instr[total].index = "";
			}
			else if (instr[total].params[0] == "add" && instr[total].params[2] == "FP")
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
			else if (instr[total].params[0] == "add" && contains(instr[total].params[2], "_offset#"))
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
			else
			{
				string operand[2] = {"", ""};
				int reg[2] = {-1, -1};
				for (i = 1; i <= 2; i++)
				{
					if (contains(instr[total].params[i], "#"))
					{
						string varname;
						getvar(instr[total].params[i], varname);
						operand[i-1] = varname;
					}
					else if (allnumber(instr[total].params[i]))
					{
						operand[i-1] = instr[total].params[i];
					}
					else if (instr[total].params[i][0] == '(')
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
					if (reg[0] == -1 && reg[1] == -1)
					{
						instr[total].isaddr = false;
						instr[total].result = "("+operand[0]+tooperator(instr[total].params[0])+operand[1]+")";						
					}
					else if (reg[0] != -1 && reg[1] == -1)
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
					else if (reg[0] == -1 && reg[1] != -1)
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
					else if (reg[0] != -1 && reg[1] != -1)
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
					if (!instr[total].isaddr && (reg[0] == -1 || !instr[reg[0]].isaddr) && (reg[1] == -1 || !instr[reg[1]].isaddr))
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
		else if (instr[total].params[0] == "move")
		{
			instr[total].func = func.size()-1;
			instr[total].type = FUNC_STORE;
			string var2;
			getvar(instr[total].params[2], var2);
			if (contains(instr[total].params[1], "#"))
			{
				string var1;
				getvar(instr[total].params[1], var1);
				instr[total].output = var2+" = "+var1+";";
				instr[total].result = var2;
				instr[total].isaddr = false;
			}
			else if (allnumber(instr[total].params[1]))
			{
				instr[total].output = var2+" = "+instr[total].params[1]+";";
				instr[total].result = var2;
				instr[total].isaddr = false;
			}
			else if (instr[total].params[1][0] == '(')
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
		else if (instr[total].params[0] == "store")
		{
			instr[total].func = func.size()-1;
			instr[total].type = FUNC_STORE;
			int reg2 = atoi(instr[total].params[2].substr(1, instr[total].params[2].length()-1).c_str());
			string val1 = "";
			if (contains(instr[total].params[1], "#"))
			{
				getvar(instr[total].params[1], val1);
			}
			else if (allnumber(instr[total].params[1]))
			{
				val1 = instr[total].params[1];
			}
			else if (instr[total].params[1][0] == '(')
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
				if (instr[reg2].index == "")
				{
					val2 = instr[reg2].arrayname;
				}
				else
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
		else if (instr[total].params[0] == "load")
		{
			instr[total].func = func.size()-1;
			instr[total].type = FUNC_OPERATION;
			instr[total].isaddr = false;
			int reg = atoi(instr[total].params[1].substr(1, instr[total].params[1].length()-1).c_str());
			if (!instr[reg].isaddr)
			{
				instr[total].result = instr[reg].result;
			}
			else if (instr[reg].index == "")
			{
				instr[total].result = instr[reg].arrayname;
			}
			else
			{
				instr[total].result = instr[reg].arrayname+"["+instr[reg].index+"]";
			}
		}
		else if (instr[total].params[0] == "param")
		{
			instr[total].func = func.size()-1;
			instr[total].type = FUNC_PARAM;
			string val;
			if (contains(instr[total].params[1], "#"))
			{
				getvar(instr[total].params[1], val);
			}
			else if (allnumber(instr[total].params[1]))
			{
				val = instr[total].params[1];
			}
			else if (instr[total].params[1][0] == '(')
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
				callparams.push_back(val);
			}			
		}
		else if (instr[total].params[0] == "call")
		{
			instr[total].func = func.size()-1;
			instr[total].type = FUNC_CALL;
			int reg = atoi(instr[total].params[1].substr(1, instr[total].params[1].length()-1).c_str());
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
		}
		else if (instr[total].params[0] == "blbc" || instr[total].params[0] == "blbs")
		{
			instr[total].func = func.size()-1;
			instr[total].type = FUNC_BRANCH_BEGIN;
			int reg1 = atoi(instr[total].params[1].substr(1, instr[total].params[1].length()-1).c_str());
			int reg2 = atoi(instr[total].params[2].substr(1, instr[total].params[2].length()-1).c_str());
			while (instr.size() <= reg2)
			{
				instr.push_back(INSTR());
			}
			instr[reg2].rightbrace++;
			instr[total].key = "if";
			if (instr[total].params[0] == "blbc")
			{
				instr[total].output = instr[reg1].result;
			}
			else
			{
				instr[total].output = inverse(instr[reg1].result);
			}
		}
		else if (instr[total].params[0] == "br")
		{
			instr[total].func = func.size()-1;
			instr[total].type = FUNC_BRANCH_END;
			int reg = atoi(instr[total].params[1].substr(1, instr[total].params[1].length()-1).c_str());
			if (reg < total)
			{
				for (i = reg+1; i < total; i++)
				{
					if (instr[i].key == "if")
					{
						instr[i].key = "while";
						break;
					}
				}
			}
			else
			{
				while (instr.size() <= reg)
				{
					instr.push_back(INSTR());
				}
				instr[reg].rightbrace++;
				instr[total+1].beginwithelse = true;
			}
		}
		else if (instr[total].params[0] == "wrl")
		{
			instr[total].func = func.size()-1;
			instr[total].type = FUNC_WRITE;
			instr[total].output = "WriteLine();";
		}
		else if (instr[total].params[0] == "write")
		{
			instr[total].func = func.size()-1;
			instr[total].type = FUNC_WRITE;
			string val;
			if (contains(instr[total].params[1], "#"))
			{
				getvar(instr[total].params[1], val);
			}
			else if (allnumber(instr[total].params[1]))
			{
				val = instr[total].params[1];
			}
			else if (instr[total].params[1][0] == '(')
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
	cout << "#include <stdio.h>" << endl;
	cout << "#define WriteLine() printf(\"\\n\");" << endl;
	cout << "#define WriteLong(x) printf(\" %lld\", (long)x);" << endl;
	cout << "#define ReadLong(a) if (fscanf(stdin, \"%lld\", &a) != 1) a = 0;" << endl;
	cout << "#define long long long" << endl << endl;
	int lastaddr = MAX_ADDR;
	map<int, string>::iterator iter = global.end();
	if (!global.empty())
	{
		do
		{
			iter--;
			if (lastaddr-(iter->first) == 8)
			{
				cout << "long " << iter->second << ";" << endl;
			}
			else
			{
				cout << "long " << iter->second << "[" << (lastaddr-(iter->first))/8 << "];" << endl;
			}
			lastaddr = iter->first;
		}
		while (iter != global.begin());
		cout<<endl;
	}
	int indent = 0;
	for (i = 1; i <= total; i++)
	{
		if (instr[i].func >= 0)
		{
			for (j = 0; j < instr[i].rightbrace; j++)
			{
				indent--;
				printindent(indent);
				cout << "}" << endl;
			}
			if (instr[i].beginwithelse)
			{
				printindent(indent);
				cout << "else" << endl;
				printindent(indent);
				cout << "{" << endl;
				indent++;
			}
			if (instr[i].type == FUNC_TITLE)
			{
				cout << "void " << func[instr[i].func].name << "(";
				iter = func[instr[i].func].params.end();
				if (!func[instr[i].func].params.empty())
				{
					iter--;
				}
				for (j = 0; j < func[instr[i].func].paramnum; j++)
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
				if (!func[instr[i].func].var.empty())
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
			else if (instr[i].type == FUNC_RET)
			{
				cout << "}" << endl << endl;
				indent = 0;
			}
			else if (instr[i].type == FUNC_STORE || instr[i].type == FUNC_WRITE || instr[i].type == FUNC_CALL)
			{
				printindent(indent);
				cout << instr[i].output << endl;
			}
			else if (instr[i].type == FUNC_BRANCH_BEGIN)
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
	return 0;
}
