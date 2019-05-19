/* copyright (c) 2010 Fredrik Kihlander, see LICENSE for more info */

#ifndef CONTAINER_ARRAY_H_INCLUDED
#define CONTAINER_ARRAY_H_INCLUDED

#include <dl/dl_defines.h>

/*
Class: CArrayNoResizeBase
An implementation of an array that cannot grow. Requires the user to provide a storage class. Alignment should be taken care of by supplied storage class. The array has an allocated size and an actual size.
*/

template <typename T, int SIZE>
class CArrayStatic
{
	T m_Storage[SIZE];
	size_t m_nElements;
public:
	/*
	Constructor: CArrayNoResizeBase
	Constructs an array
	*/
	CArrayStatic(){m_nElements = 0;}

	/*
	Destructor: CArrayNoResizeBase
	Constructs an array that is
	*/
	~CArrayStatic(){Reset();}

	/*
	Function: Reset()
	Reset used size to 0.
	*/
	inline void Reset() {SetSize(0);}

	/*
	Function: SetSize()
	Set used size.

	Parameters:
	_NewSize - New used size.
	*/
	inline void SetSize(size_t _NewSize)
	{
		DL_ASSERT(_NewSize <= SIZE);
		if(_NewSize < m_nElements) // destroy the rest
		{
			for(size_t i=_NewSize; i<m_nElements; i++)
				m_Storage[i].~T();
		}
		else if(_NewSize > m_nElements) // create new objects
		{
			for(size_t i=m_nElements; i<_NewSize; i++)
				new(&(m_Storage[i])) T;
		}
		m_nElements = _NewSize;
	}
	/*
	Function: Len()
	Get used size

	Returns:
	Returns Return used length;
	*/
	inline size_t Len(){return m_nElements;}

	/*
	Function: Full()
	Returns true if the array is full
	*/
	inline bool Full()  { return m_nElements == SIZE; }

	/*
	Function: Empty()
	Returns true if the array is empty
	*/
	inline bool Empty() { return m_nElements == 0; }

	/*
	Function: Capacity()
	Returns the amount of items that fit in the array.
	*/
	inline size_t Capacity() { return SIZE; }

	/*
	Function: Add()
	Add an element to the array.

	Parameters:
	_Element - Element to add
	*/
	void Add(const T& _Element)
	{
		DL_ASSERT(!Full() && "Array is full");
		new(&(m_Storage[m_nElements])) T(_Element);
		m_nElements++;
	}

	/*
	Function: Decr()
	Removes the last element from the stack.
	*/
	void Decr()
	{
		DL_ASSERT(m_nElements != 0 && "Array is empty");
		m_Storage[m_nElements--].~T();
	}

	/*
	Function: operator[]
	Get element.
	Parameters:
	_iEl - Index of wanted element.

	Returns:
	Returns reference to wanted element.
	*/
	T& operator[](size_t _iEl)
	{
		DL_ASSERT(_iEl < m_nElements && "Index out of bound");
		return m_Storage[_iEl];
	}

	/*
	Function: operator[]
	Get element.
	Parameters:
	_iEl - Index of wanted element. Const version.

	Returns:
	Returns const reference to wanted element.
	*/
	const T& operator[](size_t _iEl) const
	{
		DL_ASSERT(_iEl < m_nElements && "Index out of bound");
		return m_Storage[_iEl];
	}

	/*
	Function: GetBasePtr
	Get the array base pointer.

	Returns:
	Returns array base pointer.
	*/
	inline T* GetBasePtr() { return m_Storage; }
};

#endif //CONTAINER_ARRAY_H_INCLUDED
