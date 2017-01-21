#include "stdafx.h"
#include "trie.h"
#include "trie.impl.h"

void trie::validate() {
	auto trie_header = (trie_header_info*)_buffer;

	bool error = false;
	if (trie_header->items_count < 0)
	{
		std::cerr << "negative item count" << std::endl;
		error = true;
	}

	if (trie_header->used_size > trie_header->next_alloc) {
		std::cerr << "size greater than allocation" << std::endl;
		error = true;
	}

	if (trie_header->next_alloc > BUFFER_SIZE) {
		std::cerr << "allocation greater than buffer size" << std::endl;
		error = true;
	}

	if (error || trie_header->items_count == 0)
		return;

	std::stack<node_header_info*> nodes;
	nodes.push((node_header_info*)(_buffer + sizeof(trie_header_info)));

	while (nodes.size() > 0 && error == false) {
		auto current = nodes.top();
		nodes.pop();

		if (current->key_size < 0 ||
			current->children_offset < 0 ||
			current->key_offset < 0 ||
			current->value_offset < 0) {
			std::cerr << "negative offsets are not allowed" << std::endl;
			break;
		}

		if (current->key_size + current->key_offset > trie_header->next_alloc ||
			current->children_offset > trie_header->next_alloc ||
			current->key_offset > trie_header->next_alloc ||
			current->value_offset > trie_header->next_alloc) {
			std::cerr << "allocations after next_alloc" << std::endl;
			break;
		}

		if (current->children_offset != 0) {

			if (*(_buffer + current->children_offset) == 0) {
				std::cerr << "zero children but has children offsets" << std::endl;
				break;
			}

			auto children_offsets = (short*)(_buffer + current->children_offset + sizeof(char));
			for (char i = 0; i < *(_buffer + current->children_offset); i++)
			{
				if (children_offsets[i] == 0)
					continue;
				else if (children_offsets[i] > trie_header->next_alloc) {
					std::cerr << "child offset after next alloc" << std::endl;
					break;
				}
				else if (children_offsets[i] < 0) {
					std::cerr << "negative child offset" << std::endl;
					break;
				}
				auto child = (node_header_info*)(_buffer + children_offsets[i]);
				nodes.push(child);
			}
		}
	}
}


void trie::dump_to_console(bool min) {
	auto trie_header = (trie_header_info*)_buffer;

	std::cout << "entries " << trie_header->items_count
		<< " next alloc " << trie_header->next_alloc
		<< " used " << trie_header->used_size
		<< " waste " << wasted_space()
		<< std::endl;

	if (min)
		return;

	std::stack<std::pair<node_header_info*, size_t>> nodes;

	if (trie_header->items_count > 0) {
		auto node = (node_header_info*)(_buffer + sizeof(trie_header_info));
		nodes.push(std::make_pair(node, 0));
	}

	while (nodes.size() != 0) {

		auto item = nodes.top();
		nodes.pop();

		auto current = item.first;
		auto ident_level = item.second;

		for (size_t i = 0; i < ident_level; i++)
		{
			std::cout << " ";
		}

		std::cout << "key: '" << std::string(_buffer + current->key_offset, current->key_size) << "'";

		if (current->value_offset != 0) {
			std::cout << " val: " << *(long*)(_buffer + current->value_offset);
		}

		std::cout << std::endl;

		if (current->children_offset != 0) {
			auto children_offsets = (short*)(_buffer + current->children_offset + 1);
			for (char i = 0; i < *(_buffer + current->children_offset); i++)
			{
				if (children_offsets[i] == 0)
					continue;

				auto child = (node_header_info*)(_buffer + children_offsets[i]);
				nodes.push(std::make_pair(child, ident_level + 1));
			}
		}
	}
}
