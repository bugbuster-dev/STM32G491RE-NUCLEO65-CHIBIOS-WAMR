/*
    ChibiOS - Copyright (C) 2006..2018 Giovanni Di Sirio

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include <string.h>

#include "ch.h"
#include "hal.h"
#include "rt_test_root.h"
#include "oslib_test_root.h"

#include "shell.h"
#include "chprintf.h"

#include "wasm_export.h"
#include "wasm_runtime.h"
#include "test_wasm.h"


#define SIO_DEV	LPSIOD1
//#define SIO_DEV	SIOD1

/*===========================================================================*/
/* Command line related.                                                     */
/*===========================================================================*/

#define SHELL_WA_SIZE   THD_WORKING_AREA_SIZE(2048)

/* Can be measured using dd if=/dev/xxxx of=/dev/null bs=512 count=10000.*/
static void cmd_write(BaseSequentialStream *chp, int argc, char *argv[]) {
  static uint8_t buf[] =
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"
      "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";

  (void)argv;
  if (argc > 0) {
    chprintf(chp, "Usage: write\r\n");
    return;
  }

  while (chnGetTimeout((BaseChannel *)chp, TIME_IMMEDIATE) == Q_TIMEOUT) {
#if 1
    /* Writing in channel mode.*/
    chnWrite(chp, buf, sizeof buf - 1);
#else
    /* Writing in buffer mode.*/
    (void) obqGetEmptyBufferTimeout(&PORTAB_SDU1.obqueue, TIME_INFINITE);
    memcpy(PORTAB_SDU1.obqueue.ptr, buf, SERIAL_USB_BUFFERS_SIZE);
    obqPostFullBuffer(&PORTAB_SDU1.obqueue, SERIAL_USB_BUFFERS_SIZE);
#endif
  }
  chprintf(chp, "\r\n\nstopped\r\n");
}

#if STM32_CLOCK_DYNAMIC == TRUE
static void cmd_clock(BaseSequentialStream *chp, int argc, char *argv[]) {
  bool result;
  const halclkcfg_t *ccp;
  static const char usage[] = "Usage: clock [reset|default]\r\n";

  (void)argv;
  if (argc != 1) {
    chprintf(chp, usage);
    return;
  }

  if (strcmp(argv[0], "reset") == 0) {
    chprintf(chp, "\r\nSwitching to post-reset clocks: ");
    ccp = &hal_clkcfg_reset;
  }
  else if (strcmp(argv[0], "default") == 0) {
    chprintf(chp, "\r\nSwitching to default mcuconf.h clocks: ");
    ccp = &hal_clkcfg_default;
  }
  else {
    chprintf(chp, usage);
    return;
  }

  /* Time for the serial TX buffer to flush.*/
  chThdSleepMilliseconds(100);

  /* Switching clocks.*/
  result = halClockSwitchMode(ccp);

  /* Reconfiguring the peripherals because clocks frequencies could have
     changed.*/
  sioStart(&SIO_DEV, NULL);

  /* Printing result.*/
  if (result) {
    chprintf(chp, "failed\r\n");
  }
  else {
    chprintf(chp, "done\r\n");
  }
}
#endif

