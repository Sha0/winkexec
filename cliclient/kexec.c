/* WinKexec: kexec for Windows
 * Copyright (C) 2008-2009 John Stumpo
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <windows.h>
#include <stdio.h>

#include <KexecCommon.h>
#include <kexec.h>

#include "../revtag/revtag.h"


/* Handle kexec /l */
static int DoLoad(int argc, char** argv)
{
  DWORD klen, ilen, cmdlen;
  unsigned char* kbuf;
  unsigned char* ibuf = NULL;
  unsigned char* cmdline = NULL;
  int i;

  /* No args: just load the driver and do nothing else. */
  if (argc < 1) {
    if (!KxcIsDriverLoaded()) {
      printf("Loading the kexec driver... ");
      if (!KxcLoadDriver()) {
        KxcReportErrorStderr();
        exit(EXIT_FAILURE);
      }
      printf("ok\n");
    } else
      printf("The kexec driver was already loaded; nothing to do.\n");
    exit(EXIT_SUCCESS);
  }

  printf("Using kernel: %s\n", argv[0]);

  /* Read the kernel into a buffer. */
  printf("Reading kernel... ");
  if (!(kbuf = KxcLoadFile(argv[0], &klen))) {
    KxcReportErrorStderr();
    exit(EXIT_FAILURE);
  }
  printf("ok\n");

  /* Magic numbers in a Linux kernel image */
  if (*(unsigned short*)(kbuf+510) != 0xaa55 ||
    strncmp(kbuf+514, "HdrS", 4) != 0)
  {
    fprintf(stderr, "warning: This does not look like a Linux kernel.\n");
    fprintf(stderr, "warning: Loading it anyway.\n");
  }

  /* Look for an initrd. */
  ilen = cmdlen = 0;
  for (i = 1; i < argc; i++) {
    /* While we're parsing the cmdline, also tally up how much RAM we need
       to hold the final cmdline we send to kexec.sys. */
    cmdlen += strlen(argv[i]) + 1;
    if (!strncasecmp(argv[i], "initrd=", 7)) {
      printf("Using initrd: %s\n", argv[i]+7);

      /* Read the initrd into a buffer. */
      printf("Reading initrd... ");
      if (!(ibuf = KxcLoadFile(argv[i]+7, &ilen))) {
        KxcReportErrorStderr();
        exit(EXIT_FAILURE);
      }
      printf("ok\n");
    }
  }

  /* Build the final buffer for the cmdline. */
  if (cmdlen) {
    if ((cmdline = malloc(cmdlen+1)) == NULL) {
      perror("Could not allocate buffer for kernel command line");
      exit(EXIT_FAILURE);
    }
    cmdline[0] = '\0';
    for (i = 1; i < argc; i++) {
      strcat(cmdline, argv[i]);
      strcat(cmdline, " ");
    }
    cmdline[strlen(cmdline)-1] = '\0';
    printf("Kernel command line is: %s\n", cmdline);
  } else {
    printf("Kernel command line is blank.\n");
  }

  /* Now let kexec.sys know about it. */
  if (!KxcIsDriverLoaded()) {
    printf("Loading the kexec driver... ");
    if (!KxcLoadDriver()) {
      KxcReportErrorStderr();
      exit(EXIT_FAILURE);
    }
    printf("ok\n");
  }

  /* Do the kernel... */
  printf("Loading kernel into kexec driver... ");
  if (!KxcDriverOperation(KEXEC_SET | KEXEC_KERNEL, kbuf, klen, NULL, 0)) {
    KxcReportErrorStderr();
    exit(EXIT_FAILURE);
  }
  free(kbuf);
  printf("ok\n");

  /* ...and the initrd. */
  if (ibuf) {
    printf("Loading initrd into kexec driver... ");
    if (!KxcDriverOperation(KEXEC_SET | KEXEC_INITRD, ibuf, ilen, NULL, 0)) {
      KxcReportErrorStderr();
      exit(EXIT_FAILURE);
    }
    free(ibuf);
    printf("ok\n");
  } else {
    printf("Setting null initrd... ");
    if (!KxcDriverOperation(KEXEC_SET | KEXEC_INITRD, NULL, 0, NULL, 0)) {
      KxcReportErrorStderr();
      exit(EXIT_FAILURE);
    }
    printf("ok\n");
  }

  printf("Setting kernel command line... ");
  if (!KxcDriverOperation(KEXEC_SET | KEXEC_KERNEL_COMMAND_LINE, cmdline, cmdlen, NULL, 0)) {
    KxcReportErrorStderr();
    exit(EXIT_FAILURE);
  }
  if (cmdline)
    free(cmdline);
  printf("ok\n");

  exit(EXIT_SUCCESS);
}


