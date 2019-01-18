// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/******************************************************************************
 * libKD
 * zlib/libpng License
 ******************************************************************************
 * Copyright (c) 2014-2018 Kevin Schmidt
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 ******************************************************************************/

/******************************************************************************
 * KD includes
 ******************************************************************************/

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpadded"
#if __has_warning("-Wreserved-id-macro")
#pragma clang diagnostic ignored "-Wreserved-id-macro"
#endif
#endif
#if defined(__linux__) || defined(__EMSCRIPTEN__)
#define _GNU_SOURCE /* O_CLOEXEC */
#endif
#include "kdplatform.h"
#include <KD/kd.h>
#include <KD/kdext.h>
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#include "kd_internal.h"

/******************************************************************************
 * Memory allocation
 *
 * Notes:
 * - Based on DLMALLOC by Doug Lea
 ******************************************************************************/
/******************************************************************************
 * This is a version (aka dlmalloc) of malloc/free/realloc written by
 * Doug Lea and released to the public domain, as explained at
 * http://creativecommons.org/publicdomain/zero/1.0/ Send questions,
 * comments, complaints, performance data, etc to dl@cs.oswego.edu
 ******************************************************************************/

/* Vital statistics:

  Supported pointer/KDsize representation:       4 or 8 bytes
       KDsize MUST be an unsigned type of the same width as
       pointers. (If you are using an ancient system that declares
       KDsize as a signed type, or need it to be a different width
       than pointers, you can use a previous release of this malloc
       (e.g. 2.7.2) supporting these.)

  Alignment:                                     8 bytes (minimum)
       This suffices for nearly all current machines and C compilers.
       However, you can define MALLOC_ALIGNMENT to be wider than this
       if necessary (up to 128bytes), at the expense of using more space.

  Minimum overhead per allocated chunk:   4 or  8 bytes (if 4byte sizes)
                                          8 or 16 bytes (if 8byte sizes)
       Each malloced chunk has a hidden word of overhead holding size
       and status information, and additional cross-check word
       if FOOTERS is defined.

  Minimum allocated size: 4-byte ptrs:  16 bytes    (including overhead)
                          8-byte ptrs:  32 bytes    (including overhead)

       Even a request for zero bytes (i.e., malloc(0)) returns a
       pointer to something of the minimum allocatable size.
       The maximum overhead wastage (i.e., number of extra bytes
       allocated than were requested in malloc) is less than or equal
       to the minimum size, except for requests >= mmap_threshold that
       are serviced via mmap(), where the worst case wastage is about
       32 bytes plus the remainder from a system page (the minimal
       mmap unit); typically 4096 or 8192 bytes.

  Security: static-safe; optionally more or less
       The "security" of malloc refers to the ability of malicious
       code to accentuate the effects of errors (for example, freeing
       space that is not currently malloc'ed or overwriting past the
       ends of chunks) in code that calls malloc.  This malloc
       guarantees not to modify any memory locations below the base of
       heap, i.e., static variables, even in the presence of usage
       errors.  The routines additionally detect most improper frees
       and reallocs.  All this holds as long as the static bookkeeping
       for malloc itself is not corrupted by some other means.  This
       is only one aspect of security -- these checks do not, and
       cannot, detect all possible programming errors.

       If FOOTERS is defined nonzero, then each allocated chunk
       carries an additional check word to verify that it was malloced
       from its space.  These check words are the same within each
       execution of a program using malloc, but differ across
       executions, so externally crafted fake chunks cannot be
       freed. This improves security by rejecting frees/reallocs that
       could corrupt heap memory, in addition to the checks preventing
       writes to statics that are always on.  This may further improve
       security at the expense of time and space overhead.

       By default detected errors cause the program to abort (calling
       "abort()"). You can override this to instead proceed past
       errors by defining PROCEED_ON_ERROR.  In this case, a bad free
       has no effect, and a malloc that encounters a bad address
       caused by user overwrites will ignore the bad address by
       dropping pointers and indices to all known memory. This may
       be appropriate for programs that should continue if at all
       possible in the face of programming errors, although they may
       run out of memory because dropped memory is never reclaimed.

       If you don't like either of these options, you can define
       CORRUPTION_ERROR_ACTION and USAGE_ERROR_ACTION to do anything
       else. And if if you are sure that your program using malloc has
       no errors or vulnerabilities, you can define INSECURE to 1,
       which might (or might not) provide a small performance improvement.

       It is also possible to limit the maximum total allocatable
       space, using malloc_set_footprint_limit. This is not
       designed as a security feature in itself (calls to set limits
       are not screened or privileged), but may be useful as one
       aspect of a secure implementation.

  Compliance: I believe it is compliant with the Single Unix Specification
       (See http://www.unix.org). Also SVID/XPG, ANSI C, and probably
       others as well.

* Overview of algorithms

  This is not the fastest, most space-conserving, most portable, or
  most tunable malloc ever written. However it is among the fastest
  while also being among the most space-conserving, portable and
  tunable.  Consistent balance across these factors results in a good
  general-purpose allocator for malloc-intensive programs.

  In most ways, this malloc is a best-fit allocator. Generally, it
  chooses the best-fitting existing chunk for a request, with ties
  broken in approximately least-recently-used order. (This strategy
  normally maintains low fragmentation.) However, for requests less
  than 256bytes, it deviates from best-fit when there is not an
  exactly fitting available chunk by preferring to use space adjacent
  to that used for the previous small request, as well as by breaking
  ties in approximately most-recently-used order. (These enhance
  locality of series of small allocations.)  And for very large requests
  (>= 256Kb by default), it relies on system memory mapping
  facilities, if supported.  (This helps avoid carrying around and
  possibly fragmenting memory used only for large chunks.)

  All operations (except malloc_stats and mallinfo) have execution
  times that are bounded by a constant factor of the number of bits in
  a KDsize, not counting any clearing in calloc or copying in realloc,
  or actions surrounding MMAP that have times
  proportional to the number of non-contiguous regions returned by
  system allocation routines, which is often just 1. In real-time
  applications, you can optionally suppress segment traversals using
  NO_SEGMENT_TRAVERSAL, which assures bounded execution even when
  system allocators return non-contiguous spaces, at the typical
  expense of carrying around more memory and increased fragmentation.

  The implementation is not very modular and seriously overuses
  macros. Perhaps someday all C compilers will do as good a job
  inlining modular code as can now be done by brute-force expansion,
  but now, enough of them seem not to.

  Some compilers issue a lot of warnings about code that is
  dead/unreachable only on some platforms, and also about intentional
  uses of negation on unsigned types. All known cases of each can be
  ignored.

  For a longer but out of date high-level description, see
     http://gee.cs.oswego.edu/dl/html/malloc.html

 -------------------------  Compile-time options ---------------------------

Be careful in setting #define values for numerical constants of type
KDsize. On some systems, literal values are not automatically extended
to KDsize precision unless they are explicitly casted. You can also
use the symbolic values MAX_SIZE_T, SIZE_T_ONE, etc below.

WIN32                    default: defined if _WIN32 defined
  Defining WIN32 sets up defaults for MS environment and compilers.
  Otherwise defaults are for unix. Beware that there seem to be some
  cases where this malloc might not be a pure drop-in replacement for
  Win32 malloc: Random-looking failures from Win32 GDI API's (eg;
  SetDIBits()) may be due to bugs in some video driver implementations
  when pixel buffers are malloc()ed, and the region spans more than
  one VirtualAlloc()ed region. Because dlmalloc uses a small (64Kb)
  default granularity, pixel buffers may straddle virtual allocation
  regions more often than when using the Microsoft allocator.  You can
  avoid this by using VirtualAlloc() and VirtualFree() for all pixel
  buffers rather than using malloc().  If this is not possible,
  recompile this malloc with a larger DEFAULT_GRANULARITY. Note:
  in cases where MSC and gcc (cygwin) are known to differ on WIN32,
  conditions use _MSC_VER to distinguish them.

MALLOC_ALIGNMENT         default: (KDsize)(2 * sizeof(void *))
  Controls the minimum alignment for malloc'ed chunks.  It must be a
  power of two and at least 8, even on machines for which smaller
  alignments would suffice. It may be defined as larger than this
  though. Note however that code and data structures are optimized for
  the case of 8-byte alignment.

FOOTERS                  default: 0
  If true, provide extra checking and dispatching by placing
  information in the footers of allocated chunks. This adds
  space and time overhead.

INSECURE                 default: 0
  If true, omit checks for usage errors and heap space overwrites.

ABORT                    default: defined as abort()
  Defines how to abort on failed checks.  On most systems, a failed
  check cannot die with an "assert" or even print an informative
  message, because the underlying print routines in turn call malloc,
  which will fail again.  Generally, the best policy is to simply call
  abort(). It's not very useful to do more than this because many
  errors due to overwriting will show up as address faults (null, odd
  addresses etc) rather than malloc-triggered checks, so will also
  abort.  Also, most compilers know that abort() does not return, so
  can better optimize code conditionally calling it.

PROCEED_ON_ERROR           default: defined as 0 (false)
  Controls whether detected bad addresses cause them to bypassed
  rather than aborting. If set, detected bad arguments to free and
  realloc are ignored. And all bookkeeping information is zeroed out
  upon a detected overwrite of freed heap space, thus losing the
  ability to ever return it from malloc again, but enabling the
  application to proceed. If PROCEED_ON_ERROR is defined, the
  static variable malloc_corruption_error_count is compiled in
  and can be examined to see if errors have occurred. This option
  generates slower code than the default abort policy.

DEBUG                    default: NOT defined
  The DEBUG setting is mainly intended for people trying to modify
  this code or diagnose problems when porting to new platforms.
  However, it may also be able to better isolate user errors than just
  using runtime checks.  The assertions in the check routines spell
  out in more detail the assumptions and invariants underlying the
  algorithms.  The checking is fairly extensive, and will slow down
  execution noticeably. Calling malloc_stats or mallinfo with DEBUG
  set will attempt to check every non-mmapped allocated and free chunk
  in the course of computing the summaries.

ABORT_ON_ASSERT_FAILURE   default: defined as 1 (true)
  Debugging assertion failures can be nearly impossible if your
  version of the assert macro causes malloc to be called, which will
  lead to a cascade of further failures, blowing the runtime stack.
  ABORT_ON_ASSERT_FAILURE cause assertions failures to call abort(),
  which will usually make debugging easier.

MALLOC_FAILURE_ACTION     default: sets errno to ENOMEM, or no-op on win32
  The action to take before "return 0" when malloc fails to be able to
  return memory because there is none available.

NO_SEGMENT_TRAVERSAL       default: 0
  If non-zero, suppresses traversals of memory segments
  returned by CALL_MMAP. This disables
  merging of segments that are contiguous, and selectively
  releasing them to the OS if unused, but bounds execution times.

HAVE_MMAP                 default: 1 (true)
  True if this system supports mmap or an emulation of it.  
  MMAP is used for all system allocation. 
  Note: A single call to MUNMAP is assumed to be
  able to unmap memory that may have be allocated using multiple calls
  to MMAP, so long as they are adjacent.

HAVE_MREMAP               default: 1 on linux, else 0
  If true realloc() uses mremap() to re-allocate large blocks and
  extend or shrink allocation spaces.

MMAP_CLEARS               default: 1 except on WINCE.
  True if mmap clears memory so calloc doesn't need to. This is true
  for standard unix mmap using /dev/zero and on WIN32 except for WINCE.

malloc_getpagesize         default: derive from system includes, or 4096.
  The system page size. To the extent possible, this malloc manages
  memory from the system in page-size units.  This may be (and
  usually is) a function rather than a constant. This is ignored
  if WIN32, where page size is determined using getSystemInfo during
  initialization.

REALLOC_ZERO_BYTES_FREES    default: not defined
  This should be set if a call to realloc with zero bytes should
  be the same as a call to free. Some people think it should. Otherwise,
  since this malloc returns a unique pointer for malloc(0), so does
  realloc(p, 0).

DEFAULT_GRANULARITY        default:
                                system_info.dwAllocationGranularity in WIN32,
                                otherwise 64K.
  The unit for allocating and deallocating memory from the system.  On
  most systems with contiguous MORECORE, there is no reason to
  make this more than a page. However, systems with MMAP tend to
  either require or encourage larger granularities.  You can increase
  this value to prevent system allocation functions to be called so
  often, especially if they are slow.  The value must be at least one
  page and must be a power of two.  Setting to 0 causes initialization
  to either page size or win32 region size.  (Note: In previous
  versions of malloc, the equivalent of this option was called
  "TOP_PAD")

DEFAULT_TRIM_THRESHOLD    default: 2MB
      Also settable using mallopt(M_TRIM_THRESHOLD, x)
  The maximum amount of unused top-most memory to keep before
  releasing via malloc_trim in free().  Automatic trimming is mainly
  useful in long-lived programs using contiguous MORECORE.  Because
  trimming via sbrk can be slow on some systems, and can sometimes be
  wasteful (in cases where programs immediately afterward allocate
  more large chunks) the value should be high enough so that your
  overall system performance would improve by releasing this much
  memory.  As a rough guide, you might set to a value close to the
  average size of a process (program) running on your system.
  Releasing this much memory would allow such a process to run in
  memory.  Generally, it is worth tuning trim thresholds when a
  program undergoes phases where several large chunks are allocated
  and released in ways that can reuse each other's storage, perhaps
  mixed with phases where there are no such chunks at all. The trim
  value must be greater than page size to have any useful effect.  To
  disable trimming completely, you can set to MAX_SIZE_T. Note that the trick
  some people use of mallocing a huge space and then freeing it at
  program startup, in an attempt to reserve system memory, doesn't
  have the intended effect under automatic trimming, since that memory
  will immediately be returned to the system.

DEFAULT_MMAP_THRESHOLD       default: 256K
      Also settable using mallopt(M_MMAP_THRESHOLD, x)
  The request size threshold for using MMAP to directly service a
  request. Requests of at least this size that cannot be allocated
  using already-existing space will be serviced via mmap.  (If enough
  normal freed space already exists it is used instead.)  Using mmap
  segregates relatively large chunks of memory so that they can be
  individually obtained and released from the host system. A request
  serviced through mmap is never reused by any other request (at least
  not directly; the system may just so happen to remap successive
  requests to the same locations).  Segregating space in this way has
  the benefits that: Mmapped space can always be individually released
  back to the system, which helps keep the system level memory demands
  of a long-lived program low.  Also, mapped memory doesn't become
  `locked' between other chunks, as can happen with normally allocated
  chunks, which means that even trimming via malloc_trim would not
  release them.  However, it has the disadvantage that the space
  cannot be reclaimed, consolidated, and then used to service later
  requests, as happens with normal chunks.  The advantages of mmap
  nearly always outweigh disadvantages for "large" chunks, but the
  value of "large" may vary across systems.  The default is an
  empirically derived value that works well in most systems. You can
  disable mmap by setting to MAX_SIZE_T.

MAX_RELEASE_CHECK_RATE   default: 4095 unless not HAVE_MMAP
  The number of consolidated frees between checks to release
  unused segments when freeing. When using non-contiguous segments,
  especially with multiple mspaces, checking only for topmost space
  doesn't always suffice to trigger trimming. To compensate for this,
  free() will, with a period of MAX_RELEASE_CHECK_RATE (or the
  current number of segments, if greater) try to release unused
  segments to the OS when freeing chunks that result in
  consolidation. The best value for this parameter is a compromise
  between slowing down frees with relatively costly checks that
  rarely trigger versus holding on to unused memory. To effectively
  disable, set to MAX_SIZE_T. This may lead to a very slight speed
  improvement at the expense of carrying around more memory.
*/

#if defined(_MSC_VER)
#pragma warning(disable : 4146)
#pragma warning(disable : 6001)
#pragma warning(disable : 6011)
#pragma warning(disable : 6102)
#pragma warning(disable : 6239)
#pragma warning(disable : 6285)
#pragma warning(disable : 6297)
#pragma warning(disable : 6326)
#pragma warning(disable : 28112)
#pragma warning(disable : 28159)
#pragma warning(disable : 28182)
#elif defined(__clang__)
#pragma clang diagnostic ignored "-Wcast-align"
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wpadded"
#pragma clang diagnostic ignored "-Wunreachable-code"
#pragma clang diagnostic ignored "-Wunreachable-code-break"
#pragma clang diagnostic ignored "-Wunreachable-code-return"
#pragma clang diagnostic ignored "-Wunused-macros"
#if(__clang_major__ > 5)
#if defined(__APPLE__)
#if(__clang_major__ > 9)
#pragma clang diagnostic ignored "-Wnull-pointer-arithmetic"
#endif
#else
#pragma clang diagnostic ignored "-Wnull-pointer-arithmetic"
#endif
#endif
#endif

#ifndef WIN32
#ifdef _WIN32
#define WIN32 1
#endif /* _WIN32 */
#ifdef _WIN32_WCE
#define LACKS_FCNTL_H
#define WIN32 1
#endif /* _WIN32_WCE */
#endif /* WIN32 */
#ifdef WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <tchar.h>
#define HAVE_MMAP 1
#define LACKS_UNISTD_H
#define LACKS_SYS_PARAM_H
#define LACKS_SYS_MMAN_H
#ifndef MMAP_CLEARS
#ifdef _WIN32_WCE /* WINCE reportedly does not clear */
#define MMAP_CLEARS 0
#else
#define MMAP_CLEARS 1
#endif /* _WIN32_WCE */
#endif /*MMAP_CLEARS */
#endif /* WIN32 */

#if defined(DARWIN) || defined(_DARWIN)
/* Mac OSX docs advise not to use sbrk; it seems better to use mmap */
#define HAVE_MMAP 1
/* OSX allocators provide 16 byte alignment */
#ifndef MALLOC_ALIGNMENT
#define MALLOC_ALIGNMENT ((KDsize)16U)
#endif
#endif /* DARWIN */

/* The maximum possible KDsize value has all bits set */
#define MAX_SIZE_T (~(KDsize)0)

