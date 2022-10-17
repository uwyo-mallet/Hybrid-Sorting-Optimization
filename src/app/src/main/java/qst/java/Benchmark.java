package qst.java;

import picocli.CommandLine;

import java.io.*;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.concurrent.Callable;
import java.util.concurrent.TimeUnit;

import com.google.common.base.Stopwatch;

import org.apache.commons.lang3.ArrayUtils;

enum METHOD_TYPE {
    threshold, nonthreshold, all
}

enum METHOD {
    DualPartitionQuicksort,
}

public class Benchmark implements Callable<Integer> {

    final static METHOD[] thresholdMethods = {METHOD.DualPartitionQuicksort};
    final static METHOD[] nonthresholdMethods = {};
    final static METHOD[] methods = ArrayUtils.addAll(thresholdMethods, nonthresholdMethods);

    @CommandLine.Option(names = {"-V", "--version"}, description = "Print version and exit.", versionHelp = true)
    boolean versionRequested;

    @CommandLine.Option(names = {"-h", "--help"}, description = "Print help and exit.", usageHelp = true)
    boolean helpRequested;

    @CommandLine.Option(names = {"-c", "--col"}, description = "Column to pass through to the CSV.")
    String[] cols = {};

    @CommandLine.Option(names = {"-v", "--val"}, description = "Value to pass through to the CSV.")
    String[] vals = {};

    @CommandLine.Option(names = {"-m", "--method"}, description = "Sorting Method", defaultValue = "DualPartitionQuicksort", showDefaultValue = CommandLine.Help.Visibility.ALWAYS)
    METHOD method;

    @CommandLine.Option(names = {"-o", "--output"}, paramLabel = "FILE", description = "Output to FILE instead of STDOUT.")
    File outFile;

    @CommandLine.Option(names = {"-r", "--runs"}, paramLabel = "N", description = "Number of times to sort the same data.", defaultValue = "1", showDefaultValue = CommandLine.Help.Visibility.ALWAYS)
    int runs;

    @CommandLine.Option(names = {"--show-methods"}, paramLabel = "TYPE", description = "Print all supported methods, or of type 'TYPE'.")
    METHOD_TYPE showType;

    @CommandLine.Option(names = {"-t", "--threshold"}, paramLabel = "N", description = "Threshold to switch*", defaultValue = "4", showDefaultValue = CommandLine.Help.Visibility.ALWAYS)
    int threshold;

    @CommandLine.Parameters(description = "Input datafile")
    File inFile;

    Integer size;

    private void showMethods(METHOD_TYPE type) {
        switch (type) {
            case threshold:
                for (METHOD i : thresholdMethods) {
                    System.out.println(i.name());
                }
                break;
            case nonthreshold:
                for (METHOD i : nonthresholdMethods) {
                    System.out.println(i.name());
                }
                break;
            default:
                for (METHOD i : methods) {
                    System.out.println(i.name());
                }
                break;
        }
    }

    private Long time(Integer[] input) {
        Stopwatch stopwatch = Stopwatch.createUnstarted();
        if (method == METHOD.DualPartitionQuicksort) {
            stopwatch.start();
            Arrays.sort(input);
            stopwatch.stop();
        }

        return stopwatch.elapsed(TimeUnit.NANOSECONDS);
    }

    private void writeResults(ArrayList<Long> times) throws IOException {
        OutputStream outputStream = null;
        Writer writer;

        if (outFile == null) {
            writer = new BufferedWriter(new OutputStreamWriter(System.out));
            writer.write("method,input,size,threshold,wall_nsecs,user_nsecs,system_nsecs");
            for (String i : cols) {
                writer.write(String.format(",%s", i));
            }
            writer.write("\n");
        } else {
            boolean writeHeader = !outFile.exists();

            outputStream = new FileOutputStream(outFile, true);
            writer = new BufferedWriter(new OutputStreamWriter(outputStream));
            if (writeHeader) {

                writer.write("method,input,size,threshold,wall_nsecs,user_nsecs,system_nsecs");
                for (String i : cols) {
                    writer.write(String.format(",%s", i));
                }
                writer.write("\n");
            }
        }

        for (Long time : times) {
            writer.write(String.format("%s,%s,%d,%d,%d,%d,%d", method.name(), inFile.getAbsolutePath(), size, threshold, time, 0, 0));
            for (String v : vals) {
                writer.write(String.format(",%s", v));
            }
            writer.write("\n");
        }

        writer.flush();
        if (outputStream != null) {
            outputStream.close();
            writer.close();
        }
    }

    @Override
    public Integer call() throws Exception {
        if (showType != null) {
            showMethods(showType);
            return 0;
        }
        if (cols.length != vals.length) {
            throw new RuntimeException("Number of columns and values do not match!");
        }
        final Input data = new Input(inFile);
        size = data.size();

        ArrayList<Long> times = new ArrayList<Long>();
        for (Integer i = 0; i < runs; i++) {
            long duration = time(data.getData());
            times.add(duration);
        }

        writeResults(times);
        return 0;
    }
}
