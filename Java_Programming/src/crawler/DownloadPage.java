/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package crawler;

import java.io.*;
import java.net.*;
import java.util.*;
import java.util.concurrent.*;
import java.util.regex.*;

/**
 *
 * @author 1100012773, 1100012778
 */

/**
 *  DownloadPage类
 *  功能：给定URL等信息，连接互联网下载信息，交由Parser类提取数据，如果用户有需要则保存相应内容
 */
public class DownloadPage extends Thread {
    private Integer tempfileid; // 临时文件对应的序号
    private String url; // 网页链接
    private String tempfilename; // 临时文件名
    private String objectname; // 目标名称（如果URL对应的是网页则取网页标题，否则直接从URL中提取）
    private String userdefsource; // 用户自定义的正则表达式（对应从源代码中匹配）
    private String userdeftext; // 用户自定义的正则表达式（对应从正文中匹配）
    private String continueurl; // 要继续爬的URL的正则表达式
    private String saveformat; // 要存储的文件格式
    private boolean useproxy = false, sendcookie = false; // 是否使用代理服务器、是否发送Cookie
    private String proxyaddr; // 代理地址
    private String cookie; // Cookie内容
    private int proxyport; // 代理端口
    private boolean need[]; // 对应18个复选框的真值
    private boolean error = false; // 是否出错
    private CountDownLatch runningthreadnum; // 当前线程数
    boolean needstop; // 是否需要停止（由Work控制）
    
    /* 构造函数 */
    public DownloadPage(String u, Integer tfi, boolean n[], CountDownLatch rtn) {
        super();
        url = u;
        tempfileid = tfi;
        tempfilename = "~tmp"+tfi.toString();
        need = new boolean[18];
        int i;
        for (i = 0 ;i <= 17; i++)
            need[i] = n[i];
        runningthreadnum = rtn;
        needstop = false;
    }
    
    /* 获取代理服务器信息 */
    public void getproxyinfo(String paddr, String pp) {
        useproxy = true;
        proxyaddr = paddr;
        proxyport = Integer.valueOf(pp);
    }
    
    /* 获取Cookie内容 */
    public void getcookiecontent(String cookie) {
        sendcookie = true;
        this.cookie = cookie;
    }
    
    
    /* 获取输入框内容 */
    public void gettext(String userdefsource, String userdeftext, String continueurl, String saveformat) {
        this.userdefsource = userdefsource;
        this.userdeftext = userdeftext;
        this.continueurl = continueurl;
        this.saveformat = saveformat;
    }
    
    @Override
    public void run() {
        try {
            System.out.println("准备连接 - "+url);
            String charcode = null;
            if (!needstop) charcode = getcharcodefromurl(); // 分析网页编码类型
            String content = null;
            if (!needstop) content = getcontentfromurl(charcode); // 获取网页内容
            if (!needstop) outputtotempfile(content, charcode); // 将网页内容输出到临时文件中
            if (!error && !needstop) {
                Parser parser = new Parser(url, tempfileid, content, need);
                String title = parser.gettitle(); // 获取网页标题
                if (title.equals("")) System.out.println("连接成功 - "+url);
                else System.out.println("连接成功 - "+title);
                parser.parse(userdefsource, userdeftext, continueurl, saveformat); // 分析网页内容并提取需要的数据
                if (title.equals("")) System.out.println("提取信息成功 - "+url);
                else System.out.println("提取信息成功 - "+title);
                
                /* 若需要保存目标…… */
                if (need[14] && !needstop) {
                    /* 该目标为网页且有非空标题 */
                    if (!title.equals("")) {
                        File file = new File(tempfilename);
                        if (file.exists()) {
                            new File("下载\\目标").mkdirs();
                            System.out.println("目标保存为 - "+SameFileName.newfilename("下载\\目标\\", title+".html"));
                            file.renameTo(new File("下载\\目标\\"+SameFileName.newfilename("下载\\目标\\", title+".html"))); // 将之前的临时文件重命名即可
                        }
                    }
                    /* 该目标不是网页或是网页但没有非空标题 */
                    else {
                        objectname = SameFileName.newfilename("下载\\目标\\", getobjname(url));
                        new File("下载\\目标").mkdirs();
                        System.out.println("目标保存为 - "+objectname);
                        downloadbybyte(url, "下载\\目标\\"+objectname); // 重新下载（因为之前的下载很可能会丢失部分特殊字符的数据）
                    }
                }
                
                /* 若需要保存网页正文…… */
                if (need[15] && !needstop) {
                    String textname = null;
                    if (!title.equals("")) {
                        textname = title+".txt";
                    }
                    else {
                        textname = getobjname(url)+".txt";
                    }
                    new File("下载\\网页正文").mkdirs();
                    textname = SameFileName.newfilename(("下载\\网页正文\\"), textname);
                    System.out.println("网页正文保存为 - "+textname);
                    outputtext(content, charcode, "下载\\网页正文\\"+textname); // 将网页正文输出到指定的文件
                }
                
                /* 若需要下载网页中特定格式的文件…… */
                if (need[16] || need[17]) {
                    Set<String> filesuffixset = new HashSet<String>();
                    /* 需要下载图片 */
                    if (need[16]) {
                        filesuffixset.add(".jpg");
                        filesuffixset.add(".gif");
                        filesuffixset.add(".png");
                        filesuffixset.add(".jpeg");
                        filesuffixset.add(".bmp");
                    }
                    /* 需要下载自定义格式的文件 */
                    if (need[17]) {
                        Pattern pattern = Pattern.compile("\\.(\\w|\\-|\\_)+", Pattern.CASE_INSENSITIVE);
                        Matcher matcher = pattern.matcher(saveformat);
                        while (matcher.find()) {
                            filesuffixset.add(saveformat.substring(matcher.start(), matcher.end()).toLowerCase());
                        }
                    }
                    if (!filesuffixset.isEmpty() && !needstop) downloadfile(content, "下载 - "+title, filesuffixset); // 下载文件
                }
                
            }
        } catch (MalformedURLException ex) {
            System.out.println("url格式不对 - "+url); // url格式不对
            error = true;
        } catch (IOException ex) {
            System.out.println("网络连接异常 - "+url); // 网络连接异常
            error = true;
        }

        File file = new File(tempfilename);
        if (file.exists()) file.delete(); // 删除临时文件
        runningthreadnum.countDown(); // 线程数减一
    }
    
