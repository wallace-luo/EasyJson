# EasyJson
A Minimalist JSON Library, Fast and Easy to use

## Installation

EasyJSON need the compiler to support C++11. You can include the library source file in your project, or you can build a shared library (recommanded).

## Usage

As it name suggests, EzJSON is really easy to use. Use JSON string to contruct an EzJSON object, then you can access it like an array / map.

```c++
EJ::JSON j("[1, {\"foo\" : [3, 4]}, 2]");
std::cout << j[1]["foo"][0].asDouble(); // output 3
```

You can modify the JSON tree by ```set()``` and ```remove()``` method.

```c++
EJ::JSON j("{}");
j.set("foo", "[1, 2, 3, 4]");
j["foo"].remove(2);
```

EzJSON has a baked-in pretty printing functionality.

```c++
EJ::JSON j("{\"number\" : [1,2,4,6,{\"string\":  \"foobar\"},7,5]}");
std::cout << j.serialize();

The output just like this:
{
    "number" : [
        1,
        2,
        4,
        6,
        {
            "string" : foobar
        },
        7,
        5
    ]
}
```

All EzJSON exceptions are derived from std::exception.

```c++
try
{
	EJ::JSON j("{");
}
catch (const std::exception& e)
{
	// get the error message
	std::cout << e.what();
}
```

# Performance

Inspired by rapidjson, EasyJSON use a custom allocator to dramatically speed up the parsing process. It's approximately 6x faster than dropbox's json11, and it took only 1.4 second for EzJSON to build complete AST for a very large (185MB) JSON file. 

## About

Released under the BSD Licence. (see license.txt)
