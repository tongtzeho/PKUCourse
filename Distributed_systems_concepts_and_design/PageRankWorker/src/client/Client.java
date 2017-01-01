package client;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.InterfaceAddress;
import java.net.Socket;
import java.util.ArrayList;
import java.util.List;
import java.util.stream.Stream;

public class Client extends Thread
{
	static boolean loadFromCheckpoint = false;		// load checkpoint or not (used for recovery)
	//	static String masterIP = "192.168.1.3";
	static String masterIp = "localhost";
	String ckptPath = "ckpt.txt";
	String oldCkptPath = "ckpt_old.txt";	// checkpoint of the former state

	String reply;
	List<String> senddata = new ArrayList<>();
	List<String> replydata = new ArrayList<>();
	Integer clientstate = 0; // 0表示啥都没做，1表示刚做完初始化工作（算出p矩阵），2往上表示迭代进度。与server的state对应
	Integer machineid = 0; // 机器号，从MachineId.txt读取
	boolean replyfinish = false; // server端是否应答完成，如果没有应答完成，则等待其应答完成

	int maxv = 8300; // wiki-vote的顶点数（标号最大的是8297，实际上是有七千多个点，就当做有8300个点计算了）
	double eps = 0.000001;	// threshold to judge convergence


	double rank[] = new double[maxv]; // PageRank值
	double oldrank[] = new double[maxv];
	int outdegree[] = new int[maxv]; // 出度
	int indegree[] = new int[maxv]; // 入度
	List<Integer> edgeto[] = new ArrayList[maxv]; // 边（edgeto[终点]={起点}）

	// 从有向图文件中得到出入度和边
	void getr0fromdirectedgraph() {
		File file = new File("DirectedGraph.txt");
		BufferedReader reader = null;
		try {
			reader = new BufferedReader(new FileReader(file));
			String line = null;
			while ((line = reader.readLine()) != null) {
				String split[] = line.split("\t");
				int from = Integer.parseInt(split[0]);
				int to = Integer.parseInt(split[1]);
				outdegree[from]++;
				indegree[to]++;
				if (to % 2 == machineid) {
					edgeto[to].add(from); // 本地
				}
			}
			reader.close();
		} catch (IOException e) {
			e.printStackTrace();
		} finally {
			if (reader != null) {
				try {
					reader.close();
				} catch (IOException e1) {
					e1.printStackTrace();
				}
			}
		}
	}

	// 使rank之和等于顶点数（否则不会收敛），并返回是否已经收敛
	boolean adjustandcheckrank() {
		double ranksum = 0;
		for (int i = 0; i < maxv; i++) {
			ranksum += rank[i];
		}
		boolean result = true;
		for (int i = 0; i < maxv; i++) {
			rank[i] *= (maxv/ranksum);
			if (Math.abs(rank[i]-oldrank[i]) > eps) {
				result = false;
			}
			oldrank[i] = rank[i];
		}
		return result;
	}

	// 计算rank值。rank[i]=0.85*(求和：若j到i有边，rank[j]/出度[j])+0.15
	double[] getrank() {
		double newrank[] = new double[maxv];
		for (int i = 0; i < maxv; i++) {
			if (i % 2 == machineid) {
				for (int j:edgeto[i]) {
					newrank[i] += 0.85*rank[j]/(1.0*outdegree[j]);
				}
				newrank[i] += 0.15;
			}
		}
		return newrank;
	}

