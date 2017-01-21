#pragma once

struct trie_header_info;
struct node_header_info;

class trie {
public:
	static const int BUFFER_SIZE = 32 * 1024;

	enum result {
		success,
		not_enough_space,
		key_too_large,
		defrag_required,
		max_number_of_items_stored
	};

	trie();

	int entries_count();

	int wasted_space();

	int available_space_before_defrag();

	result write(const std::string& key, long val);

	std::pair<bool, long> try_read(const std::string& key);

	void dump_to_console(bool min = false);	

	void defrag();

	void validate();

	bool remove(const std::string& key);
private:

	trie::result add_node(trie_header_info* trie_header, node_header_info* start, short required_size, const std::string& key, int position_in_key, long val);

	char _buffer[BUFFER_SIZE];

};