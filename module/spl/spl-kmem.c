/*****************************************************************************\
 *  Copyright (C) 2007-2010 Lawrence Livermore National Security, LLC.
 *  Copyright (C) 2007 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Brian Behlendorf <behlendorf1@llnl.gov>.
 *  UCRL-CODE-235197
 *
 *  This file is part of the SPL, Solaris Porting Layer.
 *  For details, see <http://github.com/behlendorf/spl/>.
 *
 *  The SPL is free software; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version.
 *
 *  The SPL is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with the SPL.  If not, see <http://www.gnu.org/licenses/>.
 *****************************************************************************
 *  Solaris Porting Layer (SPL) Kmem Implementation.
\*****************************************************************************/

#include <sys/kmem.h>
#include <spl-debug.h>

/*-
 * Copyright (c) 2006-2007 Pawel Jakub Dawidek <pjd@FreeBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/kmem.h>
#include <sys/mutex.h>

//#include <vm/vm_page.h>
//#include <vm/vm_object.h>
#include <vm/vm_kern.h>
#include <vm/vm_map.h>
#include <mach/host_info.h>
#include <libkern/OSMalloc.h>

#ifdef _KERNEL

#else
#define	malloc(size, type, flags)	malloc(size)
#define	free(addr, type)		free(addr)
#endif

extern uint64_t    max_mem;
uint64_t    physmem = 0;

//extern uint64_t    max_mem;
static OSMallocTag zfs_kmem_alloc_tag = NULL;

vmem_t *zio_alloc_arena = NULL; /* arena for allocating zio memory */

static uint64_t total_in_use = 0;

void
strfree(char *str)
{
    kmem_free(str, strlen(str) + 1);
}



void *
zfs_kmem_alloc(size_t size, int kmflags)
{
	void *p;
    uint64_t times = 0;
#ifdef KMEM_DEBUG
	struct kmem_item *i;

	size += sizeof(struct kmem_item);
#endif

    do {

        times++;

#if 0
    if (kmflags & KM_NOSLEEP)
        p = OSMalloc_noblock(size, zfs_kmem_alloc_tag);
    else
#endif
        p = OSMalloc(size, zfs_kmem_alloc_tag);

#ifndef _KERNEL
	if (kmflags & KM_SLEEP)
		assert(p != NULL);
#endif
#ifdef KMEM_DEBUG
	if (p != NULL) {
		i = p;
		p = (u_char *)p + sizeof(struct kmem_item);
		stack_save(&i->stack);
		mtx_lock(&kmem_items_mtx);
		LIST_INSERT_HEAD(&kmem_items, i, next);
		mtx_unlock(&kmem_items_mtx);
	}
#endif

    if (p && (kmflags & KM_ZERO))
        bzero(p, size);

    } while(!p);

    if (times > 1)
        printf("[spl] kmem_alloc(%lu) took %d retries\n",
               size, times);

    if (!p) {
        printf("[spl] kmem_alloc(%lu) failed: \n",size);
    } else atomic_add_64(&total_in_use, size);

	return (p);
}

void
zfs_kmem_free(void *buf, size_t size)
{
    OSFree(buf, size, zfs_kmem_alloc_tag);
    atomic_sub_64(&total_in_use, size);
}

void spl_total_in_use(void)
{
    printf("SPL: memory in use %llu\n", total_in_use);
}

static uint64_t kmem_size_val;


void
spl_kmem_init(void)
{
    //OSMT_PAGEABLE
    zfs_kmem_alloc_tag = OSMalloc_Tagalloc("ZFS general purpose",
                                           //OSMT_PAGEABLE);
                                           OSMT_DEFAULT);

}

void
spl_kmem_fini(void)
{
    OSMalloc_Tagfree(zfs_kmem_alloc_tag);
}


#if 0
static void
kmem_size_init(void *unused __unused)
{
    zfs_kmem_alloc_tag = OSMalloc_Tagalloc("ZFS general purpose",
                                           OSMT_DEFAULT);
	kmem_size_val = max_mem;
}
SYSINIT(kmem_size_init, SI_SUB_KMEM, SI_ORDER_ANY, kmem_size_init, NULL);
#endif

