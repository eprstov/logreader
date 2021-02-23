# log reader
a simple tool used to quickly extract specific information from a large single-byte log file on iOS devices

### features
takes a filter in the form of a basic wildcard expression where `?` substitudes for a single character, and `*` substitudes for a sequence of zero or more characters

##### command-line tool
- outputs matching lines to the standard output
##### iOS application
- downloads and stores a log given by an URL
- produces a list of the matching lines on the screen

### requirements
- C++, no STL
- Objective-C, Storyboards, no ARC
- high performance
  - has a very simple implementation due to a very basic feature set
  - processes input in parallel, utilizing multiple CPU cores
  - provides 10-30 times better performance than grep with equivalent expressions
- low memory consumption
  - up to 10Mb in command-line, regardless of the input size
  - around 50Mb as an iOS application, regardless of neither the input nor the output size
- support for older iOS devices (iOS 9.0 and above)

### structure
- `lib/` — libreader library sources
- `app/` — iOS application sources
- `cmd/` — command-line tool sources
- `CMakeLists.txt` — cross-platfrom build project for the command-line tool
- `logreader.xcworkspace` — Xcode workspace for the iOS application

### how to build
##### comman-line tool
1. from the repository root directory call `cmake -B.make && make -C.make`
2. the executable now is at `.make/cmd/logreader`
##### iOS application
1. open logreader.xcworkspace from Xcode 4.0 or above
2. build the logreader target for the device of your choice

### how to run
##### command-line tool
```
usage: logreader <filter> <path>
```

