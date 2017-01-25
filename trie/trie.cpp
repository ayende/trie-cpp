#include "stdafx.h"
#include "trie.h"
#include "trie.impl.h"

trie::trie() {
	auto trie_header = (trie_header_info*)_buffer;
	trie_header->items_count = 0;
	trie_header->next_alloc = sizeof(trie_header_info);
	trie_header->used_size = sizeof(trie_header_info);
}

void write_trie_node(char* base, node_header_info* node_header, short offset, const std::string&key, 
	int position_in_key, long val) {

	node_header->children_offset = 0;
	node_header->key_offset = offset + sizeof(node_header_info);
	node_header->key_size = (short)(key.length() - position_in_key);
    short aligned_key_size = (short)(node_header->key_size + 8 - (node_header->key_size % 8));
	node_header->value_offset = node_header->key_offset + aligned_key_size;

	auto bound_checked_buffer = stdext::make_checked_array_iterator(
		base + node_header->key_offset,
		trie::BUFFER_SIZE - node_header->key_offset);

	std::copy(key.begin() + position_in_key, key.end(), bound_checked_buffer);

	*(long*)(base + node_header->key_offset + aligned_key_size) = val;
}

node_header_info* find_child(char* base, char first_char, short* children_offsets, short number_of_children) {

	auto match = std::lower_bound(children_offsets, children_offsets + number_of_children, 0,
		[base, first_char](const short x_offset, const short _) {
		auto node = (node_header_info*)(base + x_offset);
		auto node_first_char = *(base + node->key_offset);
		return node_first_char > first_char;
	});

	auto node = (node_header_info*)(base + *match);
	auto node_first_char = *(base + node->key_offset);
	if (node_first_char == first_char)
		return node;

	return nullptr;
}

MatchResult get_failed_result(char* base, node_header_info* current, const std::string& key, int& position_in_key) {

	auto safe_key_it = stdext::make_checked_array_iterator(key.c_str(), key.length());

	auto size_to_compare = std::min(current->key_size, (short)(key.length() - position_in_key));
	auto result = std::mismatch(base + current->key_offset, base + current->key_offset + size_to_compare, safe_key_it + position_in_key);
	auto first_diff = (short)(result.second.base() - (key.c_str() + position_in_key));

	position_in_key += first_diff;

	return MatchResult{ false, current, first_diff };// not a match
}

MatchResult find_match(char* base, node_header_info* current, const std::string& key, int& position_in_key) {

	auto reamining_match = (key.length() - position_in_key);
	if (reamining_match == 0)
		return MatchResult{ true, current, current->key_size };

	if ((size_t)current->key_size > reamining_match) {
		return get_failed_result(base, current, key, position_in_key);// current key is larger than remaining key
	}

	if (std::memcmp(base + current->key_offset, key.c_str() + position_in_key, current->key_size) != 0) {

		return get_failed_result(base, current, key, position_in_key);
	}
	position_in_key += current->key_size;
	if (position_in_key == key.length()) { // found match
		return MatchResult{ true, current, current->key_size };
	}
	if (current->children_offset == 0) {
		return MatchResult{ false, current, current->key_size }; // no children, can go forward
	}

	auto num_of_children = *(char*)(base + current->children_offset);
	auto children_offsets = (short*)(base + current->children_offset + sizeof(char));

	auto child = find_child(base, key[position_in_key], children_offsets, num_of_children);
	if (child != nullptr)
		return find_match(base, child, key, position_in_key);

	return MatchResult{ false, current, current->key_size }; // no matching children, can't go forward
}

bool has_enough_size(trie_header_info* trie_header, short required_size, trie::result& result) {

	if (trie_header->used_size + required_size > trie::BUFFER_SIZE ||
		trie_header->used_size + required_size > INT16_MAX)
	{
		result = trie::result::not_enough_space;
		return false;
	}

	if (trie_header->next_alloc + required_size > trie::BUFFER_SIZE || 
		trie_header->next_alloc + required_size > INT16_MAX)
	{
		result = trie::result::defrag_required;
		return false;
	}

	return true;
}

