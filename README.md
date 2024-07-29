# Structured file parser for C++

Works with GCC and C++20

Uses [gar](https://github.com/jgraj/gar)

### Write example:
```cpp
SFP::Writer writer = SFP::Writer::open("example.sf");
writer.write_i32("example_int", 1);
writer.write_f64("example_float", 2.3);
writer.write_cstr("example_string", "some text");
size_t array_size = 2;
writer.write_arr_bgn("example_array", array_size);
for (size_t i = 0; i < array_size; ++i) {
	writer.write_obj_bgn(nullptr);
	writer.write_i32(nullptr, i * 2);
	writer.write_i32(nullptr, i * 2 + 1);
	writer.write_obj_end();
}
writer.write_arr_end();
writer.free();
```

### Generated file:
```
example_int:i 1
example_float:f 2.3
example_string:b 9
	736f6d652074657874
example_array:a 2
	:o 
		:i 0
		:i 1
	:o 
		:i 2
		:i 3
```

### Read example:
```cpp
SFP::Reader reader = SFP::Reader::open("example.sf");
int32_t example_int = reader.read_i32("example_int");
double example_float = reader.read_f64("example_float");
char* example_string = reader.read_cstr("example_string");
size_t array_size = reader.read_arr_bgn("example_array");
for (size_t i = 0; i < array_size; ++i) {
	reader.read_obj_bgn(nullptr);
	int foo = reader.read_i32(nullptr);
	int bar = reader.read_i32(nullptr);
	reader.read_obj_end();
}
reader.read_arr_end();
reader.free();
```