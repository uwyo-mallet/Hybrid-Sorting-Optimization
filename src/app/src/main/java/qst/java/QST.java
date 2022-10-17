package qst.java;

import picocli.CommandLine;

public class QST {

    public static void main(String[] args) {
        CommandLine cli = new CommandLine(new Benchmark());
        if (cli.isUsageHelpRequested()) {
            cli.printVersionHelp(System.out);
            System.exit(0);
        }
        if (cli.isVersionHelpRequested()) {
            cli.printVersionHelp(System.out);
            System.exit(0);
        }

        int exitCode = cli.execute(args);
        System.exit(exitCode);
    }
}
