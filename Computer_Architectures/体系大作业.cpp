/*
Main.cpp
作者：唐子豪 李长松 汤子洋

程序流程：
1.从instruction.txt中读取Unicore32指令
2.调用asm2bin.h中的函数，将汇编指令转成二进制指令，并保存在指令存储器中
3.按取指->译码->执行->访存->写回的顺序，在流水线结构中运行指令
 (1)每个时钟周期，分别创建一个取指线程、一个译码线程、一个执行线程、一个访存线程和一个回写线程，然后等待这五个线程都结束后，才进入下一个时钟周期 
 (2)取指线程对指令存储器中PC寄存器的地址进行取指
 	①按PC寄存器的地址进行取指
	②如果取到的指令是0xFFFFFFFF，则下一周期不再取指，不再进行下一步
	③如果当前执行的指令不是跳转指令，或还未判定是否跳转指令，则进行PC+4操作
 (3)译码线程对上一时钟周期取的指令（如果有的话）进行译码
	①指令的高8位为操作码
	②根据操作码确定state的值，即该周期的执行方式、是否访存、是否取指
 (4)执行线程根据上一周期对指令译码的结果分情况（R型、I型、存取、分支、跳转等）执行指令
	①先读取寄存器的值
    ②执行R型指令：如果需要移位则执行移位，如果是减法则对操作数2取反加1，之后根据操作码确定ALU的op值，然后用ALU进行计算
	③执行I型指令：先对立即数做零扩展，然后移位，再根据操作码确定ALU的op值和setflag位，然后用ALU进行计算
	④执行存取指令：如果有rs2，则进行移位；如果是imm14，则进行扩展。接着用ALU计算地址
	⑤执行BEQ指令：如果Z位为1，则将PC的值变成PC+4+offset，并将该指令标为跳转指令；如果Z位为0，无操作
	⑥执行B指令：将PC的值变成PC+4+offset，并将该指令标为跳转指令
	⑦执行JUMP指令：将PC的值变成rs2寄存器的值，并将该指令标为跳转指令
 (5)访存线程对上一周期的存取指令进行访存读写操作
    ①判断是读存储器还是写存储器
	②访问Cache，通过Cache访问存储器
 (6)写回线程对上上周期执行的指令需要回写（如果有需要的话）
 (7)如果该时钟周期执行的指令发生了跳转，则下一周期不进行译码和执行（相当于停止了顺序将执行的指令的执行）
 (8)打印信息
*/
 
#include "types.h" // 定义了寄存器堆、存储器、Cache、ALU、扩展器、移位器等类
#include "asm2bin.h" // 汇编转二进制的函数

#include <iostream>
#include <fstream>
#include <cmath>
#include <sstream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <windows.h>

#define INSTRUCTION_FILE_NAME "instruction.txt"
#define STRING_MAXLENGTH 1024

using namespace std;

vector<string> vasm; // 保存输入的汇编指令
vector<unsigned> vins; // 保存汇编指令对应的二进制指令
vector<LABEL> vlabel,vbi; // 保存跳转标号的地址
int clock=0; // 时钟周期

REGISTER_FILE regf; // 寄存器堆
PC pc; // PC寄存器
ALU alu; // ALU
ADDER adder; // 加法器
MEMORY insmemory(IMEM_SIZE),datamemory(DMEM_SIZE); // 指令存储器和数据存储器
EXTENDER extender; // 扩展器
SHIFTER shifter; // 移位器
CACHE cache(&datamemory); // 数据存储器的Cache

HANDLE mutex_pc=CreateSemaphore(NULL,1,1,NULL); // 多线程编程中PC寄存器的信号量
HANDLE mutex_cpi=CreateSemaphore(NULL,1,1,NULL);

void InputInstructions();
void ParseInstruction(string instruction);
void ParseBInstruction();
void LoadInstructions();

int state[5][5]={{0,-1,-1,-1,-1},{0,0,-1,-1,-1},{0,0,0,-1,-1},{0,0,0,0,-1},{0,0,0,0,0}}; // 下一周期各线程的状态（是否需要做某一操作/需要如何操作）
unsigned instructions[5]={0}; // 某一周期取的指令
unsigned pctemp[5]={0}; // 某一周期取指时的PC寄存器值
unsigned cachetemp[5][2]={0}; // 某一周期访存的地址和数据
unsigned regwritetemp[5][3]={0}; // 某一周期执行或访存后修改的寄存器的结果
bool isjumpins=false; // 执行的指令是否跳转
bool wbtemp[5]={0}; // 写回的结果

