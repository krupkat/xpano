# Contribution guidelines

Feel free to open an issue with your comments / encountered bugs / requests for features. 

In case you want to contribute a PR, please open an issue first with a bit of details about the planned change or comment on an existing issue that you want to work on it.

The repository mostly uses the Google C++ [style guide](https://google.github.io/styleguide/cppguide.html).

## clang-format

 Formatting is checked upon submission, please format your files with the included `.clang-format` configuration. Use the provided scripts for your convenience:

 ```
 /misc/scripts/format.ps1
 /misc/scripts/format.sh
 ```
## clang-tidy

Static analysis is checked upon submission, please make sure there are 0 clang-tidy warnings in your code, using the included `.clang-tidy` configutaion. Use the provided scripts for your convenience:

 ```
 /misc/scripts/tidy.ps1
 /misc/scripts/tidy.sh
 ```
