#include "dqn.h"
#include "dqn.cpp"
#include "dqn_unit_tests.cpp"

int main(int argc, char *argv[])
{
    (void)argv; (void)argc;
    DN_Core *core = (DN_Core *)DN_OS_MemAlloc(sizeof(DN_Core), DN_ZeroMem_Yes);
    DN_Core_Init(core, DN_CoreOnInit_LogAllFeatures);
    DN_Test_RunSuite();
    return 0;
}
