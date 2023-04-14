#pragma once

#include "../Log/log.h"
#include "MemAlloc.hpp"
#include <assert.h>

// 在系统分配的内存上写入下一段内存的地址，该函数返回的是指针的引用,不引用返回是一个临时变量
static inline void *&NextBuff(void *ptr)
{
    return *((void **)ptr);
}

// 内存链表，挂载相同大小的底层内存
class BuffList
{
public:
    BuffList(size_t size) : nodeSize_(size) {}

    bool empty()
    {
        return head_ == nullptr;
    }

    // 将一段内存加入链表
    void pushRange(void *start, void *end, size_t num)
    {
        NextBuff(end) = head_;
        head_ = start;
        nodeCount += num;
    }

    // 弹出头节点
    void *pop()
    {
        assert(head_ != nullptr);
        void *ret = head_;
        head_ = NextBuff(ret);
        --nodeCount;

        return ret;
    }

    // 插入首节点
    void push(void *ptr)
    {
        assert(ptr != nullptr);
        NextBuff(ptr) = head_;
        head_ = ptr;
        ++nodeCount;
    }

    // 清空整个链表，并返回一整段内存
    void *clear()
    {
        void *ret = head_;
        head_ = nullptr;
        nodeCount = 0;
        return ret;
    }

    size_t getSize()
    {
        return nodeCount;
    }

    size_t getMaxSize()
    {
        return maxSize_;
    }

    void setMaxSize(size_t NewSize)
    {
        maxSize_ = NewSize;
    }

    ~BuffList()
    {
        if (head_ != nullptr)
        {
            SysFree(head_, nodeCount);
            head_ = nullptr;
        }
    }

private:
    void *head_ = nullptr; // 链表的头结点
    size_t nodeCount;      // 节点数量
    size_t maxSize_;       // 最多节点数量
    size_t nodeSize_;      // 节点的内存大小
};

// 页链表，同一大小的页节点链表，一个节点包含多个页
struct SamePageList
{
    SamePageList *prev_ = nullptr;
    SamePageList *next_ = nullptr;

    size_t pageCount_ = 0; // 页的数量
    size_t pageId_ = 0;    // 首页的页号
    long useCount = -1; //使用计数，-1代表还在内存池中，>=0 代表已出池

    void *buffPtr_ = nullptr; // 页内存的首地址
};

//管理不同大小的页链表
class PageListManager {
public:
    PageListManager() {
        tail_ = new SamePageList;
        tail_->prev_ = tail_;
        tail_->next_ = tail_;
    }

    bool empty() {
        return tail_->next_ == tail_;
    }

    //插入某节点的前面
    void insert(SamePageList* node, SamePageList* pos) {
        SamePageList* before = pos->prev_;
        pos->prev_ = node;
        node->next_ = pos;
        before->next_ = node;
        node->prev_ = before;
    }

    void push(SamePageList* node) {
        SamePageList* head = tail_->next_;
        tail_->next_ = node;
        node->next_ = head;
        node->prev_ = tail_;
        head->prev_ = node;
    }

    SamePageList* pop() {
        SamePageList* head = tail_->next_;
        tail_->next_ = head->next_;
        head->next_->prev_ = tail_;
        return head;
    }

    SamePageList* begin() {
        return tail_->next_;
    }

    SamePageList* end() {
        return tail_;
    }

private:
    SamePageList* tail_ = nullptr;
};
