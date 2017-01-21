#pragma once

struct trie_header_info
{
	short next_alloc;
	short used_size;
	short items_count;
	short reserved;
};

struct node_header_info
{
	short key_offset;
	short key_size;
	short children_offset;
	short value_offset;
};

struct MatchResult {
	bool success;
	node_header_info* current;
	short position_in_current_node;
};
