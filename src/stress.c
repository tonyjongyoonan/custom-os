/******************************************************************************
 *                                                                            *
 *                             Author: Hannah Pan                             *
 *                             Date:   04/15/2021                             *
 *                                                                            *
 ******************************************************************************/


#include "stress.h"

#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>


/******************************************************************************
 *                                                                            *
 *  Replace kernel.h with your own header file(s) for p_spawn and p_waitpid.  *
 *                                                                            *
 ******************************************************************************/

#include "p_pennos.h"


static void nap(void)
{
  usleep(10000);  // 10 milliseconds
}


/*
 * The function below spawns 10 nappers named child_0 through child_9 and waits
 * on them. The wait is non-blocking if nohang is true, or blocking otherwise.
 */

char* copy_string(char *destination, const char *source) {
  char* temp = destination;
  while (*source != '\0') {
        *destination = *source;
        destination++;
        source++;
    }
    *destination = '\0';
    return temp;
}

static void spawn(bool nohang)
{
  char name[] = "child_0";
  char *argv[] = { name, NULL };
  int pid = 0;

  // Spawn 10 nappers named child_0 through child_9.
  for (int i = 0; i < 10; i++) {
    argv[0][sizeof name - 2] = '0' + i;
    char* copied_argv = (char*)malloc((strlen(argv[0]) + 1) * sizeof(char));
    copy_string(copied_argv, *argv);
    const int id = p_spawn(nap, argv, STDERR_FILENO, STDERR_FILENO, copied_argv);

    if (i == 0)
      pid = id;

    dprintf(STDERR_FILENO, "%s was spawned\n", *argv);
  }

  // Wait on all children.
  while (1) {
    // printf("before stress wait\n");
    const int cpid = p_waitpid(-1, NULL, nohang);
    printf("CPID: %d\n", cpid);
    if (cpid < 0)  // no more waitable children (if block-waiting) or error
      p_exit();
      // while (1);

    // polling if nonblocking wait and no waitable children yet
    if (nohang && cpid == 0) {
      usleep(90000);  // 90 milliseconds
      continue;
    }

    dprintf(STDERR_FILENO, "child_%d was reaped\n", cpid - pid);
  }
}


/*
 * The function below recursively spawns itself 26 times and names the spawned
 * processes Gen_A through Gen_Z. Each process is block-waited by its parent.
 */

static void spawn_r(void)
{
  static int i = 0;

  int pid = 0;
  char name[] = "Gen_A";
  char *argv[] = { name, NULL };

  if (i < 26) {
    argv[0][sizeof name - 2] = 'A' + i++;
    char* copied_argv = (char*)malloc((strlen(argv[0]) + 1) * sizeof(char));
    copy_string(copied_argv, *argv);
    pid = p_spawn(spawn_r, argv, STDERR_FILENO, STDERR_FILENO, copied_argv);
    dprintf(STDERR_FILENO, "%s was spawned\n", *argv);
    usleep(10000);  // 10 milliseconds
  }

  int status;
  // printf("waiting on pid: %d\n", pid);
  if (pid > 0 && pid == p_waitpid(pid, &status, false))
  dprintf(STDERR_FILENO, "%s was reaped\n", *argv);

  // printf("AADFJKSAJDFSD\n");
}


/******************************************************************************
 *                                                                            *
 * Add commands hang, nohang, and recur to the shell as built-in subroutines  *
 * which call the following functions, respectively.                          *
 *                                                                            *
 ******************************************************************************/

void hang(void)
{
  spawn(false);
}

void nohang(void)
{
  spawn(true);
}

void recur(void)
{
  spawn_r();
}