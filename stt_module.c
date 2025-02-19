#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <pocketsphinx.h>
#include <sys/select.h>
#include <sphinxbase/err.h>
#include <sphinxbase/ad.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "smtc_module.h"

#define BUS_ADDRESS "224.0.29.200"
#define BUS_PORT "1234"

static ps_decoder_t *ps;
static cmd_ln_t *config;
static pid_t pid;
static int run;


static const arg_t cont_args_def[] = {
    POCKETSPHINX_OPTIONS,
    /* Argument file. */
    {"-argfile",
     ARG_STRING,
     NULL,
     "Argument file giving extra arguments."},
    {"-adcdev",
     ARG_STRING,
     NULL,
     "Name of audio device to use for input."},
    {"-infile",
     ARG_STRING,
     NULL,
     "Audio file to transcribe."},
    {"-inmic",
     ARG_BOOLEAN,
     "no",
     "Transcribe audio from microphone."},
    {"-time",
     ARG_BOOLEAN,
     "no",
     "Print word times in file transcription."},
    CMDLN_EMPTY_OPTION
};

void sig_handler(int signo)
{
  if (signo == SIGINT) {
      printf("stt_module received SIGINT\n");
      run = 0;
  }
}

static void
sleep_msec(int32 ms)
{
    /* ------------------- Unix ------------------ */
    struct timeval tmo;

    tmo.tv_sec = 0;
    tmo.tv_usec = ms * 1000;

    select(0, NULL, NULL, NULL, &tmo);
}


static void
recognize_from_microphone(int pipe_write)
{
    ad_rec_t *ad;
    int16 adbuf[2048];
    uint8 utt_started, in_speech;
    int32 k;
    int ret;
    char const *hyp;

    if ((ad = ad_open_dev(cmd_ln_str_r(config, "-adcdev"),
                          (int) cmd_ln_float32_r(config,
                                                 "-samprate"))) == NULL)
        E_FATAL("Failed to open audio device\n");

    ps_set_search(ps, "keyword");

    if (ad_start_rec(ad) < 0)
        E_FATAL("Failed to start recording\n");

    if (ps_start_utt(ps) < 0)
        E_FATAL("Failed to start utterance\n");
    utt_started = FALSE;
    printf("Ready....\n");

    while (run) {
        if ((k = ad_read(ad, adbuf, 2048)) < 0)
            E_FATAL("Failed to read audio\n");
        ps_process_raw(ps, adbuf, k, FALSE, FALSE);
        in_speech = ps_get_in_speech(ps);
        if (in_speech && !utt_started) {
            utt_started = TRUE;
            printf("Listening...\n");
        }
        if (!in_speech && utt_started) {
            /* speech -> silence transition, time to start new utterance  */
            ps_end_utt(ps);
            hyp = ps_get_hyp(ps, NULL );
            if (hyp != NULL) {
                //printf("%s\n", hyp);
                //printf("keyphrase = %s\n", cmd_ln_str_r(config, "-keyphrase"));
                fflush(stdout);

                if (!strcmp(ps_get_search(ps), "keyword")) {
                    if (!strcmp(hyp, cmd_ln_str_r(config, "-keyphrase"))) {
                	printf("%s\n", hyp);
                        LOG_GREEN("*******************KEYWORD MATCH*******************\n");
                        fflush(stdout);
                        ps_set_search(ps, "lm");
                    }
                } else if (!strcmp(ps_get_search(ps), "lm")) {
                    ps_set_search(ps, "keyword");
                    LOG_GREEN("============>");
                    LOG_BLUE("[");
                    LOG_BLUE(hyp);
                    LOG_BLUE("]\n");
                    LOG_GREEN("============>Processing...\n");
                    ret = command_proc(pipe_write, hyp);
                    LOG_YELLOW("*******************************\n");
                    if (ret != 0) {
                	LOG_RED("Oops! Processing Failed...Please retry the command!\n");
                    } else {
                	LOG_GREEN("============>Success!\n");
                    }
                    LOG_GREEN("***************************************************\n");
                }
            }

            if (ps_start_utt(ps) < 0)
                E_FATAL("Failed to start utterance\n");
            utt_started = FALSE;
            printf("Ready....\n");
        }
        sleep_msec(10);
    }
    ad_close(ad);
}


int main(int argc, char *argv[])
{
    char const *cfg;
    int pipefd[2];

    config = cmd_ln_parse_r(NULL, cont_args_def, argc, argv, TRUE);

    /* Handle argument file as -argfile. */
    if (config && (cfg = cmd_ln_str_r(config, "-argfile")) != NULL) {
        config = cmd_ln_parse_file_r(config, cont_args_def, cfg, FALSE);
    }

    if (config == NULL || (cmd_ln_str_r(config, "-infile") == NULL && cmd_ln_boolean_r(config, "-inmic") == FALSE)) {
	E_INFO("Specify '-infile <file.wav>' to recognize from file or '-inmic yes' to recognize from microphone.\n");
        cmd_ln_free_r(config);
	return 1;
    }

    ps_default_search_args(config);

    printf("dictionary file = %s\n", cmd_ln_str_r(config, "-dict"));
    printf("language model = %s\n", cmd_ln_str_r(config, "-lm"));
    printf("keyphrase = %s\n", cmd_ln_str_r(config, "-keyphrase"));
    printf("kws_threshold = %8f\n", cmd_ln_float_r(config, "-kws_threshold"));

    ps = ps_init(config);
    if (ps == NULL) {
        cmd_ln_free_r(config);
        return 1;
    }

    ps_set_keyphrase(ps, "keyword", cmd_ln_str_r(config, "-keyphrase"));
    ps_set_lm_file(ps, "lm", cmd_ln_str_r(config, "-lm"));

    E_INFO("%s COMPILED ON: %s, AT: %s\n\n", argv[0], __DATE__, __TIME__);

    if (cmd_ln_boolean_r(config, "-inmic")) {
        if (pipe(pipefd) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }

	pid = fork();
	if (pid < 0) {
	    //error
	    printf("create xaal protocol module failed\n");
	    perror("fork");
	} else if (pid > 0) {
	    //parent
	    run = 1;
	    close(pipefd[0]);
	    signal(SIGINT, sig_handler);
	    recognize_from_microphone(pipefd[1]);
	} else {
	    //child
	    close(pipefd[1]);
	    dummy_commander(pipefd[0], BUS_ADDRESS, BUS_PORT);
	    printf("XXXXXXXXXXXXXchild process returns\n");
	}

    } else {
	printf("stt module only takes microphone input.\n");
    }

    ps_free(ps);
    cmd_ln_free_r(config);
    wait(NULL);

    return 0;
}
