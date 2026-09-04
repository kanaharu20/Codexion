#include "header.h"

void	heap_push(t_heap *h, int coder_id, long priority_key)
{
		t_request tmp;

	h->entries[h->size].coder_id = coder_id;
	h->entries[h->size].priority_key = priority_key;
	h->size++;
	if (h->size == 2 && h->entries[1].priority_key < h->entries[0].priority_key)
	{
		tmp = h->entries[0];
		h->entries[0] = h->entries[1];
		h->entries[1] = tmp;
	}
}

int	heap_top(t_heap *h)
{
	if (h->size == 0)
		return (-1);
	return (h->entries[0].coder_id);
}

void	heap_pop(t_heap *h)
{
	if (h->size == 0)
		return ;
	if (h->size == 2)
		h->entries[0] = h->entries[1];
	h->size--;
}

void	heap_remove(t_heap *h, int coder_id)
{
	if (h->size == 1)
	{
		if (h->entries[0].coder_id == coder_id)
			h->size = 0;
		return ;
	}
	if (h->size == 2)
	{
		if (h->entries[0].coder_id == coder_id)
		{
			h->entries[0] = h->entries[1];
			h->size--;
		}
		else if (h->entries[1].coder_id == coder_id)
			h->size--;
	}
}
