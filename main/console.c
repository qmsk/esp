#include <logging.h>
#include <cli.h>
#include <uart.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// max line size
#define CLI_BUF_SIZE 512
#define CLI_MAX_ARGS 16

#define CLI_TASK_STACK 2048 // bytes
#define CLI_TASK_PRIORITY (tskIDLE_PRIORITY + 2)

static struct cli *console_cli;
static xTaskHandle console_cli_task;

// cli commands
static int console_help_cmd(int argc, char **arv, void *ctx)
{
  return cli_help(console_cli);
}

static int console_exit_cmd(int argc, char **arv, void *ctx)
{
  return cli_exit(console_cli);
}

static const struct cmd console_cli_commands[] = {
  { "help",   console_help_cmd,     .describe = "Show commands" },
  { "exit",   console_exit_cmd,     .describe = "Exit CLI" },
  {}
};

int init_console_cli()
{
  struct cli_options options = {
    .buf_size = CLI_BUF_SIZE,
    .max_args = CLI_MAX_ARGS,
  };
  int err;

  LOG_INFO("buf_size=%u max_args=%u",
    options.buf_size,
    options.max_args
  );

  // interactive CLI on stdin/stdout
  if ((err = cli_init(&console_cli, console_cli_commands, options))) {
    LOG_ERROR("cli_init");
    return err;
  }

  return 0;
}

int init_console()
{
  return init_console_cli();
}

void console_cli_main(void *arg)
{
  struct cli *cli = arg;
  int err;

  // unbuffered input
  setvbuf(stdin, NULL, _IONBF, 0);

  // line-buffered output
  setvbuf(stdout, NULL, _IOLBF, 0);

  if ((err = cli_main(cli)) < 0) {
    LOG_ERROR("cli_main");
  } else if (err) {
    LOG_INFO("cli timeout");
  } else {
    LOG_INFO("cli exit");
  }

  // exit
  console_cli_task = NULL;
  vTaskDelete(NULL);
}

int start_console()
{
  // start task
  if (xTaskCreate(&console_cli_main, "console-cli", CLI_TASK_STACK, console_cli, CLI_TASK_PRIORITY, &console_cli_task) <= 0) {
    LOG_ERROR("xTaskCreate");
    return -1;
  }

  return 0;
}
