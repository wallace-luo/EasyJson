#include "EasyJson.h"

#include "include/allocator.h"
#include "include/containers.h"
#include "include/globals.h"
#include "include/parser.h"
#include "include/text_scanner.h"

#include <sstream>

namespace EJ
{

/**
 * @brief generic AST node, every operation on it will fail
 * 
 */
class Node
{
public:

	// to make code shorter, ezjson does not use visitor pattern to implement serialization
	virtual void serialize(std::stringstream& ss, size_t indentLevel = 0) const = 0;

	virtual Node* at(size_t) const
	{
		throw NotAnArrayError();
	}

	virtual Node* key(const char*) const
	{
		throw NotAnObjectError();
	}

	virtual void append(Node*)
	{
		throw NotAnArrayError();
	}

	virtual void setAt(size_t, Node*)
	{
		throw NotAnArrayError();
	}

	virtual void setKey(const char*, Node*)
	{
		throw NotAnObjectError();
	}

	virtual void removeAt(size_t)
	{
		throw NotAnArrayError();
	}

	virtual void removeKey(const char*)
	{
		throw NotAnObjectError();
	}

	virtual size_t size() const
	{
		throw NotAnArrayOrObjectError();
	}

	virtual std::vector<std::string> fields() const
	{
		throw NotAnObjectError();
	}

	virtual double asDouble() const
	{
		throw NotConvertibleError();
	}

	virtual bool asBool() const
	{
		throw NotConvertibleError();
	}

	virtual std::string asString() const
	{
		throw NotConvertibleError();
	}

	// placement new to allocate it at memory pool

	void* operator new(size_t sz, FastAllocator& alc)
	{
		return alc.alloc(sz);
	}

	void operator delete(void*, FastAllocator&)
	{
		// do nothing (let the allocator to release the memory)
	}
};

/** 
 * Every derived class implements the operations it supports
 */
class NumberNode : public Node
{
private:
	
	double data;

public:

	NumberNode(double val) : data(val) {}

	virtual void serialize(std::stringstream& ss, size_t) const
	{
		ss << data;
	}

	double asDouble() const
	{
		return data;
	}
};

class StringNode : public Node
{
private:

	String data;
	friend class ASTBuildHandler;

public:

	StringNode(const char* b, const char* e, FastAllocator& alc)
		: data(b, e, alc) { }

	virtual void serialize(std::stringstream& ss, size_t sz) const
	{
		ss << toSTLString();
	}

	std::string toSTLString() const
	{
		return data.toSTLString();
	}
};

class BoolNode : public Node
{
private:

	bool data;

public:

	BoolNode(bool b) : data(b) {}

	virtual void serialize(std::stringstream& ss, size_t sz) const
	{
		if (data)
		{
			ss << "true";
		}
		else
		{
			ss << "false";
		}
	}

	bool asBool() const
	{
		return data;
	}
};

class NullNode : public Node
{
public:

	virtual void serialize(std::stringstream& ss, size_t sz) const
	{
		ss << "null";
	}
};

class ArrayNode : public Node
{
private:

	Array<Node*, FastAllocator> data;
	friend class ASTBuildHandler;

public:

	ArrayNode(FastAllocator& alc) : data(alc) {}

	virtual void serialize(std::stringstream& ss, size_t indentLevel) const
	{	
		ss << "[";
		if (indentLevel > 0)
		{
			ss << "\n";
		}
		else
		{
			printIndent(ss, indentLevel);
		}

		for (size_t i=1; i < data.size(); ++i)
		{
			printIndent(ss, indentLevel+1);
			data[i-1]->serialize(ss, indentLevel+1);
			ss << ",\n";
		}
		if (data.size() > 0)
		{
			printIndent(ss, indentLevel+1);
			data[size() - 1]->serialize(ss, indentLevel+1);
		}

		ss << "\n";
		printIndent(ss, indentLevel);
		ss << "]";
	}

	Node* at(size_t idx) const
	{
		return data[idx];
	}
	size_t size() const
	{
		return data.size();
	}

	void setAt(size_t idx, Node *node)
	{
		data[idx] = node;
	}

	void removeAt(size_t idx)
	{
		data.remove(idx);
	}

	void append(Node* node)
	{
		data.pushBack(node);
	}

	void printIndent(std::stringstream& ss, size_t indentLevel) const
	{
		for (size_t i = 0; i < indentLevel; i++)
		{
			// 4 spaces per indentation level
			ss << "    ";
		}
	}
};

class ObjectNode : public Node
{
private:

	Dictionary<Node*, FastAllocator> data;
	friend class ASTBuildHandler;

public:

	ObjectNode(FastAllocator& allocator)
		: data(allocator) {}