unsigned cpiwithcache=0; // 使用Cache的CPI
unsigned cpiwithoutcache=0; // 不使用Cache的CPI
unsigned execins=0;

DWORD WINAPI FetchInstructions(LPVOID lpParam);
DWORD WINAPI Decode(LPVOID lpParam);
DWORD WINAPI Execute(LPVOID lpParam);
DWORD WINAPI VisitCache(LPVOID lpParam);
DWORD WINAPI WriteBack(LPVOID lpParam);

void ExecRIns(unsigned instruction);
void ExecIIns(unsigned instruction);
void ExecSL(unsigned instruction);
void ExecBeq(unsigned instruction);
void ExecB(unsigned instruction);
void ExecJump(unsigned instruction);

// 从instruction.txt输入汇编指令
void InputInstructions()
{
	ifstream fin;
	fin.open(INSTRUCTION_FILE_NAME);
	char instruction[STRING_MAXLENGTH];
	while(fin.getline(instruction,STRING_MAXLENGTH))
	{
		ParseInstruction(instruction);
	}
	fin.close();
}

inline bool isSplit(char c)
{
	return c==' '||c==','||c=='<'||c=='>'||c=='+'||c=='['||c==']'||c=='#';
}

// 根据输入的指令的操作调用asm2bin.h的函数转化成二进制
void ParseInstruction(string instruction)
{
	int length=instruction.length();
	int i;
	bool islabel=false;
	string params[8];
	string inscopy=instruction;
	if(length==0)
	{
		return;
	}
	for(i=0;i<length;i++)
	{
		if(isSplit(instruction[i]))
		{
			instruction[i]=' ';
		}
		else if(instruction[i]==':')
		{
			islabel=true;
			break;
		}
	}
	if(!islabel)
	{
		stringstream ss;
		unsigned ins;
		i=0;
		ss<<instruction;
		while(ss>>params[i])
		{
			i++;
		}
		string operation=params[0];
		if(params[0]=="ADD")
		{
			ins=ADD2BinCode(i,params);
		}
		else if(params[0]=="SUB")
		{
			ins=SUB2BinCode(i,params);
		}
		else if(params[0]=="MUL")
		{
			ins=MUL2BinCode(i,params);
		}
		else if(params[0]=="AND")
		{
			ins=AND2BinCode(i,params);
		}
		else if(params[0]=="OR")
		{
			ins=OR2BinCode(i,params);
		}
		else if(params[0]=="MOV")
		{
			ins=MOV2BinCode(i,params);
		}
		else if(params[0]=="LDB")
		{
			ins=LDB2BinCode(i,params);
		}
		else if(params[0]=="LDW")
		{
			ins=LDW2BinCode(i,params);
		}
		else if(params[0]=="STB")
		{
			ins=STB2BinCode(i,params);
		}
		else if(params[0]=="STW")
		{
			ins=STW2BinCode(i,params);
		}
		else if(params[0]=="CMPSUB.A")
		{
			ins=CMPSUBA2BinCode(i,params);
		}
		else if(params[0]=="BEQ")
		{
			string label=params[1];
			vbi.push_back(LABEL(vins.size(),label));
			ins=BEQ2BinCode(i,params);
		}
		else if(params[0]=="B")
		{
			string label=params[1];
			vbi.push_back(LABEL(vins.size(),label));
			ins=B2BinCode(i,params);
		}
		else if(params[0]=="JUMP")
		{
			ins=JUMP2BinCode(i,params);
		}
		else return;
		vins.push_back(ins);
		vasm.push_back(inscopy);		
	}
	else
	{
		int j=-1,k;
		for(k=0;k<length;k++)
		{
			if(j==-1&&((instruction[k]>='a'&&instruction[k]<='z')||(instruction[k]>='A'&&instruction[k]<='Z')||(instruction[k]>='0'&&instruction[k]<='9')||instruction[k]=='_'))
			{
				j=k;
			}
			else if(instruction[k]==':')
			{
				break;
			}
		}
		vlabel.push_back(LABEL(vins.size(),instruction.substr(j,k-j)));
		ParseInstruction(instruction.substr(i+1,instruction.length()-i-1));
	}
}