    /* 获取网页编码类型 */
    private String getcharcodefromurl() {
        try {
            /* 若需要使用代理服务器则设置代理 */
            SocketAddress add = null;
            if (useproxy) add = new InetSocketAddress(proxyaddr, proxyport); 
            Proxy proxy = null;
            if (useproxy) proxy = new Proxy(Proxy.Type.HTTP , add);
            
            /* 连接网络，获取网页头信息 */
            URL u = new URL(url);
            HttpURLConnection urlconnection = null;
            if (useproxy) urlconnection = (HttpURLConnection)u.openConnection(proxy);
            else urlconnection = (HttpURLConnection)u.openConnection();
            
            if (sendcookie) urlconnection.setRequestProperty("Cookie", cookie);
            urlconnection.connect();
            
            String charcode = null;
            
            /* 分析网页头信息 */
            Map<String, List<String>> map = urlconnection.getHeaderFields();   
            Set<String> keys = map.keySet();   
            Iterator<String> iterator = keys.iterator();
        
            String key = null;   
            String tmp = null;   
            while (iterator.hasNext()) {
                if (needstop) return "UTF-8";
                key = iterator.next();   
                tmp = map.get(key).toString().toLowerCase();   
                
                /* 若网页头中含有"Content-Type"项且含有"charset="字段，则提取信息并返回 */
                if (key != null && key.equals("Content-Type")) {   
                    int m = tmp.indexOf("charset=");   
                    if (m != -1) {   
                        charcode = tmp.substring(m + 8).replace("]", "");   
                        return charcode;   
                    }   
                }   
            }
        
            if (needstop) return "UTF-8";
            
            /* 重新连接，逐行提取网页源代码，再从源代码中寻找字符编码信息 */
            HttpURLConnection conn = null;
            if (useproxy) conn = (HttpURLConnection)(new URL(url).openConnection(proxy));
            else conn = (HttpURLConnection)(new URL(url).openConnection());
            
            if (sendcookie) conn.setRequestProperty("Cookie", cookie);
            conn.connect();
            
            BufferedReader reader = new BufferedReader(new InputStreamReader(conn.getInputStream()));
            StringBuilder sb = new StringBuilder();
            String line;
            
            while ((line = reader.readLine()) != null) {
                if (needstop) return "UTF-8";
                line = line.toLowerCase();
                /* 在读取的字符串中寻找"charset="字段 */
                int indexofcharset = line.indexOf("charset=");
                if (indexofcharset > 0) {
                    line = line.substring(indexofcharset);
                    int indexofquotation = line.indexOf("\"");
                    if (indexofquotation > 0) {
                        return line.substring(8, indexofquotation);
                    }
                }
            }
        } catch (MalformedURLException ex) {}
          catch (IOException ex) {}

        return "UTF-8"; // 默认是UTF-8编码
    }
    