uint64_t
kmem_size(void)
{

	return (physmem * PAGE_SIZE);
}

uint64_t
kmem_used(void)
{
    return 0x1234567890;
	//return (kmem_map->size);
}

static int
kmem_std_constructor(void *mem, int size __unused, void *private, int flags)
{
	struct kmem_cache *cache = private;

	return (cache->kc_constructor(mem, cache->kc_private, flags));
}

static void
kmem_std_destructor(void *mem, int size __unused, void *private)
{
	struct kmem_cache *cache = private;

	cache->kc_destructor(mem, cache->kc_private);
}

kmem_cache_t *
kmem_cache_create(char *name, size_t bufsize, size_t align,
    int (*constructor)(void *, void *, int), void (*destructor)(void *, void *),
    void (*reclaim)(void *), void *private, vmem_t *vmp, int cflags)
{
	kmem_cache_t *cache;

	ASSERT(vmp == NULL);

	cache = kmem_alloc(sizeof(*cache), KM_SLEEP);
	strlcpy(cache->kc_name, name, sizeof(cache->kc_name));
	cache->kc_constructor = constructor;
	cache->kc_destructor = destructor;
	cache->kc_reclaim = reclaim;
	cache->kc_private = private;
	cache->kc_size = bufsize;

	return (cache);
}

void
kmem_cache_destroy(kmem_cache_t *cache)
{
	kmem_free(cache, sizeof(*cache));
}

void *
kmem_cache_alloc(kmem_cache_t *cache, int flags)
{
	void *p;

	p = kmem_alloc(cache->kc_size, flags);
	if (p != NULL && cache->kc_constructor != NULL)
		kmem_std_constructor(p, cache->kc_size, cache, flags);
	return (p);
}

void
kmem_cache_free(kmem_cache_t *cache, void *buf)
{
	if (cache->kc_destructor != NULL)
		kmem_std_destructor(buf, cache->kc_size, cache);
	kmem_free(buf, cache->kc_size);
}


/*
 * Call the registered reclaim function for a cache.  Depending on how
 * many and which objects are released it may simply repopulate the
 * local magazine which will then need to age-out.  Objects which cannot
 * fit in the magazine we will be released back to their slabs which will
 * also need to age out before being release.  This is all just best
 * effort and we do not want to thrash creating and destroying slabs.
 */
void
kmem_cache_reap_now(kmem_cache_t *skc)//, int count)
{
}





int
kmem_debugging(void)
{
	return (0);
}

void *
calloc(size_t n, size_t s)
{
	return (kmem_zalloc(n * s, KM_NOSLEEP));
}

void *
zfs_kmem_zalloc(size_t size, int kmflags)
{
    void *buf;

    buf = zfs_kmem_alloc(size, kmflags);

    if (buf != NULL) {
        bzero(buf, size);
    }
    return(buf);
}


char *kvasprintf(const char *fmt, va_list ap)
{
    unsigned int len;
    char *p;
    va_list aq;

    va_copy(aq, ap);
    len = vsnprintf(NULL, 0, fmt, aq);
    va_end(aq);

    p = OSMalloc(len+1, zfs_kmem_alloc_tag);
    if (!p)
        return NULL;

    vsnprintf(p, len+1, fmt, ap);

    return p;
}

char *
kmem_vasprintf(const char *fmt, va_list ap)
{
    va_list aq;
    char *ptr;

    do {
        va_copy(aq, ap);
        ptr = kvasprintf(fmt, aq);
        va_end(aq);
    } while (ptr == NULL);

    return ptr;
}

char *
kmem_asprintf(const char *fmt, ...)
{
    va_list ap;
    char *ptr;

    do {
        va_start(ap, fmt);
        ptr = kvasprintf(fmt, ap);
        va_end(ap);
    } while (ptr == NULL);

    return ptr;
}

