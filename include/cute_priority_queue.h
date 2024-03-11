/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_PRIORITY_QUEUE_H
#define CF_PRIORITY_QUEUE_H

#include "cute_defines.h"

#ifdef CF_CPP

#include "cute_array.h"

namespace Cute
{

/**
 * Implements a heap data structure in order to implement other more advanced algorithms within Cute Framework,
 * such as A* or branch-and-bound for the AABB tree.
 *
 * Currently not thoroughly tested.
 */

template <typename T>
struct PriorityQueue
{
	void push_min(const T& val, float cost);
	bool pop_min(T* val = NULL, float* cost = NULL);

	void push_max(const T& val, float cost);
	bool pop_max(T* val = NULL, float* cost = NULL);

	int count();
	int count() const;
	void clear();

private:
	Array<T> m_values;
	Array<float> m_costs;

	int predicate_min(int iA, int iB);
	int predicate_max(int iA, int iB);
	void swap(int iA, int iB);
};

// -------------------------------------------------------------------------------------------------

template <typename T>
void PriorityQueue<T>::push_min(const T& val, float cost)
{
	m_values.add(val);
	m_costs.add(cost);

	int i = m_values.count();
	while (i > 1 && predicate_min(i - 1, i / 2 - 1) > 0) {
		swap(i - 1, i / 2 - 1);
		i /= 2;
	}
}

template <typename T>
bool PriorityQueue<T>::pop_min(T* val, float* cost)
{
	int count = m_values.count();
	if (!count) return false;
	if (val) *val = m_values[0];
	if (cost) *cost = m_costs[0];

	count--;
	m_values.unordered_remove(0);
	m_costs.unordered_remove(0);

	int u = 0, v = 1;
	while (u != v) {
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
void PriorityQueue<T>::push_max(const T& val, float cost)
{
	m_values.add(val);
	m_costs.add(cost);

	int i = m_values.count();
	while (i > 1 && predicate_max(i - 1, i / 2 - 1) > 0) {
		swap(i - 1, i / 2 - 1);
		i /= 2;
	}
}

template <typename T>
bool PriorityQueue<T>::pop_max(T* val, float* cost)
{
	int count = m_values.count();
	if (!count) return false;
	if (val) *val = m_values[0];
	if (cost) *cost = m_costs[0];

	count--;
	m_values.unordered_remove(0);
	m_costs.unordered_remove(0);

	int u = 0, v = 1;
	while (u != v) {
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
int PriorityQueue<T>::count()
{
	return m_values.count();
}

template <typename T>
int PriorityQueue<T>::count() const
{
	return m_values.count();
}

template <typename T>
void PriorityQueue<T>::clear()
{
	m_values.clear();
	m_costs.clear();
}

template <typename T>
int PriorityQueue<T>::predicate_min(int iA, int iB)
{
	float costA = m_costs[iA];
	float costB = m_costs[iB];
	return costA < costB ? -1 : costA > costB ? 1 : 0;
}

template <typename T>
int PriorityQueue<T>::predicate_max(int iA, int iB)
{
	float costA = m_costs[iA];
	float costB = m_costs[iB];
	return costA > costB ? -1 : costA < costB ? 1 : 0;
}

template <typename T>
void PriorityQueue<T>::swap(int iA, int iB)
{
	T tval = m_values[iA];
	m_values[iA] = m_values[iB];
	m_values[iB] = tval;

	float fval = m_costs[iA];
	m_costs[iA] = m_costs[iB];
	m_costs[iB] = fval;
}

}

#endif // CF_CPP

#endif // CF_PRIORITY_QUEUE_H