/* Handle kexec /u */
static int DoUnload(int argc KEXEC_UNUSED, char** argv KEXEC_UNUSED)
{
  if (KxcIsDriverLoaded()) {
    printf("Unloading the kexec driver... ");
    if (!KxcUnloadDriver()) {
      KxcReportErrorStderr();
      exit(EXIT_FAILURE);
    }
    printf("ok\n");
  } else
    printf("The kexec driver was not loaded; nothing to do.\n");
  exit(EXIT_SUCCESS);
}


/* Handle kexec /c */
static int DoClear(int argc KEXEC_UNUSED, char** argv KEXEC_UNUSED)
{
  /* We can't do it without kexec.sys loaded... */
  if (!KxcIsDriverLoaded()) {
    printf("The kexec driver is not loaded.\n");
    exit(EXIT_FAILURE);
  }

  printf("Setting null kernel... ");
  if (!KxcDriverOperation(KEXEC_SET | KEXEC_KERNEL, NULL, 0, NULL, 0)) {
    KxcReportErrorStderr();
    exit(EXIT_FAILURE);
  }
  printf("ok\n");

  printf("Setting null initrd... ");
  if (!KxcDriverOperation(KEXEC_SET | KEXEC_INITRD, NULL, 0, NULL, 0)) {
    KxcReportErrorStderr();
    exit(EXIT_FAILURE);
  }
  printf("ok\n");

  printf("Setting null kernel command line... ");
  if (!KxcDriverOperation(KEXEC_SET | KEXEC_KERNEL_COMMAND_LINE, NULL, 0, NULL, 0)) {
    KxcReportErrorStderr();
    exit(EXIT_FAILURE);
  }
  printf("ok\n");

  exit(EXIT_SUCCESS);
}


