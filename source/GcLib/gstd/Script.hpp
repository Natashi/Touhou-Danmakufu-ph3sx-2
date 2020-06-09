/* ---------------------------------- */
/* 2004..2005 YT、mkm(本体担当ですよ)*/
/* このソースコードは煮るなり焼くなり好きにしろライセンスの元で配布します。*/
/* A-3、A-4に従い、このソースを組み込んだ.exeにはライセンスは適用されません。*/
/* ---------------------------------- */
/* NYSL Version 0.9982 */
/* A. 本ソフトウェアは Everyone'sWare です。このソフトを手にした一人一人が、*/
/*    ご自分の作ったものを扱うのと同じように、自由に利用することが出来ます。*/
/* A-1. フリーウェアです。作者からは使用料等を要求しません。*/
/* A-2. 有料無料や媒体の如何を問わず、自由に転載・再配布できます。*/
/* A-3. いかなる種類の 改変・他プログラムでの利用 を行っても構いません。*/
/* A-4. 変更したものや部分的に使用したものは、あなたのものになります。*/
/*      公開する場合は、あなたの名前の下で行って下さい。*/
/* B. このソフトを利用することによって生じた損害等について、作者は */
/*    責任を負わないものとします。各自の責任においてご利用下さい。*/
/* C. 著作者人格権は ○○○○ に帰属します。著作権は放棄します。*/
/* D. 以上の３項は、ソース・実行バイナリの双方に適用されます。 */
/* ---------------------------------- */

#pragma once

#include "../pch.h"

#pragma warning (disable:4786)	//STL Warning抑止
#pragma warning (disable:4018)	//signed と unsigned の数値を比較
#pragma warning (disable:4244)	//double' から 'float' に変換

#include "Logger.hpp"

#include "LightweightVector.hpp"
#include "Value.hpp"

namespace gstd {
	class script_engine;
	class script_machine;

	typedef value (*callback)(script_machine* machine, int argc, value const* argv);

	//Breaks IntelliSense for some reason
#define DNH_FUNCAPI_DECL_(fn) static gstd::value fn (gstd::script_machine* machine, int argc, const gstd::value* argv)
#define DNH_FUNCAPI_(fn) gstd::value fn (gstd::script_machine* machine, int argc, const gstd::value* argv)

	struct function {
		char const* name;
		callback func;
		unsigned arguments;
	};

	class script_type_manager {
		static script_type_manager* base_;
	public:
		script_type_manager();

		type_data* get_real_type() {
			return const_cast<type_data*>(&*real_type);
		}
		type_data* get_char_type() {
			return const_cast<type_data*>(&*char_type);
		}
		type_data* get_boolean_type() {
			return const_cast<type_data*>(&*boolean_type);
		}
		type_data* get_string_type() {
			return const_cast<type_data*>(&*string_type);
		}
		type_data* get_real_array_type() {
			return const_cast<type_data*>(&*real_array_type);
		}
		type_data* get_array_type(type_data* element);

		static script_type_manager* get_instance() { return base_; }
	private:
		script_type_manager(script_type_manager const&);

		std::set<type_data> types;
		std::set<type_data>::iterator real_type;
		std::set<type_data>::iterator char_type;
		std::set<type_data>::iterator boolean_type;
		std::set<type_data>::iterator string_type;
		std::set<type_data>::iterator real_array_type;
	};

	class script_engine {
	public:
		script_engine(std::string const& source, int funcc, function const* funcv);
		script_engine(std::vector<char> const& source, int funcc, function const* funcv);
		virtual ~script_engine();

		void* data;	//クライアント用空間

		bool get_error() {
			return error;
		}

		std::wstring& get_error_message() {
			return error_message;
		}

		int get_error_line() {
			return error_line;
		}

		script_type_manager* get_type_manager() {
			return script_type_manager::get_instance();
		}

		//compatibility
		type_data* get_real_type() {
			return get_type_manager()->get_real_type();
		}
		type_data* get_char_type() {
			return get_type_manager()->get_char_type();
		}
		type_data* get_boolean_type() {
			return get_type_manager()->get_boolean_type();
		}
		type_data* get_array_type(type_data* element) {
			return get_type_manager()->get_array_type(element);
		}
		type_data* get_string_type() {
			return get_type_manager()->get_string_type();
		}

#ifndef _MSC_VER
	private:
#endif

		//コピー、代入演算子の自動生成を無効に
		script_engine(script_engine const& source);
		script_engine& operator=(script_engine const& source);

		//エラー
		bool error;
		std::wstring error_message;
		int error_line;

		//中間コード
		enum class command_kind : uint8_t {
			pc_var_alloc, pc_assign, pc_assign_writable, pc_break_loop, pc_break_routine, 
			pc_call, pc_call_and_push_result,
			pc_jump_if, pc_jump_if_not,
			pc_compare_e, pc_compare_g, pc_compare_ge, pc_compare_l,
			pc_compare_le, pc_compare_ne, 
			pc_dup_n,
			pc_for, pc_for_each_and_push_first,
			pc_compare_and_loop_ascent, pc_compare_and_loop_descent,
			pc_loop_count, pc_loop_if, pc_loop_continue, pc_continue_marker, pc_loop_back /*also serves as pc_jump*/,
			pc_construct_array,
			pc_pop, pc_push_value, pc_push_variable, pc_push_variable_writable, pc_swap, pc_yield, pc_wait,

