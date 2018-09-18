#pragma once
#include <memory>
#include <vector>
#include <list>
#include <utility>
#include <algorithm>
#include <stack>
#include <stdexcept>
#include <tbb/concurrent_queue.h>


/*!
* This is a memory pool which provides a limited number of buffers. If a buffer is deleted, it is automatically returned to the buffer pool.
* With this mechanism resource consumption can be limited.
*
* Source/Reference: http://swarminglogic.com/jotting/2015_05_smartpool
*/
template <typename T>
class ObjectPool
{
private:
	struct ReturnToPool_Deleter {
		explicit ReturnToPool_Deleter(std::weak_ptr<ObjectPool<T>* > objectPool)
			: objectPool(objectPool) {}

		ReturnToPool_Deleter() = default;

		void operator()(T* ptr) {
			if(auto pool_ptr = objectPool.lock())
				(*pool_ptr.get())->add(std::unique_ptr<T>{ptr});
			else
				std::default_delete<T>{}(ptr);
		}
	private:
		std::weak_ptr<ObjectPool<T>* > objectPool;
	};

	void add(std::unique_ptr<T> t) {
		pool.push(std::move(t));
	}
public:
	using ptr_type = std::unique_ptr<T, ReturnToPool_Deleter >;

	ObjectPool(size_t initSize, const T& value = T(), bool bounded = true) :
		this_ptr(new ObjectPool<T>*(this)),
		value{ value },
		size_{ initSize },
		bounded_{ bounded }
	{
		for(size_t i = 0; i < size_; ++i) pool.push(std::make_unique<T>(value));
	}

	ObjectPool(const ObjectPool &objectPool) :
		ObjectPool(objectPool.size_, objectPool.value, objectPool.bounded_)
	{}

	virtual ~ObjectPool() {}


	ptr_type acquire()
	{
		std::unique_ptr<T> tmp;
		if(pool.try_pop(tmp))
		{
			return ptr_type(tmp.release(), ReturnToPool_Deleter(std::weak_ptr<ObjectPool<T>*>{this_ptr}));
		}
		else if(bounded_)
		{
			return nullptr;		
		}
		else
		{
			return ptr_type(new T(value), ReturnToPool_Deleter(std::weak_ptr<ObjectPool<T>*>{this_ptr}));
		}
	}	

	bool empty() const { return pool.empty(); }

	size_t size() const { return size_; }
	bool isBounded() const { return bounded_; }

private:
	std::shared_ptr<ObjectPool<T>* > this_ptr;
	size_t size_;
	const bool bounded_;
	T value;
	tbb::concurrent_queue<std::unique_ptr<T> > pool;
};