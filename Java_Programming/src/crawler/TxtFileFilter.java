/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package crawler;

import java.io.File;
import javax.swing.filechooser.*;

/**
 *
 * @author 1100012773, 1100012778
 */

/**
 *  TxtFileFilter类
 *  文件筛选器，只用于点击"浏览"按钮弹出的选择文件的对话框
 */
class TxtFileFilter extends FileFilter {
    
    @Override
    public boolean accept(File f) {
        if(f.isDirectory()) {
            return true; // 显示文件夹
        }
        String nameString = f.getName();
        return nameString.toLowerCase().endsWith(".txt"); // 显示txt文件
    }
    
    @Override
    public String getDescription() {
        return "文本文件 (*.txt)"; // 类型提示
    }
}
