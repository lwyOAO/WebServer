#pragma once

#include <new>
#include <assert.h>
#include <cstddef>
#include <cstdlib>
#include "../Log/log.h"

const size_t NLISTS = 240; // 管理自由链表数组的长度,根据对齐规则计算出来的

const size_t MAXBYTES = 64 * 1024; // ThreadCache最大可以一次分配多大的内存64K

const size_t PAGE_SHIFT = 12; // 一页是4096字节,2的12次方=4096

const size_t NPAGES = 129; // PageCache的最大可以存放NPAGES-1页

static inline void *SysAlloc(size_t pageNum)
{
    void *ptr = malloc((pageNum) << PAGE_SHIFT);
    if (ptr == nullptr)
    {
        throw std::bad_alloc();
    }
    LOG_DEBUG(INS()) << "SysAlloc " << pageNum << " *4k memory" << std::endl;
    return ptr;
}

static inline void SysFree(void *ptr, size_t len)
{
    free(ptr);
    LOG_DEBUG(INS()) << "SysFree " << len << " *4k memory" << std::endl;
}