trie::result append_child_node(char* base, short required_size, trie_header_info* trie_header, 
	node_header_info* parent, const std::string& key,	int position_in_key, long val) {

	auto old_number_of_children =
		parent->children_offset == 0 ? 0 : (*(base + parent->children_offset));

	required_size += (short)(
		sizeof(char) + // length
		sizeof(short) + // new child
		(old_number_of_children * sizeof(short)));

	trie::result fail;
	if (has_enough_size(trie_header, required_size, fail) == false)
		return fail;

	if (trie_header->items_count == UINT16_MAX)
		return trie::result::max_number_of_items_stored;

	trie_header->items_count++;

	auto old_children_offset = parent->children_offset;

	parent->children_offset = trie_header->next_alloc;
	trie_header->next_alloc += required_size;
	trie_header->used_size += required_size;
	// record the wasted space for the old children array
	trie_header->used_size -= (short)(sizeof(char) + sizeof(short) * old_number_of_children);


	auto children_offsets = ((short*)(base + parent->children_offset + sizeof(char)));
	children_offsets[0] = (short)(parent->children_offset
		+ sizeof(char) // length
		+ sizeof(short) * (old_number_of_children + 1));// offset of 1st child

	auto child = (node_header_info*)(base + children_offsets[0]);
	write_trie_node(base, child, children_offsets[0], key, position_in_key, val);

	*(char*)(base + parent->children_offset) = 1 + old_number_of_children;
	std::memcpy(
		(char*)children_offsets + sizeof(short), // skip the first value that we just entered
		(base + old_children_offset + sizeof(char)),
		sizeof(short) * old_number_of_children);

	// now sort the children by the first char 
	std::sort(children_offsets, children_offsets + old_number_of_children + 1,
		[base](short x_offset, short y_offset) {
		auto x_node = (node_header_info*)(base + x_offset);
		auto y_node = (node_header_info*)(base + y_offset);
		auto x_ch = *(base + x_node->key_offset);
		auto y_ch = *(base + y_node->key_offset);
		return x_ch > y_ch;
	});

	return trie::result::success;
}

int trie::entries_count() {
	auto trie_header = (trie_header_info*)_buffer;
	return trie_header->items_count;
}

int trie::available_space_before_defrag() {
	auto trie_header = (trie_header_info*)_buffer;
	return BUFFER_SIZE - trie_header->next_alloc;
}

int trie::wasted_space() {
	auto trie_header = (trie_header_info*)_buffer;
	return trie_header->next_alloc - trie_header->used_size;
}

trie::result trie::add_node(trie_header_info* trie_header, node_header_info* start, short required_size,
	const std::string& key, int position_in_key, long val) {

	trie::result fail;
	auto match = find_match(_buffer, start, key, position_in_key);
	if (match.success) { // overwrite
		if (match.current->value_offset == 0) {
			trie_header->items_count++; // an intermediary node now has a value, need to add it
			if (has_enough_size(trie_header, sizeof(long), fail) == false)
				return fail;
			match.current->value_offset = trie_header->next_alloc;
			trie_header->next_alloc += sizeof(long);
			trie_header->used_size += sizeof(long);
		}

		*(long*)(_buffer + match.current->value_offset) = val;

		return trie::result::success;
	}

	if (match.position_in_current_node != match.current->key_size) {
		// need to split the current node

		short size = sizeof(node_header_info) +
			sizeof(char) + // children legnth (1)
			sizeof(short) + // child offset
			sizeof(long); // holder for value

		if (has_enough_size(trie_header, size, fail) == false)
			return fail;

		auto split_node = (node_header_info*)(_buffer + trie_header->next_alloc);

		split_node->value_offset = match.current->value_offset;
		split_node->key_size = (match.current->key_size - match.position_in_current_node);
		split_node->key_offset = match.current->key_offset + match.position_in_current_node;
		match.current->value_offset = 0;
		match.current->key_size = match.position_in_current_node;
		split_node->children_offset = match.current->children_offset;
		match.current->children_offset = trie_header->next_alloc + sizeof(node_header_info);
		*(_buffer + match.current->children_offset) = 1;
		*(short*)(_buffer + match.current->children_offset + 1) = trie_header->next_alloc;

		trie_header->next_alloc += size;
		trie_header->used_size += size;

		return add_node(trie_header, match.current, required_size, key, position_in_key - match.position_in_current_node, val);
	}
	return append_child_node(_buffer, required_size, trie_header, match.current, key, position_in_key, val);
}

trie::result trie::write(const std::string& key, long val) {
	if (key.length() > UINT8_MAX)
		return trie::result::key_too_large;
    
    short aligned_key_size = (short)(key.length() + 8 - (key.length() % 8));
	short required_size = (short)aligned_key_size + sizeof(val) + sizeof(node_header_info);

	auto trie_header = (trie_header_info*)_buffer;

	trie::result fail;

	if (has_enough_size(trie_header, required_size, fail) == false)
	{
		defrag();
		if (has_enough_size(trie_header, required_size, fail) == false)
			return fail;
	}

	if (trie_header->items_count == 0) {
		// new trie
		trie_header->items_count = 1;
		auto offset = trie_header->next_alloc = sizeof(trie_header_info); // ensures that the first node is always in the beginning
		auto node_header = (node_header_info*)(_buffer + offset);
		trie_header->next_alloc += (short)required_size;
		trie_header->used_size += (short)required_size;

		write_trie_node(_buffer, node_header, offset, key, 0, val);

		return trie::result::success;
	}
	auto start = (node_header_info*)(_buffer + sizeof(trie_header_info));
	
	auto result = add_node(trie_header, start, required_size, key, 0, val);
	if (result == trie::result::defrag_required) {
		defrag();
		result = add_node(trie_header, start, required_size, key, 0, val);
	}

	return result;
}


