#include "../src/vector/fvector.hpp"
#include "../src/string/string.hpp"
#include "../src/string/strings.hpp"

#include "../src/std.hpp"

int main(void){

    mc::fvector<mc::string> vec(32);
    mc::string str{"Hello World Hello World !"};
    vec.push_back(str.substr(5,10));
    vec.push_back(str.substr(6,11));
    return 1;
}
