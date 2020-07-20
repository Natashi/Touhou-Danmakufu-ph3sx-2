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

#include "../../pch.h"

#include "ScriptFunction.hpp"
#include "Parser.hpp"

namespace gstd {
	class script_type_manager {
		static script_type_manager* base_;
	public:
		script_type_manager();

		static type_data* get_int_type() { return base_->int_type; }
		static type_data* get_real_type() { return base_->real_type; }
		static type_data* get_char_type() { return base_->char_type; }
		static type_data* get_boolean_type() { return base_->boolean_type; }
		static type_data* get_string_type() { return base_->string_type; }
		//static type_data* get_int_array_type() { return base_->int_array_type; }
		static type_data* get_real_array_type() { return base_->real_array_type; }
		type_data* get_array_type(type_data* element);

		static script_type_manager* get_instance() { return base_; }
	private:
		script_type_manager(const script_type_manager& src);

		std::set<type_data> types;

		//Common types for quick access without std::set traversal
		type_data* int_type;
		type_data* real_type;
		type_data* char_type;
		type_data* boolean_type;
		type_data* string_type;
		//type_data* int_array_type;
		type_data* real_array_type;

		inline static type_data* deref_itr(std::set<type_data>::iterator& itr) {
			return const_cast<type_data*>(&*itr);
		}
	};

	class script_engine {
	public:
		script_engine(const std::string& source, int funcc, const function* funcv);
		script_engine(const std::vector<char>& source, int funcc, const function* funcv);
		virtual ~script_engine();

		script_engine& operator=(const script_engine& source) = default;

		bool get_error() { return error; }
		std::wstring& get_error_message() { return error_message; }
		int get_error_line() { return error_line; }

		script_block* new_block(int level, block_kind kind);
	public:
		void* data;		//Client script pointer

		bool error;
		std::wstring error_message;
		int error_line;

		std::list<script_block> blocks;
		script_block* main_block;
		std::map<std::string, script_block*> events;
	};

	class script_machine {
	public:
		script_machine(script_engine* the_engine);
		virtual ~script_machine();

		void* data;	//クライアント用空間

		void run();
		void call(const std::string& event_name);
		void call(std::map<std::string, script_block*>::iterator event_itr);
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

		bool has_event(const std::string& event_name, std::map<std::string, script_block*>::iterator& res);
		int get_current_line();
		int get_current_thread_addr() { return (int)current_thread_index._Ptr; }
	private:
		script_machine();
		script_machine(const script_machine& source);
		script_machine& operator=(const script_machine& source);

		script_engine* engine;

		bool error;
		std::wstring error_message;
		int error_line;

		bool bTerminate;

		typedef script_vector<value> variables_t;
		typedef script_vector<value> stack_t;

		class environment {
		public:
			environment(std::shared_ptr<environment> parent, script_block* b);
			~environment();

			std::shared_ptr<environment> parent;
			script_block* sub;
			int ip;
			variables_t variables;
			stack_t stack;
			bool has_result;
			int waitCount;
		};
		using environment_ptr = std::shared_ptr<environment>;

		std::list<environment_ptr> list_parent_environment;

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
		value* find_variable_symbol(environment* current_env, code* var_data);
	public:
		size_t get_thread_count() { return threads.size(); }
	};

	template<int num>
	class constant {
	public:
		static value func(script_machine* machine, int argc, const value* argv) {
			return value(script_type_manager::get_real_type(), (double)num);
		}
	};

	template<const double* pVal>
	class pconstant {
	public:
		static value func(script_machine* machine, int argc, const value* argv) {
			return value(script_type_manager::get_real_type(), *pVal);
		}
	};
}