#ifndef MALLOC_ALIGNMENT
#define MALLOC_ALIGNMENT ((KDsize)(2 * sizeof(void *)))
#endif /* MALLOC_ALIGNMENT */
#ifndef MALLOC_FAILURE_ACTION
#define MALLOC_FAILURE_ACTION kdSetError(KD_ENOMEM)
#endif /* MALLOC_FAILURE_ACTION */
#ifndef FOOTERS
#define FOOTERS 0
#endif /* FOOTERS */
#ifndef ABORT
#define ABORT kdExit(-1)
#endif /* ABORT */
#ifndef ABORT_ON_ASSERT_FAILURE
#define ABORT_ON_ASSERT_FAILURE 1
#endif /* ABORT_ON_ASSERT_FAILURE */
#ifndef PROCEED_ON_ERROR
#define PROCEED_ON_ERROR 0
#endif /* PROCEED_ON_ERROR */

#ifndef INSECURE
#define INSECURE 0
#endif /* INSECURE */
#ifndef HAVE_MMAP
#define HAVE_MMAP 1
#endif /* HAVE_MMAP */
#ifndef MMAP_CLEARS
#define MMAP_CLEARS 1
#endif /* MMAP_CLEARS */
#ifndef HAVE_MREMAP
#if defined(__linux__) || defined(__EMSCRIPTEN__)
#define HAVE_MREMAP 1
#else /* linux */
#define HAVE_MREMAP 0
#endif /* linux */
#endif /* HAVE_MREMAP */
#ifndef DEFAULT_GRANULARITY
#if defined(WIN32)
#define DEFAULT_GRANULARITY (0) /* 0 means to compute in init_mparams */
#else
#define DEFAULT_GRANULARITY ((KDsize)64U * (KDsize)1024U)
#endif
#endif /* DEFAULT_GRANULARITY */
#ifndef DEFAULT_TRIM_THRESHOLD
#define DEFAULT_TRIM_THRESHOLD ((KDsize)2U * (KDsize)1024U * (KDsize)1024U)
#endif /* DEFAULT_TRIM_THRESHOLD */
#ifndef DEFAULT_MMAP_THRESHOLD
#if HAVE_MMAP
#define DEFAULT_MMAP_THRESHOLD ((KDsize)256U * (KDsize)1024U)
#else /* HAVE_MMAP */
#define DEFAULT_MMAP_THRESHOLD MAX_SIZE_T
#endif /* HAVE_MMAP */
#endif /* DEFAULT_MMAP_THRESHOLD */
#ifndef MAX_RELEASE_CHECK_RATE
#if HAVE_MMAP
#define MAX_RELEASE_CHECK_RATE 4095
#else
#define MAX_RELEASE_CHECK_RATE MAX_SIZE_T
#endif /* HAVE_MMAP */
#endif /* MAX_RELEASE_CHECK_RATE */
#ifndef NO_SEGMENT_TRAVERSAL
#define NO_SEGMENT_TRAVERSAL 0
#endif /* NO_SEGMENT_TRAVERSAL */

/*
  mallopt tuning options.  SVID/XPG defines four standard parameter
  numbers for mallopt, normally defined in malloc.h.  None of these
  are used in this malloc, so setting them has no effect. But this
  malloc does support the following options.
*/

#define M_TRIM_THRESHOLD (-1)
#define M_GRANULARITY (-2)
#define M_MMAP_THRESHOLD (-3)

/*
  Try to persuade compilers to inline. The most critical functions for
  inlining are defined as macros, so these aren't used for them.
*/

#ifndef FORCEINLINE
#if defined(__GNUC__)
#define FORCEINLINE __inline __attribute__((always_inline))
#elif defined(_MSC_VER)
#define FORCEINLINE __forceinline
#endif
#endif
#ifndef NOINLINE
#if defined(__GNUC__)
#define NOINLINE __attribute__((noinline))
#elif defined(_MSC_VER)
#define NOINLINE __declspec(noinline)
#else
#define NOINLINE
#endif
#endif

/*------------------------------ internal #includes ---------------------- */

#if HAVE_MMAP
#ifndef LACKS_SYS_MMAN_H
#include <sys/mman.h> /* for mmap */
#endif                /* LACKS_SYS_MMAN_H */
#endif                /* HAVE_MMAP */
#ifndef LACKS_UNISTD_H
#include <unistd.h> /* for sbrk, sysconf */
#endif              /* LACKS_UNISTD_H */

/* Declarations for bit scanning on win32 */
#if defined(_MSC_VER) && _MSC_VER >= 1300
#ifndef BitScanForward /* Try to avoid pulling in WinNT.h */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
unsigned char _BitScanForward(unsigned long *index, unsigned long mask);
unsigned char _BitScanReverse(unsigned long *index, unsigned long mask);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#define BitScanForward _BitScanForward
#define BitScanReverse _BitScanReverse
#pragma intrinsic(_BitScanForward)
#pragma intrinsic(_BitScanReverse)
#endif /* BitScanForward */
#endif /* defined(_MSC_VER) && _MSC_VER>=1300 */

#ifndef WIN32
#ifndef malloc_getpagesize
#ifdef _SC_PAGESIZE /* some SVR4 systems omit an underscore */
#ifndef _SC_PAGE_SIZE
#define _SC_PAGE_SIZE _SC_PAGESIZE
#endif
#endif
#ifdef _SC_PAGE_SIZE
#define malloc_getpagesize sysconf(_SC_PAGE_SIZE)
#else
#if defined(BSD) || defined(DGUX) || defined(HAVE_GETPAGESIZE)
extern KDsize getpagesize();
#define malloc_getpagesize getpagesize()
#else
#ifdef WIN32 /* use supplied emulation of getpagesize */
#define malloc_getpagesize getpagesize()
#else
#ifndef LACKS_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef EXEC_PAGESIZE
#define malloc_getpagesize EXEC_PAGESIZE
#else
#ifdef NBPG
#ifndef CLSIZE
#define malloc_getpagesize NBPG
#else
#define malloc_getpagesize (NBPG * CLSIZE)
#endif
#else
#ifdef NBPC
#define malloc_getpagesize NBPC
#else
#ifdef PAGESIZE
#define malloc_getpagesize PAGESIZE
#else /* just guess */
#define malloc_getpagesize ((KDsize)4096U)
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif
#endif

/* ------------------- KDsize and alignment properties -------------------- */

/* The byte and bit size of a KDsize */
#define SIZE_T_SIZE (sizeof(KDsize))
#define SIZE_T_BITSIZE (sizeof(KDsize) << 3)

/* Some constants coerced to KDsize */
/* Annoying but necessary to avoid errors on some platforms */
#define SIZE_T_ZERO ((KDsize)0)
#define SIZE_T_ONE ((KDsize)1)
#define SIZE_T_TWO ((KDsize)2)
#define SIZE_T_FOUR ((KDsize)4)
#define TWO_SIZE_T_SIZES (SIZE_T_SIZE << 1)
#define FOUR_SIZE_T_SIZES (SIZE_T_SIZE << 2)
#define SIX_SIZE_T_SIZES (FOUR_SIZE_T_SIZES + TWO_SIZE_T_SIZES)
#define HALF_MAX_SIZE_T (MAX_SIZE_T / 2U)

/* The bit mask value corresponding to MALLOC_ALIGNMENT */
#define CHUNK_ALIGN_MASK (MALLOC_ALIGNMENT - SIZE_T_ONE)

/* True if address a has acceptable alignment */
#define is_aligned(A) (((KDsize)((A)) & (CHUNK_ALIGN_MASK)) == 0)

/* the number of bytes to offset an address to align it */
#define align_offset(A)                          \
    ((((KDsize)(A)&CHUNK_ALIGN_MASK) == 0) ? 0 : \
                                             ((MALLOC_ALIGNMENT - ((KDsize)(A)&CHUNK_ALIGN_MASK)) & CHUNK_ALIGN_MASK))

/* -------------------------- MMAP preliminaries ------------------------- */

/* MMAP must return MFAIL on failure */
#define MFAIL ((void *)(MAX_SIZE_T))
#define CMFAIL ((char *)(MFAIL)) /* defined for convenience */

#if HAVE_MMAP

#ifndef WIN32
#define MUNMAP_DEFAULT(a, s) munmap((a), (s))
#define MMAP_PROT (PROT_READ | PROT_WRITE)
#if !defined(MAP_ANONYMOUS) && defined(MAP_ANON)
#define MAP_ANONYMOUS MAP_ANON
#endif /* MAP_ANON */
#ifdef MAP_ANONYMOUS
#define MMAP_FLAGS (MAP_PRIVATE | MAP_ANONYMOUS)
#define MMAP_DEFAULT(s) mmap(0, (s), MMAP_PROT, MMAP_FLAGS, -1, 0)
#else /* MAP_ANONYMOUS */
/*
   Nearly all versions of mmap support MAP_ANONYMOUS, so the following
   is unlikely to be needed, but is supplied just in case.
*/
#define MMAP_FLAGS (MAP_PRIVATE)
static int dev_zero_fd = -1; /* Cached file descriptor for /dev/zero. */
#define MMAP_DEFAULT(s) ((dev_zero_fd < 0) ?                       \
        (dev_zero_fd = open("/dev/zero", O_RDWR | O_CLOEXEC),      \
            mmap(0, (s), MMAP_PROT, MMAP_FLAGS, dev_zero_fd, 0)) : \
        mmap(0, (s), MMAP_PROT, MMAP_FLAGS, dev_zero_fd, 0))
#endif /* MAP_ANONYMOUS */

#define DIRECT_MMAP_DEFAULT(s) MMAP_DEFAULT(s)

#else /* WIN32 */

