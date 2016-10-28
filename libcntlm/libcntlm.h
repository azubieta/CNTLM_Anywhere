#ifndef LIBCNTLM
#define LIBCNTLM

namespace libcntlm {
    extern "C" int start(int argc, char *argv[]);
    extern "C" void stop();

    extern "C" int strsplit(const char *str, char c, char ***arr);
}


#endif // LIBCNTLM

