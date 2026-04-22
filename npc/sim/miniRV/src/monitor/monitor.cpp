#include <getopt.h>
#include "monitor.h"


void init_rand();
void init_log(const char *log_file);
void init_mem();
void init_difftest(char *ref_so_file, long img_size, int port);
void init_device();
void init_sdb();
void init_disasm();
void init_elf(const char *filename);
void sdb_mainloop();

extern uint8_t* guest_to_host(paddr_t addr);


static void welcome() {
  Log("Trace: %s", MUXDEF(CONFIG_TRACE, ANSI_FMT("ON", ANSI_FG_GREEN), ANSI_FMT("OFF", ANSI_FG_RED)));
  IFDEF(CONFIG_TRACE, Log("If trace is enabled, a log file will be generated "
        "to record the trace. This may lead to a large log file. "
        "If it is not necessary, you can disable it in menuconfig"));
  Log("Build time: %s, %s", __TIME__, __DATE__);
  printf("Welcome to %s!\n", ANSI_FMT(str(RV32-NPC), ANSI_FG_YELLOW ANSI_BG_RED));
  printf("For help, type \"help\"\n");
}

void print_statistic(Simlator* cpu) {
    SIMLOG("---------------------Simulation end statistics-------------------------");
    auto stat = cpu->get_statistic();
    extern uint64_t get_uptime();
    if (stat->inst_nr > 0 && stat->cycle_nr > 0) {
        uint64_t sim_time_us = get_uptime();
        SIMLOG("%s Simulation time: %.3f ms MIPS: %.6f", 
        cpu->get_img_name(), sim_time_us / 1000.0, 
        float(stat->inst_nr) / sim_time_us);
        SIMLOG("Cycles executed:%lu Instructions executed:%lu CPI: %.2f", 
            stat->cycle_nr, stat->inst_nr,
            (double)stat->cycle_nr / stat->inst_nr);
    }
    SIMLOG("-----------------------------------------------------------------------");
}

void print_trap_state(Simlator* cpu, int state) {
  auto stat = cpu->get_statistic();
    if(state == 0) {
        printf(ANSI_FMT("[%ld] HIT A GOOD TRAP!\n", ANSI_FG_GREEN), stat->inst_nr);
        cpu->set_state(SimState::END);
    } else {
        printf(ANSI_FMT("[%ld] HIT A BAD TRAP!\n", ANSI_FG_RED), 
        stat->inst_nr);
        cpu->set_state(SimState::ABORT);
    }
}


void sdb_set_batch_mode();

static char *log_file = NULL;
static char *diff_so_file = NULL;
static char *img_file = NULL;
static char *elf_file = NULL;
static int difftest_port = 1234;

static long load_img() {
  if (img_file == NULL) {
    Log("No image is given. Use the default build-in image.");
    return 4096; // built-in image size
  }

  FILE *fp = fopen(img_file, "rb");
  Assert(fp, "Can not open '%s'", img_file);

  fseek(fp, 0, SEEK_END);
  long size = ftell(fp);

  Log("The image is %s, size = %ld", img_file, size);

  fseek(fp, 0, SEEK_SET);
  // int ret = fread(guest_to_host(RESET_VECTOR), size, 1, fp);
  // assert(ret == 1);

  fclose(fp);
  return size;
}

static int parse_args(int argc, char *argv[]) {
  const struct option table[] = {
    {"batch"    , no_argument      , NULL, 'b'},
    {"log"      , required_argument, NULL, 'l'},
    {"diff"     , required_argument, NULL, 'd'},
    {"port"     , required_argument, NULL, 'p'},
    {"help"     , no_argument      , NULL, 'h'},
    {"elf"      , required_argument, NULL, 'e' },
    {0          , 0                , NULL,  0 },
  };
  int o;
  while ( (o = getopt_long(argc, argv, "-bhl:d:p:e:", table, NULL)) != -1) {
    switch (o) {
      case 'b': sdb_set_batch_mode(); break;
      case 'p': sscanf(optarg, "%d", &difftest_port); break;
      case 'l': log_file = optarg; break;
      case 'd': diff_so_file = optarg; break;
      case 'e': elf_file = optarg; break;
      case 1: img_file = optarg; break;
      default:
        printf("Usage: %s [OPTION...] IMAGE [args]\n\n", argv[0]);
        printf("\t-b,--batch              run with batch mode\n");
        printf("\t-l,--log=FILE           output log to FILE\n");
        printf("\t-d,--diff=REF_SO        run DiffTest with reference REF_SO\n");
        printf("\t-p,--port=PORT          run DiffTest with port PORT\n");
        printf("\t-e,--elf=ELF_FILE       load ELF_FILE as program\n");
        printf("\n");
        exit(0);
    }
  }
  return 0;
}

void init_monitor(int argc, char *argv[]) {
  /* Perform some global initialization. */

  /* Parse arguments. */
  parse_args(argc, argv);
  for(int i = 0; i < argc; i++) {
    printf("Monitor parse Argument %d: %s\n", i, argv[i]);
  }
  printf("Parsed arguments:log_file=%s, diff_so_file=%s, difftest_port=%d, elf_file=%s, img_file=%s\n",
  log_file, diff_so_file, difftest_port, elf_file, img_file);
  /* Set random seed. */
  // init_rand();

  /* Open the log file. */
  // init_log(log_file);

  /* Initialize memory. */
  init_mem();

  /* Initialize devices. */
  // IFDEF(CONFIG_DEVICE, init_device());

  /* Perform ISA dependent initialization. */
  // init_isa();

  /* Load the image to memory. This will overwrite the built-in image. */
  long img_size = load_img();

  /* Initialize differential testing. */
  init_difftest(diff_so_file, img_size, difftest_port);

  /* Initialize the simple debugger. */
  init_sdb();

  IFDEF(CONFIG_ITRACE, {
    Simlator::instance->itracer->init_disasm();
  });

  // IFDEF(CONFIG_FTRACE, init_elf(elf_file));

  /* Display welcome message. */
  welcome();

  sdb_mainloop();
}

