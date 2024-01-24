#include "runtime_image.h"
#include <ruby/ruby.h>

int main(int argc, char* argv[])
{
    if (argc != 2) { 
        std::cout << "Please provide full path to the ruby script\n";
        std::exit(1);
    }
    k3s::testgen::RuntimeImage rt_img(argc, argv);
    rt_img.Run();

    ruby_cleanup(0);
    return 0;
}
