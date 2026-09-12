#include "app.hpp"
#include <iostream>
#include <string>

int main(int argc,char** argv) {
    bool smoke=false;std::filesystem::path data;
    for(int i=1;i<argc;++i) {
        std::string arg=argv[i];
        if(arg=="--smoke")smoke=true;
        else if(arg=="--data-dir"&&i+1<argc)data=argv[++i];
        else {std::cout<<"Uso: distribuidora [--smoke --data-dir PASTA_TEMPORARIA]\n";return arg=="--help"?0:1;}
    }
    if(smoke&&data.empty()) {std::cerr<<"--smoke requer --data-dir para isolar os saves de teste.\n";return 1;}
    return runApp(smoke,data);
}
