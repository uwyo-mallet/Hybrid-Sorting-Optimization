package qst.java;

import java.io.*;
import java.nio.Buffer;
import java.util.ArrayList;
import java.util.zip.GZIPInputStream;

import com.google.common.io.Files;
import org.apache.commons.io.FilenameUtils;

public class Input {

    File inFile;
    ArrayList<Integer> data;

    Input(File in) throws IOException {
        inFile = in;
        data = new ArrayList<>();

        InputStream fp = new FileInputStream(inFile);

        BufferedReader reader;
        if (Files.getFileExtension(inFile.toString()).equals("gz")) {
            GZIPInputStream gzipStream = new GZIPInputStream(fp);
            InputStreamReader inputStream = new InputStreamReader(gzipStream);
            reader = new BufferedReader(inputStream);
        } else {
            reader = new BufferedReader(new InputStreamReader(fp));
        }

        String line;
        while ((line = reader.readLine()) != null) {
            data.add(Integer.parseInt(line));
        }

        reader.close();
        fp.close();
    }

    Integer[] getData() {
        Integer buffer[] = new Integer[data.size()];
        data.toArray(buffer);
        return buffer;
    }

    Integer size() {
        return this.data.size();
    }
}