static void cmd_clocks(BaseSequentialStream *chp, int argc, char *argv[]) {
  const char *swp;

  (void)argv;
  if (argc > 0) {
    chprintf(chp, "Usage: clocks\r\n");
    return;
  }

#if STM32_SW == STM32_SW_HSI16
  swp = "HSI16";
#elif STM32_SW == STM32_SW_HSE
  swp = "HSE";
#elif STM32_SW == STM32_SW_PLLRCLK
  swp = "PLLR";
#else
  #error "invalid STM32_SW value specified"
#endif

  chprintf(chp, "HSI16:    %10u\r\n", halClockGetPointX(CLK_HSI16));
  chprintf(chp, "HSI48:    %10u\r\n", halClockGetPointX(CLK_HSI48));
  chprintf(chp, "HSE:      %10u\r\n", halClockGetPointX(CLK_HSE));
  chprintf(chp, "SYSCLK:   %10u (%s)\r\n", halClockGetPointX(CLK_SYSCLK), swp);
  chprintf(chp, "PLLPCLK:  %10u\r\n", halClockGetPointX(CLK_PLLPCLK));
  chprintf(chp, "PLLRCLK:  %10u\r\n", halClockGetPointX(CLK_PLLRCLK));
  chprintf(chp, "PLLQCLK:  %10u\r\n", halClockGetPointX(CLK_PLLQCLK));
  chprintf(chp, "HCLK:     %10u\r\n", halClockGetPointX(CLK_HCLK));
  chprintf(chp, "PCLK1:    %10u\r\n", halClockGetPointX(CLK_PCLK1));
  chprintf(chp, "PCLK1TIM: %10u\r\n", halClockGetPointX(CLK_PCLK1TIM));
  chprintf(chp, "PCLK2:    %10u\r\n", halClockGetPointX(CLK_PCLK2));
  chprintf(chp, "PCLK2TIM: %10u\r\n", halClockGetPointX(CLK_PCLK2TIM));
  chprintf(chp, "MCO:      %10u\r\n", halClockGetPointX(CLK_MCO));
}


void iwasm_main(BaseSequentialStream *chp, int argc, char *argv[]);
void iwasm_test_sum(BaseSequentialStream *chp, int argc, char *argv[]);

static void cmd_iwasm(BaseSequentialStream *chp, int argc, char *argv[]) {
  //iwasm_main(chp, argc, argv);
  iwasm_test_sum(chp, argc, argv);

  chprintf(chp, "iwasm done\r\n");
}


static const ShellCommand commands[] = {
  {"write", cmd_write},
#if STM32_CLOCK_DYNAMIC == TRUE
  {"clock", cmd_clock},
#endif
  {"clocks", cmd_clocks},
  {"iwasm", cmd_iwasm},
  {NULL, NULL}
};

static const ShellConfig shell_cfg1 = {
  (BaseSequentialStream *)&SIO_DEV,
  commands
};

/*===========================================================================*/
/* WAMR                                                             */
/*===========================================================================*/

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;


static int app_argc;
static char **app_argv;

/**
 * Find the unique main function from a WASM module instance
 * and execute that function.
 *
 * @param module_inst the WASM module instance
 * @param argc the number of arguments
 * @param argv the arguments array
 *
 * @return true if the main function is called, false otherwise.
 */
bool
wasm_application_execute_main(wasm_module_inst_t module_inst, int32_t argc,
                              char *argv[]);

static void *
app_instance_main(BaseSequentialStream *chp, wasm_module_inst_t module_inst)
{
    const char *exception;

    wasm_application_execute_main(module_inst, app_argc, app_argv);
    if ((exception = wasm_runtime_get_exception(module_inst)))
        chprintf(chp, "%s\n", exception);
    return NULL;
}


static char global_heap_buf[WASM_GLOBAL_HEAP_SIZE] = { 0 };


static struct
{
	BaseSequentialStream   *chp;
    wasm_module_t			module;
	wasm_module_inst_t 		module_inst;

} wasm_instance = {0};