/* Win32 MMAP via VirtualAlloc */
static void *win32mmap(KDsize size)
{
    void *ptr = VirtualAlloc(0, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    return (ptr != 0) ? ptr : MFAIL;
}

/* For direct MMAP, use MEM_TOP_DOWN to minimize interference */
static void *win32direct_mmap(KDsize size)
{
    void *ptr = VirtualAlloc(0, size, MEM_RESERVE | MEM_COMMIT | MEM_TOP_DOWN,
        PAGE_READWRITE);
    return (ptr != 0) ? ptr : MFAIL;
}

/* This function supports releasing coalesed segments */
static int win32munmap(void *ptr, KDsize size)
{
    MEMORY_BASIC_INFORMATION minfo;
    char *cptr = (char *)ptr;
    while(size)
    {
        if(VirtualQuery(cptr, &minfo, sizeof(minfo)) == 0)
            return -1;
        if(minfo.BaseAddress != cptr || minfo.AllocationBase != cptr ||
            minfo.State != MEM_COMMIT || minfo.RegionSize > size)
            return -1;
        if(VirtualFree(cptr, 0, MEM_RELEASE) == 0)
            return -1;
        cptr += minfo.RegionSize;
        size -= minfo.RegionSize;
    }
    return 0;
}

#define MMAP_DEFAULT(s) win32mmap(s)
#define MUNMAP_DEFAULT(a, s) win32munmap((a), (s))
#define DIRECT_MMAP_DEFAULT(s) win32direct_mmap(s)
#endif /* WIN32 */
#endif /* HAVE_MMAP */

#if HAVE_MREMAP
#ifndef WIN32
#define MREMAP_DEFAULT(addr, osz, nsz, mv) mremap((addr), (osz), (nsz), (mv))
#endif /* WIN32 */
#endif /* HAVE_MREMAP */

/**
 * Define CALL_MMAP/CALL_MUNMAP/CALL_DIRECT_MMAP
 */
#if HAVE_MMAP
#define USE_MMAP_BIT (SIZE_T_ONE)

#ifdef MMAP
#define CALL_MMAP(s) MMAP(s)
#else /* MMAP */
#define CALL_MMAP(s) MMAP_DEFAULT(s)
#endif /* MMAP */
#ifdef MUNMAP
#define CALL_MUNMAP(a, s) MUNMAP((a), (s))
#else /* MUNMAP */
#define CALL_MUNMAP(a, s) MUNMAP_DEFAULT((a), (s))
#endif /* MUNMAP */
#ifdef DIRECT_MMAP
#define CALL_DIRECT_MMAP(s) DIRECT_MMAP(s)
#else /* DIRECT_MMAP */
#define CALL_DIRECT_MMAP(s) DIRECT_MMAP_DEFAULT(s)
#endif /* DIRECT_MMAP */
#else  /* HAVE_MMAP */
#define USE_MMAP_BIT (SIZE_T_ZERO)

#define MMAP(s) MFAIL
#define MUNMAP(a, s) (-1)
#define DIRECT_MMAP(s) MFAIL
#define CALL_DIRECT_MMAP(s) DIRECT_MMAP(s)
#define CALL_MMAP(s) MMAP(s)
#define CALL_MUNMAP(a, s) MUNMAP((a), (s))
#endif /* HAVE_MMAP */

/**
 * Define CALL_MREMAP
 */
#if HAVE_MMAP && HAVE_MREMAP
#ifdef MREMAP
#define CALL_MREMAP(addr, osz, nsz, mv) MREMAP((addr), (osz), (nsz), (mv))
#else /* MREMAP */
#define CALL_MREMAP(addr, osz, nsz, mv) MREMAP_DEFAULT((addr), (osz), (nsz), (mv))
#endif /* MREMAP */
#else  /* HAVE_MMAP && HAVE_MREMAP */
#define CALL_MREMAP(addr, osz, nsz, mv) MFAIL
#endif /* HAVE_MMAP && HAVE_MREMAP */

/* mstate bit set if continguous morecore disabled or failed */
#define USE_NONCONTIGUOUS_BIT (4U)

/* segment bit set in create_mspace_with_base */
#define EXTERN_BIT (8U)


/* --------------------------- Lock preliminaries ------------------------ */

/*
  When locks are defined, there is one global lock.

  The global lock_ensures that mparams.magic and other unique
  mparams values are initialized only once.
*/

static KDThreadMutex *malloc_global_mutex;
static KDThreadOnce malloc_global_mutex_status = KD_THREAD_ONCE_INIT;

/* Use spin loop to initialize global lock */
static void init_malloc_global_mutex(void)
{
    static KDThreadMutex staticmutex;
    kdMemset(&staticmutex, 0, sizeof(KDThreadMutex));

    static _KDMutexAttr mutexattr;
    mutexattr.staticmutex = &staticmutex;

    malloc_global_mutex = kdThreadMutexCreate(&mutexattr);
}

/* Common code for all lock types */
#define USE_LOCK_BIT (2U)

/* -----------------------  Chunk representations ------------------------ */

/*
  (The following includes lightly edited explanations by Colin Plumb.)

  The malloc_chunk declaration below is misleading (but accurate and
  necessary).  It declares a "view" into memory allowing access to
  necessary fields at known offsets from a given base.

  Chunks of memory are maintained using a `boundary tag' method as
  originally described by Knuth.  (See the paper by Paul Wilson
  ftp://ftp.cs.utexas.edu/pub/garbage/allocsrv.ps for a survey of such
  techniques.)  Sizes of free chunks are stored both in the front of
  each chunk and at the end.  This makes consolidating fragmented
  chunks into bigger chunks fast.  The head fields also hold bits
  representing whether chunks are free or in use.

  Here are some pictures to make it clearer.  They are "exploded" to
  show that the state of a chunk can be thought of as extending from
  the high 31 bits of the head field of its header through the
  prev_foot and PINUSE_BIT bit of the following chunk header.

  A chunk that's in use looks like:

   chunk-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
           | Size of previous chunk (if P = 0)                             |
           +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ |P|
         | Size of this chunk                                         1| +-+
   mem-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         |                                                               |
         +-                                                             -+
         |                                                               |
         +-                                                             -+
         |                                                               :
         +-      size - sizeof(KDsize) available payload bytes          -+
         :                                                               |
 chunk-> +-                                                             -+
         |                                                               |
         +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ |1|
       | Size of next chunk (may or may not be in use)               | +-+
 mem-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

    And if it's free, it looks like this:

   chunk-> +-                                                             -+
           | User payload (must be in use, or we would have merged!)       |
           +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ |P|
         | Size of this chunk                                         0| +-+
   mem-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         | Next pointer                                                  |
         +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         | Prev pointer                                                  |
         +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         |                                                               :
         +-      size - sizeof(struct chunk) unused bytes               -+
         :                                                               |
 chunk-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         | Size of this chunk                                            |
         +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ |0|
       | Size of next chunk (must be in use, or we would have merged)| +-+
 mem-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
       |                                                               :
       +- User payload                                                -+
       :                                                               |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                                                                     |0|
                                                                     +-+
  Note that since we always merge adjacent free chunks, the chunks
  adjacent to a free chunk must be in use.

  Given a pointer to a chunk (which can be derived trivially from the
  payload pointer) we can, in O(1) time, find out whether the adjacent
  chunks are free, and if so, unlink them from the lists that they
  are on and merge them with the current chunk.

  Chunks always begin on even word boundaries, so the mem portion
  (which is returned to the user) is also on an even word boundary, and
  thus at least double-word aligned.

  The P (PINUSE_BIT) bit, stored in the unused low-order bit of the
  chunk size (which is always a multiple of two words), is an in-use
  bit for the *previous* chunk.  If that bit is *clear*, then the
  word before the current chunk size contains the previous chunk
  size, and can be used to find the front of the previous chunk.
  The very first chunk allocated always has this bit set, preventing
  access to non-existent (or non-owned) memory. If pinuse is set for
  any given chunk, then you CANNOT determine the size of the
  previous chunk, and might even get a memory addressing fault when
  trying to do so.

  The C (CINUSE_BIT) bit, stored in the unused second-lowest bit of
  the chunk size redundantly records whether the current chunk is
  inuse (unless the chunk is mmapped). This redundancy enables usage
  checks within free and realloc, and reduces indirection when freeing
  and consolidating chunks.

  Each freshly allocated chunk must have both cinuse and pinuse set.
  That is, each allocated chunk borders either a previously allocated
  and still in-use chunk, or the base of its memory arena. This is
  ensured by making all allocations from the `lowest' part of any
  found chunk.  Further, no free chunk physically borders another one,
  so each free chunk is known to be preceded and followed by either
  inuse chunks or the ends of memory.

  Note that the `foot' of the current chunk is actually represented
  as the prev_foot of the NEXT chunk. This makes it easier to
  deal with alignments etc but can be very confusing when trying
  to extend or adapt this code.

  The exceptions to all this are

     1. The special chunk `top' is the top-most available chunk (i.e.,
        the one bordering the end of available memory). It is treated
        specially.  Top is never included in any bin, is used only if
        no other chunk is available, and is released back to the
        system if it is very large (see M_TRIM_THRESHOLD).  In effect,
        the top chunk is treated as larger (and thus less well
        fitting) than any other available chunk.  The top chunk
        doesn't update its trailing size field since there is no next
        contiguous chunk that would have to index off it. However,
        space is still allocated for it (TOP_FOOT_SIZE) to enable
        separation or merging when space is extended.

     3. Chunks allocated via mmap, have both cinuse and pinuse bits
        cleared in their head fields.  Because they are allocated
        one-by-one, each must carry its own prev_foot field, which is
        also used to hold the offset this chunk has within its mmapped
        region, which is needed to preserve alignment. Each mmapped
        chunk is trailed by the first two fields of a fake next-chunk
        for sake of usage checks.

*/

struct malloc_chunk {
    KDsize prev_foot;        /* Size of previous chunk (if free).  */
    KDsize head;             /* Size and inuse bits. */
    struct malloc_chunk *fd; /* double links -- used only if free. */
    struct malloc_chunk *bk;
};

typedef struct malloc_chunk mchunk;
typedef struct malloc_chunk *mchunkptr;
typedef struct malloc_chunk *sbinptr; /* The type of bins of chunks */
typedef unsigned int bindex_t;        /* Described below */
typedef unsigned int binmap_t;        /* Described below */
typedef unsigned int flag_t;          /* The type of various bit flag sets */

/* ------------------- Chunks sizes and alignments ----------------------- */

#define MCHUNK_SIZE (sizeof(mchunk))

#if FOOTERS
#define CHUNK_OVERHEAD (TWO_SIZE_T_SIZES)
#else /* FOOTERS */
#define CHUNK_OVERHEAD (SIZE_T_SIZE)
#endif /* FOOTERS */

/* MMapped chunks need a second word of overhead ... */
#define MMAP_CHUNK_OVERHEAD (TWO_SIZE_T_SIZES)
/* ... and additional padding for fake next-chunk at foot */
#define MMAP_FOOT_PAD (FOUR_SIZE_T_SIZES)

/* The smallest size we can malloc is an aligned minimal chunk */
#define MIN_CHUNK_SIZE \
    ((MCHUNK_SIZE + CHUNK_ALIGN_MASK) & ~CHUNK_ALIGN_MASK)

/* conversion from malloc headers to user pointers, and back */
#define chunk2mem(p) ((void *)((char *)(p) + TWO_SIZE_T_SIZES))
#define mem2chunk(mem) ((mchunkptr)((char *)(mem)-TWO_SIZE_T_SIZES))
/* chunk associated with aligned address A */
#define align_as_chunk(A) (mchunkptr)((A) + align_offset(chunk2mem(A)))

/* Bounds on request (not chunk) sizes. */
#define MAX_REQUEST ((-MIN_CHUNK_SIZE) << 2)
#define MIN_REQUEST (MIN_CHUNK_SIZE - CHUNK_OVERHEAD - SIZE_T_ONE)

/* pad request bytes into a usable size */
#define pad_request(req) \
    (((req) + CHUNK_OVERHEAD + CHUNK_ALIGN_MASK) & ~CHUNK_ALIGN_MASK)

/* pad request, checking for minimum (but not maximum) */
#define request2size(req) \
    (((req) < MIN_REQUEST) ? MIN_CHUNK_SIZE : pad_request(req))


/* ------------------ Operations on head and foot fields ----------------- */

/*
  The head field of a chunk is or'ed with PINUSE_BIT when previous
  adjacent chunk in use, and or'ed with CINUSE_BIT if this chunk is in
  use, unless mmapped, in which case both bits are cleared.

  FLAG4_BIT is not used by this malloc, but might be useful in extensions.
*/

#define PINUSE_BIT (SIZE_T_ONE)
#define CINUSE_BIT (SIZE_T_TWO)
#define FLAG4_BIT (SIZE_T_FOUR)
#define INUSE_BITS (PINUSE_BIT | CINUSE_BIT)
#define FLAG_BITS (PINUSE_BIT | CINUSE_BIT | FLAG4_BIT)

/* Head value for fenceposts */
#define FENCEPOST_HEAD (INUSE_BITS | SIZE_T_SIZE)

/* extraction of fields from head words */
#define cinuse(p) ((p)->head & CINUSE_BIT)
#define pinuse(p) ((p)->head & PINUSE_BIT)
#define flag4inuse(p) ((p)->head & FLAG4_BIT)
#define is_inuse(p) (((p)->head & INUSE_BITS) != PINUSE_BIT)
#define is_mmapped(p) (((p)->head & INUSE_BITS) == 0)

#define chunksize(p) ((p)->head & ~(FLAG_BITS))

#define clear_pinuse(p) ((p)->head &= ~PINUSE_BIT)
#define set_flag4(p) ((p)->head |= FLAG4_BIT)
#define clear_flag4(p) ((p)->head &= ~FLAG4_BIT)

/* Treat space at ptr +/- offset as a chunk */
#define chunk_plus_offset(p, s) ((mchunkptr)(((char *)(p)) + (s)))
#define chunk_minus_offset(p, s) ((mchunkptr)(((char *)(p)) - (s)))

/* Ptr to next or previous physical malloc_chunk. */
#define next_chunk(p) ((mchunkptr)(((char *)(p)) + ((p)->head & ~FLAG_BITS)))
#define prev_chunk(p) ((mchunkptr)(((char *)(p)) - ((p)->prev_foot)))

/* extract next chunk's pinuse bit */
#define next_pinuse(p) ((next_chunk(p)->head) & PINUSE_BIT)

/* Get/set size at footer */
#define get_foot(p, s) (((mchunkptr)((char *)(p) + (s)))->prev_foot)
#define set_foot(p, s) (((mchunkptr)((char *)(p) + (s)))->prev_foot = (s))

/* Set size, pinuse bit, and foot */
#define set_size_and_pinuse_of_free_chunk(p, s) \
    ((p)->head = ((s) | PINUSE_BIT), set_foot(p, s))

/* Set size, pinuse bit, foot, and clear next pinuse */
#define set_free_with_pinuse(p, s, n) \
    (clear_pinuse(n), set_size_and_pinuse_of_free_chunk(p, s))

/* Get the internal overhead associated with chunk p */
#define overhead_for(p) \
    (is_mmapped(p) ? MMAP_CHUNK_OVERHEAD : CHUNK_OVERHEAD)

/* Return true if malloced space is not necessarily cleared */
#if MMAP_CLEARS
#define calloc_must_clear(p) (!is_mmapped(p))
#else /* MMAP_CLEARS */
#define calloc_must_clear(p) (1)
#endif /* MMAP_CLEARS */

/* ---------------------- Overlaid data structures ----------------------- */

/*
  When chunks are not in use, they are treated as nodes of either
  lists or trees.

  "Small"  chunks are stored in circular doubly-linked lists, and look
  like this:

    chunk-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Size of previous chunk                            |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    `head:' |             Size of chunk, in bytes                         |P|
      mem-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Forward pointer to next chunk in list             |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Back pointer to previous chunk in list            |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Unused space (may be 0 bytes long)                .
            .                                                               .
            .                                                               |
nextchunk-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    `foot:' |             Size of chunk, in bytes                           |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  Larger chunks are kept in a form of bitwise digital trees (aka
  tries) keyed on chunksizes.  Because malloc_tree_chunks are only for
  free chunks greater than 256 bytes, their size doesn't impose any
  constraints on user chunk sizes.  Each node looks like:

    chunk-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Size of previous chunk                            |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    `head:' |             Size of chunk, in bytes                         |P|
      mem-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Forward pointer to next chunk of same size        |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Back pointer to previous chunk of same size       |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Pointer to left child (child[0])                  |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Pointer to right child (child[1])                 |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Pointer to parent                                 |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             bin index of this chunk                           |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            |             Unused space                                      .
            .                                                               |
nextchunk-> +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    `foot:' |             Size of chunk, in bytes                           |
            +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

  Each tree holding treenodes is a tree of unique chunk sizes.  Chunks
  of the same size are arranged in a circularly-linked list, with only
  the oldest chunk (the next to be used, in our FIFO ordering)
  actually in the tree.  (Tree members are distinguished by a non-null
  parent pointer.)  If a chunk with the same size an an existing node
  is inserted, it is linked off the existing node using pointers that
  work in the same way as fd/bk pointers of small chunks.

  Each tree contains a power of 2 sized range of chunk sizes (the
  smallest is 0x100 <= x < 0x180), which is is divided in half at each
  tree level, with the chunks in the smaller half of the range (0x100
  <= x < 0x140 for the top nose) in the left subtree and the larger
  half (0x140 <= x < 0x180) in the right subtree.  This is, of course,
  done by inspecting individual bits.

  Using these rules, each node's left subtree contains all smaller
  sizes than its right subtree.  However, the node at the root of each
  subtree has no particular ordering relationship to either.  (The
  dividing line between the subtree sizes is based on trie relation.)
  If we remove the last chunk of a given size from the interior of the
  tree, we need to replace it with a leaf node.  The tree ordering
  rules permit a node to be replaced by any leaf below it.

  The smallest chunk in a tree (a common operation in a best-fit
  allocator) can be found by walking a path to the leftmost leaf in
  the tree.  Unlike a usual binary tree, where we follow left child
  pointers until we reach a null, here we follow the right child
  pointer any time the left one is null, until we reach a leaf with
  both child pointers null. The smallest chunk in the tree will be
  somewhere along that path.

  The worst case number of steps to add, find, or remove a node is
  bounded by the number of bits differentiating chunks within
  bins. Under current bin calculations, this ranges from 6 up to 21
  (for 32 bit sizes) or up to 53 (for 64 bit sizes). The typical case
  is of course much better.
*/

struct malloc_tree_chunk;
struct malloc_tree_chunk {
    /* The first four fields must be compatible with malloc_chunk */
    KDsize prev_foot;
    KDsize head;
    struct malloc_tree_chunk *fd;
    struct malloc_tree_chunk *bk;

    struct malloc_tree_chunk *child[2];
    struct malloc_tree_chunk *parent;
    bindex_t index;
};

typedef struct malloc_tree_chunk tchunk;
typedef struct malloc_tree_chunk *tchunkptr;
typedef struct malloc_tree_chunk *tbinptr; /* The type of bins of trees */

/* A little helper macro for trees */
#define leftmost_child(t) ((t)->child[0] != 0 ? (t)->child[0] : (t)->child[1])

/* ----------------------------- Segments -------------------------------- */

/*
  Each malloc space may include non-contiguous segments, held in a
  list headed by an embedded malloc_segment record representing the
  top-most space. Segments also include flags holding properties of
  the space. Large chunks that are directly allocated by mmap are not
  included in this list. They are instead independently created and
  destroyed without otherwise keeping track of them.

  Segment management mainly comes into play for spaces allocated by
  MMAP.  Any call to MMAP might or might not return memory that is
  adjacent to an existing segment.  MORECORE normally contiguously
  extends the current space, so this space is almost always adjacent,
  which is simpler and faster to deal with. (This is why MORECORE is
  used preferentially to MMAP when both are available -- see
  sys_alloc.)  When allocating using MMAP, we don't use any of the
  hinting mechanisms (inconsistently) supported in various
  implementations of unix mmap, or distinguish reserving from
  committing memory. Instead, we just ask for space, and exploit
  contiguity when we get it.  It is probably possible to do
  better than this on some systems, but no general scheme seems
  to be significantly better.

  Management entails a simpler variant of the consolidation scheme
  used for chunks to reduce fragmentation -- new adjacent memory is
  normally prepended or appended to an existing segment. However,
  there are limitations compared to chunk consolidation that mostly
  reflect the fact that segment processing is relatively infrequent
  (occurring only when getting memory from system) and that we
  don't expect to have huge numbers of segments:

  * Segments are not indexed, so traversal requires linear scans.  (It
    would be possible to index these, but is not worth the extra
    overhead and complexity for most programs on most platforms.)
  * New segments are only appended to old ones when holding top-most
    memory; if they cannot be prepended to others, they are held in
    different segments.

  Except for the top-most segment of an mstate, each segment record
  is kept at the tail of its segment. Segments are added by pushing
  segment records onto the list headed by &mstate.seg for the
  containing mstate.

  Segment flags control allocation/merge/deallocation policies:
  * If EXTERN_BIT set, then we did not allocate this segment,
    and so should not try to deallocate or merge with others.
    (This currently holds only for the initial segment passed
    into create_mspace_with_base.)
  * If USE_MMAP_BIT set, the segment may be merged with
    other surrounding mmapped segments and trimmed/de-allocated
    using munmap.
*/

struct malloc_segment {
    char *base;                  /* base address */
    KDsize size;                 /* allocated size */
    struct malloc_segment *next; /* ptr to next segment */
    flag_t sflags;               /* mmap and extern flag */
};

#define is_mmapped_segment(S) ((S)->sflags & USE_MMAP_BIT)
#define is_extern_segment(S) ((S)->sflags & EXTERN_BIT)

typedef struct malloc_segment msegment;
typedef struct malloc_segment *msegmentptr;

/* ---------------------------- malloc_state ----------------------------- */

/*
   A malloc_state holds all of the bookkeeping for a space.
   The main fields are:

  Top
    The topmost chunk of the currently active segment. Its size is
    cached in topsize.  The actual size of topmost space is
    topsize+TOP_FOOT_SIZE, which includes space reserved for adding
    fenceposts and segment records if necessary when getting more
    space from the system.  The size at which to autotrim top is
    cached from mparams in trim_check, except that it is disabled if
    an autotrim fails.

  Designated victim (dv)
    This is the preferred chunk for servicing small requests that
    don't have exact fits.  It is normally the chunk split off most
    recently to service another small request.  Its size is cached in
    dvsize. The link fields of this chunk are not maintained since it
    is not kept in a bin.

  SmallBins
    An array of bin headers for free chunks.  These bins hold chunks
    with sizes less than MIN_LARGE_SIZE bytes. Each bin contains
    chunks of all the same size, spaced 8 bytes apart.  To simplify
    use in double-linked lists, each bin header acts as a malloc_chunk
    pointing to the real first node, if it exists (else pointing to
    itself).  This avoids special-casing for headers.  But to avoid
    waste, we allocate only the fd/bk pointers of bins, and then use
    repositioning tricks to treat these as the fields of a chunk.

  TreeBins
    Treebins are pointers to the roots of trees holding a range of
    sizes. There are 2 equally spaced treebins for each power of two
    from TREE_SHIFT to TREE_SHIFT+16. The last bin holds anything
    larger.

  Bin maps
    There is one bit map for small bins ("smallmap") and one for
    treebins ("treemap).  Each bin sets its bit when non-empty, and
    clears the bit when empty.  Bit operations are then used to avoid
    bin-by-bin searching -- nearly all "search" is done without ever
    looking at bins that won't be selected.  The bit maps
    conservatively use 32 bits per map word, even if on 64bit system.
    For a good description of some of the bit-based techniques used
    here, see Henry S. Warren Jr's book "Hacker's Delight" (and
    supplement at http://hackersdelight.org/). Many of these are
    intended to reduce the branchiness of paths through malloc etc, as
    well as to reduce the number of memory locations read or written.

  Segments
    A list of segments headed by an embedded malloc_segment record
    representing the initial space.

  Address check support
    The least_addr field is the least address ever obtained from MMAP. 
    Attempted frees and reallocs of any address less
    than this are trapped (unless INSECURE is defined).

  Magic tag
    A cross-check field that should always hold same value as mparams.magic.

  Max allowed footprint
    The maximum allowed bytes to allocate from system (zero means no limit)

  Flags
    Bits recording whether to use MMAP or locks

  Statistics
    Each space keeps track of current and maximum system memory
    obtained via MMAP.

  Trim support
    Fields holding the amount of unused topmost memory that should trigger
    trimming, and a counter to force periodic scanning to release unused
    non-topmost segments.

  Extension support
    A void* pointer and a KDsize field that can be used to help implement
    extensions to this malloc.
*/

/* Bin types, widths and sizes */
#define NSMALLBINS (32U)
#define NTREEBINS (32U)
#define SMALLBIN_SHIFT (3U)
#define SMALLBIN_WIDTH (SIZE_T_ONE << SMALLBIN_SHIFT)
#define TREEBIN_SHIFT (8U)
#define MIN_LARGE_SIZE (SIZE_T_ONE << TREEBIN_SHIFT)
#define MAX_SMALL_SIZE (MIN_LARGE_SIZE - SIZE_T_ONE)
#define MAX_SMALL_REQUEST (MAX_SMALL_SIZE - CHUNK_ALIGN_MASK - CHUNK_OVERHEAD)

struct malloc_state {
    binmap_t smallmap;
    binmap_t treemap;
    KDsize dvsize;
    KDsize topsize;
    char *least_addr;
    mchunkptr dv;
    mchunkptr top;
    KDsize trim_check;
    KDsize release_checks;
    KDsize magic;
    mchunkptr smallbins[(NSMALLBINS + 1) * 2];
    tbinptr treebins[NTREEBINS];
    KDsize footprint;
    KDsize max_footprint;
    KDsize footprint_limit; /* zero means no limit */
    flag_t mflags;
    KDThreadMutex *mutex; /* locate lock among fields that rarely change */
    msegment seg;
    void *extp; /* Unused but available for extensions */
    KDsize exts;
};

typedef struct malloc_state *mstate;

/* ------------- Global malloc_state and malloc_params ------------------- */

/*
  malloc_params holds global properties, including those that can be
  dynamically set using mallopt. There is a single instance, mparams,
  initialized in init_mparams. Note that the non-zeroness of "magic"
  also serves as an initialization flag.
*/

struct malloc_params {
    KDsize magic;
    KDsize page_size;
    KDsize granularity;
    KDsize mmap_threshold;
    KDsize trim_threshold;
    flag_t default_mflags;
};

static struct malloc_params mparams;

/* Ensure mparams initialized */
#define ensure_initialization() (void)(mparams.magic != 0 || init_mparams())

/* The global malloc_state used for all non-"mspace" calls */
static struct malloc_state _gm_;
#define gm (&_gm_)
#define is_global(M) ((M) == &_gm_)

#define is_initialized(M) ((M)->top != 0)

/* -------------------------- system alloc setup ------------------------- */

/* Operations on mflags */

#define use_lock(M) ((M)->mflags & USE_LOCK_BIT)
#define enable_lock(M) ((M)->mflags |= USE_LOCK_BIT)
#define disable_lock(M) ((M)->mflags &= ~USE_LOCK_BIT)

#define use_mmap(M) ((M)->mflags & USE_MMAP_BIT)
#define enable_mmap(M) ((M)->mflags |= USE_MMAP_BIT)
#if HAVE_MMAP
#define disable_mmap(M) ((M)->mflags &= ~USE_MMAP_BIT)
#else
#define disable_mmap(M)
#endif

#define use_noncontiguous(M) ((M)->mflags & USE_NONCONTIGUOUS_BIT)
#define disable_contiguous(M) ((M)->mflags |= USE_NONCONTIGUOUS_BIT)

#define set_lock(M, L)                     \
    ((M)->mflags = (L) ?                   \
            ((M)->mflags | USE_LOCK_BIT) : \
            ((M)->mflags & ~USE_LOCK_BIT))

/* page-align a size */
#define page_align(S) \
    (((S) + (mparams.page_size - SIZE_T_ONE)) & ~(mparams.page_size - SIZE_T_ONE))

/* granularity-align a size */
#define granularity_align(S) \
    (((S) + (mparams.granularity - SIZE_T_ONE)) & ~(mparams.granularity - SIZE_T_ONE))


/* For mmap, use granularity alignment on windows, else page-align */
#ifdef WIN32
#define mmap_align(S) granularity_align(S)
#else
#define mmap_align(S) page_align(S)
#endif

/* For sys_alloc, enough padding to ensure can malloc request on success */
#define SYS_ALLOC_PADDING (TOP_FOOT_SIZE + MALLOC_ALIGNMENT)

#define is_page_aligned(S) \
    (((KDsize)(S) & (mparams.page_size - SIZE_T_ONE)) == 0)
#define is_granularity_aligned(S) \
    (((KDsize)(S) & (mparams.granularity - SIZE_T_ONE)) == 0)

/*  True if segment S holds address A */
#define segment_holds(S, A) \
    ((char *)(A) >= (S)->base && (char *)(A) < (S)->base + (S)->size)

/* Return segment holding given address */
static msegmentptr segment_holding(mstate m, const char *addr)
{
    msegmentptr sp = &m->seg;
    for(;;)
    {
        if(addr >= sp->base && addr < sp->base + sp->size)
        {
            return sp;
        }
        if((sp = sp->next) == 0)
        {
            return 0;
        }
    }
}

/* Return true if segment contains a segment link */
static int has_segment_link(mstate m, msegmentptr ss)
{
    msegmentptr sp = &m->seg;
    for(;;)
    {
        if((char *)sp >= ss->base && (char *)sp < ss->base + ss->size)
        {
            return 1;
        }
        if((sp = sp->next) == 0)
        {
            return 0;
        }
    }
}

#define should_trim(M, s) ((s) > (M)->trim_check)

/*
  TOP_FOOT_SIZE is padding at the end of a segment, including space
  that may be needed to place segment records and fenceposts when new
  noncontiguous segments are added.
*/
#define TOP_FOOT_SIZE \
    (align_offset(chunk2mem(0)) + pad_request(sizeof(struct malloc_segment)) + MIN_CHUNK_SIZE)


/* -------------------------------  Hooks -------------------------------- */

/*
  PREACTION should be defined to return 0 on success, and nonzero on
  failure. If you are not using locking, you can redefine these to do
  anything you like.
*/

#define PREACTION(M) ((use_lock(M)) ? kdThreadMutexLock((M)->mutex) : 0)
#define POSTACTION(M)                        \
    {                                        \
        if(use_lock(M))                      \
        {                                    \
            kdThreadMutexUnlock((M)->mutex); \
        }                                    \
    }

/*
  CORRUPTION_ERROR_ACTION is triggered upon detected bad addresses.
  USAGE_ERROR_ACTION is triggered on detected bad frees and
  reallocs. The argument p is an address that might have triggered the
  fault. It is ignored by the two predefined actions, but might be
  useful in custom actions that try to help diagnose errors.
*/

#if PROCEED_ON_ERROR

/* A count of the number of corruption errors causing resets */
int malloc_corruption_error_count;

/* default corruption action */
static void reset_on_error(mstate m);

#define CORRUPTION_ERROR_ACTION(m) reset_on_error(m)
#define USAGE_ERROR_ACTION(m, p)

#else /* PROCEED_ON_ERROR */

#ifndef CORRUPTION_ERROR_ACTION
#define CORRUPTION_ERROR_ACTION(m) ABORT
#endif /* CORRUPTION_ERROR_ACTION */

#ifndef USAGE_ERROR_ACTION
#define USAGE_ERROR_ACTION(m, p) ABORT
#endif /* USAGE_ERROR_ACTION */

#endif /* PROCEED_ON_ERROR */


/* -------------------------- Debugging setup ---------------------------- */

#if defined(KD_NDEBUG)

#define check_free_chunk(M, P)
#define check_inuse_chunk(M, P)
#define check_malloced_chunk(M, P, N)
#define check_mmapped_chunk(M, P)
#define check_malloc_state(M)
#define check_top_chunk(M, P)

#else /* KD_NDEBUG */
#define check_free_chunk(M, P) do_check_free_chunk(M, P)
#define check_inuse_chunk(M, P) do_check_inuse_chunk(M, P)
#define check_top_chunk(M, P) do_check_top_chunk(M, P)
#define check_malloced_chunk(M, P, N) do_check_malloced_chunk(M, P, N)
#define check_mmapped_chunk(M, P) do_check_mmapped_chunk(M, P)
#define check_malloc_state(M) do_check_malloc_state(M)

static void do_check_any_chunk(mstate m, mchunkptr p);
static void do_check_top_chunk(mstate m, mchunkptr p);
static void do_check_mmapped_chunk(mstate m, mchunkptr p);
static void do_check_inuse_chunk(mstate m, mchunkptr p);
static void do_check_free_chunk(mstate m, mchunkptr p);
static void do_check_malloced_chunk(mstate m, void *mem, KDsize s);
static void do_check_tree(mstate m, tchunkptr t);
static void do_check_treebin(mstate m, bindex_t i);
static void do_check_smallbin(mstate m, bindex_t i);
static void do_check_malloc_state(mstate m);
static int bin_find(mstate m, mchunkptr x);
static KDsize traverse_and_check(mstate m);
#endif /* KD_NDEBUG */

/* ---------------------------- Indexing Bins ---------------------------- */

#define is_small(s) (((s) >> SMALLBIN_SHIFT) < NSMALLBINS)
#define small_index(s) (bindex_t)((s) >> SMALLBIN_SHIFT)
#define small_index2size(i) (((KDuint64)(i)) << SMALLBIN_SHIFT)
#define MIN_SMALL_INDEX (small_index(MIN_CHUNK_SIZE))

/* addressing by index. See above about smallbin repositioning */
#define smallbin_at(M, i) ((sbinptr)((char *)&((M)->smallbins[(i) << 1])))
#define treebin_at(M, i) (&((M)->treebins[i]))

/* assign tree index for size S to variable I. Use x86 asm if possible  */
#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
#define compute_tree_index(S, I)                                                                  \
    {                                                                                             \
        unsigned int X = (S) >> TREEBIN_SHIFT;                                                    \
        if(X == 0)                                                                                \
        {                                                                                         \
            (I) = 0;                                                                              \
        }                                                                                         \
        else if(X > 0xFFFF)                                                                       \
        {                                                                                         \
            (I) = NTREEBINS - 1;                                                                  \
        }                                                                                         \
        else                                                                                      \
        {                                                                                         \
            unsigned int K = (unsigned)sizeof(X) * __CHAR_BIT__ - 1 - (unsigned)__builtin_clz(X); \
            (I) = (bindex_t)((K << 1) + (((S) >> (K + (TREEBIN_SHIFT - 1)) & 1)));                \
        }                                                                                         \
    }

#elif defined(__INTEL_COMPILER)
#define compute_tree_index(S, I)                                               \
    {                                                                          \
        KDsize X = S >> TREEBIN_SHIFT;                                         \
        if(X == 0)                                                             \
        {                                                                      \
            I = 0;                                                             \
        }                                                                      \
        else if(X > 0xFFFF)                                                    \
        {                                                                      \
            I = NTREEBINS - 1;                                                 \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            unsigned int K = _bit_scan_reverse(X);                             \
            I = (bindex_t)((K << 1) + ((S >> (K + (TREEBIN_SHIFT - 1)) & 1))); \
        }                                                                      \
    }

#elif defined(_MSC_VER) && _MSC_VER >= 1300
#define compute_tree_index(S, I)                                               \
    {                                                                          \
        KDsize X = S >> TREEBIN_SHIFT;                                         \
        if(X == 0)                                                             \
        {                                                                      \
            I = 0;                                                             \
        }                                                                      \
        else if(X > 0xFFFF)                                                    \
        {                                                                      \
            I = NTREEBINS - 1;                                                 \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            unsigned int K;                                                    \
            _BitScanReverse((DWORD *)&K, (DWORD)X);                            \
            I = (bindex_t)((K << 1) + ((S >> (K + (TREEBIN_SHIFT - 1)) & 1))); \
        }                                                                      \
    }

#else /* GNUC */
#define compute_tree_index(S, I)                                   \
    {                                                              \
        KDsize X = S >> TREEBIN_SHIFT;                             \
        if(X == 0)                                                 \
        {                                                          \
            I = 0;                                                 \
        }                                                          \
        else if(X > 0xFFFF)                                        \
        {                                                          \
            I = NTREEBINS - 1;                                     \
        }                                                          \
        else                                                       \
        {                                                          \
            unsigned int Y = (unsigned int)X;                      \
            unsigned int N = ((Y - 0x100) >> 16) & 8;              \
            unsigned int K = (((Y <<= N) - 0x1000) >> 16) & 4;     \
            N += K;                                                \
            N += K = (((Y <<= K) - 0x4000) >> 16) & 2;             \
            K = 14 - N + ((Y <<= K) >> 15);                        \
            I = (K << 1) + ((S >> (K + (TREEBIN_SHIFT - 1)) & 1)); \
        }                                                          \
    }
#endif /* GNUC */

/* Bit representing maximum resolved size in a treebin at i */
#define bit_for_tree_index(i) \
    ((i) == NTREEBINS - 1) ? (SIZE_T_BITSIZE - 1) : (((i) >> 1) + TREEBIN_SHIFT - 2)

/* Shift placing maximum resolved bit in a treebin at i as sign bit */
#define leftshift_for_tree_index(i) \
    (((i) == NTREEBINS - 1) ? 0 :   \
                              ((SIZE_T_BITSIZE - SIZE_T_ONE) - (((i) >> 1) + TREEBIN_SHIFT - 2)))

/* The size of the smallest chunk held in bin with index i */
#define minsize_for_tree_index(i)                   \
    ((SIZE_T_ONE << (((i) >> 1) + TREEBIN_SHIFT)) | \
        (((KDsize)((i)&SIZE_T_ONE)) << (((i) >> 1) + TREEBIN_SHIFT - 1)))


/* ------------------------ Operations on bin maps ----------------------- */

/* bit corresponding to given index */
#define idx2bit(i) ((binmap_t)(1) << (i))

/* Mark/Clear bits with given index */
#define mark_smallmap(M, i) ((M)->smallmap |= idx2bit(i))
#define clear_smallmap(M, i) ((M)->smallmap &= ~idx2bit(i))
#define smallmap_is_marked(M, i) ((M)->smallmap & idx2bit(i))

#define mark_treemap(M, i) ((M)->treemap |= idx2bit(i))
#define clear_treemap(M, i) ((M)->treemap &= ~idx2bit(i))
#define treemap_is_marked(M, i) ((M)->treemap & idx2bit(i))

/* isolate the least set bit of a bitmap */
#define least_bit(x) ((x) & -(x))

/* mask with all bits to left of least bit of x on */
#define left_bits(x) (((x) << 1) | -((x) << 1))

/* mask with all bits to left of or equal to least bit of x on */
#define same_or_left_bits(x) ((x) | -(x))

/* index corresponding to given bit. Use x86 asm if possible */

#if defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
#define compute_bit2idx(X, I)   \
    {                           \
        unsigned int J;         \
        J = __builtin_ctz((X)); \
        (I) = (bindex_t)J;      \
    }

#elif defined(__INTEL_COMPILER)
#define compute_bit2idx(X, I)       \
    {                               \
        unsigned int J;             \
        J = _bit_scan_forward((X)); \
        (I) = (bindex_t)J;          \
    }

#elif defined(_MSC_VER) && _MSC_VER >= 1300
#define compute_bit2idx(X, I)              \
    {                                      \
        unsigned int J;                    \
        _BitScanForward((DWORD *)&J, (X)); \
        (I) = (bindex_t)J;                 \
    }

#else
#define compute_bit2idx(X, I)                \
    {                                        \
        unsigned int Y = (X)-1;              \
        unsigned int K = Y >> (16 - 4) & 16; \
        unsigned int N = K;                  \
        Y >>= K;                             \
        N += K = Y >> (8 - 3) & 8;           \
        Y >>= K;                             \
        N += K = Y >> (4 - 2) & 4;           \
        Y >>= K;                             \
        N += K = Y >> (2 - 1) & 2;           \
        Y >>= K;                             \
        N += K = Y >> (1 - 0) & 1;           \
        Y >>= K;                             \
        (I) = (bindex_t)(N + Y);             \
    }
#endif /* GNUC */


/* ----------------------- Runtime Check Support ------------------------- */

/*
  For security, the main invariant is that malloc/free/etc never
  writes to a static address other than malloc_state, unless static
  malloc_state itself has been corrupted, which cannot occur via
  malloc (because of these checks). In essence this means that we
  believe all pointers, sizes, maps etc held in malloc_state, but
  check all of those linked or offsetted from other embedded data
  structures.  These checks are interspersed with main code in a way
  that tends to minimize their run-time cost.

  When FOOTERS is defined, in addition to range checking, we also
  verify footer fields of inuse chunks, which can be used guarantee
  that the mstate controlling malloc/free is intact.  This is a
  streamlined version of the approach described by William Robertson
  et al in "Run-time Detection of Heap-based Overflows" LISA'03
  http://www.usenix.org/events/lisa03/tech/robertson.html The footer
  of an inuse chunk holds the xor of its mstate and a random seed,
  that is checked upon calls to free() and realloc().  This is
  (probabalistically) unguessable from outside the program, but can be
  computed by any code successfully malloc'ing any chunk, so does not
  itself provide protection against code that has already broken
  security through some other means.  Unlike Robertson et al, we
  always dynamically check addresses of all offset chunks (previous,
  next, etc). This turns out to be cheaper than relying on hashes.
*/

#if !INSECURE
/* Check if address a is at least as high as any from MMAP */
#define ok_address(M, a) ((char *)(a) >= (M)->least_addr)
/* Check if address of next chunk n is higher than base chunk p */
#define ok_next(p, n) ((char *)(p) < (char *)(n))
/* Check if p has inuse status */
#define ok_inuse(p) is_inuse(p)
/* Check if p has its pinuse bit on */
#define ok_pinuse(p) pinuse(p)

#else /* !INSECURE */
#define ok_address(M, a) (1)
#define ok_next(b, n) (1)
#define ok_inuse(p) (1)
#define ok_pinuse(p) (1)
#endif /* !INSECURE */

#if(FOOTERS && !INSECURE)
/* Check if (alleged) mstate m has expected magic field */
#define ok_magic(M) ((M)->magic == mparams.magic)
#else /* (FOOTERS && !INSECURE) */
#define ok_magic(M) (1)
#endif /* (FOOTERS && !INSECURE) */

/* In gcc, use __builtin_expect to minimize impact of checks */
#if !INSECURE
#if defined(__GNUC__) && __GNUC__ >= 3
#define RTCHECK(e) __builtin_expect(e, 1)
#else /* GNUC */
#define RTCHECK(e) (e)
#endif /* GNUC */
#else  /* !INSECURE */
#define RTCHECK(e) (1)
#endif /* !INSECURE */

/* macros to set up inuse chunks with or without footers */

#if !FOOTERS

#define mark_inuse_foot(M, p, s)

/* Macros for setting head/foot of non-mmapped chunks */

/* Set cinuse bit and pinuse bit of next chunk */
#define set_inuse(M, p, s)                                      \
    ((p)->head = (((p)->head & PINUSE_BIT) | (s) | CINUSE_BIT), \
        ((mchunkptr)(((char *)(p)) + (s)))->head |= PINUSE_BIT)

/* Set cinuse and pinuse of this chunk and pinuse of next chunk */
#define set_inuse_and_pinuse(M, p, s)             \
    ((p)->head = ((s) | PINUSE_BIT | CINUSE_BIT), \
        ((mchunkptr)(((char *)(p)) + (s)))->head |= PINUSE_BIT)

/* Set size, cinuse and pinuse bit of this chunk */
#define set_size_and_pinuse_of_inuse_chunk(M, p, s) \
    ((p)->head = ((s) | PINUSE_BIT | CINUSE_BIT))

#else /* FOOTERS */

/* Set foot of inuse chunk to be xor of mstate and seed */
#define mark_inuse_foot(M, p, s) \
    (((mchunkptr)((char *)(p) + (s)))->prev_foot = ((KDsize)(M) ^ mparams.magic))

#define get_mstate_for(p)                \
    ((mstate)(((mchunkptr)((char *)(p) + \
                   (chunksize(p))))      \
                  ->prev_foot ^          \
        mparams.magic))

#define set_inuse(M, p, s)                                        \
    ((p)->head = (((p)->head & PINUSE_BIT) | s | CINUSE_BIT),     \
        (((mchunkptr)(((char *)(p)) + (s)))->head |= PINUSE_BIT), \
        mark_inuse_foot(M, p, s))

#define set_inuse_and_pinuse(M, p, s)                             \
    ((p)->head = (s | PINUSE_BIT | CINUSE_BIT),                   \
        (((mchunkptr)(((char *)(p)) + (s)))->head |= PINUSE_BIT), \
        mark_inuse_foot(M, p, s))

#define set_size_and_pinuse_of_inuse_chunk(M, p, s) \
    ((p)->head = (s | PINUSE_BIT | CINUSE_BIT),     \
        mark_inuse_foot(M, p, s))

#endif /* !FOOTERS */

/* ---------------------------- setting mparams -------------------------- */

/* Initialize mparams */
static int init_mparams(void)
{
    kdThreadOnce(&malloc_global_mutex_status, init_malloc_global_mutex);

    kdThreadMutexLock(malloc_global_mutex);
    if(mparams.magic == 0)
    {
        KDsize magic;
        KDsize psize;
        KDsize gsize;

#ifndef WIN32
        psize = malloc_getpagesize;
        gsize = ((DEFAULT_GRANULARITY != 0) ? DEFAULT_GRANULARITY : psize);
#else  /* WIN32 */
        {
            SYSTEM_INFO system_info;
            GetSystemInfo(&system_info);
            psize = system_info.dwPageSize;
            gsize = ((DEFAULT_GRANULARITY != 0) ?
                    DEFAULT_GRANULARITY :
                    system_info.dwAllocationGranularity);
        }
#endif /* WIN32 */

        /* Sanity-check configuration:
       KDsize must be unsigned and as wide as pointer type.
       ints must be at least 4 bytes.
       alignment must be at least 8.
       Alignment, min chunk size, and page size must all be powers of 2.
    */
        if((sizeof(KDsize) != sizeof(char *)) ||
            (MAX_SIZE_T < MIN_CHUNK_SIZE) ||
            (sizeof(int) < 4) ||
            (MALLOC_ALIGNMENT < (KDsize)8U) ||
            ((MALLOC_ALIGNMENT & (MALLOC_ALIGNMENT - SIZE_T_ONE)) != 0) ||
            ((MCHUNK_SIZE & (MCHUNK_SIZE - SIZE_T_ONE)) != 0) ||
            ((gsize & (gsize - SIZE_T_ONE)) != 0) ||
            ((psize & (psize - SIZE_T_ONE)) != 0))
        {
            ABORT;
        }
        mparams.granularity = gsize;
        mparams.page_size = psize;
        mparams.mmap_threshold = DEFAULT_MMAP_THRESHOLD;
        mparams.trim_threshold = DEFAULT_TRIM_THRESHOLD;
        mparams.default_mflags = USE_LOCK_BIT | USE_MMAP_BIT | USE_NONCONTIGUOUS_BIT;

        /* Set up lock for main malloc area */
        gm->mflags = mparams.default_mflags;

        static KDThreadMutex staticmutex;
        kdMemset(&staticmutex, 0, sizeof(KDThreadMutex));

        static _KDMutexAttr mutexattr;
        mutexattr.staticmutex = &staticmutex;

        gm->mutex = kdThreadMutexCreate(&mutexattr);

        {
            magic = (KDsize)(kdTime(KD_NULL) ^ (KDsize)0x55555555U);
            magic |= (KDsize)8U;  /* ensure nonzero */
            magic &= ~(KDsize)7U; /* improve chances of fault for bad values */
            /* Until memory modes commonly available, use volatile-write */
            (*(volatile KDsize *)(&(mparams.magic))) = magic;
        }
    }

    kdThreadMutexUnlock(malloc_global_mutex);
    return 1;
}

/* support for mallopt */
static int change_mparam(int param_number, int value)
{
    KDsize val;
    ensure_initialization();
    val = (value == -1) ? MAX_SIZE_T : (KDsize)value;
    switch(param_number)
    {
        case M_TRIM_THRESHOLD:
        {
            mparams.trim_threshold = val;
            return 1;
        }
        case M_GRANULARITY:
        {
            if(val >= mparams.page_size && ((val & (val - 1)) == 0))
            {
                mparams.granularity = val;
                return 1;
            }
            else
            {
                return 0;
            }
        }
        case M_MMAP_THRESHOLD:
        {
            mparams.mmap_threshold = val;
            return 1;
        }
        default:
        {
            return 0;
        }
    }
}

#if !defined(KD_NDEBUG)
/* ------------------------- Debugging Support --------------------------- */

/* Check properties of any chunk, whether free, inuse, mmapped etc  */
static void do_check_any_chunk(mstate m, mchunkptr p)
{
    kdAssert((is_aligned(chunk2mem(p))) || (p->head == FENCEPOST_HEAD));
    kdAssert(ok_address(m, p));
}

/* Check properties of top chunk */
static void do_check_top_chunk(mstate m, mchunkptr p)
{
    msegmentptr sp = segment_holding(m, (char *)p);
    KDsize sz = p->head & ~INUSE_BITS; /* third-lowest bit can be set! */
    kdAssert(sp != 0);
    kdAssert((is_aligned(chunk2mem(p))) || (p->head == FENCEPOST_HEAD));
    kdAssert(ok_address(m, p));
    kdAssert(sz == m->topsize);
    kdAssert(sz > 0);
    if(sp) /* Make static analyzers happy */
    {
        kdAssert(sz == ((sp->base + sp->size) - (char *)p) - TOP_FOOT_SIZE);
    }
    kdAssert(pinuse(p));
    kdAssert(!pinuse(chunk_plus_offset(p, sz)));
}

/* Check properties of (inuse) mmapped chunks */
static void do_check_mmapped_chunk(mstate m, mchunkptr p)
{
    KDsize sz = chunksize(p);
    KDsize len = (sz + (p->prev_foot) + MMAP_FOOT_PAD);
    kdAssert(is_mmapped(p));
    kdAssert(use_mmap(m));
    kdAssert((is_aligned(chunk2mem(p))) || (p->head == FENCEPOST_HEAD));
    kdAssert(ok_address(m, p));
    kdAssert(!is_small(sz));
    kdAssert((len & (mparams.page_size - SIZE_T_ONE)) == 0);
    kdAssert(chunk_plus_offset(p, sz)->head == FENCEPOST_HEAD);
    kdAssert(chunk_plus_offset(p, sz + SIZE_T_SIZE)->head == 0);
}

/* Check properties of inuse chunks */
static void do_check_inuse_chunk(mstate m, mchunkptr p)
{
    do_check_any_chunk(m, p);
    kdAssert(is_inuse(p));
    kdAssert(next_pinuse(p));
    /* If not pinuse and not mmapped, previous chunk has OK offset */
    kdAssert(is_mmapped(p) || pinuse(p) || next_chunk(prev_chunk(p)) == p);
    if(is_mmapped(p))
    {
        do_check_mmapped_chunk(m, p);
    }
}

/* Check properties of free chunks */
static void do_check_free_chunk(mstate m, mchunkptr p)
{
    KDsize sz = chunksize(p);
    mchunkptr next = chunk_plus_offset(p, sz);
    do_check_any_chunk(m, p);
    kdAssert(!is_inuse(p));
    kdAssert(!next_pinuse(p));
    kdAssert(!is_mmapped(p));
    if(p != m->dv && p != m->top)
    {
        if(sz >= MIN_CHUNK_SIZE)
        {
            kdAssert((sz & CHUNK_ALIGN_MASK) == 0);
            kdAssert(is_aligned(chunk2mem(p)));
            kdAssert(next->prev_foot == sz);
            kdAssert(pinuse(p));
            kdAssert(next == m->top || is_inuse(next));
            kdAssert(p->fd->bk == p);
            kdAssert(p->bk->fd == p);
        }
        else /* markers are always of size SIZE_T_SIZE */
        {
            kdAssert(sz == SIZE_T_SIZE);
        }
    }
}

/* Check properties of malloced chunks at the point they are malloced */
static void do_check_malloced_chunk(mstate m, void *mem, KDsize s)
{
    if(mem != 0)
    {
        mchunkptr p = mem2chunk(mem);
        KDsize sz = p->head & ~INUSE_BITS;
        do_check_inuse_chunk(m, p);
        kdAssert((sz & CHUNK_ALIGN_MASK) == 0);
        kdAssert(sz >= MIN_CHUNK_SIZE);
        kdAssert(sz >= s);
        /* unless mmapped, size is less than MIN_CHUNK_SIZE more than request */
        kdAssert(is_mmapped(p) || sz < (s + MIN_CHUNK_SIZE));
    }
}

/* Check a tree and its subtrees.  */
static void do_check_tree(mstate m, tchunkptr t)
{
    tchunkptr head = 0;
    tchunkptr u = t;
    bindex_t tindex = t->index;
    KDsize tsize = chunksize(t);
    bindex_t idx;
    compute_tree_index(tsize, idx);
    kdAssert(tindex == idx);
    kdAssert(tsize >= MIN_LARGE_SIZE);
    kdAssert(tsize >= minsize_for_tree_index(idx));
    kdAssert((idx == NTREEBINS - 1) || (tsize < minsize_for_tree_index((idx + 1))));

    do
    { /* traverse through chain of same-sized nodes */
        do_check_any_chunk(m, ((mchunkptr)u));
        kdAssert(u->index == tindex);
        kdAssert(chunksize(u) == tsize);
        kdAssert(!is_inuse(u));
        kdAssert(!next_pinuse(u));
        kdAssert(u->fd->bk == u);
        kdAssert(u->bk->fd == u);
        if(u->parent == 0)
        {
            kdAssert(u->child[0] == 0);
            kdAssert(u->child[1] == 0);
        }
        else
        {
            kdAssert(head == 0); /* only one node on chain has parent */
            head = u;
            kdAssert(u->parent != u);
            kdAssert(u->parent->child[0] == u ||
                u->parent->child[1] == u ||
                *((tbinptr *)(u->parent)) == u);
            if(u->child[0] != 0)
            {
                kdAssert(u->child[0]->parent == u);
                kdAssert(u->child[0] != u);
                do_check_tree(m, u->child[0]);
            }
            if(u->child[1] != 0)
            {
                kdAssert(u->child[1]->parent == u);
                kdAssert(u->child[1] != u);
                do_check_tree(m, u->child[1]);
            }
            if(u->child[0] != 0 && u->child[1] != 0)
            {
                kdAssert(chunksize(u->child[0]) < chunksize(u->child[1]));
            }
        }
        u = u->fd;
    } while(u != t);
    kdAssert(head != 0);
}

/*  Check all the chunks in a treebin.  */
static void do_check_treebin(mstate m, bindex_t i)
{
    tbinptr *tb = treebin_at(m, i);
    tchunkptr t = *tb;
    int empty = (m->treemap & (1U << i)) == 0;
    if(t == 0)
    {
        kdAssert(empty);
    }
    if(!empty)
    {
        do_check_tree(m, t);
    }
}

/*  Check all the chunks in a smallbin.  */
static void do_check_smallbin(mstate m, bindex_t i)
{
    sbinptr b = smallbin_at(m, i);
    mchunkptr p = b->bk;
    unsigned int empty = (m->smallmap & (1U << i)) == 0;
    if(p == b)
    {
        kdAssert(empty);
    }
    if(!empty)
    {
        for(; p != b; p = p->bk)
        {
            KDsize size = chunksize(p);
            mchunkptr q;
            /* each chunk claims to be free */
            do_check_free_chunk(m, p);
            /* chunk belongs in bin */
            kdAssert(small_index(size) == i);
            kdAssert(p->bk == b || chunksize(p->bk) == chunksize(p));
            /* chunk is followed by an inuse chunk */
            q = next_chunk(p);
            if(q->head != FENCEPOST_HEAD)
            {
                do_check_inuse_chunk(m, q);
            }
        }
    }
}

/* Find x in a bin. Used in other check functions. */
static int bin_find(mstate m, mchunkptr x)
{
    KDsize size = chunksize(x);
    if(is_small(size))
    {
        bindex_t sidx = small_index(size);
        sbinptr b = smallbin_at(m, sidx);
        if(smallmap_is_marked(m, sidx))
        {
            mchunkptr p = b;
            do
            {
                if(p == x)
                {
                    return 1;
                }
            } while((p = p->fd) != b);
        }
    }
    else
    {
        bindex_t tidx;
        compute_tree_index(size, tidx);
        if(treemap_is_marked(m, tidx))
        {
            tchunkptr t = *treebin_at(m, tidx);
            KDsize sizebits = size << leftshift_for_tree_index(tidx);
            while(t != 0 && chunksize(t) != size)
            {
                t = t->child[(sizebits >> (SIZE_T_BITSIZE - SIZE_T_ONE)) & 1];
                sizebits <<= 1;
            }
            if(t != 0)
            {
                tchunkptr u = t;
                do
                {
                    if(u == (tchunkptr)x)
                    {
                        return 1;
                    }
                } while((u = u->fd) != t);
            }
        }
    }
    return 0;
}

/* Traverse each chunk and check it; return total */
static KDsize traverse_and_check(mstate m)
{
    KDsize sum = 0;
    if(is_initialized(m))
    {
        msegmentptr s = &m->seg;
        sum += m->topsize + TOP_FOOT_SIZE;
        while(s != 0)
        {
            mchunkptr q = align_as_chunk(s->base);
            mchunkptr lastq = 0;
            kdAssert(pinuse(q));
            while(segment_holds(s, q) && q != m->top && q->head != FENCEPOST_HEAD)
            {
                sum += chunksize(q);
                if(is_inuse(q))
                {
                    kdAssert(!bin_find(m, q));
                    do_check_inuse_chunk(m, q);
                }
                else
                {
                    kdAssert(q == m->dv || bin_find(m, q));
                    kdAssert(lastq == 0 || is_inuse(lastq)); /* Not 2 consecutive free */
                    do_check_free_chunk(m, q);
                }
                lastq = q;
                q = next_chunk(q);
            }
            s = s->next;
        }
    }
    return sum;
}


/* Check all properties of malloc_state. */
static void do_check_malloc_state(mstate m)
{
    bindex_t i;
    KDsize total;
    /* check bins */
    for(i = 0; i < NSMALLBINS; ++i)
    {
        do_check_smallbin(m, i);
    }
    for(i = 0; i < NTREEBINS; ++i)
    {
        do_check_treebin(m, i);
    }

    if(m->dvsize != 0)
    { /* check dv chunk */
        do_check_any_chunk(m, m->dv);
        kdAssert(m->dvsize == chunksize(m->dv));
        kdAssert(m->dvsize >= MIN_CHUNK_SIZE);
        kdAssert(bin_find(m, m->dv) == 0);
    }

    if(m->top != 0)
    { /* check top chunk */
        do_check_top_chunk(m, m->top);
        /*kdAssert(m->topsize == chunksize(m->top)); redundant */
        kdAssert(m->topsize > 0);
        kdAssert(bin_find(m, m->top) == 0);
    }

    total = traverse_and_check(m);
    kdAssert(total <= m->footprint);
    kdAssert(m->footprint <= m->max_footprint);
}
#endif /* KD_NDEBUG */

/* ----------------------- Operations on smallbins ----------------------- */

/*
  Various forms of linking and unlinking are defined as macros.  Even
  the ones for trees, which are very long but have very short typical
  paths.  This is ugly but reduces reliance on inlining support of
  compilers.
*/

/* Link a free chunk into a smallbin  */
#define insert_small_chunk(M, P, S)            \
    {                                          \
        bindex_t I = small_index(S);           \
        mchunkptr B = smallbin_at(M, I);       \
        mchunkptr F = B;                       \
        kdAssert((S) >= MIN_CHUNK_SIZE);       \
        if(!smallmap_is_marked(M, I))          \
        {                                      \
            mark_smallmap(M, I);               \
        }                                      \
        else if(RTCHECK(ok_address(M, B->fd))) \
        {                                      \
            F = B->fd;                         \
        }                                      \
        else                                   \
        {                                      \
            CORRUPTION_ERROR_ACTION(M);        \
        }                                      \
        B->fd = P;                             \
        F->bk = P;                             \
        (P)->fd = F;                           \
        (P)->bk = B;                           \
    }

/* Unlink a chunk from a smallbin  */
#define unlink_small_chunk(M, P, S)                                                   \
    {                                                                                 \
        mchunkptr F = (P)->fd;                                                        \
        mchunkptr B = (P)->bk;                                                        \
        bindex_t I = small_index((S));                                                \
        kdAssert((P) != B);                                                           \
        kdAssert((P) != F);                                                           \
        kdAssert(chunksize(P) == small_index2size(I));                                \
        if(RTCHECK(F == smallbin_at((M), I) || (ok_address((M), F) && F->bk == (P)))) \
        {                                                                             \
            if(B == F)                                                                \
            {                                                                         \
                clear_smallmap(M, I);                                                 \
            }                                                                         \
            else if(RTCHECK(B == smallbin_at(M, I) ||                                 \
                        (ok_address((M), B) && B->fd == (P))))                        \
            {                                                                         \
                F->bk = B;                                                            \
                B->fd = F;                                                            \
            }                                                                         \
            else                                                                      \
            {                                                                         \
                CORRUPTION_ERROR_ACTION((M));                                         \
            }                                                                         \
        }                                                                             \
        else                                                                          \
        {                                                                             \
            CORRUPTION_ERROR_ACTION((M));                                             \
        }                                                                             \
    }

/* Unlink the first chunk from a smallbin */
#define unlink_first_small_chunk(M, B, P, I)                 \
    {                                                        \
        mchunkptr F = (P)->fd;                               \
        kdAssert((P) != (B));                                \
        kdAssert((P) != F);                                  \
        kdAssert(chunksize((P)) == small_index2size((I)));   \
        if((B) == F)                                         \
        {                                                    \
            clear_smallmap((M), (I));                        \
        }                                                    \
        else if(RTCHECK(ok_address((M), F) && F->bk == (P))) \
        {                                                    \
            F->bk = (B);                                     \
            (B)->fd = F;                                     \
        }                                                    \
        else                                                 \
        {                                                    \
            CORRUPTION_ERROR_ACTION((M));                    \
        }                                                    \
    }

/* Replace dv node, binning the old one */
/* Used only when dvsize known to be small */
#define replace_dv(M, P, S)                   \
    {                                         \
        KDsize DVS = (M)->dvsize;             \
        kdAssert(is_small(DVS));              \
        if(DVS != 0)                          \
        {                                     \
            mchunkptr DV = (M)->dv;           \
            insert_small_chunk((M), DV, DVS); \
        }                                     \
        (M)->dvsize = (S);                    \
        (M)->dv = (P);                        \
    }

/* ------------------------- Operations on trees ------------------------- */

/* Insert chunk into tree */
#define insert_large_chunk(M, X, S)                                                       \
    {                                                                                     \
        tbinptr *H;                                                                       \
        bindex_t I;                                                                       \
        compute_tree_index((S), I);                                                       \
        H = treebin_at(M, I);                                                             \
        (X)->index = I;                                                                   \
        (X)->child[0] = (X)->child[1] = 0;                                                \
        if(!treemap_is_marked(M, I))                                                      \
        {                                                                                 \
            mark_treemap(M, I);                                                           \
            *H = (X);                                                                     \
            (X)->parent = (tchunkptr)H;                                                   \
            (X)->fd = (X)->bk = (X);                                                      \
        }                                                                                 \
        else                                                                              \
        {                                                                                 \
            tchunkptr T = *H;                                                             \
            KDsize K = (S) << leftshift_for_tree_index(I);                                \
            for(;;)                                                                       \
            {                                                                             \
                if(chunksize(T) != (S))                                                   \
                {                                                                         \
                    tchunkptr *C = &(T->child[(K >> (SIZE_T_BITSIZE - SIZE_T_ONE)) & 1]); \
                    K <<= 1;                                                              \
                    if(*C != 0)                                                           \
                    {                                                                     \
                        T = *C;                                                           \
                    }                                                                     \
                    else if(RTCHECK(ok_address(M, C)))                                    \
                    {                                                                     \
                        *C = (X);                                                         \
                        (X)->parent = T;                                                  \
                        (X)->fd = (X)->bk = (X);                                          \
                        break;                                                            \
                    }                                                                     \
                    else                                                                  \
                    {                                                                     \
                        CORRUPTION_ERROR_ACTION(M);                                       \
                        break;                                                            \
                    }                                                                     \
                }                                                                         \
                else                                                                      \
                {                                                                         \
                    tchunkptr F = T->fd;                                                  \
                    if(RTCHECK(ok_address(M, T) && ok_address(M, F)))                     \
                    {                                                                     \
                        T->fd = F->bk = (X);                                              \
                        (X)->fd = F;                                                      \
                        (X)->bk = T;                                                      \
                        (X)->parent = 0;                                                  \
                        break;                                                            \
                    }                                                                     \
                    else                                                                  \
                    {                                                                     \
                        CORRUPTION_ERROR_ACTION(M);                                       \
                        break;                                                            \
                    }                                                                     \
                }                                                                         \
            }                                                                             \
        }                                                                                 \
    }

/*
  Unlink steps:

  1. If x is a chained node, unlink it from its same-sized fd/bk links
     and choose its bk node as its replacement.
  2. If x was the last node of its size, but not a leaf node, it must
     be replaced with a leaf node (not merely one with an open left or
     right), to make sure that lefts and rights of descendents
     correspond properly to bit masks.  We use the rightmost descendent
     of x.  We could use any other leaf, but this is easy to locate and
     tends to counteract removal of leftmosts elsewhere, and so keeps
     paths shorter than minimally guaranteed.  This doesn't loop much
     because on average a node in a tree is near the bottom.
  3. If x is the base of a chain (i.e., has parent links) relink
     x's parent and children to x's replacement (or null if none).
*/

#define unlink_large_chunk(M, X)                                          \
    {                                                                     \
        tchunkptr XP = (X)->parent;                                       \
        tchunkptr R;                                                      \
        if((X)->bk != (X))                                                \
        {                                                                 \
            tchunkptr F = (X)->fd;                                        \
            R = (X)->bk;                                                  \
            if(RTCHECK(ok_address(M, F) && F->bk == (X) && R->fd == (X))) \
            {                                                             \
                F->bk = R;                                                \
                R->fd = F;                                                \
            }                                                             \
            else                                                          \
            {                                                             \
                CORRUPTION_ERROR_ACTION(M);                               \
            }                                                             \
        }                                                                 \
        else                                                              \
        {                                                                 \
            tchunkptr *RP;                                                \
            if(((R = *(RP = &((X)->child[1]))) != 0) ||                   \
                ((R = *(RP = &((X)->child[0]))) != 0))                    \
            {                                                             \
                tchunkptr *CP;                                            \
                while((*(CP = &(R->child[1])) != 0) ||                    \
                    (*(CP = &(R->child[0])) != 0))                        \
                {                                                         \
                    R = *(RP = CP);                                       \
                }                                                         \
                if(RTCHECK(ok_address(M, RP)))                            \
                    *RP = 0;                                              \
                else                                                      \
                {                                                         \
                    CORRUPTION_ERROR_ACTION(M);                           \
                }                                                         \
            }                                                             \
        }                                                                 \
        if(XP != 0)                                                       \
        {                                                                 \
            tbinptr *H = treebin_at(M, (X)->index);                       \
            if((X) == *H)                                                 \
            {                                                             \
                if((*H = R) == 0)                                         \
                    clear_treemap(M, (X)->index);                         \
            }                                                             \
            else if(RTCHECK(ok_address(M, XP)))                           \
            {                                                             \
                if(XP->child[0] == (X))                                   \
                    XP->child[0] = R;                                     \
                else                                                      \
                    XP->child[1] = R;                                     \
            }                                                             \
            else                                                          \
                CORRUPTION_ERROR_ACTION(M);                               \
            if(R != 0)                                                    \
            {                                                             \
                if(RTCHECK(ok_address(M, R)))                             \
                {                                                         \
                    tchunkptr C0, C1;                                     \
                    R->parent = XP;                                       \
                    if((C0 = (X)->child[0]) != 0)                         \
                    {                                                     \
                        if(RTCHECK(ok_address(M, C0)))                    \
                        {                                                 \
                            R->child[0] = C0;                             \
                            C0->parent = R;                               \
                        }                                                 \
                        else                                              \
                            CORRUPTION_ERROR_ACTION(M);                   \
                    }                                                     \
                    if((C1 = (X)->child[1]) != 0)                         \
                    {                                                     \
                        if(RTCHECK(ok_address(M, C1)))                    \
                        {                                                 \
                            R->child[1] = C1;                             \
                            C1->parent = R;                               \
                        }                                                 \
                        else                                              \
                            CORRUPTION_ERROR_ACTION(M);                   \
                    }                                                     \
                }                                                         \
                else                                                      \
                    CORRUPTION_ERROR_ACTION(M);                           \
            }                                                             \
        }                                                                 \
    }

/* Relays to large vs small bin operations */

#define insert_chunk(M, P, S)          \
    if(is_small(S))                    \
    {                                  \
        insert_small_chunk(M, P, S)    \
    }                                  \
    else                               \
    {                                  \
        tchunkptr TP = (tchunkptr)(P); \
        insert_large_chunk(M, TP, S);  \
    }

#define unlink_chunk(M, P, S)          \
    if(is_small(S))                    \
    {                                  \
        unlink_small_chunk(M, P, S)    \
    }                                  \
    else                               \
    {                                  \
        tchunkptr TP = (tchunkptr)(P); \
        unlink_large_chunk(M, TP);     \
    }


/* Relays to internal calls to malloc/free from realloc, memalign etc */
#define internal_malloc(m, b) kdMalloc(b)
#define internal_free(m, mem) kdFree(mem)

/* -----------------------  Direct-mmapping chunks ----------------------- */

/*
  Directly mmapped chunks are set up with an offset to the start of
  the mmapped region stored in the prev_foot field of the chunk. This
  allows reconstruction of the required argument to MUNMAP when freed,
  and also allows adjustment of the returned chunk to meet alignment
  requirements (especially in memalign).
*/

/* Malloc using mmap */
static void *mmap_alloc(mstate m, KDsize nb)
{
    KDsize mmsize = mmap_align(nb + SIX_SIZE_T_SIZES + CHUNK_ALIGN_MASK);
    if(m->footprint_limit != 0)
    {
        KDsize fp = m->footprint + mmsize;
        if(fp <= m->footprint || fp > m->footprint_limit)
        {
            return 0;
        }
    }
    if(mmsize > nb)
    { /* Check for wrap around 0 */
        char *mm = (char *)(CALL_DIRECT_MMAP(mmsize));
        if(mm != CMFAIL)
        {
            KDsize offset = align_offset(chunk2mem(mm));
            KDsize psize = mmsize - offset - MMAP_FOOT_PAD;
            mchunkptr p = (mchunkptr)(mm + offset);
            p->prev_foot = offset;
            p->head = psize;
            mark_inuse_foot(m, p, psize);
            chunk_plus_offset(p, psize)->head = FENCEPOST_HEAD;
            chunk_plus_offset(p, psize + SIZE_T_SIZE)->head = 0;

            if(m->least_addr == 0 || mm < m->least_addr)
            {
                m->least_addr = mm;
            }
            if((m->footprint += mmsize) > m->max_footprint)
            {
                m->max_footprint = m->footprint;
            }
            kdAssert(is_aligned(chunk2mem(p)));
            check_mmapped_chunk(m, p);
            return chunk2mem(p);
        }
    }
    return 0;
}

/* Realloc using mmap */
static mchunkptr mmap_resize(mstate m, mchunkptr oldp, KDsize nb, int flags)
{
    KDsize oldsize = chunksize(oldp);
    (void)flags;     /* placate people compiling -Wunused */
    if(is_small(nb)) /* Can't shrink mmap regions below small size */
    {
        return 0;
    }
    /* Keep old chunk if big enough but not too big */
    if(oldsize >= nb + SIZE_T_SIZE && (oldsize - nb) <= (mparams.granularity << 1))
    {
        return oldp;
    }
    else
    {
        KDsize offset = oldp->prev_foot;
        KDsize oldmmsize = oldsize + offset + MMAP_FOOT_PAD;
        KDsize newmmsize = mmap_align(nb + SIX_SIZE_T_SIZES + CHUNK_ALIGN_MASK);
        char *cp = (char *)CALL_MREMAP((char *)oldp - offset,
            oldmmsize, newmmsize, flags);
        if(cp != CMFAIL)
        {
            mchunkptr newp = (mchunkptr)(cp + offset);
            KDsize psize = newmmsize - offset - MMAP_FOOT_PAD;
            newp->head = psize;
            mark_inuse_foot(m, newp, psize);
            chunk_plus_offset(newp, psize)->head = FENCEPOST_HEAD;
            chunk_plus_offset(newp, psize + SIZE_T_SIZE)->head = 0;

            if(cp < m->least_addr)
            {
                m->least_addr = cp;
            }
            if((m->footprint += newmmsize - oldmmsize) > m->max_footprint)
            {
                m->max_footprint = m->footprint;
            }
            check_mmapped_chunk(m, newp);
            return newp;
        }
    }
    return 0;
}


/* -------------------------- mspace management -------------------------- */

/* Initialize top chunk and its size */
static void init_top(mstate m, mchunkptr p, KDsize psize)
{
    /* Ensure alignment */
    KDsize offset = align_offset(chunk2mem(p));
    p = (mchunkptr)((char *)p + offset);
    psize -= offset;

    m->top = p;
    m->topsize = psize;
    p->head = psize | PINUSE_BIT;
    /* set size of fake trailing chunk holding overhead space only once */
    chunk_plus_offset(p, psize)->head = TOP_FOOT_SIZE;
    m->trim_check = mparams.trim_threshold; /* reset on each update */
}

/* Initialize bins for a new mstate that is otherwise zeroed out */
static void init_bins(mstate m)
{
    /* Establish circular links for smallbins */
    bindex_t i;
    for(i = 0; i < NSMALLBINS; ++i)
    {
        sbinptr bin = smallbin_at(m, i);
        bin->fd = bin->bk = bin;
    }
}

#if PROCEED_ON_ERROR

/* default corruption action */
static void reset_on_error(mstate m)
{
    int i;
    ++malloc_corruption_error_count;
    /* Reinitialize fields to forget about all memory */
    m->smallmap = m->treemap = 0;
    m->dvsize = m->topsize = 0;
    m->seg.base = 0;
    m->seg.size = 0;
    m->seg.next = 0;
    m->top = m->dv = 0;
    for(i = 0; i < NTREEBINS; ++i)
    {
        *treebin_at(m, i) = 0;
    }
    init_bins(m);
}
#endif /* PROCEED_ON_ERROR */

/* Allocate chunk and prepend remainder with chunk in successor base. */
static void *prepend_alloc(mstate m, char *newbase, char *oldbase,
    KDsize nb)
{
    mchunkptr p = align_as_chunk(newbase);
    mchunkptr oldfirst = align_as_chunk(oldbase);
    KDsize psize = (char *)oldfirst - (char *)p;
    mchunkptr q = chunk_plus_offset(p, nb);
    KDsize qsize = psize - nb;
    set_size_and_pinuse_of_inuse_chunk(m, p, nb);

    kdAssert((char *)oldfirst > (char *)q);
    kdAssert(pinuse(oldfirst));
    kdAssert(qsize >= MIN_CHUNK_SIZE);

    /* consolidate remainder with first chunk of old base */
    if(oldfirst == m->top)
    {
        KDsize tsize = m->topsize += qsize;
        m->top = q;
        q->head = tsize | PINUSE_BIT;
        check_top_chunk(m, q);
    }
    else if(oldfirst == m->dv)
    {
        KDsize dsize = m->dvsize += qsize;
        m->dv = q;
        set_size_and_pinuse_of_free_chunk(q, dsize);
    }
    else
    {
        if(!is_inuse(oldfirst))
        {
            KDsize nsize = chunksize(oldfirst);
            unlink_chunk(m, oldfirst, nsize);
            oldfirst = chunk_plus_offset(oldfirst, nsize);
            qsize += nsize;
        }
        set_free_with_pinuse(q, qsize, oldfirst);
        insert_chunk(m, q, qsize);
        check_free_chunk(m, q);
    }

    check_malloced_chunk(m, chunk2mem(p), nb);
    return chunk2mem(p);
}

/* Add a segment to hold a new noncontiguous region */
static void add_segment(mstate m, char *tbase, KDsize tsize, flag_t mmapped)
{
    /* Determine locations and sizes of segment, fenceposts, old top */
    char *old_top = (char *)m->top;
    msegmentptr oldsp = segment_holding(m, old_top);
    char *old_end = oldsp->base + oldsp->size;
    KDsize ssize = pad_request(sizeof(struct malloc_segment));
    char *rawsp = old_end - (ssize + FOUR_SIZE_T_SIZES + CHUNK_ALIGN_MASK);
    KDsize offset = align_offset(chunk2mem(rawsp));
    char *asp = rawsp + offset;
    char *csp = (asp < (old_top + MIN_CHUNK_SIZE)) ? old_top : asp;
    mchunkptr sp = (mchunkptr)csp;
    msegmentptr ss = (msegmentptr)(chunk2mem(sp));
    mchunkptr tnext = chunk_plus_offset(sp, ssize);
    mchunkptr p = tnext;
    int nfences = 0;

    /* reset top to new space */
    init_top(m, (mchunkptr)tbase, tsize - TOP_FOOT_SIZE);

    /* Set up segment record */
    kdAssert(is_aligned(ss));
    set_size_and_pinuse_of_inuse_chunk(m, sp, ssize);
    *ss = m->seg; /* Push current record */
    m->seg.base = tbase;
    m->seg.size = tsize;
    m->seg.sflags = mmapped;
    m->seg.next = ss;

    /* Insert trailing fenceposts */
    for(;;)
    {
        mchunkptr nextp = chunk_plus_offset(p, SIZE_T_SIZE);
        p->head = FENCEPOST_HEAD;
        ++nfences;
        if((char *)(&(nextp->head)) < old_end)
        {
            p = nextp;
        }
        else
        {
            break;
        }
    }
    kdAssert(nfences >= 2);

    /* Insert the rest of old top into a bin as an ordinary free chunk */
    if(csp != old_top)
    {
        mchunkptr q = (mchunkptr)old_top;
        KDsize psize = csp - old_top;
        mchunkptr tn = chunk_plus_offset(q, psize);
        set_free_with_pinuse(q, psize, tn);
        insert_chunk(m, q, psize);
    }

    check_top_chunk(m, m->top);
}

/* -------------------------- System allocation -------------------------- */

/* Get memory from system using MMAP */
static void *sys_alloc(mstate m, KDsize nb)
{
    char *tbase = CMFAIL;
    KDsize tsize = 0;
    flag_t mmap_flag = 0;
    KDsize asize; /* allocation size */

    ensure_initialization();

    /* Directly map large chunks, but only if already initialized */
    if(use_mmap(m) && nb >= mparams.mmap_threshold && m->topsize != 0)
    {
        void *mem = mmap_alloc(m, nb);
        if(mem != 0)
        {
            return mem;
        }
    }

    asize = granularity_align(nb + SYS_ALLOC_PADDING);
    if(asize <= nb)
    {
        return 0; /* wraparound */
    }
    if(m->footprint_limit != 0)
    {
        KDsize fp = m->footprint + asize;
        if(fp <= m->footprint || fp > m->footprint_limit)
        {
            return 0;
        }
    }

    if(HAVE_MMAP && tbase == CMFAIL)
    { /* Try MMAP */
        char *mp = (char *)(CALL_MMAP(asize));
        if(mp != CMFAIL)
        {
            tbase = mp;
            tsize = asize;
            mmap_flag = USE_MMAP_BIT;
        }
    }

    if(tbase != CMFAIL)
    {

        if((m->footprint += tsize) > m->max_footprint)
        {
            m->max_footprint = m->footprint;
        }

        if(!is_initialized(m))
        { /* first-time initialization */
            if(m->least_addr == 0 || tbase < m->least_addr)
            {
                m->least_addr = tbase;
            }
            m->seg.base = tbase;
            m->seg.size = tsize;
            m->seg.sflags = mmap_flag;
            m->magic = mparams.magic;
            m->release_checks = MAX_RELEASE_CHECK_RATE;
            init_bins(m);
            if(is_global(m))
            {
                init_top(m, (mchunkptr)tbase, tsize - TOP_FOOT_SIZE);
            }
            else
            {
                /* Offset top by embedded malloc_state */
                mchunkptr mn = next_chunk(mem2chunk(m));
                init_top(m, mn, (KDsize)((tbase + tsize) - (char *)mn) - TOP_FOOT_SIZE);
            }
        }

        else
        {
            /* Try to merge with an existing segment */
            msegmentptr sp = &m->seg;
            /* Only consider most recent segment if traversal suppressed */
            while(sp != 0 && tbase != sp->base + sp->size)
            {
                sp = (NO_SEGMENT_TRAVERSAL) ? 0 : sp->next;
            }
            if(sp != 0 &&
                !is_extern_segment(sp) &&
                (sp->sflags & USE_MMAP_BIT) == mmap_flag &&
                segment_holds(sp, m->top))
            { /* append */
                sp->size += tsize;
                init_top(m, m->top, m->topsize + tsize);
            }
            else
            {
                if(tbase < m->least_addr)
                {
                    m->least_addr = tbase;
                }
                sp = &m->seg;
                while(sp != 0 && sp->base != tbase + tsize)
                {
                    sp = (NO_SEGMENT_TRAVERSAL) ? 0 : sp->next;
                }
                if(sp != 0 &&
                    !is_extern_segment(sp) &&
                    (sp->sflags & USE_MMAP_BIT) == mmap_flag)
                {
                    char *oldbase = sp->base;
                    sp->base = tbase;
                    sp->size += tsize;
                    return prepend_alloc(m, tbase, oldbase, nb);
                }
                else
                {
                    add_segment(m, tbase, tsize, mmap_flag);
                }
            }
        }

        if(nb < m->topsize)
        { /* Allocate from new or extended top space */
            KDsize rsize = m->topsize -= nb;
            mchunkptr p = m->top;
            mchunkptr r = m->top = chunk_plus_offset(p, nb);
            r->head = rsize | PINUSE_BIT;
            set_size_and_pinuse_of_inuse_chunk(m, p, nb);
            check_top_chunk(m, m->top);
            check_malloced_chunk(m, chunk2mem(p), nb);
            return chunk2mem(p);
        }
    }

    MALLOC_FAILURE_ACTION;
    return 0;
}

/* -----------------------  system deallocation -------------------------- */

/* Unmap and unlink any mmapped segments that don't contain used chunks */
static KDsize release_unused_segments(mstate m)
{
    KDsize released = 0;
    int nsegs = 0;
    msegmentptr pred = &m->seg;
    msegmentptr sp = pred->next;
    while(sp != 0)
    {
        char *base = sp->base;
        KDsize size = sp->size;
        msegmentptr next = sp->next;
        ++nsegs;
        if(is_mmapped_segment(sp) && !is_extern_segment(sp))
        {
            mchunkptr p = align_as_chunk(base);
            KDsize psize = chunksize(p);
            /* Can unmap if first chunk holds entire segment and not pinned */
            if(!is_inuse(p) && (char *)p + psize >= base + size - TOP_FOOT_SIZE)
            {
                tchunkptr tp = (tchunkptr)p;
                kdAssert(segment_holds(sp, (char *)sp));
                if(p == m->dv)
                {
                    m->dv = 0;
                    m->dvsize = 0;
                }
                else
                {
                    unlink_large_chunk(m, tp);
                }
                if(CALL_MUNMAP(base, size) == 0)
                {
                    released += size;
                    m->footprint -= size;
                    /* unlink obsoleted record */
                    sp = pred;
                    sp->next = next;
                }
                else
                { /* back out if cannot unmap */
                    insert_large_chunk(m, tp, psize);
                }
            }
        }
        if(NO_SEGMENT_TRAVERSAL) /* scan only first segment */
        {
            break;
        }
        pred = sp;
        sp = next;
    }
    /* Reset check counter */
    m->release_checks = (((KDsize)nsegs > (KDsize)MAX_RELEASE_CHECK_RATE) ?
            (KDsize)nsegs :
            (KDsize)MAX_RELEASE_CHECK_RATE);
    return released;
}

static int sys_trim(mstate m, KDsize pad)
{
    KDsize released = 0;
    ensure_initialization();
    if(pad < MAX_REQUEST && is_initialized(m))
    {
        pad += TOP_FOOT_SIZE; /* ensure enough room for segment overhead */

        if(m->topsize > pad)
        {
            /* Shrink top space in granularity-size units, keeping at least one */
            KDsize unit = mparams.granularity;
            KDsize extra = ((m->topsize - pad + (unit - SIZE_T_ONE)) / unit - SIZE_T_ONE) * unit;
            msegmentptr sp = segment_holding(m, (char *)m->top);

            if(!is_extern_segment(sp))
            {
                if(is_mmapped_segment(sp))
                {
                    if(HAVE_MMAP && sp->size >= extra && !has_segment_link(m, sp))
                    { /* can't shrink if pinned */
                        KDsize newsize = sp->size - extra;
                        (void)newsize; /* placate people compiling -Wunused-variable */
                        /* Prefer mremap, fall back to munmap */
                        if((CALL_MREMAP(sp->base, sp->size, newsize, 0) != MFAIL) ||
                            (CALL_MUNMAP(sp->base + newsize, extra) == 0))
                        {
                            released = extra;
                        }
                    }
                }
            }

            if(released != 0)
            {
                sp->size -= released;
                m->footprint -= released;
                init_top(m, m->top, m->topsize - released);
                check_top_chunk(m, m->top);
            }
        }

        /* Unmap any unused mmapped segments */
        if(HAVE_MMAP)
        {
            released += release_unused_segments(m);
        }

        /* On failure, disable autotrim to avoid repeated failed future calls */
        if(released == 0 && m->topsize > m->trim_check)
        {
            m->trim_check = MAX_SIZE_T;
        }
    }

    return (released != 0) ? 1 : 0;
}

/* Consolidate and bin a chunk. Differs from exported versions
   of free mainly in that the chunk need not be marked as inuse.
*/
static void dispose_chunk(mstate m, mchunkptr p, KDsize psize)
{
    mchunkptr next = chunk_plus_offset(p, psize);
    if(!pinuse(p))
    {
        mchunkptr prev;
        KDsize prevsize = p->prev_foot;
        if(is_mmapped(p))
        {
            psize += prevsize + MMAP_FOOT_PAD;
            if(CALL_MUNMAP((char *)p - prevsize, psize) == 0)
            {
                m->footprint -= psize;
            }
            return;
        }
        prev = chunk_minus_offset(p, prevsize);
        psize += prevsize;
        p = prev;
        if(RTCHECK(ok_address(m, prev)))
        { /* consolidate backward */
            if(p != m->dv)
            {
                unlink_chunk(m, p, prevsize);
            }
            else if((next->head & INUSE_BITS) == INUSE_BITS)
            {
                m->dvsize = psize;
                set_free_with_pinuse(p, psize, next);
                return;
            }
        }
        else
        {
            CORRUPTION_ERROR_ACTION(m);
            return;
        }
    }
    if(RTCHECK(ok_address(m, next)))
    {
        if(!cinuse(next))
        { /* consolidate forward */
            if(next == m->top)
            {
                KDsize tsize = m->topsize += psize;
                m->top = p;
                p->head = tsize | PINUSE_BIT;
                if(p == m->dv)
                {
                    m->dv = 0;
                    m->dvsize = 0;
                }
                return;
            }
            else if(next == m->dv)
            {
                KDsize dsize = m->dvsize += psize;
                m->dv = p;
                set_size_and_pinuse_of_free_chunk(p, dsize);
                return;
            }
            else
            {
                KDsize nsize = chunksize(next);
                psize += nsize;
                unlink_chunk(m, next, nsize);
                set_size_and_pinuse_of_free_chunk(p, psize);
                if(p == m->dv)
                {
                    m->dvsize = psize;
                    return;
                }
            }
        }
        else
        {
            set_free_with_pinuse(p, psize, next);
        }
        insert_chunk(m, p, psize);
    }
    else
    {
        CORRUPTION_ERROR_ACTION(m);
    }
}

/* ---------------------------- malloc --------------------------- */

/* allocate a large request from the best fitting chunk in a treebin */
static void *tmalloc_large(mstate m, KDsize nb)
{
    tchunkptr v = 0;
    KDsize rsize = -nb; /* Unsigned negation */
    tchunkptr t;
    bindex_t idx;
    compute_tree_index(nb, idx);
    if((t = *treebin_at(m, idx)) != 0)
    {
        /* Traverse tree for this bin looking for node with size == nb */
        KDsize sizebits = nb << leftshift_for_tree_index(idx);
        tchunkptr rst = 0; /* The deepest untaken right subtree */
        for(;;)
        {
            tchunkptr rt;
            KDsize trem = chunksize(t) - nb;
            if(trem < rsize)
            {
                v = t;
                if((rsize = trem) == 0)
                {
                    break;
                }
            }
            rt = t->child[1];
            t = t->child[(sizebits >> (SIZE_T_BITSIZE - SIZE_T_ONE)) & 1];
            if(rt != 0 && rt != t)
            {
                rst = rt;
            }
            if(t == 0)
            {
                t = rst; /* set t to least subtree holding sizes > nb */
                break;
            }
            sizebits <<= 1;
        }
    }
    if(t == 0 && v == 0)
    { /* set t to root of next non-empty treebin */
        binmap_t leftbits = left_bits(idx2bit(idx)) & m->treemap;
        if(leftbits != 0)
        {
            bindex_t i;
            binmap_t leastbit = least_bit(leftbits);
            compute_bit2idx(leastbit, i);
            t = *treebin_at(m, i);
        }
    }

    while(t != 0)
    { /* find smallest of tree or subtree */
        KDsize trem = chunksize(t) - nb;
        if(trem < rsize)
        {
            rsize = trem;
            v = t;
        }
        t = leftmost_child(t);
    }

    /*  If dv is a better fit, return 0 so malloc will use it */
    if(v != 0 && rsize < (KDsize)(m->dvsize - nb))
    {
        if(RTCHECK(ok_address(m, v)))
        { /* split */
            mchunkptr r = chunk_plus_offset(v, nb);
            kdAssert(chunksize(v) == rsize + nb);
            if(RTCHECK(ok_next(v, r)))
            {
                unlink_large_chunk(m, v);
                if(rsize < MIN_CHUNK_SIZE)
                {
                    set_inuse_and_pinuse(m, v, (rsize + nb));
                }
                else
                {
                    set_size_and_pinuse_of_inuse_chunk(m, v, nb);
                    set_size_and_pinuse_of_free_chunk(r, rsize);
                    insert_chunk(m, r, rsize);
                }
                return chunk2mem(v);
            }
        }
        CORRUPTION_ERROR_ACTION(m);
    }
    return 0;
}

/* allocate a small request from the best fitting chunk in a treebin */
static void *tmalloc_small(mstate m, KDsize nb)
{
    tchunkptr t, v;
    KDsize rsize;
    bindex_t i;
    binmap_t leastbit = least_bit(m->treemap);
    compute_bit2idx(leastbit, i);
    v = t = *treebin_at(m, i);
    rsize = chunksize(t) - nb;

    while((t = leftmost_child(t)) != 0)
    {
        KDsize trem = chunksize(t) - nb;
        if(trem < rsize)
        {
            rsize = trem;
            v = t;
        }
    }

    if(RTCHECK(ok_address(m, v)))
    {
        mchunkptr r = chunk_plus_offset(v, nb);
        kdAssert(chunksize(v) == rsize + nb);
        if(RTCHECK(ok_next(v, r)))
        {
            unlink_large_chunk(m, v);
            if(rsize < MIN_CHUNK_SIZE)
            {
                set_inuse_and_pinuse(m, v, (rsize + nb));
            }
            else
            {
                set_size_and_pinuse_of_inuse_chunk(m, v, nb);
                set_size_and_pinuse_of_free_chunk(r, rsize);
                replace_dv(m, r, rsize);
            }
            return chunk2mem(v);
        }
    }

    CORRUPTION_ERROR_ACTION(m);
    return 0;
}

/* kdMalloc: Allocate memory. */
#if defined(__GNUC__) || defined(__clang__)
__attribute__((__malloc__))
#endif
KD_API void *KD_APIENTRY
kdMalloc(KDsize size)
{
    KDsize bytes = size;
    /*
     Basic algorithm:
     If a small request (< 256 bytes minus per-chunk overhead):
       1. If one exists, use a remainderless chunk in associated smallbin.
          (Remainderless means that there are too few excess bytes to
          represent as a chunk.)
       2. If it is big enough, use the dv chunk, which is normally the
          chunk adjacent to the one used for the most recent small request.
       3. If one exists, split the smallest available chunk in a bin,
          saving remainder in dv.
       4. If it is big enough, use the top chunk.
       5. If available, get memory from system and use it
     Otherwise, for a large request:
       1. Find the smallest available binned chunk that fits, and use it
          if it is better fitting than dv chunk, splitting if necessary.
       2. If better fitting than any binned chunk, use the dv chunk.
       3. If it is big enough, use the top chunk.
       4. If request size >= mmap threshold, try to directly mmap this chunk.
       5. If available, get memory from system and use it

     The ugly goto's here ensure that postaction occurs along all paths.
  */

    ensure_initialization(); /* initialize in sys_alloc if not using locks */

    if(!PREACTION(gm))
    {
        void *mem;
        KDsize nb;
        if(bytes <= MAX_SMALL_REQUEST)
        {
            bindex_t idx;
            binmap_t smallbits;
            nb = (bytes < MIN_REQUEST) ? MIN_CHUNK_SIZE : pad_request(bytes);
            idx = small_index(nb);
            smallbits = gm->smallmap >> idx;

            if((smallbits & 0x3U) != 0)
            { /* Remainderless fit to a smallbin. */
                mchunkptr b, p;
                idx += ~smallbits & 1; /* Uses next bin if idx empty */
                b = smallbin_at(gm, idx);
                p = b->fd;
                kdAssert(chunksize(p) == small_index2size(idx));
                unlink_first_small_chunk(gm, b, p, idx);
                set_inuse_and_pinuse(gm, p, small_index2size(idx));
                mem = chunk2mem(p);
                check_malloced_chunk(gm, mem, nb);
                goto postaction;
            }

            else if(nb > gm->dvsize)
            {
                if(smallbits != 0)
                { /* Use chunk in next nonempty smallbin */
                    mchunkptr b, p, r;
                    KDsize rsize;
                    bindex_t i;
                    binmap_t leftbits = (smallbits << idx) & left_bits(idx2bit(idx));
                    binmap_t leastbit = least_bit(leftbits);
                    compute_bit2idx(leastbit, i);
                    b = smallbin_at(gm, i);
                    p = b->fd;
                    kdAssert(chunksize(p) == small_index2size(i));
                    unlink_first_small_chunk(gm, b, p, i);
                    rsize = small_index2size(i) - nb;
                    /* Fit here cannot be remainderless if 4byte sizes */
                    if(SIZE_T_SIZE != 4 && rsize < MIN_CHUNK_SIZE)
                    {
                        set_inuse_and_pinuse(gm, p, small_index2size(i));
                    }
                    else
                    {
                        set_size_and_pinuse_of_inuse_chunk(gm, p, nb);
                        r = chunk_plus_offset(p, nb);
                        set_size_and_pinuse_of_free_chunk(r, rsize);
                        replace_dv(gm, r, rsize);
                    }
                    mem = chunk2mem(p);
                    check_malloced_chunk(gm, mem, nb);
                    goto postaction;
                }

                else if(gm->treemap != 0 && (mem = tmalloc_small(gm, nb)) != 0)
                {
                    check_malloced_chunk(gm, mem, nb);
                    goto postaction;
                }
            }
        }
        else if(bytes >= MAX_REQUEST)
        {
            nb = MAX_SIZE_T; /* Too big to allocate. Force failure (in sys alloc) */
        }
        else
        {
            nb = pad_request(bytes);
            if(gm->treemap != 0 && (mem = tmalloc_large(gm, nb)) != 0)
            {
                check_malloced_chunk(gm, mem, nb);
                goto postaction;
            }
        }

        if(nb <= gm->dvsize)
        {
            KDsize rsize = gm->dvsize - nb;
            mchunkptr p = gm->dv;
            if(rsize >= MIN_CHUNK_SIZE)
            { /* split dv */
                mchunkptr r = gm->dv = chunk_plus_offset(p, nb);
                gm->dvsize = rsize;
                set_size_and_pinuse_of_free_chunk(r, rsize);
                set_size_and_pinuse_of_inuse_chunk(gm, p, nb);
            }
            else
            { /* exhaust dv */
                KDsize dvs = gm->dvsize;
                gm->dvsize = 0;
                gm->dv = 0;
                set_inuse_and_pinuse(gm, p, dvs);
            }
            mem = chunk2mem(p);
            check_malloced_chunk(gm, mem, nb);
            goto postaction;
        }

        else if(nb < gm->topsize)
        { /* Split top */
            KDsize rsize = gm->topsize -= nb;
            mchunkptr p = gm->top;
            mchunkptr r = gm->top = chunk_plus_offset(p, nb);
            r->head = rsize | PINUSE_BIT;
            set_size_and_pinuse_of_inuse_chunk(gm, p, nb);
            mem = chunk2mem(p);
            check_top_chunk(gm, gm->top);
            check_malloced_chunk(gm, mem, nb);
            goto postaction;
        }

        mem = sys_alloc(gm, nb);

    postaction:
        POSTACTION(gm);
        return mem;
    }

    return 0;
}

/* kdFree: Free allocated memory block. */
KD_API void KD_APIENTRY kdFree(void *ptr)
{
    void *mem = ptr;
    /*
     Consolidate freed chunks with preceeding or succeeding bordering
     free chunks, if they exist, and then place in a bin.  Intermixed
     with special cases for top, dv, mmapped chunks, and usage errors.
  */

    if(mem != 0)
    {
        mchunkptr p = mem2chunk(mem);
#if FOOTERS
        mstate fm = get_mstate_for(p);
        if(!ok_magic(fm))
        {
            USAGE_ERROR_ACTION(fm, p);
            return;
        }
#else /* FOOTERS */
#define fm gm
#endif /* FOOTERS */
        if(!PREACTION(fm))
        {
            check_inuse_chunk(fm, p);
            if(RTCHECK(ok_address(fm, p) && ok_inuse(p)))
            {
                KDsize psize = chunksize(p);
                mchunkptr next = chunk_plus_offset(p, psize);
                if(!pinuse(p))
                {
                    KDsize prevsize = p->prev_foot;
                    if(is_mmapped(p))
                    {
                        psize += prevsize + MMAP_FOOT_PAD;
                        if(CALL_MUNMAP((char *)p - prevsize, psize) == 0)
                        {
                            fm->footprint -= psize;
                        }
                        goto postaction;
                    }
                    else
                    {
                        mchunkptr prev = chunk_minus_offset(p, prevsize);
                        psize += prevsize;
                        p = prev;
                        if(RTCHECK(ok_address(fm, prev)))
                        { /* consolidate backward */
                            if(p != fm->dv)
                            {
                                unlink_chunk(fm, p, prevsize);
                            }
                            else if((next->head & INUSE_BITS) == INUSE_BITS)
                            {
                                fm->dvsize = psize;
                                set_free_with_pinuse(p, psize, next);
                                goto postaction;
                            }
                        }
                        else
                        {
                            goto erroraction;
                        }
                    }
                }

                if(RTCHECK(ok_next(p, next) && ok_pinuse(next)))
                {
                    if(!cinuse(next))
                    { /* consolidate forward */
                        if(next == fm->top)
                        {
                            KDsize tsize = fm->topsize += psize;
                            fm->top = p;
                            p->head = tsize | PINUSE_BIT;
                            if(p == fm->dv)
                            {
                                fm->dv = 0;
                                fm->dvsize = 0;
                            }
                            if(should_trim(fm, tsize))
                            {
                                sys_trim(fm, 0);
                            }
                            goto postaction;
                        }
                        else if(next == fm->dv)
                        {
                            KDsize dsize = fm->dvsize += psize;
                            fm->dv = p;
                            set_size_and_pinuse_of_free_chunk(p, dsize);
                            goto postaction;
                        }
                        else
                        {
                            KDsize nsize = chunksize(next);
                            psize += nsize;
                            unlink_chunk(fm, next, nsize);
                            set_size_and_pinuse_of_free_chunk(p, psize);
                            if(p == fm->dv)
                            {
                                fm->dvsize = psize;
                                goto postaction;
                            }
                        }
                    }
                    else
                    {
                        set_free_with_pinuse(p, psize, next);
                    }

                    if(is_small(psize))
                    {
                        insert_small_chunk(fm, p, psize);
                        check_free_chunk(fm, p);
                    }
                    else
                    {
                        tchunkptr tp = (tchunkptr)p;
                        insert_large_chunk(fm, tp, psize);
                        check_free_chunk(fm, p);
                        if(--fm->release_checks == 0)
                        {
                            release_unused_segments(fm);
                        }
                    }
                    goto postaction;
                }
            }
        erroraction:
            USAGE_ERROR_ACTION(fm, p);
        postaction:
            POSTACTION(fm);
        }
    }
#if !FOOTERS
#undef fm
#endif /* FOOTERS */
}

/* ------------ Internal support for realloc, memalign, etc -------------- */

/* Try to realloc; only in-place unless can_move true */
static mchunkptr try_realloc_chunk(mstate m, mchunkptr p, KDsize nb,
    int can_move)
{
    mchunkptr newp = 0;
    KDsize oldsize = chunksize(p);
    mchunkptr next = chunk_plus_offset(p, oldsize);
    if(RTCHECK(ok_address(m, p) && ok_inuse(p) &&
           ok_next(p, next) && ok_pinuse(next)))
    {
        if(is_mmapped(p))
        {
            newp = mmap_resize(m, p, nb, can_move);
        }
        else if(oldsize >= nb)
        { /* already big enough */
            KDsize rsize = oldsize - nb;
            if(rsize >= MIN_CHUNK_SIZE)
            { /* split off remainder */
                mchunkptr r = chunk_plus_offset(p, nb);
                set_inuse(m, p, nb);
                set_inuse(m, r, rsize);
                dispose_chunk(m, r, rsize);
            }
            newp = p;
        }
        else if(next == m->top)
        { /* extend into top */
            if(oldsize + m->topsize > nb)
            {
                KDsize newsize = oldsize + m->topsize;
                KDsize newtopsize = newsize - nb;
                mchunkptr newtop = chunk_plus_offset(p, nb);
                set_inuse(m, p, nb);
                newtop->head = newtopsize | PINUSE_BIT;
                m->top = newtop;
                m->topsize = newtopsize;
                newp = p;
            }
        }
        else if(next == m->dv)
        { /* extend into dv */
            KDsize dvs = m->dvsize;
            if(oldsize + dvs >= nb)
            {
                KDsize dsize = oldsize + dvs - nb;
                if(dsize >= MIN_CHUNK_SIZE)
                {
                    mchunkptr r = chunk_plus_offset(p, nb);
                    mchunkptr n = chunk_plus_offset(r, dsize);
                    set_inuse(m, p, nb);
                    set_size_and_pinuse_of_free_chunk(r, dsize);
                    clear_pinuse(n);
                    m->dvsize = dsize;
                    m->dv = r;
                }
                else
                { /* exhaust dv */
                    KDsize newsize = oldsize + dvs;
                    set_inuse(m, p, newsize);
                    m->dvsize = 0;
                    m->dv = 0;
                }
                newp = p;
            }
        }
        else if(!cinuse(next))
        { /* extend into next free chunk */
            KDsize nextsize = chunksize(next);
            if(oldsize + nextsize >= nb)
            {
                KDsize rsize = oldsize + nextsize - nb;
                unlink_chunk(m, next, nextsize);
                if(rsize < MIN_CHUNK_SIZE)
                {
                    KDsize newsize = oldsize + nextsize;
                    set_inuse(m, p, newsize);
                }
                else
                {
                    mchunkptr r = chunk_plus_offset(p, nb);
                    set_inuse(m, p, nb);
                    set_inuse(m, r, rsize);
                    dispose_chunk(m, r, rsize);
                }
                newp = p;
            }
        }
    }
    else
    {
        USAGE_ERROR_ACTION(m, chunk2mem(p));
    }
    return newp;
}

static void *internal_memalign(mstate m, KDsize alignment, KDsize bytes)
{
    void *mem = 0;
    if(alignment < MIN_CHUNK_SIZE) /* must be at least a minimum chunk size */
    {
        alignment = MIN_CHUNK_SIZE;
    }
    if((alignment & (alignment - SIZE_T_ONE)) != 0)
    { /* Ensure a power of 2 */
        KDsize a = MALLOC_ALIGNMENT << 1;
        while(a < alignment)
        {
            a <<= 1;
        }
        alignment = a;
    }
    if(bytes >= MAX_REQUEST - alignment)
    {
        if(m != 0)
        { /* Test isn't needed but avoids compiler warning */
            MALLOC_FAILURE_ACTION;
        }
    }
    else
    {
        KDsize nb = request2size(bytes);
        KDsize req = nb + alignment + MIN_CHUNK_SIZE - CHUNK_OVERHEAD;
        mem = internal_malloc(m, req);
        if(mem != 0)
        {
            mchunkptr p = mem2chunk(mem);
            if(PREACTION(m))
            {
                return 0;
            }
            if((((KDsize)(mem)) & (alignment - 1)) != 0)
            { /* misaligned */
                /*
          Find an aligned spot inside chunk.  Since we need to give
          back leading space in a chunk of at least MIN_CHUNK_SIZE, if
          the first calculation places us at a spot with less than
          MIN_CHUNK_SIZE leader, we can move to the next aligned spot.
          We've allocated enough total room so that this is always
          possible.
        */
                char *br = (char *)mem2chunk((KDsize)(((KDsize)((char *)mem + alignment -
                                                          SIZE_T_ONE)) &
                    -alignment));
                char *pos = ((KDsize)(br - (char *)(p)) >= MIN_CHUNK_SIZE) ?
                    br :
                    br + alignment;
                mchunkptr newp = (mchunkptr)pos;
                KDsize leadsize = pos - (char *)(p);
                KDsize newsize = chunksize(p) - leadsize;

                if(is_mmapped(p))
                { /* For mmapped chunks, just adjust offset */
                    newp->prev_foot = p->prev_foot + leadsize;
                    newp->head = newsize;
                }
                else
                { /* Otherwise, give back leader, use the rest */
                    set_inuse(m, newp, newsize);
                    set_inuse(m, p, leadsize);
                    dispose_chunk(m, p, leadsize);
                }
                p = newp;
            }

            /* Give back spare room at the end */
            if(!is_mmapped(p))
            {
                KDsize size = chunksize(p);
                if(size > nb + MIN_CHUNK_SIZE)
                {
                    KDsize remainder_size = size - nb;
                    mchunkptr remainder = chunk_plus_offset(p, nb);
                    set_inuse(m, p, nb);
                    set_inuse(m, remainder, remainder_size);
                    dispose_chunk(m, remainder, remainder_size);
                }
            }

            mem = chunk2mem(p);
            kdAssert(chunksize(p) >= nb);
            kdAssert(((KDsize)mem & (alignment - 1)) == 0);
            check_inuse_chunk(m, p);
            POSTACTION(m);
        }
    }
    return mem;
}

/*
  Common support for independent_X routines, handling
    all of the combinations that can result.
  The opts arg has:
    bit 0 set if all elements are same size (using sizes[0])
    bit 1 set if elements should be zeroed
*/
static void **ialloc(mstate m,
    KDsize n_elements,
    const KDsize *sizes,
    int opts,
    void *chunks[])
{

    KDsize element_size;   /* chunksize of each element, if all same */
    KDsize contents_size;  /* total size of elements */
    KDsize array_size;     /* request size of pointer array */
    void *mem;             /* malloced aggregate space */
    mchunkptr p;           /* corresponding chunk */
    KDsize remainder_size; /* remaining bytes while splitting */
    void **marray;         /* either "chunks" or malloced ptr array */
    mchunkptr array_chunk; /* chunk for malloced ptr array */
    flag_t was_enabled;    /* to disable mmap */
    KDsize size;
    KDsize i;

    ensure_initialization();
    /* compute array length, if needed */
    if(chunks != 0)
    {
        if(n_elements == 0)
        {
            return chunks; /* nothing to do */
        }
        marray = chunks;
        array_size = 0;
    }
    else
    {
        /* if empty req, must still return chunk representing empty array */
        if(n_elements == 0)
        {
            return (void **)internal_malloc(m, 0);
        }
        marray = 0;
        array_size = request2size(n_elements * (sizeof(void *)));
    }

    /* compute total element size */
    if(opts & 0x1)
    { /* all-same-size */
        element_size = request2size(*sizes);
        contents_size = n_elements * element_size;
    }
    else
    { /* add up all the sizes */
        element_size = 0;
        contents_size = 0;
        for(i = 0; i != n_elements; ++i)
        {
            contents_size += request2size(sizes[i]);
        }
    }

    size = contents_size + array_size;

    /*
     Allocate the aggregate chunk.  First disable direct-mmapping so
     malloc won't use it, since we would not be able to later
     free/realloc space internal to a segregated mmap region.
  */
    was_enabled = use_mmap(m);
    disable_mmap(m);
    mem = internal_malloc(m, size - CHUNK_OVERHEAD);
    if(was_enabled)
    {
        enable_mmap(m);
    }
    if(mem == 0)
    {
        return 0;
    }

    if(PREACTION(m))
    {
        return 0;
    }
    p = mem2chunk(mem);
    remainder_size = chunksize(p);

    kdAssert(!is_mmapped(p));

    if(opts & 0x2)
    { /* optionally clear the elements */
        kdMemset((KDsize *)mem, 0, remainder_size - SIZE_T_SIZE - array_size);
    }

    /* If not provided, allocate the pointer array as final part of chunk */
    if(marray == 0)
    {
        KDsize array_chunk_size;
        array_chunk = chunk_plus_offset(p, contents_size);
        array_chunk_size = remainder_size - contents_size;
        marray = (void **)(chunk2mem(array_chunk));
        set_size_and_pinuse_of_inuse_chunk(m, array_chunk, array_chunk_size);
        remainder_size = contents_size;
    }

    /* split out elements */
    for(i = 0;; ++i)
    {
        marray[i] = chunk2mem(p);
        if(i != n_elements - 1)
        {
            if(element_size != 0)
            {
                size = element_size;
            }
            else
            {
                size = request2size(sizes[i]);
            }
            remainder_size -= size;
            set_size_and_pinuse_of_inuse_chunk(m, p, size);
            p = chunk_plus_offset(p, size);
        }
        else
        { /* the final element absorbs any overallocation slop */
            set_size_and_pinuse_of_inuse_chunk(m, p, remainder_size);
            break;
        }
    }

#if !defined(KD_NDEBUG)
    if(marray != chunks)
    {
        /* final element must have exactly exhausted chunk */
        if(element_size != 0)
        {
            kdAssert(remainder_size == element_size);
        }
        else
        {
            kdAssert(remainder_size == request2size(sizes[i]));
        }
        check_inuse_chunk(m, mem2chunk(marray));
    }
    for(i = 0; i != n_elements; ++i)
    {
        check_inuse_chunk(m, mem2chunk(marray[i]));
    }

#endif /* KD_NDEBUG */

    POSTACTION(m);
    return marray;
}

/* Try to free all pointers in the given array.
   Note: this could be made faster, by delaying consolidation,
   at the price of disabling some user integrity checks, We
   still optimize some consolidations by combining adjacent
   chunks before freeing, which will occur often if allocated
   with ialloc or the array is sorted.
*/
static KDsize internal_bulk_free(mstate m, void *array[], KDsize nelem)
{
    KDsize unfreed = 0;
    if(!PREACTION(m))
    {
        void **a;
        void **fence = &(array[nelem]);
        for(a = array; a != fence; ++a)
        {
            void *mem = *a;
            if(mem != 0)
            {
                mchunkptr p = mem2chunk(mem);
                KDsize psize = chunksize(p);
#if FOOTERS
                if(get_mstate_for(p) != m)
                {
                    ++unfreed;
                    continue;
                }
#endif
                check_inuse_chunk(m, p);
                *a = 0;
                if(RTCHECK(ok_address(m, p) && ok_inuse(p)))
                {
                    void **b = a + 1; /* try to merge with next chunk */
                    mchunkptr next = next_chunk(p);
                    if(b != fence && *b == chunk2mem(next))
                    {
                        KDsize newsize = chunksize(next) + psize;
                        set_inuse(m, p, newsize);
                        *b = chunk2mem(p);
                    }
                    else
                    {
                        dispose_chunk(m, p, psize);
                    }
                }
                else
                {
                    CORRUPTION_ERROR_ACTION(m);
                    break;
                }
            }
        }
        if(should_trim(m, m->topsize))
        {
            sys_trim(m, 0);
        }
        POSTACTION(m);
    }
    return unfreed;
}

/* kdRealloc: Resize memory block. */
#if defined(__GNUC__) || defined(__clang__)
__attribute__((__malloc__))
#endif
KD_API void *KD_APIENTRY
kdRealloc(void *ptr, KDsize size)
{
    void *oldmem = ptr;
    KDsize bytes = size;

    void *mem = 0;
    if(oldmem == 0)
    {
        mem = kdMalloc(bytes);
    }
    else if(bytes >= MAX_REQUEST)
    {
        MALLOC_FAILURE_ACTION;
    }
#ifdef REALLOC_ZERO_BYTES_FREES
    else if(bytes == 0)
    {
        kdFree(oldmem);
    }
#endif /* REALLOC_ZERO_BYTES_FREES */
    else
    {
        KDsize nb = request2size(bytes);
        mchunkptr oldp = mem2chunk(oldmem);
#if !FOOTERS
        mstate m = gm;
#else  /* FOOTERS */
        mstate m = get_mstate_for(oldp);
        if(!ok_magic(m))
        {
            USAGE_ERROR_ACTION(m, oldmem);
            return 0;
        }
#endif /* FOOTERS */
        if(!PREACTION(m))
        {
            mchunkptr newp = try_realloc_chunk(m, oldp, nb, 1);
            POSTACTION(m);
            if(newp != 0)
            {
                check_inuse_chunk(m, newp);
                mem = chunk2mem(newp);
            }
            else
            {
                mem = internal_malloc(m, bytes);
                if(mem != 0)
                {
                    KDsize oc = chunksize(oldp) - overhead_for(oldp);
                    kdMemcpy(mem, oldmem, (oc < bytes) ? oc : bytes);
                    internal_free(m, oldmem);
                }
            }
        }
    }
    return mem;
}

/* kdCallocVEN: Allocate and zero-initialize memory. */
#if defined(__GNUC__) || defined(__clang__)
__attribute__((__malloc__))
#endif
KD_API void *KD_APIENTRY
kdCallocVEN(KDsize num, KDsize size)
{
  KDsize len = num * size;
  void *ptr = kdMalloc(len);
  if(!ptr) 
  {
    return KD_NULL;
  }
  return kdMemset(ptr, 0, len);
}