/* Handle kexec /s */
static int DoShow(int argc KEXEC_UNUSED, char** argv KEXEC_UNUSED)
{
  DWORD bytecount;
  unsigned char* cmdline;

  /* kexec.sys can't tell us anything if it's not loaded... */
  if (!KxcIsDriverLoaded()) {
    printf("The kexec driver is not loaded.  (Use `kexec /l' to load it.)\n");
    exit(EXIT_FAILURE);
  }

  /* Well, we know this much. */
  printf("The kexec driver is active.  (Use `kexec /u' to unload it.)\n");

  /* Check the loaded kernel. */
  if (!KxcDriverOperation(KEXEC_GET_SIZE | KEXEC_KERNEL, NULL, 0, &bytecount, sizeof(DWORD))) {
    fprintf(stderr, "getting kernel size: ");
    KxcReportErrorStderr();
    exit(EXIT_FAILURE);
  }
  if (bytecount > 0)
    printf("Kernel size: %lu bytes\n", bytecount);
  else {
    printf("No kernel is loaded.\n");
    exit(EXIT_SUCCESS);  /* no point in continuing... */
  }

  /* Check the loaded initrd. */
  if (!KxcDriverOperation(KEXEC_GET_SIZE | KEXEC_INITRD, NULL, 0, &bytecount, sizeof(DWORD))) {
    fprintf(stderr, "getting initrd size: ");
    KxcReportErrorStderr();
    exit(EXIT_FAILURE);
  }
  if (bytecount > 0)
    printf("Initrd size: %lu bytes\n", bytecount);
  else
    printf("No initrd is loaded.\n");  

  /* Check the loaded kernel command line. */
  if (!KxcDriverOperation(KEXEC_GET_SIZE | KEXEC_KERNEL_COMMAND_LINE, NULL, 0, &bytecount, sizeof(DWORD))) {
    fprintf(stderr, "getting kernel command line size: ");
    KxcReportErrorStderr();
    exit(EXIT_FAILURE);
  }
  if (bytecount > 0) {
    if (!(cmdline = malloc(bytecount + 1))) {
      perror("malloc failure");
      exit(EXIT_FAILURE);
    }
    if (!KxcDriverOperation(KEXEC_GET | KEXEC_KERNEL_COMMAND_LINE, NULL, 0, cmdline, bytecount)) {
      fprintf(stderr, "getting kernel command line: ");
      KxcReportErrorStderr();
      exit(EXIT_FAILURE);
    }
    cmdline[bytecount] = '\0';
    printf("Kernel command line: %s\n", cmdline);
    free(cmdline);
  } else
    printf("No kernel command line is set.\n");
  

  exit(EXIT_SUCCESS);
}


/* Show help on cmdline usage of kexec. */
static void usage()
{
  fprintf(stderr, "%s",
"\n\
WinKexec: kexec for Windows (v1.0, svn revision " SVN_REVISION_STR ")\n\
Copyright (C) 2008-2009 John Stumpo\n\
\n\
This program is free software; you may redistribute or modify it under the\n\
terms of the GNU General Public License, version 3 or later.  There is\n\
ABSOLUTELY NO WARRANTY, not even for MERCHANTABILITY or FITNESS FOR A\n\
PARTICULAR PURPOSE.  See the GPL version 3 for full details.\n\
\n\
Usage: kexec [action] [options]...\n\
Actions:\n\
  /l /load     Load a Linux kernel.\n\
    The next option is the kernel filename.  All subsequent options are\n\
    passed as the kernel command line.  If an initrd= option is given,\n\
    the named file will be loaded as an initrd.  The kexec driver will\n\
    be loaded automatically if it is not loaded.  With no options, just\n\
    load the kexec driver without loading a kernel.\n\
  /u /unload   Unload the kexec driver.  (Naturally, this causes it to\n\
    forget the currently loaded kernel, if any, as well.)\n\
  /c /clear    Clear the currently loaded Linux kernel, but leave the\n\
    kexec driver loaded.\n\
  /s /show     Show current state of kexec.\n\
  /h /? /help  Show this help.\n\
\n");
  exit(EXIT_FAILURE);
}


/* Entry point */
int main(int argc, char** argv)
{
  int (*action)(int, char**) = NULL;

  /* No args given, no sense in doing anything. */
  if (argc < 2)
    usage();

  KxcInit();

  /* Allow Unix-style cmdline options. */
  if (argv[1][0] == '-') {
    if (argv[1][1] == '-')
      argv[1]++;
    argv[1][0] = '/';
  }

  /* Decide what to do... */
  if (!strcasecmp(argv[1], "/l") || !strcasecmp(argv[1], "/load"))
    action = DoLoad;

  if (!strcasecmp(argv[1], "/u") || !strcasecmp(argv[1], "/unload"))
    action = DoUnload;

  if (!strcasecmp(argv[1], "/c") || !strcasecmp(argv[1], "/clear"))
    action = DoClear;

  if (!strcasecmp(argv[1], "/s") || !strcasecmp(argv[1], "/show"))
    action = DoShow;

  if (!action)
    usage();

  /* ...and do it. */
  exit(action(argc - 2, argv + 2));
}