	/**
	 * Load checkpoint
	 * @param inpath checkpoint path
	 * @return true if successfully loaded, false if error occurs (e.g. incomplete checkpoint)
	 */
	boolean loadCheckpoint(String inpath) {
		senddata.clear();
		replydata.clear();
		File file = new File(inpath);
		BufferedReader reader = null;
		boolean completeCkpt = false;
		try {
			reader = new BufferedReader(new FileReader(file));
			String line = reader.readLine().trim();	// first line is state line
			clientstate = Integer.parseInt(line.split(" ")[1]);
			if (clientstate == 0) return true;

			getr0fromdirectedgraph();	// load graph structure
			boolean readingOldRank = false;
			while ((line = reader.readLine()) != null) {
				if (line.split(" ").length <= 1) {	// empty lines or flags
					if (line.trim().equals("old_rank:")) // start of oldrank records
						readingOldRank = true;
					if (line.trim().equals("end"))	// end of checkpoint, indicating the checkpoint is complete
						completeCkpt = true;
					continue;
				}
				int id = Integer.parseInt(line.split(" ")[0]);
				double value = Double.parseDouble(line.split(" ")[1]);
				if (readingOldRank)
					oldrank[id] = value;
				else
					rank[id] = value;
			}
			reader.close();
		} catch (IOException e) {
			e.printStackTrace();
		} finally {
			if (reader != null) {
				try {
					reader.close();
				} catch (IOException e1) {
					e1.printStackTrace();
				}
			}
		}
		return completeCkpt;
	}

	void saveCheckpoint() {
		System.out.println("saving checkpoint...");
		FileWriter fr = null;
		try {
			// Two checkpoint is used, the old checkpoint is used when current checkpoint is not complete,
			// and when it needs to recover to previous state (when another client just recovered to a previous state)
			Util.fileCopy(new File(ckptPath), new File(oldCkptPath));
			fr = new FileWriter(ckptPath, false);
			fr.write(String.format("state %d\n", clientstate));
			if (clientstate >= 2) {
				fr.write("rank:\n");
				for (int i = 0; i < maxv; i++) {
					fr.write(i+" "+rank[i]+"\n");
				}
				fr.write("old_rank:\n");
				for (int i = 0; i < maxv; i++) {
					fr.write(i+" "+oldrank[i]+"\n");
				}
			}
			fr.write("end\n");
		} catch (Exception e) {
			e.printStackTrace();
		} finally {
			if (fr != null) {
				try {
					fr.close();
				} catch (IOException e1) {
					e1.printStackTrace();
				}
			}
		}
		System.out.println("checkpoint is saved");
	}

	public Client()
	{
		// 从MachineId.txt读取machineid
		File file = new File("MachineId.txt");
		BufferedReader reader = null;
		try {
			reader = new BufferedReader(new FileReader(file));
			String line = null;
			line = reader.readLine();
			if (line.startsWith("0")) machineid = 0;
			else if (line.startsWith("1")) machineid = 1;
			reader.close();
		} catch (IOException e) {
			e.printStackTrace();
		} finally {
			if (reader != null) {
				try {
					reader.close();
				} catch (IOException e1) {
					e1.printStackTrace();
				}
			}
		}
		for (int i = 0; i < maxv; i++) {
			edgeto[i] = new ArrayList<>();
			rank[i] = 1;
		}
	}