// 计算分支和跳转指令的地址
void ParseBInstructions()
{
	int i,j;
	for(i=0;i<vbi.size();i++)
	{
		for(j=0;j<vlabel.size();j++)
		{
			if(vbi[i].name==vlabel[j].name)
			{
				break;
			}
		}
		int index=vbi[i].pos;
		int imm24=(vlabel[j].pos-index)*4-4;
		vins[index]|=SignedInt(imm24,24);
	}
}

// 将二进制指令读入指令存储器
void LoadInstructions()
{
	int i;
	cout<<"指令:"<<endl;
	for(i=0;i<vins.size();i++)
	{
		insmemory.addr=i*4;
		insmemory.data=vins[i];
		insmemory.Write(4);
		printf("0x%08X : ",i*4);
		BinOut(vins[i]);
		cout<<vasm[i]<<endl;
	}
	insmemory.addr=i*4;
	insmemory.data=0xFFFFFFFF; // 最后一条指令之后的4个字节用0xFFFFFFFF表示，作为指令的结束
	insmemory.Write(4);
}

// 取指线程
DWORD WINAPI FetchInstructions(LPVOID lpParam)
{ 
	if(state[clock%5][0]==-1)
	{
		state[(clock+1)%5][0]=state[(clock+1)%5][1]=-1;
		return 0;
	}
	WaitForSingleObject(mutex_output,INFINITE);
	cout<<"取指 - "<<vasm[pc.pc/4]<<endl;
	ReleaseSemaphore(mutex_output,1,NULL);
	insmemory.addr=pc.pc;
	insmemory.Read(4);
	instructions[clock%5]=insmemory.data;
	if(instructions[clock%5]==0xFFFFFFFF)
	{
		state[(clock+1)%5][0]=state[(clock+1)%5][1]=-1;
		return 0;
	}
	else
	{
		state[(clock+1)%5][0]=state[(clock+1)%5][1]=0;
	}
	pctemp[(clock+2)%5]=pc.pc;
	
	//如果该周期的执行过程判断了执行的指令是跳转指令，且已经修改了PC寄存器的值，就不进行PC+4操作，否则加锁后进行PC+4
	WaitForSingleObject(mutex_pc,INFINITE);
	if(isjumpins)
	{
		ReleaseSemaphore(mutex_pc,1,NULL);
		return 0;
	}
	else
	{
		adder.inA=pc.pc;
		adder.inB=4;
		adder.add();
		pc.pcsrc[0]=pc.pcsrc[1]=0;
		pc.next(regf,adder,alu);
	}
	ReleaseSemaphore(mutex_pc,1,NULL);
	
	return 0;
}

// 译码线程
DWORD WINAPI Decode(LPVOID lpParam)
{
	if(state[clock%5][1]==-1)
	{
		state[(clock+1)%5][2]=-1;
		return 0;
	}
	WaitForSingleObject(mutex_output,INFINITE);
	cout<<"译码 - "<<vasm[pctemp[(clock+1)%5]/4]<<endl;
	ReleaseSemaphore(mutex_output,1,NULL);
	unsigned instruction=instructions[(clock+4)%5];
	unsigned opcode=(instruction>>24)&0xFF;
	if(opcode==0xA0) // 分支（BEQ）指令
	{
		state[(clock+1)%5][2]=3;
		state[(clock+2)%5][3]=state[(clock+3)%5][4]=-1;
	}
	else if(opcode==0xBC) // B跳转指令
	{
		state[(clock+1)%5][2]=4;
		state[(clock+2)%5][3]=state[(clock+3)%5][4]=-1;
	}
	else if(opcode==0x10) // JUMP跳转指令
	{
		state[(clock+1)%5][2]=5;
		state[(clock+2)%5][3]=state[(clock+3)%5][4]=-1;		
	}
	else if((opcode&0x60)==0) // R型指令
	{
		state[(clock+1)%5][2]=0;
		
		// CMPSUB.A指令无需写回，其他要写回，下同
		if(opcode!=0x15)
		{
			state[(clock+2)%5][3]=-1;
			state[(clock+3)%5][4]=0;
		}
		else
		{
			state[(clock+2)%5][3]=state[(clock+3)%5][4]=-1;
		}
	}
	else if((opcode&0x60)==0x20) // I型指令
	{
		state[(clock+1)%5][2]=1;
		if(opcode!=0x35)
		{
			state[(clock+2)%5][3]=-1;
			state[(clock+3)%5][4]=0;
		}
		else
		{
			state[(clock+2)%5][3]=state[(clock+3)%5][4]=-1;
		}
	}
	else if((opcode&0x40)==0x40) // 存取（ST/LD）指令
	{
		state[(clock+1)%5][2]=2;
		state[(clock+2)%5][3]=state[(clock+3)%5][4]=0;
	}
	return 0;
}

