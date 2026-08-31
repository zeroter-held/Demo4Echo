#ifndef AUDIO_HELPER_H
#define AUDIO_HELPER_H

#include <unistd.h>
#include <sys/wait.h>

/**
 * Play a WAV file asynchronously without blocking the UI thread.
 *
 * Why not system()?
 *   main is a large multi-threaded process on RV1106. system() forks a
 *   child that inherits the (already heavily fragmented) address space,
 *   which makes ALSA's DMA buffer allocation fail with "not enough memory".
 *
 * Why vfork()+exec()?
 *   vfork() does not copy the parent's address space; exec() replaces it
 *   with a fresh aplay process, just like running aplay from an SSH shell.
 *
 * The trailing '&' in the shell command lets aplay run in the background,
 * so the shell exits immediately and the UI thread is not blocked.
 */
static inline void audio_play_wav_async(const char *path)
{
    pid_t pid = vfork();
    if (pid == 0) {
        /* child: kill any previous aplay, then start the new one */
        execl("/bin/sh", "sh", "-c",
              "killall aplay 2>/dev/null; aplay -B 10000 \"$1\" > /dev/null 2>&1 &",
              "sh", path, (char *)NULL);
        _exit(1);  /* exec should never return */
    }
    /* parent returns immediately */
}

#endif /* AUDIO_HELPER_H */
