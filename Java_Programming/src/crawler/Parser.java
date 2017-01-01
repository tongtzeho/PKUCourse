/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package crawler;

import java.io.*;
import java.util.*;
import java.util.regex.*;

/**
 *
 * @author 1100012773, 1100012778
 */

/**
 *  Parser类
 *  功能：分析网页源文件，提取需要的信息并输出
 */
public class Parser {
    
    private String url; // 网页链接
    private String tempfilename; // 输出的临时文件名
    private String content; // 网页源代码
    private boolean need[]; // 对应DownloadPage的need
    
    /* 构造函数 */
    public Parser(String u, Integer tfi, String ct, boolean n[]) {
        url = u;
        tempfilename = "~"+tfi.toString()+"saveurls.txt";
        content = ct;
        int i;
        need = new boolean[18];
        for (i = 0; i < 17; i++)
            need[i] = n[i];
    }
    
    /* 分析源代码内容，提取需要的信息并输出 */
    public void parse(String userdefsource, String userdeftext, String continueurl, String saveformat) {
        String title = gettitle();
        try {
            String text = content.replace("\n","").replace("\r","").replaceAll("<(s|S)(c|C)(r|R)(i|I)(p|P)(t|T)[^>]*?>.*?</(s|S)(c|C)(r|R)(i|I)(p|P)(t|T)>", "").replaceAll("<(s|S)(t|T)(y|Y)(l|L)(e|E)[^>]*?>.*?</(s|S)(t|T)(y|Y)(l|L)(e|E)>","").replaceAll("<.*?>", "").replaceAll("\\&((nbsp)|(\\#12288)|(\\#160))(\\;)?", " ").replaceAll("\\&((lt)|(\\#60))(\\;)?","<").replaceAll("\\&((gt)|(\\#62))(\\;)?",">").replaceAll("\\&((quot)|(#34))(\\;)?","\"").replaceAll("\\&((apos)|(\\#39))(\\;)?","'").replaceAll("\\&copy(\\;)?","©").replaceAll("\\&reg(\\;)?","®").replaceAll("\\&((amp)|(#38))(\\;)?","&");
            
            /* regex[0]到regex[10]为URL、电子邮箱地址、ip地址、手机号码、电话号码、QQ号码、身份证号码、日期、时间、电驴链接、迅雷链接的正则表达式 */
            String regex[] = new String[18];
            regex[0] = new String("(http://|ftp://|https://|rstp://|telnet://|file://)([\\w-]+\\.)+[\\w-]+(/[\\w\\-\\_\\.\\/\\?\\%\\&\\=\\:\\+\\,]*)?");
            regex[1] = new String("[\\w]+([\\.\\_\\-]*[\\w])*\\@([\\w]+[\\w\\-]*[\\w]+\\.)+[\\w]+");
            regex[2] = new String("(((25)[0-5][0-5])|(2[0-4][0-9])|(1[0-9][0-9])|([1-9]?[0-9])|(\\*))\\.(((25)[0-5][0-5])|(2[0-4][0-9])|(1[0-9][0-9])|([1-9]?[0-9])|(\\*))\\.(((25)[0-5][0-5])|(2[0-4][0-9])|(1[0-9][0-9])|([1-9]?[0-9])|(\\*))\\.(((25)[0-5][0-5])|(2[0-4][0-9])|(1[0-9][0-9])|([1-9]?[0-9])|(\\*))");
            regex[3] = new String("((\\+)?86(\\-)?)?(1)(((3|5|8)[0-9])|(47))[0-9]{8}");
            regex[4] = new String("(((0?)|[1-9])(0?|[1-9])([0-9][0-9])\\-)?[1-9]([0-9]{6})[0-9]?((\\-)[0-9]{1,4})?");
            regex[5] = new String("[1-9][0-9]{4,9}");
            regex[6] = new String("(([1-5][1-9])|6[1-5]|(71)|(81)|(82))([0-9]{4})((18)|(19)|(20))([0-9]{2})((0[1-9])|(11)|(12))(([0-2][0-9])|30|31)[0-9]{3}([0-9]|x|X)");
            regex[7] = new String("([0-9]{2,4}(\\-)((0?[1-9])|(10)|(11)|(12))(\\-)(([1-2][0-9])|(30)|(31)|((0)?[1-9])))|([0-9]{2,4}(\\.)((0?[1-9])|(10)|(11)|(12))(\\.)(([1-2][0-9])|(30)|(31)|((0)?[1-9])))|([0-9]{2,4}(\\/)((0?[1-9])|(10)|(11)|(12))(\\/)(([1-2][0-9])|(30)|(31)|((0)?[1-9])))|((([1-2][0-9])|(30)|(31)|((0)?[1-9]))\\-((0?[1-9])|(10)|(11)|(12))\\-([0-9]{2,4}))|((([1-2][0-9])|(30)|(31)|((0)?[1-9]))\\.((0?[1-9])|(10)|(11)|(12))\\.([0-9]{2,4}))|((([1-2][0-9])|(30)|(31)|((0)?[1-9]))\\/((0?[1-9])|(10)|(11)|(12))\\/([0-9]{2,4}))|(((0?[1-9])|(10)|(11)|(12))\\-(([1-2][0-9])|(30)|(31)|((0)?[1-9]))\\-([0-9]{2,4}))|(((0?[1-9])|(10)|(11)|(12))\\.(([1-2][0-9])|(30)|(31)|((0)?[1-9]))\\.([0-9]{2,4}))|(((0?[1-9])|(10)|(11)|(12))\\/(([1-2][0-9])|(30)|(31)|((0)?[1-9]))\\/([0-9]{2,4}))");
            regex[8] = new String("(((1[0-9])|(2[0-3])|(0?[0-9]))\\:([0-5][0-9])(\\:[0-5][0-9])?)|(24\\:00(\\:00)?)");
            regex[9] = new String("ed2k://\\|file\\|[\\w\\-\\%\\(\\)\\[\\]\\.\\!]*[\\w]+\\|[0-9]+\\|[0-9A-F]+\\|((p|h)\\=[0-9A-Z]+(\\|)?)?(\\/)?");
            regex[10] = new String("thunder://[\\w\\+\\/\\=]+");
            regex[11] = userdefsource; // 用户自定义的正则表达式1
            regex[12] = userdeftext; // 用户自定义的正则表达式2
            regex[13] = new String("(http://|ftp://|https://|rstp://|telnet://|file://)([\\w-]+\\.)+[\\w-]+(/[\\w\\-\\_\\.\\/\\?\\%\\&\\=\\:\\+\\,]*)?"); // 同为URL正则表达式
            
            /* 输出文件的前缀以及保存的文件夹的名称 */
            String filetitleprefix[] = new String[18];
            filetitleprefix[0] = new String("URL");
            filetitleprefix[1] = new String("电子邮箱地址");
            filetitleprefix[2] = new String("ip地址");
            filetitleprefix[3] = new String("手机号码");
            filetitleprefix[4] = new String("电话号码");
            filetitleprefix[5] = new String("QQ号码");
            filetitleprefix[6] = new String("身份证号码");
            filetitleprefix[7] = new String("日期");
            filetitleprefix[8] = new String("时间");
            filetitleprefix[9] = new String("电驴链接");
            filetitleprefix[10] = new String("迅雷链接");
            filetitleprefix[11] = new String("自定义从源代码提取");
            filetitleprefix[12] = new String("自定义在正文中提取");
            
            int i;
            for (i = 0; i <= 13; i++) {
                if (need[i]) {
                    Pattern pattern = Pattern.compile(regex[i], Pattern.CASE_INSENSITIVE);
                    String matchstr;
                    if ((i >= 3 && i <= 6) || (i >= 9 && i <= 10) || (i == 12)) matchstr = text; // 手机号码、电话号码、QQ号码、身份证号码、电驴链接、迅雷链接、自定义1在正文中提取
                    else matchstr = content; // 其余从源代码中提取
                    Matcher matcher = pattern.matcher(matchstr);
                    Set<String> set = new HashSet<String>();
                    while (matcher.find()) {
                        String newitem = matchstr.substring(matcher.start(), matcher.end());
                        set.add(newitem);                   
                    }
                    
                    /* 额外分析形如"<a href="的相对链接 */
                    if (i == 0 || i == 13) {
                        Pattern patternrelativelink = Pattern.compile("\\<a\\shref\\=\"\\/[\\w\\-\\_\\.\\/\\?\\%\\&\\=\\:\\+\\,]*", Pattern.CASE_INSENSITIVE);
                        Matcher matcherrelativelink = patternrelativelink.matcher(content);
                        while (matcherrelativelink.find()) {
                            String newitem = content.substring(matcherrelativelink.start(), matcherrelativelink.end());
                            int slashpos = newitem.indexOf("/");
                            newitem = newitem.substring(slashpos);
                            slashpos = url.indexOf("://");
                            slashpos = url.indexOf("/", slashpos+5);
                            if (slashpos == -1) newitem = url+newitem;
                            else newitem = url.substring(0, slashpos)+newitem;
                            set.add(newitem);
                        }
                    }
                    
                    if (i != 13) new File(filetitleprefix[i]).mkdir();
                    File file;
                    if (i != 13) file = new File(filetitleprefix[i]+"\\"+SameFileName.newfilename(filetitleprefix[i]+"\\", filetitleprefix[i]+" - "+title+".txt"));
                    else file = new File(tempfilename);
                    
                    /* 输出提取的结果(i!=13)或临时文件(i==13) */
                    OutputStream os = null;
                    FileOutputStream fos = new FileOutputStream(file, false);
                    Writer out = new OutputStreamWriter(fos, "UTF-8");
                    Iterator<String> iter = set.iterator();
                    while (iter.hasNext()) {
                        out.write(iter.next()+"\r\n");
                    }
                    out.close();
                    fos.close();
                }
            }
            
        } catch (PatternSyntaxException ex) {
            System.out.println("自定义正则表达式语法错误");
        } catch (FileNotFoundException ex) {
            System.out.println("输出失败");
        } catch (UnsupportedEncodingException ex) {
            System.out.println("无法识别的编码方式");
        } catch (IOException ex) {
            System.out.println("输出失败");
        }
    }
    
    /* 从源代码的<title>...</title>段中获取网页标题 */
    public String gettitle() {
        String contentwithoutline = content;
        contentwithoutline = contentwithoutline.replace("\n", "").replace("\r", "").replaceAll("[\\s]+"," ");
        String regex = "<title>[\\s\\S]*?</title>";
        String title = "";
        Pattern pattern = Pattern.compile(regex, Pattern.CANON_EQ);
        Matcher matcher = pattern.matcher(contentwithoutline);
        if (matcher.find()) {
            title = contentwithoutline.substring(matcher.start(), matcher.end()).replaceAll("<.*?>", "").replace(":", "").replace("<","").replace(">","").replace("?","").replace("|","").replace("*","").replace("/","").replace("\\","").replace("\"", "");
            if (title.startsWith(" ")) title = title.substring(1);
            return title;
        }
        return "";
    }
    
}
