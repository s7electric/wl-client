This repo provides a **very** straightforward library to set up a simple wayland client with arbitrary rendering. See example code for usage and/or reach out on github. The public interface is found in `wlclient.h`.
## Usage
1. copy libwlclient.a and wlclient.h to the source directory of another project
2. include wlclient.h in a file
3. link against libwlclient.a
## Dependencies
- Installed wayland compositor (and libwayland)
## Build Instructions
```
$ git clone https://github.com/s7electric/wl-client.git
$ cd wl-client/
```
### Build library only
```
$ make libwlclient.a
```
### Build (and run) example code
```
$ make example
$ ./example/example
```
