/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package crawler;

import java.awt.*;
import java.awt.event.*;
import java.io.*;
import javax.swing.*;

/**
 *  程序名：Crawler
 *  作者：---------------------------------
 *  编译环境：Microsoft Windows 7(64-bit)下的NetBeans IDE 7.3
 *  源文件：Crawler.java, DownloadPage.java, Parser.java, SameFileName.java, TxtFileFilter.java, Work.java
 *  功能：
 *  1.多线程地连接互联网，获取页面源代码，在工作中随时可以停止或退出
 *  2.通过正则表达式匹配，根据用户的选择可提取URL、电子邮箱、QQ号码、日期、电驴链接等信息
 *  3.用户可自定义正则表达式，从页面源代码或在正文中提取信息
 *  4.用户可自定义URL的正则表达式，当页面含有匹配的URL时，继续连接并提取信息
 *  5.允许下载网页或URL指向的文件（如exe、mp3等）
 *  6.获取网页正文（去掉源代码中的html标签和js脚本等）
 *  7.下载并保存网页中的图片或网页中含有的自定义格式的文件
 *  8.通过设置代理服务器连接互联网
 *  9.请求网页时发送给定的Cookie
 */

/**
 *  主类：Crawler
 *  功能：画界面，捕捉按键
 */
public class Crawler extends JApplet {

    static JFrame frame; // 界面
    static JTextField inputurljtf, importtxtjtf, proxyaddrjtf, proxyportjtf; // 输入网址、导入文件、代理地址、代理端口的输入框
    static JTextField userdefsourcejtf, userdeftextjtf, continueurljtf, saveformatjtf, cookiejtf; // 自定义正则表达式（两个）、继续搜索的网址、存储格式、Cookie的输入框
    static ButtonGroup bgrp; // 两个单选框的组
    static JRadioButton inputurljrb, importtxtjrb; // 输入网址和导入文件的单选框
    static JButton choosefilejb, startjb, stopjb; // 浏览文件、开始、停止按钮
    static JCheckBox arrjcb[], useproxyjcb, sendcookiejcb; // 18个功能选项、是否使用代理、是否发送Cookie的复选框
    static JLabel proxyaddrjlb, proxyportjlb; // "地址"和"端口"提示标语
    static Work work = null; // 启动的工作
    
