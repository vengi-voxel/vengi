/**
 * @file
 */

#include "tb_list.h"
#include "tb_core.h"
#include "core/Assert.h"

namespace tb {

bool TBListBackend::add(void *data)
{
	if (!growIfNeeded())
		return false;
	m_data->list[m_data->num] = data;
	m_data->num++;
	return true;
}

bool TBListBackend::add(void *data, int index)
{
	core_assert(index >= 0 && index <= getNumItems());
	if (!growIfNeeded())
		return false;
	if (index < m_data->num)
		SDL_memmove(&m_data->list[index + 1], &m_data->list[index], (m_data->num - index) * sizeof(void*));
	m_data->list[index] = data;
	m_data->num++;
	return true;
}

void TBListBackend::set(void *data, int index)
{
	core_assert(index >= 0 && index < getNumItems());
	m_data->list[index] = data;
}

void *TBListBackend::removeFast(int index)
{
	core_assert(index >= 0 && index < getNumItems());
	void *data = m_data->list[index];
	m_data->list[index] = m_data->list[m_data->num - 1];
	m_data->num--;
	return data;
}

void *TBListBackend::remove(int index)
{
	core_assert(index >= 0 && index < getNumItems());
	void *data = m_data->list[index];
	if(index < m_data->num - 1)
		SDL_memmove(&m_data->list[index], &m_data->list[index + 1], (m_data->num - index - 1) * sizeof(void*));
	m_data->num--;
	return data;
}

void TBListBackend::removeAll()
{
	SDL_free(m_data);
	m_data = nullptr;
}

void TBListBackend::swap(int index1, int index2)
{
	core_assert(index1 >= 0 && index1 < getNumItems());
	core_assert(index2 >= 0 && index2 < getNumItems());
	void *tmp = m_data->list[index1];
	m_data->list[index1] = m_data->list[index2];
	m_data->list[index2] = tmp;
}

int TBListBackend::find(void *data) const
{
	int num = getNumItems();
	for(int i = 0; i < num; i++)
	{
		if (get(i) == data)
			return i;
	}
	return -1;
}

void *TBListBackend::get(int index) const
{
	core_assert(index >= 0 && index < getNumItems());
	return m_data->list[index];
}

bool TBListBackend::reserve(int newCapacity)
{
	core_assert(newCapacity > 0);
	if (newCapacity > getCapacity())
	{
		int num = getNumItems();
		if (char *new_data = (char *) SDL_realloc(m_data, sizeof(TBLIST_DATA) + sizeof(void *) * (newCapacity)))
		{
			m_data = (TBLIST_DATA *) new_data;
			m_data->num = num;
			m_data->capacity = newCapacity;
			m_data->list = (void**) (new_data + sizeof(TBLIST_DATA));
			return true;
		}
		return false;
	}
	return true;
}

bool TBListBackend::growIfNeeded()
{
	int capacity = getCapacity();
	if (getNumItems() == capacity)
		return reserve(capacity == 0 ? 4 : capacity * 2);
	return true;
}

} // namespace tb