// 执行线程
DWORD WINAPI Execute(LPVOID lpParam)
{
	if(state[clock%5][2]==-1)
	{
		state[(clock+1)%5][3]=state[(clock+2)%5][4]=-1;
		return 0;
	}
	WaitForSingleObject(mutex_output,INFINITE);
	cout<<"执行 - "<<vasm[pctemp[clock%5]/4]<<endl;
	ReleaseSemaphore(mutex_output,1,NULL);
	WaitForSingleObject(mutex_cpi,INFINITE);
	cpiwithcache++;
	cpiwithoutcache++;
	ReleaseSemaphore(mutex_cpi,1,NULL);
	execins++;
	unsigned instruction=instructions[(clock+3)%5];
	unsigned opcode=(instruction>>24)&0xFF;
	regf.readreg1=(instruction>>19)&0x1F;
	regf.readreg2=instruction&0x1F;
	regf.writereg=(instruction>>14)&0x1F;
	
	// 对存取指令，计算数据存储器的地址
	if(state[clock%5][2]==3||state[clock%5][2]==4)
	{
		alu.srcA=1;
		alu.srcB[0]=0;
		alu.srcB[1]=1;
		alu.op[0]=alu.op[1]=0;
		alu.setflag=0;
		extender.sign=1;
		extender.in=instruction&0xFFFFFF;
		extender.extend(24);
		WaitForSingleObject(mutex_pc,INFINITE);
		unsigned tmppc=pc.pc;
		pc.pc=pctemp[clock%5]+4;
		alu.calculate(regf,pc,shifter,extender);
		pc.pc=tmppc;
		ReleaseSemaphore(mutex_pc,1,NULL);
	}
	
	/*
	  先读取寄存器堆
	  后进行数据冒险前递处理：
	    如果该周期需要的寄存器的值，是上一周期或上上周期写寄存器的值，
		则可能还未写回寄存器堆，这时需要从regwritetemp临时变量中读取数据
		而如果这种情况下，上一指令是LD指令，则在同一周期的访存操作结束后，
		才能获取数值，这时需要等待访存操作结束
	  另外，regwritetemp巧妙地解决了结构冒险的问题：
	    当同一周期的写回线程在写某一寄存器的值时，同时执行线程也需要读
		该寄存器的值，那么执行线程最终读取的数据来自于regwritetemp，而不是
		regf。这是因为在上上周期，该寄存器的值被修改，因此会被保存在regwritetemp中 
	*/
	regf.run();
	if(regwritetemp[(clock+3)%5][0]!=0&&regwritetemp[(clock+3)%5][1]==regf.readreg1)
	{
		regf.regA=regwritetemp[(clock+3)%5][2];
	}
	if(regwritetemp[(clock+3)%5][0]!=0&&regwritetemp[(clock+3)%5][1]==regf.readreg2)
	{
		regf.regB=regwritetemp[(clock+3)%5][2];
	}
	if(regwritetemp[(clock+3)%5][0]!=0&&regwritetemp[(clock+3)%5][1]==regf.writereg)
	{
		regf.regC=regwritetemp[(clock+3)%5][2];
	}	
	if(regwritetemp[(clock+4)%5][0]!=0&&regwritetemp[(clock+4)%5][1]==regf.readreg1)
	{
		while(regwritetemp[(clock+4)%5][0]!=1)
		{
			Sleep(10);
		}
		regf.regA=regwritetemp[(clock+4)%5][2];
	}
	if(regwritetemp[(clock+4)%5][0]!=0&&regwritetemp[(clock+4)%5][1]==regf.readreg2)
	{
		while(regwritetemp[(clock+4)%5][0]!=1)
		{
			Sleep(10);
		}
		regf.regB=regwritetemp[(clock+4)%5][2];
	}
	if(regwritetemp[(clock+4)%5][0]!=0&&regwritetemp[(clock+4)%5][1]==regf.writereg)
	{
		while(regwritetemp[(clock+4)%5][0]!=1)
		{
			Sleep(10);
		}
		regf.regC=regwritetemp[(clock+4)%5][2];
	}
	
	if(opcode==0x1A||opcode==0x3A)
	{
		regf.regA=0;
	}
	if(state[clock%5][2]==0)
	{
		ExecRIns(instruction);
	}
	else if(state[clock%5][2]==1)
	{
		ExecIIns(instruction);
	}
	else if(state[clock%5][2]==2)
	{
		ExecSL(instruction);
	}
	else if(state[clock%5][2]==3)
	{
		ExecBeq(instruction);
	}
	else if(state[clock%5][2]==4)
	{
		ExecB(instruction);
	}
	else
	{
		ExecJump(instruction);
	}
	wbtemp[clock%5]=regf.write;
	return 0;
}

