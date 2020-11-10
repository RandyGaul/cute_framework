/*
	Cute Framework
	Copyright (C) 2019 Randy Gaul https://randygaul.net

	This software is provided 'as-is', without any express or implied
	warranty.  In no event will the authors be held liable for any damages
	arising from the use of this software.

	Permission is granted to anyone to use this software for any purpose,
	including commercial applications, and to alter it and redistribute it
	freely, subject to the following restrictions:

	1. The origin of this software must not be misrepresented; you must not
	   claim that you wrote the original software. If you use this software
	   in a product, an acknowledgment in the product documentation would be
	   appreciated but is not required.
	2. Altered source versions must be plainly marked as such, and must not be
	   misrepresented as being the original software.
	3. This notice may not be removed or altered from any source distribution.
*/

#ifndef CUTE_PRIORITY_QUEUE_H
#define CUTE_PRIORITY_QUEUE_H

#include <cute_array.h>

namespace cute
{

/**
 * Implements a heap data structure in order to implement other more advanced algorithms within Cute Framework,
 * such as A* or branch-and-bound for the AABB tree.
 * 
 * Currently not thoroughly tested.
 */

template <typename T>
struct priority_queue
{
	void push_min(const T& val, float cost);
	bool pop_min(T* val = NULL, float* cost = NULL);

	void push_max(const T& val, float cost);
	bool pop_max(T* val = NULL, float* cost = NULL);

	int count();
	int count() const;
	void clear();

private:
	array<T> m_values;
	array<float> m_costs;

	int predicate_min(int iA, int iB);
	int predicate_max(int iA, int iB);
	void swap(int iA, int iB);
};

// -------------------------------------------------------------------------------------------------

template <typename T>
void priority_queue<T>::push_min(const T& val, float cost)
{
	m_values.add(val);
	m_costs.add(cost);

	int i = m_values.count();
	while (i > 1 && predicate_min(i - 1, i / 2 - 1) > 0)
	{
		swap(i - 1, i / 2 - 1);
		i /= 2;
	}
}

template <typename T>
bool priority_queue<T>::pop_min(T* val, float* cost)
{
	int count = m_values.count();
	if (!count) return false;
	if (val) *val = m_values[0];
	if (cost) *cost = m_costs[0];

	count--;
	m_values.unordered_remove(0);
	m_costs.unordered_remove(0);

	int u = 0, v = 1;
	while (u != v)
	{
		u = v;
		if (2 * u + 1 <= count) {
			if (predicate_min(u - 1, 2 * u - 1) <= 0) v = 2 * u;
			if (predicate_min(v - 1, 2 * u + 1 - 1) <= 0) v = 2 * u + 1;
		} else if (2 * u <= count) {
			if (predicate_min(u - 1, 2 * u - 1) <= 0) v = 2 * u;
		}

		if (u != v) {
			swap(u - 1, v - 1);
		}
	}

	return true;
}

template <typename T>
void priority_queue<T>::push_max(const T& val, float cost)
{
	m_values.add(val);
	m_costs.add(cost);

	int i = m_values.count();
	while (i > 1 && predicate_max(i - 1, i / 2 - 1) > 0)
	{
		swap(i - 1, i / 2 - 1);
		i /= 2;
	}
}

template <typename T>
bool priority_queue<T>::pop_max(T* val, float* cost)
{
	int count = m_values.count();
	if (!count) return false;
	if (val) *val = m_values[0];
	if (cost) *cost = m_costs[0];

	count--;
	m_values.unordered_remove(0);
	m_costs.unordered_remove(0);

	int u = 0, v = 1;
	while (u != v)
	{
		u = v;
		if (2 * u + 1 <= count) {
			if (predicate_max(u - 1, 2 * u - 1) <= 0) v = 2 * u;
			if (predicate_max(v - 1, 2 * u + 1 - 1) <= 0) v = 2 * u + 1;
		} else if (2 * u <= count) {
			if (predicate_max(u - 1, 2 * u - 1) <= 0) v = 2 * u;
		}

		if (u != v) {
			swap(u - 1, v - 1);
		}
	}

	return true;
}

template <typename T>
int priority_queue<T>::count()
{
	return m_values.count();
}

template <typename T>
int priority_queue<T>::count() const
{
	return m_values.count();
}

template <typename T>
void priority_queue<T>::clear()
{
	m_values.clear();
	m_costs.clear();
}

template <typename T>
int priority_queue<T>::predicate_min(int iA, int iB)
{
	float costA = m_costs[iA];
	float costB = m_costs[iB];
	return costA < costB ? -1 : costA > costB ? 1 : 0;
}

template <typename T>
int priority_queue<T>::predicate_max(int iA, int iB)
{
	float costA = m_costs[iA];
	float costB = m_costs[iB];
	return costA > costB ? -1 : costA < costB ? 1 : 0;
}

template <typename T>
void priority_queue<T>::swap(int iA, int iB)
{
	T tval = m_values[iA];
	m_values[iA] = m_values[iB];
	m_values[iB] = tval;

	float fval = m_costs[iA];
	m_costs[iA] = m_costs[iB];
	m_costs[iB] = fval;
}

}

#endif // CUTE_PRIORITY_QUEUE_H