bool trie::remove(const std::string& key) {
	auto trie_header = (trie_header_info*)_buffer;
	if (trie_header->items_count == 0)
		return false;

	auto start = (node_header_info*)(_buffer + sizeof(trie_header_info));
	int position_in_key = 0;
	auto match = find_match(_buffer, start, key, position_in_key);
	if (match.success == false || match.current->value_offset == 0)
		return false;

	trie_header->items_count--;
	trie_header->used_size -= match.current->key_size + sizeof(node_header_info) + sizeof(long);
	match.current->value_offset = 0;

	return true;
}

std::pair<bool, long> trie::try_read(const std::string& key) {

	auto trie_header = (trie_header_info*)_buffer;

	if (trie_header->items_count == 0)
		return std::make_pair(false, 0);

	int position_in_key = 0;
	auto match = find_match(_buffer, (node_header_info*)(_buffer + sizeof(trie_header_info)), key, position_in_key);
	if (match.success == false || match.current->value_offset == 0)
		return std::make_pair(false, 0);

	auto val = *(long*)(_buffer + match.current->value_offset);
	return std::make_pair(true, val);
}

void trie::defrag() {
	std::string s(BUFFER_SIZE, 0);
	auto temp_buffer = &(s[0]);
	std::memcpy(temp_buffer, _buffer, BUFFER_SIZE);
	std::memset(_buffer, 0, BUFFER_SIZE);

	auto temp_trie_header = (trie_header_info*)temp_buffer;
	auto trie_header = (trie_header_info*)_buffer;
	trie_header->items_count = 0;
	trie_header->next_alloc = sizeof(trie_header_info);
	trie_header->used_size = sizeof(trie_header_info);

	if (temp_trie_header->items_count == 0)
		return; // nothing else to do

	std::stack<std::pair<node_header_info*, short*>> nodes;
	nodes.push(std::make_pair((node_header_info*)(temp_buffer + sizeof(trie_header_info)), (short*)nullptr));

	node_header_info* current;

	while (nodes.size() > 0) {
		
		short* write_position;
		std::tie(current, write_position) = nodes.top();
		nodes.pop();

		auto required_size = current->key_size + sizeof(long) + sizeof(node_header_info);
		if (current->children_offset != 0) {
			required_size += sizeof(char) + (*(temp_buffer + current->children_offset) * sizeof(short));
		}
		
		if (write_position != nullptr)
			*write_position = trie_header->next_alloc;

		auto defraged = (node_header_info*)(_buffer + trie_header->next_alloc);
		defraged->key_offset = trie_header->next_alloc + sizeof(node_header_info);
		defraged->key_size = current->key_size;
		
		std::memcpy(_buffer + trie_header->next_alloc + sizeof(node_header_info), temp_buffer + current->key_offset, current->key_size);
		if (current->value_offset == 0) {
			defraged->value_offset = 0;
		}
		else {
			defraged->value_offset = defraged->key_offset + defraged->key_size;
			std::memcpy(_buffer + defraged->value_offset, temp_buffer + current->value_offset, sizeof(long));
			trie_header->items_count++;
			
		}

		if (current->children_offset == 0) {
			defraged->children_offset = 0;
		} 
		else {
			defraged->children_offset = defraged->key_offset + defraged->key_size + sizeof(long);
			auto number_of_children_p = (_buffer + defraged->children_offset);
			auto children_offsets = (short*)(temp_buffer + current->children_offset + 1);
			auto defraged_children = (short*)(_buffer + defraged->children_offset + 1);
			for (char i = 0; i < *(temp_buffer + current->children_offset); i++)
			{
				if (children_offsets[i] == 0)
					continue;
				auto child = (node_header_info*)(temp_buffer + children_offsets[i]);
			
				nodes.push(std::make_pair(child, &defraged_children[*number_of_children_p]));
				(*number_of_children_p)++;
			}
		}

		trie_header->next_alloc += (short)required_size;
		trie_header->used_size += (short)required_size;

	}
#if DEBUG
	validate();
#endif
}
