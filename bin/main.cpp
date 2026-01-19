#include <omfl/parser.h>
#include <iostream>

using namespace omfl;

int main(int, char**) {
    auto root_ = parse("/Users/mikle/Desktop/labwork 3/example/config.omfl", true);

    std::cout << root_.valid() << '\n';

    std::string tr = root_.Get("servers.first").Get("ip").AsString();

    std::cout << tr << '\n';

    return 0;
}
