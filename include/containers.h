#ifndef __EASY_JSON_CONTAINERS__
#define __EASY_JSON_CONTAINERS__

#include "globals.h"

namespace EJ
{

/**
 * @brief Immutable and copyable string class,
 *        Do not manage memory itself
 */
class String
{
private:

	const char* beginPtr;
	const char* endPtr;

public:

	String(const char* b)
	{
		size_t len = strlen(b);
		beginPtr = b;
		endPtr = b + len;
	}

	template <typename ALLOCATOR>
	String(const char* b, const char* e, ALLOCATOR& allocator)
	{
		size_t sz = e - b;
		if (sz < 0)
		{
			throw InvalidCStringError();
		}
		else if (sz > 0)
		{
			void* buff = allocator.alloc(sz);
			memcpy(buff, b, sz);
			beginPtr = (const char*)buff;
			endPtr = beginPtr + sz;
		}
		else
		{
			beginPtr = nullptr;
			endPtr = nullptr;
		}
	}

	size_t size() const
	{
		return endPtr - beginPtr;
	}

	bool operator==(const String& other) const
	{
		size_t sz = size();
		if (sz != other.size())
		{
			return false;
		}
		for (size_t i=0; i<sz; ++i)
		{
			if(this->beginPtr[i] != other.beginPtr[i])
			{
				return false;
			}
		}
		return true;
	}

	char operator[](size_t idx) const
	{
		size_t sz = size();
		if (idx > sz)
		{
			throw IndexOutOfRangeError();
		}
		return *(beginPtr + idx);
//		return beginPtr[idx];
	}

	const char* begin() const
	{
		return beginPtr;
	}

	const char* end() const
	{
		return endPtr;
	}

	std::string toSTLString() const
	{
		if (beginPtr == nullptr || endPtr == nullptr)
		{
			return std::string("");
		}
		return std::string(beginPtr, endPtr);
	}
};

/**
 * Dynamic array that use allocator to manage memory,
 */

template <typename T, typename ALLOCATOR>
class Array : public UnCopyable
{
private:

	ALLOCATOR& allocator;
	const static size_t INIT_CAPACITY = 10;
	size_t capacity;
	size_t sz;
	T* data;

public:

	Array(ALLOCATOR& a, size_t cap = INIT_CAPACITY)
		: capacity(cap), sz(0), allocator(a)
	{
		data = static_cast<T*>(allocator.alloc(cap * sizeof(T)));
	}

	void pushBack(const T& e)
	{
		if (sz == capacity)
		{
			void* newData = allocator.reAlloc(data, capacity * sizeof(T), capacity * 2 * sizeof(T));
			capacity *= 2;
			data = static_cast<T*>(newData);
		}
		data[sz++] = e;
	}

	T popBack()
	{
		if (sz == 0)
		{
			throw IndexOutOfRangeError();
		}
		return data[--sz];
	}

	void shrink(size_t amount)
	{
		if (amount > sz)
		{
			throw IndexOutOfRangeError();
		}
		sz -= amount;
	}

	void remove(size_t idx)
	{
		if (idx >= sz)
		{
			throw IndexOutOfRangeError();
		}
		for (size_t i = idx + 1; i < sz; ++i)
		{
			data[i-1] = data[i];
		}
		sz--;
	}

	T& operator[](size_t idx)
	{
		if (idx >= sz)
		{
			throw IndexOutOfRangeError();
		}
		return data[idx];
	}

	size_t size() const
	{
		return sz;
	}

	const T& operator[](size_t idx) const
	{
		if (idx >= sz)
		{
			throw IndexOutOfRangeError();
		}
		return data[idx];
	}

	const T* const begin() const
	{
		return data;
	}

	const T* const end() const
	{
		return data + sz;
	}
};


/**
 * Naive dictionary implementation backed by a dynamic array,
 */
template <typename T, typename ALLOCATOR>

class Dictionary : public UnCopyable
{
private:

	Array<std::pair<String, T>, ALLOCATOR> data;

public:

	Dictionary(ALLOCATOR& allocator) : data(allocator){	}

	size_t size() const
	{
		return data.size();
	}

	bool contains(const String& key) const
	{
		return find(key) != -1;
	}

	const T& get(const String& key) const
	{
		int result = find(key);
		if (result == -1)
		{
			throw IndexOutOfRangeError();
		}
		return data[result].second;
	}

	void set(const String& key, const T& value)
	{
		int result = find(key);
		if (result == -1)
		{
			data.pushBack(std::make_pair(key, value));
		}
		else
		{
			data[result].second = value;
		}
	}

	void remove(const String& key)
	{
		int result = find(key);
		if (result == -1)
		{
			throw IndexOutOfRangeError();
		}
		else
		{
			data.remove(result);
		}
	}

	std::vector<std::string> keys() const
	{
		std::vector<std::string> result;
		for (auto i = data.begin(); i != data.end(); ++i)
		{
			result.push_back(i->first.toSTLString());
		}
		return result;
	}

	const std::pair<String, T>* begin() const
	{
		return data.begin();
	}

	const std::pair<String, T>* end() const
	{
		return data.end();
	}


private:
	int find(const String& key) const
	{
		int result = 0;
		int sz = static_cast<int>(data.size());
		for ( ; result < sz; ++result)
		{
			if (data[result].first == key)
			{
				return result;
			}
		}
		return -1;
	}
};

} // end of namespace EJ

#endif