/// version: 3

#ifndef SFP_PANIC
#define SFP_PANIC(...) std::printf(__VA_ARGS__); std::exit(1);
#endif

namespace SFP {
	static const size_t MAX_TAB_LEN = 32;
	static const size_t MAX_LABEL_LEN = 32;
	static const size_t MAX_VALUE_LEN = 32;
	static const size_t MAX_LINE_LEN = MAX_TAB_LEN + MAX_LABEL_LEN + MAX_VALUE_LEN;
	
	static void close_file(FILE* file) {
		if (file == nullptr) {
			return;
		}
		std::fclose(file);
	}

	struct Writer {
		FILE* file = nullptr;
		uint32_t indent = 0;

		static Writer open(const char* file_path) {
			Writer writer;
			writer.file = std::fopen(file_path, "wb");
			return writer;
		}

		void close() {
			close_file(this->file);
			this->file = nullptr;
		}

		template <typename T>
		void write(const T* src_ptr, size_t count) {
			size_t write_count = std::fwrite(src_ptr, sizeof(T), count, this->file);
			if (write_count == 0 || write_count < count) {
				SFP_PANIC("SFP: fwrite failed");
			}
		}

		void write_label(const char* label, char type) {
			if (label != nullptr) {
				size_t label_len = std::strlen(label);
				if (label_len > MAX_LABEL_LEN) {
					SFP_PANIC("SFP: label too long (max length: %i)", MAX_LABEL_LEN);
				}
				this->write<char>(label, label_len);
			}
			this->write<char>(":", 1);
			this->write<char>(&type, 1);
			this->write<char>(" ", 1);
		}

		void write_indent() {
			for (size_t i = 0; i < this->indent; ++i) {
				this->write<char>("\t", 1);
			}
		}

		void write_newline() {
			this->write<char>("\n", 1);
		}

		template <typename T>
		void write_num(const char* format, T value) {
			char buf[MAX_VALUE_LEN];
			int len = sprintf(buf, format, value);
			this->write<char>(buf, len);
		}

		void write_i32(const char* label, int32_t value) {
			this->write_indent();
			this->write_label(label, 'i');
			this->write_num<int32_t>("%i", value);
			this->write_newline();
		}
		
		void write_u32(const char* label, uint32_t value) {
			this->write_indent();
			this->write_label(label, 'u');
			this->write_num<uint32_t>("%u", value);
			this->write_newline();
		}

		void write_f64(const char* label, double value) {
			this->write_indent();
			this->write_label(label, 'f');
			this->write_num<double>("%f", value);
			this->write_newline();
		}

		static char half_byte_to_hex_char(uint8_t half_byte) {
			return half_byte < 10 ? ('0' + half_byte) : 'a' + (half_byte - 10);
		}

		void write_byte_hex(uint8_t byte) {
			char hex[2];
			hex[0] = half_byte_to_hex_char((byte >> 4) & 0x0f);
			hex[1] = half_byte_to_hex_char(byte & 0x0f);
			this->write<char>(hex, 2);
		}

		void write_bin(const char* label, ar<uint8_t> value) {
			this->write_indent();
			this->write_label(label, 'b');
			this->write_num<size_t>("%zu\n", value.len);
			this->indent += 1;
			this->write_indent();
			for (size_t i = 0; i < value.len; ++i) {
				this->write_byte_hex(value[i]);
			}
			this->indent -= 1;
			this->write_newline();
		}

		void write_cstr(const char* label, const char* value) {
			ar<uint8_t> array = ar<uint8_t>();
			array.buf = (uint8_t*)value;
			array.len = std::strlen(value);
			this->write_bin(label, array);
		}

		void write_arr_bgn(const char* label, size_t size) {
			this->write_indent();
			this->write_label(label, 'a');
			this->write_num<size_t>("%zu", size);
			this->write_newline();
			this->indent += 1;
		}

		void write_arr_end() {
			this->indent -= 1;
		}

		void write_obj_bgn(const char* label) {
			this->write_indent();
			this->write_label(label, 'o');
			this->write_newline();
			this->indent += 1;
		}

		void write_obj_end() {
			this->indent -= 1;
		}
	};

	struct Reader {
		FILE* file = nullptr;
		uint32_t indent = 0;

		static Reader open(const char* file_path) {
			Reader reader;
			reader.file = std::fopen(file_path, "rb");
			return reader;
		}

		void close() {
			close_file(this->file);
			this->file = nullptr;
		}

		template <typename T>
		void read(T* dst_ptr, size_t count) {
			size_t read_count = std::fread(dst_ptr, sizeof(T), count, this->file);
			if (read_count == 0 || read_count < count) {
				SFP_PANIC("SFP: fread failed");
			}
		}

