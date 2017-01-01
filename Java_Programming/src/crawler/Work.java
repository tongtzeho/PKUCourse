/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package crawler;

import java.awt.Color;
import java.util.*;
import java.io.*;
import java.util.concurrent.*;
import java.util.regex.*;

/**
 *
 * @author 1100012773, 1100012778
 */

/**
 *  Work类
 *  网络爬虫的工作类，启动DownloadPage线程，并保存要连接的URL的队列
 */
public class Work extends Thread {

    boolean needstop; // 是否需要停止（由Crawler控制）
    
    /* 构造函数 */
    public Work() {
        needstop = false;
    }
    
    @Override
    public void run() {
        /* 开始工作时，更改三个按钮的颜色 */
        Crawler.startjb.setBackground(Color.LIGHT_GRAY);
        Crawler.stopjb.setBackground(null);
        Crawler.choosefilejb.setBackground(Color.LIGHT_GRAY);
        
        Set<String> set = new HashSet<String>(); // 记录连接的URL
        Queue<String> queue = new LinkedList<String>(); // 记录即将连接的URL
        
        /* 直接输入网址 */
        if(Crawler.inputurljrb.isSelected()) {
            queue.offer(Crawler.inputurljtf.getText());
        }
        
        /* 从txt文件中导入网址 */
        else {
            String importfilename = Crawler.importtxtjtf.getText();
            FileInputStream fis = null;
            InputStreamReader isr = null;
            BufferedReader br = null;
            try {
                String readstr;
                fis = new FileInputStream(importfilename);
                isr = new InputStreamReader(fis);
                br = new BufferedReader(isr);
                while ((readstr = br.readLine()) != null) {
                    queue.offer(readstr);
                }
            } catch(FileNotFoundException e) {
                System.out.println("找不到指定文件");
            } catch(IOException e) {
                System.out.println("读取文件失败");
            } finally {
                try {
                    if (br != null) br.close();
                    if (isr != null) isr.close();
                    if (fis != null) fis.close();
                } catch (IOException ex) {
                    System.out.println("关闭文件失败");
                }
            }
        }
        
        /* 判断是否需要代理服务器，若需要则读取代理服务器的地址和端口 */
        boolean useproxy = Crawler.useproxyjcb.isSelected();
        if (useproxy) {
            if (!Pattern.matches("(((25)[0-5][0-5])|(2[0-4][0-9])|(1[0-9][0-9])|([1-9]?[0-9]))\\.(((25)[0-5][0-5])|(2[0-4][0-9])|(1[0-9][0-9])|([1-9]?[0-9]))\\.(((25)[0-5][0-5])|(2[0-4][0-9])|(1[0-9][0-9])|([1-9]?[0-9]))\\.(((25)[0-5][0-5])|(2[0-4][0-9])|(1[0-9][0-9])|([1-9]?[0-9]))", Crawler.proxyaddrjtf.getText())) {
                useproxy = false;
                System.out.println("代理服务器地址格式错误");
            }
            else if (!Pattern.matches(("[0-9]{1,5}"), Crawler.proxyportjtf.getText())) {
                useproxy = false;
                System.out.println("代理服务器端口格式错误");
            }
        }
        
        /* 是否需要发送Cookie */
        boolean sendcookie = Crawler.sendcookiejcb.isSelected();
        
        /* 直到即将连接的URL为空才结束循环 */
        while (!queue.isEmpty()) {
            if (needstop) break;
            String str;
            int queuesize = queue.size();
            if (queuesize > 500) queuesize = 500; // 设置DownloadPage线程数最多为500
            Integer i;
            boolean b[] = new boolean[18];
            for (i = 0; i <= 17; i++)
                b[i] = Crawler.arrjcb[i].isSelected();
            CountDownLatch runningthreadnum = new CountDownLatch(queue.size());
            DownloadPage task[] = new DownloadPage[queuesize+1];
            for (i = 1; i <= queuesize; i++) {
                if (needstop) {
                    int j;
                    for (j = 1; j < i; j++)
                        if (task[j] != null) task[j].needstop = true;
                    break;
                }
                str = queue.poll();
                str = str.replace(" ", "");
                if (!str.equals("")) {
                    if (str.indexOf("://") == -1 || str.indexOf("://") > 12) str = "http://" + str; // 若输入的网址没有http://，则补上
                    set.add(str);
                    task[i] = new DownloadPage(str, i, b, runningthreadnum); // 新建任务
                    if (useproxy) task[i].getproxyinfo(Crawler.proxyaddrjtf.getText(), Crawler.proxyportjtf.getText()); // 需要的话设置代理
                    if (sendcookie) task[i].getcookiecontent(Crawler.cookiejtf.getText());
                    task[i].gettext(Crawler.userdefsourcejtf.getText(), Crawler.userdeftextjtf.getText(), Crawler.continueurljtf.getText(), Crawler.saveformatjtf.getText()); // 获取输入框内容
                    task[i].start(); // 任务开始
                }
                else {
                    runningthreadnum.countDown(); // 网址为空，没有新建任务，线程数自动减一
                }          
            }
            
            try {
                /* 等候全部DownloadPage线程结束 */
                while (runningthreadnum.getCount() > 0) {
                    sleep(200);
                    if (needstop) {
                        int j;
                        for (j = 1; j <= queuesize; j++)
                            if (task[j] != null) task[j].needstop = true;
                        sleep(1000);
                        break;
                    }
                }
            } catch (InterruptedException ex) {
                System.out.println("线程中断异常");
            }
            
            /* 若还需要爬新的网页，则加入队列中 */
            if (b[13]) {
                for (i = 1; i <= queuesize; i++) {
                    if (needstop) break;
                    String urlsavefile = "~"+i.toString()+"saveurls.txt";
                    File file = new File(urlsavefile);
                    if (file.exists()) {
                        FileInputStream fis = null;
                        InputStreamReader isr = null;
                        BufferedReader br = null;
                        try {
                            fis = new FileInputStream(file);
                            isr = new InputStreamReader(fis);
                            br = new BufferedReader(isr);
                            String line;
                            String regex = Crawler.continueurljtf.getText();
                            while ((line = br.readLine()) != null) {
                                boolean pass = false;
                                
                                /* 若在输入框中没有输入任何字符，视为继续爬任何网页，否则只爬符合那条正则表达式的内容 */
                                if (regex.equals("")) {
                                    pass = true;
                                }
                                else {
                                    pass = Pattern.matches(regex, line);
                                }
                                if (pass) {
                                    if (set.add(line)) {
                                        queue.offer(line);
                                    }
                                }
                            }
                        } catch (PatternSyntaxException ex) {
                            System.out.println("自定义正则表达式语法错误");
                        } catch (FileNotFoundException ex) {
                            System.out.println("临时保存url的文件不存在");
                        } catch (IOException ex) {
                            System.out.println("读取临时保存url的文件失败");
                        } finally {
                            try {
                                if (br != null) br.close();
                                if (isr != null) isr.close();
                                if (fis != null) fis.close();
                            } catch (IOException ex) {
                            System.out.println("关闭临时文件失败");
                            }
                        }
                        file.delete();
                    } // end of if(file.exists())
                } // end of for
            } // end of if(b[13])
        } // end of while
        
        /* 工作结束时，更改三个按钮的颜色 */
        Crawler.startjb.setBackground(null);
        Crawler.stopjb.setBackground(Color.LIGHT_GRAY);
        Crawler.choosefilejb.setBackground(null);
    }
    
}