    @Override
    public void init() {
        frame = new JFrame("Crawler");
        frame.setSize(406, 446); // 窗体大小
	frame.setLocation(350, 130); // 窗体初始位置
        frame.setResizable(false); // 不可以改变大小
        
        /* 点击窗口右上角的×时退出程序 */
        frame.addWindowListener(new WindowAdapter() {
            @Override
            public void windowClosing(WindowEvent arg0) {
                deletetempfile();
                System.out.println("退出");
                System.exit(0);
            }
        });
        
        /* 设置输入网址的输入框 */
        inputurljtf=new JTextField("", 8192);
        inputurljtf.setSize(286, 20);
        inputurljtf.setLocation(110, 10);
        inputurljtf.setBackground(Color.WHITE);
        frame.add(inputurljtf);
        
        /* 设置导入文件的输入框 */
        importtxtjtf=new JTextField("", 8192);
        importtxtjtf.setSize(205, 20);
        importtxtjtf.setLocation(110, 40);
        importtxtjtf.setBackground(Color.WHITE);
        frame.add(importtxtjtf);
        
        /* 设置两个单选框和组 */
        bgrp = new ButtonGroup();
        inputurljrb = new JRadioButton("输入网页地址 ", true);
        importtxtjrb = new JRadioButton("从txt导入网址", false);
        bgrp.add(inputurljrb);
        bgrp.add(importtxtjrb);
        inputurljrb.setSize(110, 20);
        importtxtjrb.setSize(110, 20);
        inputurljrb.setLocation(0, 10);
        importtxtjrb.setLocation(0, 40);
        frame.add(inputurljrb);
        frame.add(importtxtjrb);
        
        /* 设置浏览按钮 */
        choosefilejb = new JButton("浏览...");
        choosefilejb.setSize(75, 20);
        choosefilejb.setLocation(320, 40);
        frame.add(choosefilejb);
        
        arrjcb = new JCheckBox[18];
        
        /* 设置提取URL的复选框 */
        arrjcb[0] = new JCheckBox("提取URL");
        arrjcb[0].setSize(92, 20);
        arrjcb[0].setLocation(0, 70);
        
        /* 设置提取电子邮箱地址的复选框 */
        arrjcb[1] = new JCheckBox("提取电子邮箱地址");
        arrjcb[1].setSize(132, 20);
        arrjcb[1].setLocation(135,70);
        
        /* 设置提取ip地址的复选框 */
        arrjcb[2] = new JCheckBox("提取ip地址");
        arrjcb[2].setSize(98, 20);
        arrjcb[2].setLocation(270, 70);
        
        /* 设置提取手机号码的复选框 */
        arrjcb[3] = new JCheckBox("提取手机号码");
        arrjcb[3].setSize(113, 20);
        arrjcb[3].setLocation(0, 95);
        
        /* 设置提取电话号码的复选框 */
        arrjcb[4] = new JCheckBox("提取电话号码");
        arrjcb[4].setSize(113, 20);
        arrjcb[4].setLocation(135, 95);
        
        /* 设置提取QQ号码的复选框 */
        arrjcb[5] = new JCheckBox("提取QQ号码");
        arrjcb[5].setSize(99, 20);
        arrjcb[5].setLocation(270, 95);
        
        /* 设置提取身份证号码的复选框 */
        arrjcb[6] = new JCheckBox("提取身份证号码");
        arrjcb[6].setSize(119, 20);
        arrjcb[6].setLocation(0, 120);
        
        /* 设置提取日期的复选框 */
        arrjcb[7] = new JCheckBox("提取日期");
        arrjcb[7].setSize(93, 20);
        arrjcb[7].setLocation(135, 120);
        
        /* 设置提取时间的复选框 */
        arrjcb[8] = new JCheckBox("提取时间");
        arrjcb[8].setSize(93, 20);
        arrjcb[8].setLocation(270, 120);
        
        /* 设置提取电驴链接的复选框 */
        arrjcb[9] = new JCheckBox("提取电驴链接(ed2k://...)");
        arrjcb[9].setSize(176, 20);
        arrjcb[9].setLocation(0, 145);
        
        /* 设置提取迅雷链接的复选框 */
        arrjcb[10] = new JCheckBox("提取迅雷链接(thunder://...)");
        arrjcb[10].setSize(184, 20);
        arrjcb[10].setLocation(186, 145);
        
        /* 设置从源代码提取自定义正则表达式内容的复选框和对应的输入框 */
        arrjcb[11] = new JCheckBox("从源代码提取以下内容");
        arrjcb[11].setSize(160, 20);
        arrjcb[11].setLocation(0, 175);
        userdefsourcejtf = new JTextField("", 8192);
        userdefsourcejtf.setSize(234, 20);
        userdefsourcejtf.setLocation(162, 175);
        frame.add(userdefsourcejtf);
        
        /* 设置从正文提取自定义正则表达式内容的复选框和对应的输入框 */
        arrjcb[12] = new JCheckBox("在正文中提取以下内容");
        arrjcb[12].setSize(160, 20);
        arrjcb[12].setLocation(0, 205);
        userdeftextjtf = new JTextField("", 8192);
        userdeftextjtf.setSize(234, 20);
        userdeftextjtf.setLocation(162, 205);
        frame.add(userdeftextjtf);
        
        /* 设置继续爬网页的复选框和对应的输入框 */
        arrjcb[13] = new JCheckBox("继续爬以下网页");
        arrjcb[13].setSize(119, 20);
        arrjcb[13].setLocation(0, 235);
        continueurljtf = new JTextField("", 8192);
        continueurljtf.setSize(275, 20);
        continueurljtf.setLocation(121, 235);
        frame.add(continueurljtf);
        
        /* 设置保存目标的复选框 */
        arrjcb[14] = new JCheckBox("保存目标");
        arrjcb[14].setSize(93, 20);
        arrjcb[14].setLocation(0, 265);
        
        /* 设置保存网页正文的复选框 */
        arrjcb[15] = new JCheckBox("保存网页正文");
        arrjcb[15].setSize(113, 20);
        arrjcb[15].setLocation(135, 265);
        
        /* 设置下载网页图片的复选框 */
        arrjcb[16] = new JCheckBox("下载网页图片");
        arrjcb[16].setSize(113, 20);
        arrjcb[16].setLocation(270, 265);
        
        /* 设置下载指定格式文件的复选框和对应的输入框 */
        arrjcb[17] = new JCheckBox("下载以下格式的文件");
        arrjcb[17].setSize(142, 20);
        arrjcb[17].setLocation(0, 295);
        saveformatjtf = new JTextField("", 8192);
        saveformatjtf.setSize(252, 20);
        saveformatjtf.setLocation(144, 295);
        frame.add(saveformatjtf);
        
        /* 设置以上复选框的字体并显示 */
        int i;
        for(i = 0; i <= 17; i++) {
            arrjcb[i].setFont(new Font(Font.DIALOG, Font.PLAIN, 13));
            frame.add(arrjcb[i]);
        }
        
        /* 设置使用代理服务器的复选框 */
        useproxyjcb = new JCheckBox("使用代理服务器");
        useproxyjcb.setSize(119, 20);
        useproxyjcb.setLocation(0, 325);
        useproxyjcb.setFont(new Font(Font.DIALOG, Font.PLAIN, 13));
        frame.add(useproxyjcb);
        
        /* 设置代理地址的提示 */
        proxyaddrjlb = new JLabel("地址:");
        proxyaddrjlb.setSize(33, 20);
        proxyaddrjlb.setLocation(139, 325);
        proxyaddrjlb.setFont(new Font(Font.DIALOG, Font.PLAIN, 13));
        frame.add(proxyaddrjlb);
        
        /* 设置代理地址的输入框 */
        proxyaddrjtf = new JTextField("", 256);
        proxyaddrjtf.setSize(108, 20);
        proxyaddrjtf.setLocation(173, 325);
        frame.add(proxyaddrjtf);
        
        /* 设置代理端口的提示 */
        proxyportjlb = new JLabel("端口:");
        proxyportjlb.setSize(33, 20);
        proxyportjlb.setLocation(306, 325);
        proxyportjlb.setFont(new Font(Font.DIALOG, Font.PLAIN, 13));
        frame.add(proxyportjlb);
        
        /* 设置代理端口的输入框 */
        proxyportjtf = new JTextField("", 16);
        proxyportjtf.setSize(56, 20);
        proxyportjtf.setLocation(340, 325);
        frame.add(proxyportjtf);
        
        /* 设置发送Cookie的复选框 */
        sendcookiejcb = new JCheckBox("发送Cookie");
        sendcookiejcb.setSize(97, 20);
        sendcookiejcb.setLocation(0, 355);
        sendcookiejcb.setFont(new Font(Font.DIALOG, Font.PLAIN, 13));
        frame.add(sendcookiejcb);
        
        /* 设置Cookie的输入框 */
        cookiejtf = new JTextField("", 8192);
        cookiejtf.setSize(295, 20);
        cookiejtf.setLocation(101, 355);
        frame.add(cookiejtf);
        
        /* 设置开始按钮 */
        startjb = new JButton("开 始 (Enter)");
        startjb.setSize(190, 23);
        startjb.setLocation(4, 386);
        frame.add(startjb);
        
        /* 设置停止按钮 */
        stopjb = new JButton("停 止");
        stopjb.setSize(190, 23);
        stopjb.setLocation(205, 386);
        stopjb.setBackground(Color.LIGHT_GRAY);
        frame.add(stopjb);
        
        frame.setLayout(null); // 取消默认布局管理器
    }
    