// 执信R型指令
void ExecRIns(unsigned instruction)
{
	unsigned opcode=(instruction>>24)&0xFF;
	unsigned imm5=(instruction>>9)&0x1F;
	if(imm5!=0)
	{
		alu.srcA=0;
		alu.srcB[0]=1;
		alu.srcB[1]=0;
		shifter.right=0;
		shifter.in=regf.regB;
		shifter.shift(imm5);
		if(opcode==0x04||opcode==0x15)
		{
			shifter.out=(~shifter.out)+1;
		}
	}
	else
	{
		alu.srcA=0;
		alu.srcB[0]=alu.srcB[1]=0;
		if(opcode==0x04||opcode==0x15)
		{
			regf.regB=(~regf.regB)+1;
		}
	}
	if(opcode==0x08||opcode==0x04||opcode==0x15||opcode==0x1A)
	{
		alu.op[0]=alu.op[1]=0;
	}
	else if(opcode==0&&((instruction>>5)&1)==1)
	{
		alu.op[0]=1;
		alu.op[1]=0;
	}
	else if(opcode==0)
	{
		alu.op[0]=0;
		alu.op[1]=1;
	}
	else
	{
		alu.op[0]=alu.op[1]=1;
	}
	if(opcode==0x15)
	{
		alu.setflag=1;
		regwritetemp[clock%5][0]=0;
	}
	else
	{
		alu.setflag=0;
		regwritetemp[clock%5][0]=1;
		regwritetemp[clock%5][1]=regf.writereg;
		regf.src=0;
		regf.write=1;
	}
	alu.calculate(regf,pc,shifter,extender);
	regwritetemp[clock%5][2]=alu.out;
}

// 执行I型指令
void ExecIIns(unsigned instruction)
{
	unsigned opcode=(instruction>>24)&0xFF;
	unsigned imm5=(instruction>>9)&0x1F;
	unsigned imm9=instruction&0x1FF;
	extender.in=imm9;
	extender.sign=0;
	shifter.in=extender.extend(9);
	shifter.right=1;
	shifter.shift(imm5);
	if(opcode==0x24||opcode==0x35)
	{
		shifter.out=~shifter.out+1;
	}
	alu.srcA=0;
	alu.srcB[0]=1;
	alu.srcB[1]=0;
	if(opcode==0x28||opcode==0x24||opcode==0x35||opcode==0x3A)
	{
		alu.op[0]=alu.op[1]=0;
	}
	else if(opcode==0x20)
	{
		alu.op[0]=0;
		alu.op[1]=1;
	}
	else
	{
		alu.op[0]=alu.op[1]=1;
	}
	if(opcode==0x35)
	{
		alu.setflag=1;
		regwritetemp[clock%5][0]=0;
	}
	else
	{
		alu.setflag=0;
		regwritetemp[clock%5][0]=1;
		regwritetemp[clock%5][1]=regf.writereg;
		regf.src=0;
		regf.write=1;		
	}
	alu.calculate(regf,pc,shifter,extender);
	regwritetemp[clock%5][2]=alu.out;
}

