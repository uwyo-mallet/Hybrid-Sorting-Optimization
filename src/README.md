# QST-Java

Java frontend for
[quicksort-tuning](https://github.com/uwyo-mallet/quicksort-tuning). This allows
easily benchmarking Java sorting algorithms, utilizing existing plotting and
data analysis scripts.

## Usage

1. Install JDK 17.

2. Compile the executable, with GNU make.

```sh
$ make
```

3. Run the executable.

```sh
$ ./QST-Java
Usage: QST-Java [-m=<method>] [-o=FILE] [-r=N] [-t=N] [-c=<cols>]...
                [-v=<vals>]... (-V | -h | --show-methods=TYPE | <inFile>)
Java frontend for the quicksort-tuning project.
      <inFile>              Input datafile
  -c, --cols=<cols>         Column to pass through to the CSV.
  -h, --help                Print help and exit.
  -m, --method=<method>     Sorting Method
                              Default: DualPivotQuicksort_Lib
  -o, --output=FILE         Output to FILE instead of STDOUT.
  -r, --runs=N              Number of times to sort the same data.
                              Default: 1
      --show-methods=TYPE   Print all supported methods, or of type 'TYPE'.
  -t, --threshold=N         Threshold to switch*
                              Default: 4
  -v, --vals=<vals>         Value to pass through to the CSV.
  -V, --version             Print version and exit.
Report bugs to <jarulsam@uwyo.edu>
```

> This `Makefile` is purely a for convenience. It generates a JAR through
> gradle, then adds the system dependent shebang to generate a single
> executable. This is for easy integration with the rest of the QST project.
> _Beware_, the resulting binary is not distributable. Alternatively, you can
> manually build with `./gradlew shadowJar`, and call the resulting JAR with
> `java -jar app/build/libs/QST-Java-VERSION-all.jar`. This JAR is platform
> agnostic.

## Docs

Reference the docs at the primary project
[here](https://github.com/uwyo-mallet/quicksort-tuning).
