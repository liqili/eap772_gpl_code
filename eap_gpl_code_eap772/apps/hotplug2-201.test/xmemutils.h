#ifndef XMEMUTILS_H
#define XMEMUTILS_H 1

static inline void *xmalloc(size_t);
static inline void *xrealloc(void *, size_t);

/**
 * A malloc wrapper. Exits if no memory.
 *
 * @1 Ammount of memory to allocate
 *
 * Returns: Pointer to freshly allocated memory
 */
static inline void *xmalloc(size_t size) {
        void *ptr;
        ptr = malloc(size);
        if (ptr == NULL) {
                fprintf(stderr, "MALLOC FAILURE!\n");
                exit(127);
        }
        return ptr;
}

/**
 * A realloc wrapper. Exits if no memory.
 *
 * @1 Old pointer
 * @2 Ammount of memory to allocate
 *
 * Returns: Pointer to reallocated memory
 */
static inline void *xrealloc(void *inptr, size_t size) {
        void *ptr;
        ptr = realloc(inptr, size);
        if (ptr == NULL) {
                fprintf(stderr, "MALLOC FAILURE!\n");
                exit(127);
        }
        return ptr;
}

#endif /* XMEMUTILS */
