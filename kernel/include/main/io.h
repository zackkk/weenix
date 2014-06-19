/******************************************************************************/
/* Important CSCI 402 usage information:                                      */
/*                                                                            */
/* This fils is part of CSCI 402 kernel programming assignments at USC.       */
/* Please understand that you are NOT permitted to distribute or publically   */
/*         display a copy of this file (or ANY PART of it) for any reason.    */
/* If anyone (including your prospective employer) asks you to post the code, */
/*         you must inform them that you do NOT have permissions to do so.    */
/* You are also NOT permitted to remove this comment block from this file.    */
/******************************************************************************/

#pragma once

#include "kernel.h"
#include "types.h"

static inline void outb(uint16_t port, uint8_t val)
{
        __asm__ volatile("outb %0,%1" :: "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port)
{
        uint8_t ret;
        __asm__ volatile("inb %1,%0" : "=a"(ret) : "Nd"(port));
        return ret;
}

static inline void outw(uint16_t port, uint16_t val)
{
        __asm__ volatile("outw %0,%1" :: "a"(val), "Nd"(port));
}

static inline uint16_t inw(uint16_t port)
{
        uint16_t ret;
        __asm__ volatile("inw %1,%0" : "=a"(ret) : "Nd"(port));
        return ret;
}

static inline void outl(uint16_t port, uint32_t val)
{
        __asm__ volatile("outl %0,%1" :: "a"(val), "Nd"(port));
}

static inline uint32_t inl(uint16_t port)
{
        uint32_t ret;
        __asm__ volatile("inl %1,%0" : "=a"(ret) : "Nd"(port));
        return ret;
}

