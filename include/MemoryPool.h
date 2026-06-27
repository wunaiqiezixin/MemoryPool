#pragma once

#include <climits>
#include <cstddef>

template <typename T, size_t BlockSize = 4096>
class MemoryPool
{
public:
    //无缝对接整个 C++ 标准库生态(为了通用性)
    typedef T               value_type;
    typedef T*              pointer;
    typedef T&              reference;
    typedef const T*        const_pointer;
    typedef const T&        const_reference;
    typedef size_t          size_type;
    typedef ptrdiff_t       difference_type;
    typedef std::false_type propagate_on_container_copy_assignment;  /*复制赋值时不传播分配器*/
    typedef std::true_type  propagate_on_container_move_assignment;  /*移动赋值时传播分配器*/
    typedef std::true_type  propagate_on_container_swap;             /*交换时传播分配器*/

    // MemoryPool<T>::rebind<U>::other  
    // 编译后等价于 MemoryPool<U>
    template <typename U>
    struct rebind /* 嵌套模板类 */
    {
        typedef MemoryPool<U> other;
    };

public:
    //构造函数
    MemoryPool() noexcept;
    MemoryPool(const MemoryPool& memoryPool) noexcept;
    MemoryPool(MemoryPool&& memoryPool) noexcept;
    template <typename U> MemoryPool(const MemoryPool<U>& memoryPool) noexcept;
    //析构函数
    ~MemoryPool() noexcept;
    //赋值运算符
    MemoryPool& operator=(const MemoryPool& memoryPool) = delete; // 禁用拷贝赋值
    MemoryPool& operator=(MemoryPool&& memoryPool) noexcept;

    // 一次只能分配一个对象，hint和n可以被忽略
    pointer allocate(size_type n = 1, const_pointer hint = nullptr);
    void deallocate(pointer p, size_type n = 1);

    // 在已分配的内存上初始化对象
    template <class U, class... Args> void construct(U* p, Args&&... args);
    template <class U> void destroyed(U* p);

    // 将对象的内存分配和生命周期管理合并
    template <class... Args> pointer newElement(Args&&... args);
    void deleteElement(pointer p);

    // 取地址函数(STL接口要求)
    pointer address(reference x) const noexcept;
    const_pointer address(const_reference x) const noexcept;
    // 最大分配字节数
    size_type max_size() const noexcept;
    
private:
    //联合体(槽)：内存得到充分利用
    union Slot_
    {
        value_type element;
        Slot_* next;
    };

    typedef unsigned char*  byte_pointer_;
    typedef Slot_           slot_type_;
    typedef Slot_*          slot_pointer_;
    
    slot_pointer_ currentBlock_;    // 指向内存块链表头部的指针(每个内存块的首8字节)
    slot_pointer_ currentSlot_;     // 指向当前内存块中下一个空闲的槽 
    slot_pointer_ freeSlots_;       // 指向空闲链表头部的指针
    slot_pointer_ lastSlot_;        // 内存块内存不够用的标志

    
    // 计算内存对齐需要填充的最少字节数
    size_type padPointer(byte_pointer_ p, size_type align) const noexcept;
    // 向系统申请一大块内存(block)
    void allocateBlock(); 


    static_assert(BlockSize >= 2 * sizeof(slot_type_), "BlockSize too small.");

};


#include "MemoryPool.tcc"
