#ifndef LIBCNTLM
#define LIBCNTLM

namespace libcntlm {
    extern "C" int start(int argc, char *argv[], char **errorMsg);
    extern "C" void stop();
}


#endif // LIBCNTLM

