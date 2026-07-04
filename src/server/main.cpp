#include "Server.hh"
#include "core/Config.hh"

using namespace jstine;

int main(int argc, char** argv) {
    auto config = Config::load(argc, argv);

    return 0;
}
