#ifndef CST_H
#define CST_H

struct StringNode {
	char* lit;
};

struct ArrayNode {
	char* arr;
	char* key;
};

enum NodeType {
	NT_UNKNOWN,
	NT_STRING,
	NT_ARRAY,
};

struct Node {
	enum NodeType type;
	union {
		struct StringNode str;
		struct ArrayNode arr;
	};
};

#endif
