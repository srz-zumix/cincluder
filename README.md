# cincluder
c/c++ include analysis and make dot file


## Usage

```
USAGE: cincluder.exe [options] <source0> [... <sourceN>]

OPTIONS:

Generic Options:

  -help                      - Display available options (-help-hidden for more)
  -help-list                 - Display list of available options (-help-list-hidden for more)
  -version                   - Display the version of this program

cincluder:

  -dot=<string>              - output dot file
  -extra-arg=<string>        - Additional argument to append to the compiler command line
  -extra-arg-before=<string> - Additional argument to prepend to the compiler command line
  -ignore-system             - ignore system include file
  -p=<string>                - Build path
  -report-redundant          - report redundant include file
```