	public void run()
	{
		try {
			if (loadFromCheckpoint) {
				// If the current checkpoint is corrupted, use the previous one
				boolean ckptLoaded = loadCheckpoint(ckptPath);
				if (!ckptLoaded) ckptLoaded = loadCheckpoint(oldCkptPath);
				if (!ckptLoaded) {
					System.err.println("Can not load checkpoint!");
					System.exit(1);
				}
			} else {
				// clear checkpoint if start from scratch
				saveCheckpoint();
			}

			new ClientThread(machineid+" ready "+clientstate+" "+senddata.size()).start(); // 每次开客户端都发ready信息给服务器
			while (!replyfinish) {
				sleep(20);
			}

			String replycmd = reply.split(" ")[0];
			int replystate = Integer.parseInt(reply.split(" ")[1]);

			while (replystate >= 0) {
				replystate = Integer.parseInt(reply.split(" ")[1]);

				// 服务器把另一个客户端得到的rank值传过来，接收
				for (String str:replydata) {
					String type = str.split(" ")[0];
					int id = Integer.parseInt(str.split(" ")[1]);
					double value = Double.parseDouble(str.split(" ")[2]);
					if (type.equals("rank")) {
						rank[id] = value;
					}
				}
				replydata.clear();

				// When another client recovers to a previous state, this client should also recover to that state
				if (replycmd.equals("recover")) {
					loadCheckpoint(ckptPath);
					if (clientstate != replystate) loadCheckpoint(oldCkptPath);
					if (clientstate != replystate) {
						System.err.printf("Can not recovery to state %d\n", clientstate);
						System.exit(1);
					}
				}

				saveCheckpoint();	// save checkpoint at the beginning of each epoch

				// 服务器命令客户端工作或者刚恢复到一次迭代的起点
				if (replycmd.equals("do") || replycmd.equals("recover")) {
					if (replystate == 1) {
						getr0fromdirectedgraph();
					} else {
						boolean solved = adjustandcheckrank();
						// 已收敛，输出结果
						if (solved) {
							FileWriter fr = new FileWriter("Result.txt", false);
							fr.write("PageRank:\n");
							for (int i = 0; i < maxv; i++) {
								fr.write(i+" "+rank[i]+"\n");
							}
							fr.write("InDegree:\n");
							for (int i = 0; i < maxv; i++) {
								fr.write(i+" "+indegree[i]+"\n");
							}
							fr.close();
							new ClientThread(machineid+" solve "+clientstate+" "+senddata.size()).start();
							System.out.println("Solved!");
							break;
						}
						// 不收敛，继续算
						double newrank[] = getrank();

						for (int i = 0; i < maxv; i++) {
							if (i % 2 == machineid) {
								senddata.add("rank "+i+" "+newrank[i]);
								rank[i] = newrank[i]; // 自己算的那一半结果
							}
						}
					}
					clientstate++;
				}

				// 算完了，给服务器发finish。当两个客户端都算完后，服务器会给大家发do下一个工作。否则服务器会给先做完的客户端发wait
				do
				{
					// finished clientstate-1, going to do clientstate
					new ClientThread(machineid+" finish "+clientstate+" "+senddata.size()).start();
					while (!replyfinish) {
						sleep(100);		// 20
					}
					sleep(200);		// 50
					replycmd = reply.split(" ")[0];
				}
				while (replycmd.equals("wait"));
			}

		} catch (InterruptedException | IOException e) {
			e.printStackTrace();
		}
	}

	public static void main(String [] args)
	{
		if (args.length == 0) {
			System.out.println("Usage:\n\t" +
					"parameter_1: master IP\n\t" +
					"parameter_2: load from checkpoint or not (true/false, default false)");
			System.exit(1);
		}
		masterIp = args[0];
		if (args.length == 2) {
			switch (args[1].toLowerCase()) {
				case "false": loadFromCheckpoint = false; break;
				case "true": loadFromCheckpoint = true; break;
				default:
					System.out.printf("Unrecognized value for loadFromCheckpoint: %s\n", args[0]);
					System.exit(1);
			}
		}
		new Client().start();
	}

	class ClientThread extends Thread
	{
		Socket sk = null;
		BufferedReader reader = null;
		PrintWriter wtr = null;
		String msg = null;

		public ClientThread(String msg)
		{
			replyfinish = false;
			this.msg = msg;
			System.out.println("准备发送的消息：" + this.msg);
			try
			{
				sk = new Socket(masterIp, 1921); // Master的地址和端口
			}
			catch (Exception e)
			{
				e.printStackTrace();
			}
		}

		public void run()
		{
			try
			{
				reader = new BufferedReader(new InputStreamReader(sk.getInputStream()));
				wtr = new PrintWriter(sk.getOutputStream());
				wtr.println(msg);
				wtr.flush();
				for (String str:senddata) {
					wtr.println(str);
					wtr.flush();
				}
				senddata.clear();
				reply = reader.readLine();
				String str;
				int replydatasize = Integer.parseInt(reply.split(" ")[2]);
				for (int i = 0; i < replydatasize; i++) {
					str = reader.readLine();
					replydata.add(str);
				}
				wtr.close();
				reader.close();
				sk.close();
				System.out.println("从服务器来的消息："+reply);
				System.out.println("client thread terminates.\n");
				replyfinish = true;
			}
			catch (Exception e)
			{
				e.printStackTrace();
			}
		}

	}
}