    /* 获取网页内容 */
    private String getcontentfromurl(String charcode) {
        try {
            /* 若需要使用代理服务器则先设置代理 */
            SocketAddress add = null;
            if (useproxy) add = new InetSocketAddress(proxyaddr, proxyport); 
            Proxy proxy = null;
            if (useproxy) proxy = new Proxy(Proxy.Type.HTTP , add);
            
            /* 连接网络 */
            HttpURLConnection conn = null;
            if (useproxy) conn = (HttpURLConnection)(new URL(url).openConnection(proxy));
            else conn = (HttpURLConnection)(new URL(url).openConnection());
            
            if (sendcookie) conn.setRequestProperty("Cookie", cookie);
            conn.connect();
            
            InputStream is = conn.getInputStream();
            if (needstop) return "";
            String content = readfromstream(is, charcode); // 在InputStream中获取内容
            return content;
        } catch(MalformedURLException e) {
            System.out.println("url格式不对 - "+url);
            error = true;
        } catch (IOException ex) {
            System.out.println("网络连接异常 - "+url);
            error = true;
        }
        return "";
    }
    
    /* 从网页给的输入流中获取内容并保存在String中 */
    private String readfromstream(InputStream stream, String charcode) throws IOException {
        try {
            BufferedReader reader = new BufferedReader(new InputStreamReader(stream, charcode));
            StringBuilder sb = new StringBuilder();
            String line;
            
            /* 逐行读取数据 */
            while ((line = reader.readLine()) != null) {
                if (needstop) return "";
                sb.append(line+"\r\n");
            }
            return sb.toString();
        } catch (UnsupportedEncodingException ex) {
            System.out.println("无法识别的编码方式 - "+url);
            error = true;
        }
        return "";
    }
    
    /* 将网页内容输出到临时文件 */
    private void outputtotempfile(String content, String charcode) {
        File file = new File(tempfilename);
        FileOutputStream fos = null;
        Writer out = null;
        try {
            fos = new FileOutputStream(file, false);
            out = new OutputStreamWriter(fos, charcode);
            out.write(content);
        } catch (FileNotFoundException ex) {
            System.out.println("临时文件名出错");
            error = true;
        } catch (IOException ex) {
            System.out.println("临时文件输出失败");
            error = true;
        } finally {
            try {
                if (out != null) out.close();
                if (fos != null) fos.close();
            } catch (IOException ex) {
                System.out.println("无法关闭临时文件");
            }
        }
    }
    
    /* 逐字节地下载网页目标 */
    private boolean downloadbybyte(String url, String savefile) throws MalformedURLException {
        boolean succeed = true;
        URL u = new URL(url);
        DataInputStream dis = null;
        FileOutputStream fos = null;
        try {
            if (needstop) return false;
            /* 设置代理、连网等同上 */
            SocketAddress add = null;
            if (useproxy) add = new InetSocketAddress(proxyaddr, proxyport); 
            Proxy proxy = null;
            if (useproxy) proxy = new Proxy(Proxy.Type.HTTP , add);
            HttpURLConnection conn = null;
            if (useproxy) conn = (HttpURLConnection)(new URL(url).openConnection(proxy));
            else conn = (HttpURLConnection)(new URL(url).openConnection());
            
            if (sendcookie) conn.setRequestProperty("Cookie", cookie);
            conn.connect();
            
            dis = new DataInputStream(conn.getInputStream());
            if (needstop) return false;
            fos = new FileOutputStream(new File(savefile));
            byte buffer[] = new byte[65536];
            int length;
            
            /* 每次固定读若干字节的内容 */
            while ((length = dis.read(buffer)) > 0) {
                fos.write(buffer, 0, length);
                if (needstop) return false;
            }
        } catch (IOException ex) {
            System.out.println("下载失败 - "+savefile.substring(savefile.lastIndexOf("\\")+1));
            succeed = false;
        } finally {
            try {
                if (dis != null) dis.close();
                if (fos != null) fos.close();
            } catch (IOException ex) {
                System.out.println("关闭下载的文件失败");
            }
        }
        return succeed;
    }
    