		void read_indent() {
			if (this->indent == 0) {
				return;
			}
			char buf[MAX_TAB_LEN];
			this->read<char>(buf, this->indent);
			for (size_t i = 0; i < this->indent; ++i) {
				if (buf[i] != '\t') {
					SFP_PANIC("SFP: expected indent");
				}
			}
		}

		char* read_line() {
			this->read_indent();
			char* buf_ptr = (char*)std::malloc(sizeof(char) * MAX_LINE_LEN);
			buf_ptr = std::fgets(buf_ptr, MAX_LINE_LEN, this->file);
			if (buf_ptr == nullptr) {
				SFP_PANIC("SFP: fgets failed");
			}
			buf_ptr[strlen(buf_ptr) - 1] = '\0';
			return buf_ptr;
		}

		char* read_label(const char* label, char expected_type, char* line) {
			if (label != nullptr) {
				size_t label_len = std::strlen(label);
				if (std::memcmp(line, label, label_len) != 0) {
					SFP_PANIC("SFP: label mismatch (expected %s, got %s)", label, line);
				}
				line += label_len;
			}
			if (line[0] != ':') {
				SFP_PANIC("SFP: expected ':' before type");
			}
			line += 1;
			char type = line[0];
			if (type != expected_type) {
				SFP_PANIC("SFP: type mismatch (expected '%c', got '%c')", expected_type, type);
			}
			line += 1;
			if (line[0] != ' ') {
				SFP_PANIC("SFP: expected ' ' after type");
			}
			line += 1;
			return line;
		}

		int32_t read_i32(const char* label) {
			char* line_ptr = this->read_line();
			char* value_ptr = this->read_label(label, 'i', line_ptr);
			char* value_end;
			long value = std::strtol(value_ptr, &value_end, 10);
			if (*value_end != '\0') {
				SFP_PANIC("SFP: i32 parse error");
			}
			std::free(line_ptr);
			return value;
		}

		uint32_t read_u32(const char* label) {
			char* line_ptr = this->read_line();
			char* value_ptr = this->read_label(label, 'u', line_ptr);
			char* value_end;
			long value = std::strtol(value_ptr, &value_end, 10);
			if (*value_end != '\0' || value < 0) {
				SFP_PANIC("SFP: u32 parse error");
			}
			std::free(line_ptr);
			return 0;
		}

		double read_f64(const char* label) {
			char* line_ptr = this->read_line();
			char* value_ptr = this->read_label(label, 'f', line_ptr);
			char* value_end;
			double value = std::strtod(value_ptr, &value_end);
			if (*value_end != '\0') {
				SFP_PANIC("SFP: f64 parse error");
			}
			std::free(line_ptr);
			return value;
		}

		static uint8_t hex_char_to_byte(char hex_char) {
			return hex_char >= '0' && hex_char <= '9' ? (hex_char - '0') : (hex_char - 'a' + 10);
		}

		uint8_t read_byte_hex() {
			char hex_digits[2];
			this->read<char>(hex_digits, 2);
			return (hex_char_to_byte(hex_digits[0]) << 4) | hex_char_to_byte(hex_digits[1]);
		}

		ar<uint8_t> read_bin(const char* label) {
			char* line_ptr = this->read_line();
			char* value_ptr = this->read_label(label, 'b', line_ptr);
			std::free(line_ptr);
			char* value_end;
			size_t value = std::strtol(value_ptr, &value_end, 10);
			if (*value_end != '\0') {
				SFP_PANIC("SFP: size parse error");
			}
			this->indent += 1;
			this->read_indent();
			ar<uint8_t> array = ar<uint8_t>::alloc(value + 1);
			array.len -= 1;
			for (size_t i = 0; i < value; ++i) {
				array[i] = this->read_byte_hex();
			}
			char new_line;
			this->read<char>(&new_line, 1);
			if (new_line != '\n') {
				SFP_PANIC("SFP: expected new line");
			}
			this->indent -= 1;
			return array;
		}

		char* read_cstr(const char* label) {
			ar<uint8_t> array = read_bin(label);
			array[array.len] = '\0';
			return (char*)array.buf;
		}

		size_t read_arr_bgn(const char* label) {
			char* line_ptr = this->read_line();
			char* value_ptr = this->read_label(label, 'a', line_ptr);
			char* value_end;
			long value = std::strtol(value_ptr, &value_end, 10);
			if (*value_end != '\0') {
				SFP_PANIC("SFP: size parse error");
			}
			this->indent += 1;
			std::free(line_ptr);
			return value;
		}

		void read_arr_end() {
			this->indent -= 1;
		}
		
		void read_obj_bgn(const char* label) {
			char* line_ptr = this->read_line();
			this->read_label(label, 'o', line_ptr);
			this->indent += 1;
			std::free(line_ptr);
		}

		void read_obj_end() {
			this->indent -= 1;
		}
	};
}