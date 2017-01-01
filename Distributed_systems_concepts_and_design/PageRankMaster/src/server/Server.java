package server;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.Semaphore;

public class Server extends Thread
{

	ServerSocket server = null;
	Socket sk = null;
	ServerThread th = null;
	BufferedReader rdr = null;
	PrintWriter wtr = null;
	int port = 1921;
	
	List<String> receivedatafrom[] = new ArrayList[2];
	int clientstate[] = {0, 0};
	boolean clientsolve[] = {false, false};
	
	int state = 0;	// 0表示啥都没做，1表示刚做完初始化工作（算出p矩阵），2往上表示迭代进度。与server的state对应

	public Server()
	{
		try
		{
			server = new ServerSocket(port);
		}
		catch (IOException e)
		{
			e.printStackTrace();
		}
		receivedatafrom[0] = new ArrayList<>();
		receivedatafrom[1] = new ArrayList<>();
	}

	public void run()
	{

		while (true)
		{
			System.out.printf("Listening on port %d...\n", port);
			try
            {
				// 每个请求交给一个线程去处理
                sk = server.accept();
                th = new ServerThread(sk);
                th.start();
				th.join(); // 同一时刻只能有一个线程占用socket。不加这条语句会出大问题
				sleep(20);

				// 两个客户端都得出结果，退出
				if (clientsolve[0] && clientsolve[1])
				{
					server.close();
					break;
				}
            }
            catch (Exception e)
            {
                e.printStackTrace();
            }
			
		}
	}

	public static void main(String [] args)
	{
		new Server().start();
	}

	class ServerThread extends Thread
	{

		Socket sk = null;
		public ServerThread(Socket sk)
		{
			this.sk = sk;
		}
		public void run()
		{
			try
            {
	            wtr = new PrintWriter(sk.getOutputStream());
	            rdr = new BufferedReader(new InputStreamReader(sk
				        .getInputStream()));
				String line = rdr.readLine();
				String split[] = line.split(" ");
				String st;
				int frommachine = Integer.parseInt(split[0]);
				String machinecmd = split[1];
				int machinestate = Integer.parseInt(split[2]);
				int receivedatasize = Integer.parseInt(split[3]);
				for (int i = 0; i < receivedatasize; i++) {
					st = rdr.readLine();
					receivedatafrom[frommachine].add(st);
				}
				System.out.println("从客户端来的信息：" + line);
				String reply = "";
				List<String> replydata = receivedatafrom[1-frommachine]; // 给客户端回信时，把另一客户端发来的数据也传过去

				// 服务器状态=max{客户端状态}
				// 如果某客户端状态等于服务器状态，则该客户端要做该状态对应的工作
				// 如果某客户端状态等于服务器状态+1，则该客户端已经做完了工作，在等待另一客户端完成工作
				if (machinecmd.equals("ready"))		// worker start
				{
					clientstate[frommachine] = machinestate;
					state = Math.min(clientstate[0], clientstate[1]);
					if (state == machinestate)
					{
						if (machinestate == 0) reply = "do 1";
						else reply = "do " + state;
					}
					else if (state < machinestate)
					{
						if (machinestate - state == 1) reply = "wait " + state;
						else if (state == 0 && machinestate > 2) {		// when all client start from a checkpoing
							state = machinestate;
							reply = "do " + state;
						} else {
							// if the client is ahead of current state by more than 2,
							// it should recover to previous state
							// (actually, only "machinestate - state == 2" is recoverable,
							// other situation will report error)
							reply = "recover " + state;
						}
					}
				}
				else if (machinecmd.equals("finish"))	// this state is finished
				{
					clientstate[frommachine] = machinestate;
					if (clientstate[0] == clientstate[1]) // 两个客户端都完成了同一阶段的工作才能继续
					{
						reply = "do " + machinestate;	// machinestate is already the next state
						state = machinestate;
					}
					else
					{
						if (machinestate - state == 1) reply = "wait " + state;
						else {
							// if the client is ahead of current state by more than 2,
							// it should recover to previous state
							// (actually, only "machinestate - state == 2" is recoverable,
							// other situation will report error)
							reply = "recover " + state;	// recover to previous state
						}
					}
				}
				else if (machinecmd.equals("solve"))	// converged
				{
					clientsolve[frommachine] = true;
					if (clientsolve[1-frommachine])
					{
						System.out.println("Solved!");
					}
					reply = "solve " + state;
				}

				// Data passing is needed only at the beginning of an epoch
				// Or the recovered client will receive the result of the other client of the next epoch
				if (clientstate[0] == clientstate[1]) {
					reply += " " + replydata.size();
					wtr.println(reply);
					wtr.flush();
					for (String str:replydata)
					{
						wtr.println(str);
						wtr.flush();
					}
					replydata.clear();
				} else {
					reply += " " + 0;
					wtr.println(reply);
					wtr.flush();
				}

				System.out.println("已经返回给客户端：" + reply);
				rdr.close();
				wtr.close();
				sk.close();
            }
            catch (IOException e)
            {
	            e.printStackTrace();
            }
			
		}
		
	}

}