    @Override
    public void start() {
        
        /* 点击"浏览"按钮时的响应 */
        ActionListener choosefileal = new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent ae) {
                /* 若已开始工作则不响应 */
                if (work == null || !work.isAlive()) {
                    importtxtjrb.setSelected(true);
                    
                    /* 弹出选择文件的对话框 */
                    JFileChooser jfc = new JFileChooser (".");
                    jfc.setAcceptAllFileFilterUsed(false);
                    jfc.addChoosableFileFilter(new TxtFileFilter());
                    int result = jfc.showOpenDialog(null);
                    if(result == JFileChooser.APPROVE_OPTION) {
                        String path = jfc.getSelectedFile().getAbsolutePath();
                        importtxtjtf.setText(path);
                    }
                }
            }       
        };       
        choosefilejb.addActionListener(choosefileal);
        
        final Crawler crawler = this;
        
        /* 点击"开始"按钮时的响应 */
        ActionListener startal = new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent ae) {
                /* 若已开始工作则不响应 */
                if (work == null || !work.isAlive()) {
                    System.out.println("开始");
                    work = new Work(); // 启动一项新工作
                    work.start();
                }
            }
        };
        startjb.addActionListener(startal);
        
        /* 点击停止按钮时的响应 */
        ActionListener stopal = new ActionListener() {
            @Override
            public void actionPerformed(ActionEvent ae) {
                /* 若当前没有工作则不响应 */
                if (work != null && work.isAlive()) {
                    work.needstop = true; // 依次将所有子线程的needstop设为true，使各子线程尽快终止
                    System.out.println("准备停止...");
                    deletetempfile(); // 删除临时文件
                    while (work.isAlive()) {}
                    System.out.println("已停止");
                }
            }
        };
        stopjb.addActionListener(stopal);
        
        /* 在任意一个输入框按回车键视为点击"开始"按钮 */
        addenterlistener(inputurljtf);
        addenterlistener(importtxtjtf);
        addenterlistener(userdefsourcejtf);
        addenterlistener(userdeftextjtf);
        addenterlistener(continueurljtf);
        addenterlistener(saveformatjtf);    
        addenterlistener(proxyaddrjtf);
        addenterlistener(proxyportjtf);
        addenterlistener(cookiejtf);
              
        frame.setVisible(true); // 将界面显示
        
    }
    
    void addenterlistener(JTextField jtf) {            
        jtf.addKeyListener(new KeyAdapter() {
            @Override
            public void keyPressed(KeyEvent event) 
            { 
                if (event.getKeyText(event.getKeyCode()).compareToIgnoreCase("Enter")==0) { 
                    startjb.doClick(); // 模拟点击"开始"按钮
                } 
            }
        });
    }
    
    /* 删除所有临时文件 */
    void deletetempfile() {
        Integer i;
        for (i = 1; i <= 500; i++) {
            new File("~tmp"+i.toString()).delete();
            new File("~"+i.toString()+"saveurls.txt").delete();
        }
    }
    
    /* 主函数 */
    public static void main(String[] args) {
        JApplet applet = new Crawler();
        System.out.println("欢迎使用java版网络爬虫Crawler");
        applet.init();
        applet.start();
    }
}