    /* 根据URL获取目标名称 */
    private String getobjname(String url) {
        if (needstop) return url;
        String destfile = new String(url);
        if (destfile.endsWith("/")) destfile = destfile.substring(0, destfile.length() - 1);
        int lastslashpos = destfile.lastIndexOf("/");
        destfile = destfile.substring(lastslashpos + 1);
        int lastdotpos = destfile.lastIndexOf(".");
        if (lastdotpos > 0) {
            int tmpindex = destfile.indexOf("?", lastdotpos);
            if (tmpindex > 0) destfile = destfile.substring(0, tmpindex);
            if ((tmpindex = destfile.indexOf("%", lastdotpos)) > 0) destfile = destfile.substring(0, tmpindex);
            if ((tmpindex = destfile.indexOf("&", lastdotpos)) > 0) destfile = destfile.substring(0, tmpindex);
            if ((tmpindex = destfile.indexOf("=", lastdotpos)) > 0) destfile = destfile.substring(0, tmpindex);
            if ((tmpindex = destfile.indexOf("+", lastdotpos)) > 0) destfile = destfile.substring(0, tmpindex);
            if ((tmpindex = destfile.indexOf(":", lastdotpos)) > 0) destfile = destfile.substring(0, tmpindex);          
        }
        destfile = destfile.replace(":", "").replace("<","").replace(">","").replace("?","").replace("|","").replace("*","").replace("/","").replace("\\","").replace("\"", "");
        if (destfile.length() > 127) destfile = destfile.substring(destfile.length() - 127);
        return destfile;
    }
    
    /* 输入网页正文 */
    private void outputtext(String content, String charcode, String savefile) {
        if (needstop) return;
        String text = new String(content);
        /* 删去所有js脚本、html标签，并将&***;变为原有字符 */
        text = text.replaceAll("<(s|S)(c|C)(r|R)(i|I)(p|P)(t|T)[^>]*?>[\\s\\S]*?</(s|S)(c|C)(r|R)(i|I)(p|P)(t|T)>", "").replaceAll("<(s|S)(t|T)(y|Y)(l|L)(e|E)[^>]*?>[\\s\\S]*?</(s|S)(t|T)(y|Y)(l|L)(e|E)>","").replaceAll("<br>","\r\n").replaceAll("<[\\s\\S]*?>", "").replaceAll("(\r\n)+","\r\n").replaceAll("(\\s)+"," ").replaceAll("\\&((nbsp)|(\\#12288)|(\\#160))(\\;)?", " ").replaceAll("\\&((lt)|(\\#60))(\\;)?","<").replaceAll("\\&((gt)|(\\#62))(\\;)?",">").replaceAll("\\&((quot)|(#34))(\\;)?","\"").replaceAll("\\&((apos)|(\\#39))(\\;)?","'").replaceAll("\\&copy(\\;)?","©").replaceAll("\\&reg(\\;)?","®").replaceAll("\\&((amp)|(#38))(\\;)?","&");
        File file = new File(savefile);
        FileOutputStream fos = null;
        Writer out = null;
        try {
            fos = new FileOutputStream(file, false);
            out = new OutputStreamWriter(fos, charcode);
            if (needstop) return;
            out.write(text);
        } catch (FileNotFoundException ex) {
            System.out.println("网页正文文件名出错");
            error = true;
        } catch (IOException ex) {
            System.out.println("网页正文输出失败");
            error = true;
        } finally {
            try {
                if (out != null) out.close();
                if (fos != null) fos.close();
            } catch (IOException ex) {
                System.out.println("无法关闭网页正文文件");
            }
        }        
    }
    
    /* 下载指定格式的文件 */
    void downloadfile(String content, String dir, Set<String> suffixset) throws MalformedURLException, IOException {
        Set<String> downloaded = new HashSet<String>();
        Pattern pattern = Pattern.compile("(http://|ftp://|https://|rstp://|telnet://|file://)([\\w-]+\\.)+[\\w-]+(/[\\w\\-\\_\\.\\/\\?\\%\\&\\=\\:\\+\\,]*)?", Pattern.CASE_INSENSITIVE);
        Matcher matcher = pattern.matcher(content);
        
        /* 从网页源码中提取指定格式的URL */
        while (matcher.find()) {
            if (needstop) return;
            String url = content.substring(matcher.start(), matcher.end());
            String destfile = getobjname(url);
            destfile = SameFileName.newfilename("下载\\"+dir+"\\", destfile);
            int lastdotpos = destfile.lastIndexOf(".");
            if (lastdotpos > 0) {
                if (suffixset.contains(destfile.substring(lastdotpos).toLowerCase()) && downloaded.add(url.toLowerCase())) {
                    new File("下载\\"+dir).mkdirs();
                    if (downloadbybyte(url, "下载\\"+dir+"\\"+destfile)) System.out.println("下载成功 - "+destfile); /* 下载该URL */
                }
            }
        }
    }
    
}
