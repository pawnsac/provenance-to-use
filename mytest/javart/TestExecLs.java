import java.io.*;  
public class TestExecLs {  
    public static void main(String[] args) {  
        try {  
            Process p = Runtime.getRuntime().exec("/bin/ls .");  
            BufferedReader in = new BufferedReader(  
                                new InputStreamReader(p.getInputStream()));  
            String line = null;  
            while ((line = in.readLine()) != null) {  
                System.out.println(line);  
            }  
        } catch (IOException e) {  
            e.printStackTrace();  
        }  
    }  
}
