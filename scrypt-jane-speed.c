#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <omp.h> // OpenMP for multi-core CPU parallelism

#ifdef __cplusplus
extern "C" {
#endif
#include "scrypt-jane.c"
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C" {
#endif
#include "cuda-scrypt-jane.cu" // CUDA for GPU acceleration
#ifdef __cplusplus
}
#endif

/* ticks - not tested on anything other than x86 */
static uint64_t get_ticks(void) {
    uint64_t ticks = 0;
#if defined(CPU_X86) || defined(CPU_X86_64)
    #if defined(COMPILER_INTEL)
        ticks = __rdtsc();
    #elif defined(COMPILER_MSVC)
        ticks = __rdtsc();
    #elif defined(COMPILER_GCC)
        uint32_t lo, hi;
        __asm__ __volatile__("rdtsc" : "=a" (lo), "=d" (hi));
        ticks = ((uint64_t)lo | ((uint64_t)hi << 32));
    #else
        #error "rdtsc not supported for this compiler"
    #endif
#else
    #error "Ticks not supported for this platform"
#endif
    return ticks;
}

#define maxticks 0xffffffffffffffffull

typedef struct scrypt_speed_settings_t {
    const char *desc;
    uint8_t Nfactor, rfactor, pfactor;
} scrypt_speed_settings;

/* Define scrypt settings */
static const scrypt_speed_settings settings[] = {
    {"scrypt high volume     ( ~4mb)", 11, scrypt_r_32kb - 5, 0},
    {"scrypt interactive     (~16mb)", 13, scrypt_r_32kb - 5, 0},
    {"scrypt non-interactive (~ 1gb)", 19, scrypt_r_32kb - 5, 0},
    {0}
};

int main(void) {
    const scrypt_speed_settings *s;
    uint8_t password[64], salt[24], digest[64];
    uint64_t minticks, ticks;
    size_t i, passes;
    size_t num_threads = omp_get_max_threads(); // Get the number of available CPU threads

    // Initialize password and salt
    for (i = 0; i < sizeof(password); i++)
        password[i] = (uint8_t)i;
    for (i = 0; i < sizeof(salt); i++)
        salt[i] = 255 - (uint8_t)i;

    // Warm up
    scrypt(password, sizeof(password), salt, sizeof(salt), 15, 3, 4, digest, sizeof(digest));

    while (1) {
        printf("Speed test for scrypt\n");

        // Parallelize scrypt computation across CPU cores
        #pragma omp parallel for private(s, minticks, passes) shared(settings)
        for (i = 0; i < sizeof(settings) / sizeof(settings[0]); i++) {
            s = &settings[i];
            minticks = maxticks;

            // Perform multiple passes to get average time
            for (passes = 0; passes < 16; passes++) {
                ticks = get_ticks();
                scrypt(password, sizeof(password), salt, sizeof(salt), s->Nfactor, s->rfactor, s->pfactor, digest, sizeof(digest));
                ticks = get_ticks() - ticks;

                // Update minimum ticks
                if (ticks < minticks)
                    minticks = ticks;
            }

            // Output results
            printf("%s, %.0f ticks\n", s->desc, (double)minticks);
        }

        break; // Exit the loop (modify as needed for further iterations)
    }

    return 0;
}