int
iwasm_init(void)
{
    uint8 *wasm_file_buf = NULL;
    uint32 wasm_file_size;
    wasm_module_inst_t wasm_module_inst = NULL;
    RuntimeInitArgs init_args;
    char error_buf[128];

#if WASM_ENABLE_LOG != 0
    int log_verbose_level = 2;
#endif

    if (wasm_instance.module && wasm_instance.module_inst) return;

    memset(&init_args, 0, sizeof(RuntimeInitArgs));

    init_args.mem_alloc_type = Alloc_With_Pool;
    init_args.mem_alloc_option.pool.heap_buf = global_heap_buf;
    init_args.mem_alloc_option.pool.heap_size = sizeof(global_heap_buf);

    /* initialize runtime environment */
    if (!wasm_runtime_full_init(&init_args)) {
        chprintf(wasm_instance.chp, "Init runtime environment failed.\n");
        return;
    }

#if WASM_ENABLE_LOG != 0
    bh_log_set_verbose_level(log_verbose_level);
#endif

    /* load WASM byte buffer from byte buffer of include file */
    wasm_file_buf = (uint8 *)wasm_test_file;
    wasm_file_size = sizeof(wasm_test_file);

    /* load WASM module */
    if (!(wasm_instance.module = wasm_runtime_load(wasm_file_buf, wasm_file_size,
                                          error_buf, sizeof(error_buf)))) {
        chprintf(wasm_instance.chp, "%s\n", error_buf);
        return;
    }

    /* instantiate the module */
    if (!(wasm_instance.module_inst = wasm_runtime_instantiate(
    		wasm_instance.module, 8 * 1024, 8 * 1024, error_buf, sizeof(error_buf)))) {
        chprintf(wasm_instance.chp, "%s\n", error_buf);
        wasm_runtime_unload(wasm_instance.module);
    }
}

void
iwasm_test_sum(BaseSequentialStream *chp, int argc, char *argv[])
{
	wasm_instance.chp = chp;
	iwasm_init();

	if (argc < 2) {
		chprintf(chp, "usage: <int> <int>\r\n");
		return;
	}

    {
		const char *exception;
		uint32 wargv[2];
		WASMFunctionInstanceCommon *func = wasm_runtime_lookup_function(wasm_instance.module_inst, "sum");
		WASMExecEnv *exec_env = wasm_runtime_create_exec_env(wasm_instance.module_inst, 4*1024);

		wargv[0] = atoi(argv[0]);
		wargv[1] = atoi(argv[1]);
		wasm_runtime_call_wasm(exec_env, func, 2, wargv);
		/* the return value is stored in argv[0] */
		chprintf(chp, "function return: %d\n", wargv[0]);

	    if ((exception = wasm_runtime_get_exception(wasm_instance.module_inst)))
	        chprintf(chp, "%s\n", exception);
	    return NULL;
    }
}


void
iwasm_main(BaseSequentialStream *chp, int argc, char *argv[])
{
	wasm_instance.chp = chp;
	iwasm_init();

    app_instance_main(chp, wasm_instance.module_inst);
}

#define DEFAULT_THREAD_STACKSIZE (6 * 1024)
#define DEFAULT_THREAD_PRIORITY 50


/*===========================================================================*/
/* Generic code.                                                             */
/*===========================================================================*/

/*
 * Green LED blinker thread, times are in milliseconds.
 */
static THD_WORKING_AREA(waThread1, 128);
static THD_FUNCTION(Thread1, arg) {

  (void)arg;
  chRegSetThreadName("blinker");
  while (true) {
    palClearLine(LINE_LED_GREEN);
    chThdSleepMilliseconds(500);
    palSetLine(LINE_LED_GREEN);
    chThdSleepMilliseconds(500);
  }
}

/*
 * Application entry point.
 */
int main(void) {

  /*
   * System initializations.
   * - HAL initialization, this also initializes the configured device drivers
   *   and performs the board-specific initializations.
   * - Kernel initialization, the main() function becomes a thread and the
   *   RTOS is active.
   */
  halInit();
  chSysInit();

  /*
   * Activates the Serial or SIO driver using the default configuration.
   */
  sioStart(&SIO_DEV, NULL);

  /*
   * Shell manager initialization.
   */
  shellInit();

  /*
   * Creates the blinker thread.
   */
  chThdCreateStatic(waThread1, sizeof(waThread1), NORMALPRIO, Thread1, NULL);

  /*
   * Normal main() thread activity, in this demo it does nothing except
   * sleeping in a loop and check the button state.
   */
  while (true) {
    thread_t *shelltp = chThdCreateFromHeap(NULL, SHELL_WA_SIZE,
                                            "shell", NORMALPRIO + 1,
                                            shellThread, (void *)&shell_cfg1);
    chThdWait(shelltp);               /* Waiting termination.             */
    chThdSleepMilliseconds(500);
  }
}

