#ifndef __EASY_JSON_ALLOCATOR__
#define __EASY_JSON_ALLOCATOR__

#include "globals.h"

#include <cstdlib>
#include <cstring>

namespace EJ
{

/**
 * @brief Custom allocator
 * @details A simple allocator with very hight throuhtput
 */

class FastAllocator : public UnCopyable
{
private:

	/**
	 * @brief Page information that store at the begining of a page
	 *
	 */
	struct PageInfo
	{
		// next page pointer
		PageInfo* next;
		size_t capacity;
		size_t used;
	};

	/**
	 * @brief define the smallest page size is 16KB
	 */ 
	const static size_t PAGE_SIZE = 16 * 1024;

	PageInfo* current;
	PageInfo* firstPage;

public:

	/**
	 * @brief Initialize the allocator
	 */
	FastAllocator() : current(nullptr), firstPage(nullptr)
	{
		newPage(PAGE_SIZE);
	}

	/**
	 * @brief Destroy the allocator
	 */
	~FastAllocator()
	{
		clearAll();
	}


	/**
	 * @brief Allocate sz bytes from pool
	 * 
	 * @param sz size of required block
	 * @return address to the memory block
	 */
	void* alloc(size_t sz)
	{
		if (sz > current->capacity - current->used)
		{
			newPage(sz + sizeof(PageInfo));
		}

		void* ret = (char*)current + current->used;
		current->used += sz;
		return ret;
	}

	/**
	 * @brief Expand existing memory block
	 * 
	 * @param old Pointer to old block
	 * @param old_sz size of old block
	 * @param new_sz size of new block
	 * @return Pointer to expanded block
	 */
	void* reAlloc(void* old, size_t old_sz, size_t new_sz)
	{
		// new size is smaller, just set used value
		if (new_sz <= old_sz)
		{
			size_t diff = old_sz - new_sz;
			current->used -= diff;
			return old;
		}
		// if new size not out of current page size
		else if ((char*)old + old_sz == (char *)current + new_sz)
		{
			size_t diff = new_sz - old_sz;
			if (current->used + diff <= current->capacity)
			{
				current->used += diff;
				return old;
			}
		}
		// just allocator an new page
		else
		{
			void* ret = alloc(new_sz);
			memcpy(ret, old, old_sz);
			return ret;
		}
	}

private:

	/**
	 * @brief allocator a new page from system
	 * @parm sz is the size of the page
	 */
	void newPage(size_t sz)
	{
		if (sz < PAGE_SIZE)
		{
			sz = PAGE_SIZE;
		}

		PageInfo* ret = (PageInfo*)malloc(sz);
		if (ret == nullptr)
		{
			throw OutOfMemoryError();
		}

		ret->capacity = sz;
		ret->used = sizeof(PageInfo);
		ret->next = nullptr;

		if (firstPage == nullptr)
		{
			firstPage = ret;
		}
		else
		{
			current->next = ret;
		}
		current = ret;
	}

	/**
	 * @brief Deallocator the memory pool
	 */
	void clearAll()
	{
		for (PageInfo* f = firstPage; f != nullptr; )
		{
			PageInfo* next = f->next;
			free(f);
			f = next;
		}
		firstPage = nullptr;
		current = nullptr;
	}
};

}


#endif