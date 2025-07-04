#pragma once
#include <cassert>
#include <cstdlib>
#include <new>
#include <utility>
#include <memory>
#include <algorithm>

template <typename T>
class RawMemory {
public:
    RawMemory() = default;
    RawMemory(const RawMemory&) = delete;
    RawMemory& operator=(const RawMemory& rhs) = delete;
    RawMemory(RawMemory&& other) noexcept 
    : buffer_(other.buffer_), 
      capacity_(other.capacity_) {
    other.buffer_ = nullptr; 
    other.capacity_ = 0;     
}
    RawMemory& operator=(RawMemory&& rhs) noexcept {
    if (this != &rhs) {  
        Deallocate(buffer_); 

        buffer_ = rhs.buffer_;
        capacity_ = rhs.capacity_;

        rhs.buffer_ = nullptr;
        rhs.capacity_ = 0;
    }
    return *this; 
}

    explicit RawMemory(size_t capacity)
        : buffer_(Allocate(capacity))
        , capacity_(capacity) {
    }

    ~RawMemory() {
        Deallocate(buffer_);
    }

    T* operator+(size_t offset) noexcept {
        // Разрешается получать адрес ячейки памяти, следующей за последним элементом массива
        assert(offset <= capacity_);
        return buffer_ + offset;
    }

    const T* operator+(size_t offset) const noexcept {
        return const_cast<RawMemory&>(*this) + offset;
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<RawMemory&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < capacity_);
        return buffer_[index];
    }

    void Swap(RawMemory& other) noexcept {
        std::swap(buffer_, other.buffer_);
        std::swap(capacity_, other.capacity_);
    }

    const T* GetAddress() const noexcept {
        return buffer_;
    }

    T* GetAddress() noexcept {
        return buffer_;
    }

    size_t Capacity() const {
        return capacity_;
    }

private:
    // Выделяет сырую память под n элементов и возвращает указатель на неё
    static T* Allocate(size_t n) {
        return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
    }

    // Освобождает сырую память, выделенную ранее по адресу buf при помощи Allocate
    static void Deallocate(T* buf) noexcept {
        operator delete(buf);
    }

    T* buffer_ = nullptr;
    size_t capacity_ = 0;
};


template <typename T>
class Vector  {
public:
    
    using iterator = T*;
    using const_iterator = const T*;
    
    iterator begin() noexcept{
        return data_.GetAddress();
    }
    iterator end() noexcept{
        return data_.GetAddress() + size_;
    }
    const_iterator begin() const noexcept{
        return data_.GetAddress();
    }
    const_iterator end() const noexcept{
        return data_.GetAddress() + size_;
    }
    const_iterator cbegin() const noexcept{
        return data_.GetAddress();
    }
    const_iterator cend() const noexcept{
        return data_.GetAddress() + size_;
    }

