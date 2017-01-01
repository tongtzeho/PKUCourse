package client;


import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.nio.ByteBuffer;
import java.nio.channels.FileChannel;
import java.util.Date;

public class Util {
    /**
     * File copy function
     * @param srcFile source file
     * @param dstFile destination file
     * @return  milliseconds used
     * @throws Exception
     */
    public static long fileCopy(File srcFile, File dstFile) throws Exception{
        long time = new Date().getTime();
        int length = 2097152;
        FileInputStream in = new FileInputStream(srcFile);
        FileOutputStream out = new FileOutputStream(dstFile);
        FileChannel inC = in.getChannel();
        FileChannel outC = out.getChannel();
        ByteBuffer b = null;
        while(true){
            if(inC.position() == inC.size()){
                inC.close();
                outC.close();
                return new Date().getTime() - time;
            }
            if((inC.size()-inC.position())<length){
                length = (int)(inC.size()-inC.position());
            }else
                length = 2097152;
            b = ByteBuffer.allocateDirect(length);
            inC.read(b);
            b.flip();
            outC.write(b);
            outC.force(false);
        }
    }
}