	virtual void serialize(std::stringstream& ss, size_t indentLevel) const
	{
		ss << "{";
		if (indentLevel > 0)
		{
			ss << "\n";
		}
		else
		{
			printIndent(ss, indentLevel);
			ss << "\n";
		}

		for (auto i = data.begin(); i < data.end() - 1; ++i)
		{
			printIndent(ss, indentLevel + 1);
			ss << '"' << i->first.toSTLString() << "\" : ";
			i->second->serialize(ss, indentLevel + 1);
			ss << ",\n";
		}
		if (data.size() > 0)
		{
			auto last = data.end() - 1;
			printIndent(ss, indentLevel + 1);
			ss << '"' << last->first.toSTLString() << "\" : ";
			last->second->serialize(ss, indentLevel + 1);
		}
		ss << "\n";
		printIndent(ss, indentLevel);
		ss << "}";
	}

	Node* key(const char* k) const
	{
		return data.get(k);
	}

	std::vector<std::string> fields() const
	{
		return data.keys();
	}

	size_t size() const
	{
		return data.size();
	}

	void setKey(const char *key, Node *node)
	{
		data.set(key, node);
	}

	void removeKey(const char *k)
	{
		data.remove(k);
	}

	void printIndent(std::stringstream& ss, size_t indentLevel) const
	{
		for (size_t i = 0; i < indentLevel; i++)
		{
			// 4 spaces per indentation level
			ss << "    ";
		}
	}
};

/**
 * @brief Parser callbacks
 * 
 */
class ASTBuildHandler : public UnCopyable
{
private:

	FastAllocator& allocator;
	Array<Node*, FastAllocator> parseStack;

public:

	ASTBuildHandler(FastAllocator& a) : allocator(a), parseStack(a)
	{
	}

	Node* getAST()
	{
		// the parse stack MUST has only one element after parsing
		if (parseStack.size() == 1)
		{
			return parseStack.popBack();
		}
		else
		{
			ParseError("Illegal JSON format.");
		}
	}

	void stringAction(const char *b, const char *e)
	{
		parseStack.pushBack(new (allocator)StringNode(b, e, allocator));
	}

	void numberAction(double val)
	{
		parseStack.pushBack(new (allocator)NumberNode(val));
	}

	void boolAction(bool b)
	{
		parseStack.pushBack(new (allocator)BoolNode(b));
	}

	void nullAction()
	{
		parseStack.pushBack(new (allocator)NullNode());
	}

	void beginArrayAction() {}

	void endArrayAction(size_t size)
	{
		// pop size nodes from parse stack, and construct a array node from them
		auto arr = new (allocator)ArrayNode(allocator);
		auto last = parseStack.end();
		for (auto iter = parseStack.end() - size; iter != last; ++iter)
		{
			arr->data.pushBack(*iter);
		}
		parseStack.shrink(size);
		// push the newly constructed array node to the parse stack
		parseStack.pushBack(arr);
	}

	void beginObjectAction() {}

	void endObjectAction(size_t size)
	{
		// key + value
		size *= 2;
		// pop size key and value nodes from parse stack
		// construct a object node from them
		auto obj = new (allocator)ObjectNode(allocator);
		auto last = parseStack.end();
		for (auto iter = parseStack.end() - size; iter != last; iter += 2)
		{
			obj->data.set(static_cast<StringNode*>(*iter)->data, *(iter + 1));
		}
		parseStack.shrink(size);
		// push the newly constructed object node to he parse stack
		parseStack.pushBack(obj);
	}
};


JSON::JSON(const char *content)
	: allocator(std::make_shared<FastAllocator>())
{
	node = parse(content, *allocator);
}


JSON::JSON(Node* nd, std::shared_ptr<FastAllocator> alc)
	: node(nd), allocator(alc)
{

}


// support chaining indexing

JSON JSON::at(size_t idx) const
{
	return JSON(node->at(idx), allocator);
}

JSON JSON::key(const char *key) const
{
	return JSON(node->key(key), allocator);
}


// every operations delegate to underlying Node object

double JSON::asDouble() const
{
	return node->asDouble();
}

bool JSON::asBool() const
{
	return node->asBool();
}

std::string JSON::asString() const
{
	return node->asString();
}

size_t JSON::size() const
{
	return node->size();
}

std::vector<std::string> JSON::keys() const
{
	return node->fields();
}

std::string JSON::serialize() const
{
	std::stringstream ss;
	node->serialize(ss);
	return ss.str();
}

void JSON::append(const char* content)
{
	node->append(parse(content, *allocator));
}

void JSON::setAt(size_t idx, const char *content)
{
	node->setAt(idx, parse(content, *allocator));
}

void JSON::setKey(const char *k, const char *content)
{
	node->setKey(k, parse(content, *allocator));
}

void JSON::removeAt(size_t idx)
{
	node->removeAt(idx);
}

void JSON::removeKey(const char *k)
{
	node->removeKey(k);
}

Node* JSON::parse(const char *content, FastAllocator& alc) const
{
	ASTBuildHandler handler(alc);
	Parser<TextScanner, ASTBuildHandler>(TextScanner(content), handler).parseValue();
	Node *node = handler.getAST();
	return node;
}

}