    template <typename... Args>
    iterator Emplace(const_iterator pos, Args&&... args){
        size_t n = pos - cbegin();
        if (size_ == Capacity()) {
            size_t new_capacity = (size_ == 0 ? 1 : size_ * 2);
            RawMemory<T> new_data(new_capacity);
            
            size_t after = std::distance(pos,cend());
            new (new_data.GetAddress() + n) T(std::forward<Args>(args)...);
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), n, new_data.GetAddress());
                std::uninitialized_move_n(data_.GetAddress() + n, after, new_data.GetAddress() + n + 1);
            } else {
                std::uninitialized_copy_n(data_.GetAddress(), n, new_data.GetAddress());
                std::uninitialized_copy_n(data_.GetAddress() + n, after, new_data.GetAddress() + n + 1);
            }
            std::destroy_n(data_.GetAddress(), size_);
       
            data_.Swap(new_data);
            ++size_;
            return data_.GetAddress() + n;
        }
        else{
            T temp(std::forward<Args>(args)...);
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(end() - 1, 1, end());
                
            } else {
                //std::uninitialized_copy_n(end() - 1, 1, end());
                std::construct_at(end(), *(end() - 1));
                
            }
            iterator it = begin() + n;
            std::move_backward(it,end()-1,end());
            *it = std::move(temp);
            ++size_;
            return it;
        }
        
    }
    iterator Erase(const_iterator pos) noexcept(std::is_nothrow_move_assignable_v<T>){
        size_t n = std::distance(cbegin(),pos);
        if constexpr (std::is_move_assignable_v<T>) {
            for (size_t i = n + 1; i < size_ ; ++i){
               *(begin() + i - 1) = std::move(*(begin() + i));
            }        
       
                
        } else {
            for (size_t i = n + 1; i < size_ ; ++i){
               *(begin() + i - 1) = *(begin() + i);
            }  
            
                
            }
        std::destroy_n(data_.GetAddress() + size_ - 1, 1);
        --size_;
        iterator it = begin() + n;
        return it;
    }
    iterator Insert(const_iterator pos, const T& value){
        return Emplace(pos,value);
    }
    iterator Insert(const_iterator pos, T&& value){
        return Emplace(pos, std::move(value));
    }
    Vector() = default;

    
    
    explicit Vector(size_t size)
        : data_(size)
        , size_(size)  //
    {
        std::uninitialized_value_construct_n(data_.GetAddress(), size);    
    }
    
    ~Vector() {
        std::destroy_n(data_.GetAddress(), size_);
        
    }
    
    size_t Size() const noexcept {
        return size_;
    }

    size_t Capacity() const noexcept {
        return data_.Capacity();
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<Vector&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < size_);
        return data_[index];
    }
    
    Vector(const Vector& other)
        : data_(other.size_)
        , size_(other.size_)  //
    {
            
        std::uninitialized_copy_n(other.data_.GetAddress(), other.size_, data_.GetAddress());
       
    } 
    Vector(Vector&& other) noexcept
        : data_(std::move(other.data_))
        , size_(other.size_){  
            other.size_ = 0;
    }
    
    Vector& operator=(Vector&& rhs) noexcept{
        if(this != &rhs){
            data_ = std::move(rhs.data_);
            size_ = rhs.size_;
            rhs.size_ = 0;
        }
        return *this;
    }
    
    void Swap(Vector& other) noexcept {
        data_.Swap(other.data_);
        std::swap(size_, other.size_);
    }
    
    Vector& operator=(const Vector& rhs) {
        if (this != &rhs) {
            if (data_.Capacity() < rhs.size_){
                Vector rhs_copy(rhs);
                Swap(rhs_copy);
            }
            else{
                if (size_ >= rhs.size_){
                    std::copy_n(rhs.data_.GetAddress(), rhs.size_, data_.GetAddress());
                    size_t delta = size_ - rhs.size_;
                    std::destroy_n(data_.GetAddress() + rhs.size_, delta);               
                }
                else{
                    std::copy_n(rhs.data_.GetAddress(), size_, data_.GetAddress());
                    size_t delta = rhs.size_ - size_;
                    std::uninitialized_copy_n(rhs.data_.GetAddress() + size_, delta, data_.GetAddress() + size_);    
                }
                size_ = rhs.size_;
            }
        }
        return *this;
    }
    
    void Reserve(size_t new_capacity) {
        if (new_capacity <= this -> Capacity()) {
            return;
        }
        RawMemory<T> new_data(new_capacity);
        
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
        } else {
            std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
        }
        std::destroy_n(data_.GetAddress(), size_);
       
        data_.Swap(new_data);
        
    }
    
    void Resize(size_t new_size){
        if (new_size < size_){
            size_t delta = size_ - new_size;
            std::destroy_n(data_.GetAddress() + size_, delta);   
        }
        if (new_size > size_){
            this->Reserve(new_size);
            size_t delta = new_size - size_;
            std::uninitialized_value_construct_n(data_.GetAddress() + size_, delta);
        }
        size_ = new_size;
    }
    void PushBack(const T& value) {
        EmplaceBack(value);
    }
    void PushBack(T&& value){
        EmplaceBack(std::move(value));
    }
    
    void PopBack()  noexcept {
        --size_;
        std::destroy_n(data_.GetAddress() + size_, 1);
    }
    
    template <typename... Args>
    T& EmplaceBack(Args&&... args){
        if (size_ == Capacity()) {
            size_t new_capacity = (size_ == 0 ? 1 : size_ * 2);
            RawMemory<T> new_data(new_capacity);
            new (new_data.GetAddress() + size_) T(std::forward<Args>(args)...);
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(data_.GetAddress(), size_, new_data.GetAddress());
            } else {
                std::uninitialized_copy_n(data_.GetAddress(), size_, new_data.GetAddress());
            }
            std::destroy_n(data_.GetAddress(), size_);
       
            data_.Swap(new_data);
        }
        else{
            new (data_.GetAddress() + size_) T(std::forward<Args>(args)...);
        }
        ++size_;
        return *(data_.GetAddress() + (size_ - 1));
    }
    
private:
    
    // Вызывает деструкторы n объектов массива по адресу buf
    static void DestroyN(T* buf, size_t n) noexcept {
        for (size_t i = 0; i != n; ++i) {
            Destroy(buf + i);
        }
    }

    // Создаёт копию объекта elem в сырой памяти по адресу buf
    static void CopyConstruct(T* buf, const T& elem) {
        new (buf) T(elem);
    }

    // Вызывает деструктор объекта по адресу buf
    static void Destroy(T* buf) noexcept {
        buf->~T();
    }
    
    RawMemory<T> data_;
    size_t size_ = 0;
};