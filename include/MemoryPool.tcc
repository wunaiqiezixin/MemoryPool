template <typename T, size_t BlockSize>
inline typename MemoryPool<T, BlockSize>::size_type
MemoryPool<T, BlockSize>::padPointer(byte_pointer_ p, size_type align) 
const noexcept 
{
    uintptr_t result = reinterpret_cast<uintptr_t>(p);
    return (align - (result % align)) % align;
}

template <typename T, size_t BlockSize>
void 
MemoryPool<T, BlockSize>::allocateBlock()
{
    //向系统申请新的内存块
    byte_pointer_ newBlock = reinterpret_cast<byte_pointer_>(::operator new(BlockSize));
    //将内存块的前8个字节作为一个指向内存块链表的指针
    reinterpret_cast<slot_pointer_>(newBlock)->next = currentBlock_;
    currentBlock_ = reinterpret_cast<slot_pointer_>(newBlock);
    //可用的内存的起始地址
    byte_pointer_ body = newBlock + sizeof(slot_pointer_);
    //计算内存对齐所要填充的最少字节数
    size_type bodyPadding = padPointer(body, alignof(value_type));
    //更新对应指向新的内存块的指针
    currentSlot_ = reinterpret_cast<slot_pointer_>(body + bodyPadding);
    lastSlot_ = reinterpret_cast<slot_pointer_>(newBlock + BlockSize - sizeof(slot_type_) + 1);
    freeSlots_ = nullptr;
}

template <typename T, size_t BlockSize>
MemoryPool<T, BlockSize>::MemoryPool()
noexcept
    : currentBlock_(nullptr),
      currentSlot_(nullptr),
      freeSlots_(nullptr),
      lastSlot_(nullptr)
{}

template <typename T, size_t BlockSize>
MemoryPool<T, BlockSize>::MemoryPool(const MemoryPool& memoryPool)
noexcept
    : MemoryPool() /*委托构造函数*/
{}

template <typename T, size_t BlockSize>
template <typename U>
MemoryPool<T, BlockSize>::MemoryPool(const MemoryPool<U>& memoryPool)
noexcept
    : MemoryPool() /*委托构造函数*/
{}

template <typename T, size_t BlockSize>
MemoryPool<T, BlockSize>::MemoryPool(MemoryPool&& memoryPool)
noexcept
    : currentBlock_(memoryPool.currentBlock_),
      currentSlot_(memoryPool.currentSlot_),
      freeSlots_(memoryPool.freeSlots_),
      lastSlot_(memoryPool.lastSlot_)
{
    memoryPool.currentBlock_ = nullptr; /*将对象变为可析构对象*/
}

template <typename T, size_t BlockSize>
MemoryPool<T, BlockSize>::~MemoryPool()
noexcept
{
    // 只需要释放所有申请的内存块的内存(内存块链表)
    slot_pointer_ cur = currentBlock_;
    while (cur)
    {
        slot_pointer_ prev = cur;
        cur = cur->next;
        // 转成void*的指针，不会调用对象的析构函数
        ::operator delete(reinterpret_cast<void*>(prev));
    }
}

template <typename T, size_t BlockSize>
MemoryPool<T, BlockSize>&
MemoryPool<T, BlockSize>::operator=(MemoryPool&& memoryPool)
noexcept 
{
    if (this != &memoryPool)
    {
        std::swap(currentBlock_, memoryPool.currentBlock_);//将原对象变为可析构对象
        currentSlot_ = memoryPool.currentSlot_;
        freeSlots_ = memoryPool.freeSlots_;
        lastSlot_ = memoryPool.lastSlot_;
    }
    return *this;
}


template <typename T, size_t BlockSize>
typename MemoryPool<T, BlockSize>::pointer
MemoryPool<T, BlockSize>::allocate(size_type n, const_pointer hint)
{
    // 优先检查空闲链表
    if (freeSlots_)
    {
        pointer result = reinterpret_cast<pointer>(freeSlots_);
        freeSlots_ = freeSlots_->next;
        return result;
    }
    // 再检查当前内存块的内存是否够用
    if (currentSlot_ >= lastSlot_)
        allocateBlock(); // 不够用就重新申请一块内存
    // 将空闲的一个内存槽返回
    return reinterpret_cast<pointer>(currentSlot_++);
}

template <typename T, size_t BlockSize>
void
MemoryPool<T, BlockSize>::deallocate(pointer p, size_type n)
{
    // 将析构对象的内存放到空闲链表头部
    if (p)
    {
        reinterpret_cast<slot_pointer_>(p)->next = freeSlots_;
        freeSlots_ = reinterpret_cast<slot_pointer_>(p);
    }
}


template <typename T, size_t BlockSize>
template <class U, class... Args>
void 
MemoryPool<T, BlockSize>::construct(U* p, Args&&... args)
{
    // 在已分配的内存上构造一个对象
    if (p)
    {
        // 调用 placement new 
        new (p) U(std::forward<Args>(args)...); // 手动调用构造对象的构造函数
    }
}

template <typename T, size_t BlockSize>
template <class U>
void
MemoryPool<T, BlockSize>::destroyed(U* p)
{
    // 手动调用析构函数
    if (p)
    {
        p->~U();
    }
}


template <typename T, size_t BlockSize>
template <class... Args>
typename MemoryPool<T, BlockSize>::pointer
MemoryPool<T, BlockSize>::newElement(Args&&... args)
{
    // 分配一个对象的内存
    pointer result = allocate();
    // 在已分配的内存上构造一个对象
    construct(result, std::forward<Args>(args)...);
    //返回指向对象的指针
    return result; 
}

template <typename T, size_t BlockSize>
void 
MemoryPool<T, BlockSize>::deleteElement(pointer p)
{
    if (p)
    {
        // 向析构对象
        destroyed(p);
        // 再回收内存
        deallocate(p);
    }
}

// 取地址函数
template <typename T, size_t BlockSize>
typename MemoryPool<T, BlockSize>::pointer 
MemoryPool<T, BlockSize>::address(reference x)
const noexcept
{
    return &x;
}
template <typename T, size_t BlockSize>
typename MemoryPool<T, BlockSize>::const_pointer
MemoryPool<T, BlockSize>::address(const_reference x)
const noexcept
{
    return &x;
}


template <typename T, size_t BlockSize>
typename MemoryPool<T, BlockSize>::size_type
MemoryPool<T, BlockSize>::max_size()
const noexcept 
{
    size_type maxBlocks = -1 / BlockSize; /*最多可以分配的内存块数量*/
    return (BlockSize - sizeof(byte_pointer_)) / sizeof(slot_type_) * maxBlocks;
}
