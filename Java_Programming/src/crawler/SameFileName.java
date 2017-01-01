/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package crawler;

import java.io.*;
/**
 *
 * @author 1100012773, 1100012778
 */

/**
 *  SameFileName类
 *  若某个目录下已存在某文件，更改新文件的文件名（如a.txt->a[2].txt，ab[3]->ab[4]）
 */
public class SameFileName {
    
    public static String newfilename(String dir, String oldfilename) {
        String filename = oldfilename;
        File file = new File(dir+oldfilename);
        if (!file.exists()) return filename;
        int lastdotpos = oldfilename.lastIndexOf(".");
        Integer index = 1;
        if (lastdotpos == -1) {
            while (true) {
                index++;
                filename = oldfilename+"["+index.toString()+"]";
                if (!new File(dir+filename).exists()) return filename;
            }
        }
        else {
            String suffix = oldfilename.substring(lastdotpos);
            while (true) {
                index++;
                filename = oldfilename.substring(0, lastdotpos)+"["+index.toString()+"]"+suffix;
                if (!new File(dir+filename).exists()) return filename;
            }
        }
    }
    
}