// 执行存取指令
void ExecSL(unsigned instruction)
{
	unsigned opcode=(instruction>>24)&0xFF;
	unsigned imm5=(instruction>>9)&0x1F;
	unsigned imm14=instruction&0x3FFF;
	alu.setflag=0;
	alu.srcA=0;
	alu.op[0]=alu.op[1]=0;
	if((opcode&0x70)==0x50)
	{
		alu.srcB[0]=1;
		alu.srcB[1]=0;
		shifter.right=0;
		shifter.in=regf.regB;
		shifter.shift(imm5);
	}
	else
	{
		alu.srcB[0]=0;
		alu.srcB[1]=1;
		extender.sign=0;
		extender.in=imm14;
		extender.extend(14);
	}	
	cachetemp[clock%5][0]=alu.calculate(regf,pc,shifter,extender);
	if(!(opcode&1))
	{
		cachetemp[clock%5][1]=regf.regC;
		regf.write=0;
		regwritetemp[clock%5][0]=0;
	}
	else
	{
		regf.write=1;
		regwritetemp[clock%5][0]=2;
		regwritetemp[clock%5][1]=regf.writereg;
	}
}

// 执行BEQ指令
void ExecBeq(unsigned instruction)
{
	if(regf.flag.Z)
	{
		WaitForSingleObject(mutex_pc,INFINITE);
		pc.pcsrc[0]=1;
		pc.pcsrc[1]=0;
		pc.next(regf,adder,alu);
		isjumpins=true;
		ReleaseSemaphore(mutex_pc,1,NULL);
	}
	regwritetemp[clock%5][0]=0;
}

// 执行B指令
void ExecB(unsigned instruction)
{
	WaitForSingleObject(mutex_pc,INFINITE);
	pc.pcsrc[0]=1;
	pc.pcsrc[1]=0;
	pc.next(regf,adder,alu);
	isjumpins=true;
	ReleaseSemaphore(mutex_pc,1,NULL);
	regwritetemp[clock%5][0]=0;
}

// 执行JUMP指令
void ExecJump(unsigned instruction)
{
	WaitForSingleObject(mutex_pc,INFINITE);	
	pc.pcsrc[0]=0;
	pc.pcsrc[1]=1;
	pc.next(regf,adder,alu);
	isjumpins=true;
	ReleaseSemaphore(mutex_pc,1,NULL);
	regwritetemp[clock%5][0]=0;
}

// 访存线程
DWORD WINAPI VisitCache(LPVOID lpParam)
{
	if(state[clock%5][3]==-1)
	{
		return 0;
	}
	WaitForSingleObject(mutex_output,INFINITE);
	cout<<"访存 - "<<vasm[pctemp[(clock+4)%5]/4]<<endl;
	ReleaseSemaphore(mutex_output,1,NULL);
	WaitForSingleObject(mutex_cpi,INFINITE);
	cpiwithoutcache+=99;
	ReleaseSemaphore(mutex_cpi,1,NULL);
	unsigned instruction=instructions[(clock+2)%5];
	unsigned opcode=(instruction>>24)&0xFF;
	if(opcode&1)
	{
		cache.read=1;
		cache.write=0;
	} // opcode最低位为1则读存储器，否则写存储器
	else
	{
		cache.read=0;
		cache.write=1;
	}
	WaitForSingleObject(mutex_cpi,INFINITE);
	unsigned ret=cache.rw(1+3*(((instruction>>26)&1)==0),clock,cachetemp[(clock+4)%5][0],cachetemp[(clock+4)%5][1],cpiwithcache); // 读或写Cache
	ReleaseSemaphore(mutex_cpi,1,NULL);
	if(opcode&1)
	{
		regwritetemp[(clock+4)%5][2]=ret;
		regwritetemp[(clock+4)%5][0]=1;
	}
	return 0;
}

