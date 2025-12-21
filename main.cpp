#include "debug.h"
#include "files.2/AsyncFilesystem.h"

int main(int argc, char *argv[])
{
    lg("Hello World, I'm mlapi.");
    ml::AsyncFilesystem fs;
    fs.write("/tmp/test.txt", _S"Yolooo this is a test.");

    th::sleep(1);
    return 0;
}