			//Inline operations
			pc_inline_inc, pc_inline_dec,
			pc_inline_add_asi, pc_inline_sub_asi, pc_inline_mul_asi, pc_inline_div_asi, pc_inline_mod_asi, pc_inline_pow_asi,
			pc_inline_neg, pc_inline_not, pc_inline_abs,
			pc_inline_add, pc_inline_sub, pc_inline_mul, pc_inline_div, pc_inline_mod, pc_inline_pow,
			pc_inline_app, pc_inline_cat,
			pc_inline_cmp_e, pc_inline_cmp_g, pc_inline_cmp_ge, pc_inline_cmp_l, pc_inline_cmp_le, pc_inline_cmp_ne,
			pc_inline_logic_and, pc_inline_logic_or,
		};

		struct block;

		struct code {
			command_kind command;
			int line;	//ソースコード上の行
			value data;	//pc_push_valueでpushするデータ
#ifdef _DEBUG
			std::string var_name;	//For assign/push_variable
#endif

			union {
				struct {	//assign/push_variable
					int level;
					size_t variable;
				};
				struct {	//call/call_and_push_result
					block* sub;
					size_t arguments;
				};
				struct {	//loop_back
					size_t ip;
				};
			};

			code() {}

			code(int the_line, command_kind the_command) : line(the_line), command(the_command) {}

			code(int the_line, command_kind the_command, int the_level, size_t the_variable, std::string the_name) : line(the_line),
				command(the_command), level(the_level), variable(the_variable)
			{
#ifdef _DEBUG
				var_name = the_name;
#endif
			}

			code(int the_line, command_kind the_command, block* the_sub, int the_arguments) : line(the_line), command(the_command), sub(the_sub),
				arguments(the_arguments) {
			}

			code(int the_line, command_kind the_command, size_t the_ip) : line(the_line), command(the_command), ip(the_ip) {}

			code(int the_line, command_kind the_command, value& the_data) : line(the_line), command(the_command), data(the_data) {}
		};

		enum class block_kind : uint8_t {
			bk_normal, bk_loop, bk_sub, bk_function, bk_microthread
		};

		friend struct block;

		typedef std::vector<code> codes_t;

		struct block {
			int level;
			int arguments;
			std::string name;
			callback func;
			codes_t codes;
			block_kind kind;

			block(int the_level, block_kind the_kind) :
				level(the_level), arguments(0), name(), func(nullptr), codes(), kind(the_kind) {
			}
		};

		std::list<block> blocks;	//中身のポインタを使うのでアドレスが変わらないようにlist
		block* main_block;
		std::map<std::string, block*> events;

		block* new_block(int level, block_kind kind) {
			block x(level, kind);
			return &*blocks.insert(blocks.end(), x);
		}

		friend class parser;
		friend class script_machine;
	};

	class script_machine {
	public:
		script_machine(script_engine* the_engine);
		virtual ~script_machine();

		void* data;	//クライアント用空間

		void run();
		void call(std::string event_name);
		void call(std::map<std::string, script_engine::block*>::iterator event_itr);
		void resume();
		void stop() {
			finished = true;
			stopped = true;
		}

		bool get_stopped() { return stopped; }
		bool get_resuming() { return resuming; }

		bool get_error() { return error; }
		std::wstring& get_error_message() { return error_message; }
		int get_error_line() { return error_line; }

		void raise_error(const std::wstring& message) {
			error = true;
			error_message = message;
			finished = true;
		}
		void terminate(const std::wstring& message) {
			bTerminate = true;
			raise_error(message);
		}

		script_engine* get_engine() { return engine; }

		bool has_event(std::string event_name, std::map<std::string, script_engine::block*>::iterator& res);
		int get_current_line();
		int get_current_thread_addr() { return (int)current_thread_index._Ptr; }
	private:
		script_machine();
		script_machine(script_machine const& source);
		script_machine& operator=(script_machine const& source);

		script_engine* engine;

		bool error;
		std::wstring error_message;
		int error_line;

		bool bTerminate;

		typedef script_vector<value> variables_t;
		typedef script_vector<value> stack_t;
		//typedef std::vector<value> variables_t;
		//typedef std::vector<value> stack_t;

		class environment {
		public:
			environment(std::shared_ptr<environment> parent, script_engine::block* b);
			~environment();

			std::shared_ptr<environment> parent;
			script_engine::block* sub;
			int ip;
			variables_t variables;
			stack_t stack;
			bool has_result;
			int waitCount;
		};
		using environment_ptr = std::shared_ptr<environment>;

		std::list<environment_ptr> call_start_parent_environment_list;

		std::list<environment_ptr> threads;
		std::list<environment_ptr>::iterator current_thread_index;
		bool finished;
		bool stopped;
		bool resuming;

		void yield() {
			if (current_thread_index == threads.begin())
				current_thread_index = std::prev(threads.end());
			else
				--current_thread_index;
		}

		void run_code();
		value* find_variable_symbol(environment* current_env, script_engine::code* var_data);
	public:
		bool append_check(size_t arg0_size, type_data* arg0_type, type_data* arg1_type);

		size_t get_thread_count() { return threads.size(); }
	};
	inline bool script_machine::has_event(std::string event_name, std::map<std::string, script_engine::block*>::iterator& res) {
		res = engine->events.find(event_name);
		return res != engine->events.end();
	}
	inline int script_machine::get_current_line() {
		environment_ptr current = *current_thread_index;
		return (current->sub->codes[current->ip]).line;
	}

	template<int num>
	class constant {
	public:
		static value func(script_machine* machine, int argc, value const* argv) {
			return value(script_type_manager::get_instance()->get_real_type(), (double)num);
		}
	};

	template<const double* pVal>
	class pconstant {
	public:
		static value func(script_machine* machine, int argc, value const* argv) {
			return value(script_type_manager::get_instance()->get_real_type(), *pVal);
		}
	};
}
