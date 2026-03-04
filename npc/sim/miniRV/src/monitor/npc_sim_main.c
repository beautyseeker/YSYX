#include <common.h>

Simulator* npc_sim = nullptr;

int main(int argc, char **argv) {
    init_monitor(argc, argv);
    sdb_mainloop();
    int good = (nemu_state.state == NEMU_END && nemu_state.halt_ret == 0) ||
    (nemu_state.state == NEMU_QUIT);
    return !good;
}
