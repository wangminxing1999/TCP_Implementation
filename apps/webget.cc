#include "tcp_sponge_socket.hh"
#include "util.hh"
#include "address.hh"
#include <cstdlib>
#include <iostream>

using namespace std;

void get_URL(const string &host, const string &path) {
    // Your code here.

    // You will need to connect to the "http" service on
    // the computer whose name is in the "host" string,
    // then request the URL path given in the "path" string.

    // Then you'll need to print out everything the server sends back,
    // (not just one call to read() -- everything) until you reach
    // the "eof" (end of file).

    CS144TCPSocket sock2;
    sock2.connect(Address(host,"http"));

    string s1 = "GET ";
    string s2 = " HTTP/1.1\r\n";
    std::string target1 = s1+path+s2;

    string s3 = "Host: ";
    string s4 = "\r\n";
    std::string target2 = s3+host+s4;

    sock2.write(target1);
    sock2.write(target2);
    sock2.write("Connection: close\r\n");
    sock2.write("\r\n");

    while(1) {
        if(sock2.eof())
            break;
        auto recvd = sock2.read();
        printf("%s",recvd.c_str());
    }
    sock2.wait_until_closed();
}

int main(int argc, char *argv[]) {
    try {
        if (argc <= 0) {
            abort();  // For sticklers: don't try to access argv[0] if argc <= 0.
        }

        // The program takes two command-line arguments: the hostname and "path" part of the URL.
        // Print the usage message unless there are these two arguments (plus the program name
        // itself, so arg count = 3 in total).
        if (argc != 3) {
            cerr << "Usage: " << argv[0] << " HOST PATH\n";
            cerr << "\tExample: " << argv[0] << " stanford.edu /class/cs144\n";
            return EXIT_FAILURE;
        }

        // Get the command-line arguments.
        const string host = argv[1];
        const string path = argv[2];

        // Call the student-written function.
        get_URL(host, path);
    } catch (const exception &e) {
        cerr << e.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
