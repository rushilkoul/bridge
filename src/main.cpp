#include <iostream>
#include "net/peer.hpp"

int main() {
    Peer p(5001);

    p.start();
    p.connect(RemotePeer{"10.218.136.229", 5000, "ABCDEFG"});
    getchar();
    return 0;
}