// 写回线程
DWORD WINAPI WriteBack(LPVOID lpParam)
{	
	if(state[clock%5][4]==-1||!wbtemp[(clock+3)%5])
	{
		return 0;
	}
	WaitForSingleObject(mutex_output,INFINITE);
	cout<<"写回 - "<<vasm[pctemp[(clock+3)%5]/4]<<endl;
	ReleaseSemaphore(mutex_output,1,NULL);
	regf.regs[regwritetemp[(clock+3)%5][1]]=regwritetemp[(clock+3)%5][2];
	return 0;
}

int main()
{
	FILE *fout=fopen("output.txt","w");
	*stdout=*fout;
	mutex_output=CreateSemaphore(NULL,1,1,NULL);
	InputInstructions();
	ParseBInstructions();
	LoadInstructions();
	vasm.push_back("FFFFFFFF");
	
	/*
	________________		________________		________________		________________		________________
	|Fetchins      |		|Decode        |		|Execute       |		|VisitMemory   |		|WriteBack     |		________________
	----------------		|Fetchins      |		|Decode        |		|Execute       |		|VisitMemory   |		|WriteBack     |		________________
	                		----------------		|Fetchins      |		|Decode        |		|Execute       |		|VisitMemory   |		|WriteBack     |		________________
	                		                		----------------		|Fetchins      |		|Decode        |		|Execute       |		|VisitMemory   |		|WriteBack     |		________________
	                		                		                		----------------		|Fetchins      |		|Decode        |		|Execute       |		|VisitMemory   |		|WriteBack     |		________________
	                		                		                		                		----------------		|Fetchins      |		|Decode        |		|Execute       |		|VisitMemory   |		|WriteBack     |
	                		                		                		                								----------------		----------------		----------------		----------------		----------------
	
	while循环的每次迭代过程执行的相当于是同一个框中的几个线程，while循环整体就是从左到右依次执行各个框
	*/ 
	while(state[clock%5][0]!=-1||state[clock%5][1]!=-1||state[clock%5][2]!=-1||state[clock%5][3]!=-1||state[clock%5][4]!=-1)
	{
		cout<<"\n\n=========================================================================\n";
		cout<<"时钟周期 : "<<clock<<endl;
		isjumpins=false;
		
		// 创建五个线程，再等待五个线程都结束后才进入下一周期
		HANDLE handle_getins=CreateThread(NULL,0,FetchInstructions,NULL,0,NULL);
		HANDLE handle_decode=CreateThread(NULL,0,Decode,NULL,0,NULL);
		HANDLE handle_exec=CreateThread(NULL,0,Execute,NULL,0,NULL);
		HANDLE handle_visitcache=CreateThread(NULL,0,VisitCache,NULL,0,NULL);
		HANDLE handle_wb=CreateThread(NULL,0,WriteBack,NULL,0,NULL);
		WaitForSingleObject(handle_getins,INFINITE);
		WaitForSingleObject(handle_decode,INFINITE);
		WaitForSingleObject(handle_exec,INFINITE);
		WaitForSingleObject(handle_visitcache,INFINITE);
		WaitForSingleObject(handle_wb,INFINITE);
		
		if(isjumpins)
		{
			state[(clock+1)%5][1]=state[(clock+1)%5][2]=-1; // 对于跳转指令，下一周期停止译码和执行 
			cout<<"当前周期执行的是跳转指令！\n";
		}
		cout<<"-------------------------------------------------------------------------\n";
		regf.Print();
		cache.Print();
		datamemory.Print();
		clock++;			
	}
	cache.Buf2Mem();
	cout<<"================最=========终=========结===========果====================\n";
	regf.Print();
	cache.Print();
	datamemory.Print();
	cout<<"CPI with Cache = "<<cpiwithcache/double(execins)<<endl;
	cout<<"CPI without Cache = "<<cpiwithoutcache/double(execins)<<endl;
	return 0;
}

/*
测试数据: 
MOV r0,21
MOV r2,2
LOOP: STW r0,[r2]
ADD r2,r2,4
SUB r0,r0,1
CMPSUB.A r0,1
BEQ OUT
B LOOP
OUT:
MOV r8,12
MOV r9,10
OR r10,r8,r9
AND r11,r8,r9
MUL r12,r10,r11
MOV r5,4
MOV r5,r5<<4
ADD r5,r5,8
JUMP r5
B LOOP
ADD r0,r0,2
LDB r13,[r0]
*/
