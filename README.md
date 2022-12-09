## Sponge quickstart

To set up your build directory:

	$ mkdir build
	$ cd build
	$ cmake ..

**Note:** all further commands listed below should be run from the `build` dir.

To build:

    $ make

To test (after building; make sure you've got the [build prereqs](https://web.stanford.edu/class/cs144/vm_howto) installed!)

    $ make check_labN *(replacing N with a checkpoint number)* (N = 0,1,2,3,4)

The first time you run `make check_lab...`, it will run `sudo` to configure two
[TUN](https://www.kernel.org/doc/Documentation/networking/tuntap.txt) devices for use during
testing.

To test performance:
    $ ./apps/tcp_benchmark
    
Use webget to communicate with real server with own tcp implementation :
    $ ./apps/webget stanford.edu /class/cs144
