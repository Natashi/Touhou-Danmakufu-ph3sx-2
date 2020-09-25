#include "source/GcLib/pch.h"

#include "../GstdUtility.hpp"
#include "Script.hpp"
#include "ScriptLexer.hpp"

using namespace gstd;

/* script_type_manager */

script_type_manager* script_type_manager::base_ = nullptr;
script_type_manager::script_type_manager() {
	if (base_) return;
	base_ = this;

	int_type = deref_itr(types.insert(type_data(type_data::tk_int)).first);
	real_type = deref_itr(types.insert(type_data(type_data::tk_real)).first);
	char_type = deref_itr(types.insert(type_data(type_data::tk_char)).first);
	boolean_type = deref_itr(types.insert(type_data(type_data::tk_boolean)).first);
	string_type = deref_itr(types.insert(type_data(type_data::tk_array,
		char_type)).first);		//Char array (string)
	int_array_type = deref_itr(types.insert(type_data(type_data::tk_array,
		int_type)).first);		//Int array
	real_array_type = deref_itr(types.insert(type_data(type_data::tk_array,
		real_type)).first);		//Real array

	//Bool array
	types.insert(type_data(type_data::tk_array, boolean_type));
	//String array (Might not really be all that necessary to initialize it here)
	types.insert(type_data(type_data::tk_array, string_type));
}

type_data* script_type_manager::get_array_type(type_data* element) {
	type_data target = type_data(type_data::tk_array, element);
	auto itr = types.find(target);
	if (itr == types.end()) {
		//No types found, insert and return the new type
		itr = types.insert(target).first;
	}
	return deref_itr(itr);
}

/* script_engine */

script_engine::script_engine(const std::string& source, int funcc, const function* funcv) {
	main_block = new_block(1, block_kind::bk_normal);

	const char* end = &source[0] + source.size();
	script_scanner s(source.c_str(), end);
	parser p(this, &s, funcc, funcv);

	events = p.events;

	error = p.error;
	error_message = p.error_message;
	error_line = p.error_line;
}

script_engine::script_engine(const std::vector<char>& source, int funcc, const function* funcv) {
	main_block = new_block(1, block_kind::bk_normal);

	if (false) {
		wchar_t* pStart = (wchar_t*)&source[0];
		wchar_t* pEnd = (wchar_t*)(&source[0] + std::min(source.size(), 64U));
		std::wstring str = std::wstring(pStart, pEnd);
		//Logger::WriteTop(str);
	}
	const char* end = &source[0] + source.size();
	script_scanner s(&source[0], end);
	parser p(this, &s, funcc, funcv);

	events = p.events;

	error = p.error;
	error_message = p.error_message;
	error_line = p.error_line;
}

script_engine::~script_engine() {
	blocks.clear();
}

script_block* script_engine::new_block(int level, block_kind kind) {
	script_block x(level, kind);
	return &*blocks.insert(blocks.end(), x);
}

/* script_machine */

script_machine::script_machine(script_engine* the_engine) {
	assert(!the_engine->get_error());
	engine = the_engine;

	error = false;
	bTerminate = false;
}

script_machine::~script_machine() {
}

script_machine::environment::environment(std::shared_ptr<environment> parent, script_block* b) {
	this->parent = parent;
	this->sub = b;
	this->ip = 0;
	this->variables.clear();
	this->stack.clear();
	this->has_result = false;
	this->waitCount = 0;
}
script_machine::environment::~environment() {
	this->variables.clear();
	this->stack.clear();
}

bool script_machine::has_event(const std::string& event_name, std::map<std::string, script_block*>::iterator& res) {
	res = engine->events.find(event_name);
	return res != engine->events.end();
}
int script_machine::get_current_line() {
	environment_ptr current = *current_thread_index;
	return (current->sub->codes[current->ip]).line;
}

void script_machine::run() {
	if (bTerminate) return;

	assert(!error);
	if (threads.size() == 0) {
		error_line = -1;

		threads.push_back(std::make_shared<environment>(nullptr, engine->main_block));
		current_thread_index = threads.begin();

		finished = false;
		stopped = false;
		resuming = false;

		run_code();
	}
}

void script_machine::resume() {
	if (bTerminate) return;

	assert(!error);
	assert(stopped);

	stopped = false;
	finished = false;
	resuming = true;

	run_code();
}

void script_machine::call(const std::string& event_name) {
	call(engine->events.find(event_name));
}
void script_machine::call(std::map<std::string, script_block*>::iterator event_itr) {
	if (bTerminate) return;

	assert(!error);
	assert(!stopped);
	if (event_itr != engine->events.end()) {
		run();

		auto prev_thread = current_thread_index;
		current_thread_index = threads.begin();

		environment_ptr& env_first = *current_thread_index;
		env_first = std::make_shared<environment>(env_first, event_itr->second);

		finished = false;

		list_parent_environment.push_back(env_first->parent->parent);
		run_code();
		list_parent_environment.pop_back();

		finished = false;

		//Resume previous routine
		current_thread_index = prev_thread;
	}
}

void script_machine::run_code() {
	environment_ptr current = *current_thread_index;

	while (!finished && !bTerminate) {
		if (current->waitCount > 0) {
			yield();
			--current->waitCount;
			current = *current_thread_index;
			continue;
		}

		if (current->ip >= current->sub->codes.size()) {
			environment_ptr parent = current->parent;

			bool bFinish = false;
			if (parent == nullptr)
				bFinish = true;
			else if (list_parent_environment.size() > 1 && parent->parent != nullptr) {
				if (parent->parent == list_parent_environment.back())
					bFinish = true;
			}

			if (bFinish) {
				finished = true;
			}
			else {
				if (current->sub->kind == block_kind::bk_microthread) {
					current_thread_index = threads.erase(current_thread_index);
					yield();
					current = *current_thread_index;
				}
				else {
					if (current->has_result && parent != nullptr)
						parent->stack.push_back(current->variables[0]);
					*current_thread_index = parent;
					current = parent;
				}
			}
		}
		else {
			code* c = &(current->sub->codes[current->ip]);
			error_line = c->line;
			++(current->ip);

			switch (c->command) {
			case command_kind::pc_wait:
			{
				stack_t& stack = current->stack;
				value* t = &(stack.back());
				current->waitCount = (int)t->as_int() - 1;
				stack.pop_back();
				if (current->waitCount < 0) break;
			}
			//Fallthrough
			case command_kind::pc_yield:
				yield();
				current = *current_thread_index;
				break;

			case command_kind::pc_var_alloc:
				current->variables.resize(c->ip);
				break;
			case command_kind::pc_var_format:
			{
				for (size_t i = c->off; i < c->off + c->len; ++i) {
					if (i >= current->variables.capacity) break;
					current->variables[i] = value::val_empty;
				}
				break;
			}

			//case command_kind::_pc_jump_target:
			//	break;
			case command_kind::pc_jump:
				current->ip = c->ip;
				break;
			case command_kind::pc_jump_if:
			case command_kind::pc_jump_if_not:
			{
				stack_t& stack = current->stack;
				value* top = &stack.back();
				bool bJE = c->command == command_kind::pc_jump_if;
				if ((bJE && top->as_boolean()) || (!bJE && !top->as_boolean()))
					current->ip = c->ip;
				stack.pop_back();
				break;
			}
			case command_kind::pc_jump_if_nopop:
			case command_kind::pc_jump_if_not_nopop:
			{
				stack_t& stack = current->stack;
				value* top = &stack.back();
				bool bJE = c->command == command_kind::pc_jump_if_nopop;
				if ((bJE && top->as_boolean()) || (!bJE && !top->as_boolean()))
					current->ip = c->ip;
				break;
			}

			case command_kind::pc_assign:
			{
				stack_t& stack = current->stack;
				assert(stack.size() > 0);

				for (environment* i = current.get(); i != nullptr; i = (i->parent).get()) {
					if (i->sub->level == c->level) {
						variables_t& vars = i->variables;

						//Should never happen with pc_var_alloc, but it's here as a failsafe anyway
						if (vars.capacity <= c->variable) {
							vars.resize(c->variable + 4);
						}

						value* dest = &(vars[c->variable]);
						value* src = &stack.back();
						if (BaseFunction::_type_assign_check(this, src, dest)) {
							*dest = *src;
							dest->unique();
							stack.pop_back();
						}

						break;
					}
				}

				break;
			}
			case command_kind::pc_assign_writable:
			{
				stack_t& stack = current->stack;
				assert(stack.size() >= 2);

				value* dest = &stack[stack.size() - 2];
				value* src = &stack[stack.size() - 1];

				if (BaseFunction::_type_assign_check(this, src, dest)) {
					dest->overwrite(*src);
					stack.pop_back(2U);
				}

				break;
			}
			case command_kind::pc_break_routine:
				for (environment* i = current.get(); i != nullptr; i = (i->parent).get()) {
					i->ip = i->sub->codes.size();

					if (i->sub->kind == block_kind::bk_sub || i->sub->kind == block_kind::bk_function
						|| i->sub->kind == block_kind::bk_microthread)
						break;
				}
				break;
			case command_kind::pc_call:
			case command_kind::pc_call_and_push_result:
			{
				stack_t& current_stack = current->stack;
				//assert(current_stack.size() >= c->arguments);

				if (current_stack.size() < c->arguments) {
					std::wstring error = StringUtility::FormatToWide(
						"Unexpected script error: Stack size[%d] is less than the number of arguments[%d].\r\n",
						current_stack.size(), c->arguments);
					raise_error(error);
					break;
				}

				if (c->sub->func) {
					//Default functions

					size_t sizePrev = current_stack.size();

					value* argv = nullptr;
					if (sizePrev > 0 && c->arguments > 0)
						argv = current_stack.at + (sizePrev - c->arguments);
					value ret = c->sub->func(this, c->arguments, argv);

					if (stopped) {
						--(current->ip);
					}
					else {
						resuming = false;

						//Erase the passed arguments from the stack
						current_stack.pop_back(c->arguments);

						//Return value
						if (c->command == command_kind::pc_call_and_push_result)
							current_stack.push_back(ret);
					}
				}
				else if (c->sub->kind == block_kind::bk_microthread) {
					//Tasks
					environment_ptr e = std::make_shared<environment>(current, c->sub);
					threads.insert(++current_thread_index, e);
					--current_thread_index;

					//Pass the arguments
					for (size_t i = 0; i < c->arguments; ++i) {
						e->stack.push_back(current_stack.back());
						current_stack.pop_back();
					}

					current = *current_thread_index;
				}
				else {
					//User-defined functions or internal blocks
					environment_ptr e = std::make_shared<environment>(current, c->sub);
					e->has_result = c->command == command_kind::pc_call_and_push_result;
					*current_thread_index = e;

					//Pass the arguments
					for (size_t i = 0; i < c->arguments; ++i) {
						e->stack.push_back(current_stack.back());
						current_stack.pop_back();
					}

					current = e;
				}

				break;
			}

			case command_kind::pc_compare_e:
			case command_kind::pc_compare_g:
			case command_kind::pc_compare_ge:
			case command_kind::pc_compare_l:
			case command_kind::pc_compare_le:
			case command_kind::pc_compare_ne:
			{
				value* t = &current->stack.back();
				int r = t->as_int();
				bool b = false;
				switch (c->command) {
				case command_kind::pc_compare_e:
					b = r == 0;
					break;
				case command_kind::pc_compare_g:
					b = r > 0;
					break;
				case command_kind::pc_compare_ge:
					b = r >= 0;
					break;
				case command_kind::pc_compare_l:
					b = r < 0;
					break;
				case command_kind::pc_compare_le:
					b = r <= 0;
					break;
				case command_kind::pc_compare_ne:
					b = r != 0;
					break;
				}
				t->set(script_type_manager::get_boolean_type(), b);
				break;
			}
			case command_kind::pc_dup_n:
			case command_kind::pc_dup_n_unique:
			{
				stack_t& stack = current->stack;
				value* val = &stack.back() - c->ip + 1;
				if (stack.size() < stack.capacity) {
					stack.push_back(*val);
				}
				else {
					value valCopy = *val;
					stack.push_back(valCopy);
				}
				if (c->command == command_kind::pc_dup_n_unique)
					stack.back().unique();
				break;
			}
			case command_kind::pc_swap:
			{
				size_t len = current->stack.size();
				assert(len >= 2);
				std::swap(current->stack[len - 1], current->stack[len - 2]);
				break;
			}

			//Loop commands
			case command_kind::pc_loop_ascent:
			case command_kind::pc_loop_descent:
			{
				value* cmp_arg = (value*)(&current->stack.back() - 1);
				value cmp_res = BaseFunction::compare(this, 2, cmp_arg);

				/*
				{
					std::wstring str = c->command == command_kind::pc_compare_and_loop_ascent ? L"Asc: " : L"Dsc: ";
					str += cmp_arg->as_string() + L" " + (cmp_arg + 1)->as_string();
					Logger::WriteTop(str);
				}
				*/

				bool bSkip = c->command == command_kind::pc_loop_ascent ?
					(cmp_res.as_int() <= 0) : (cmp_res.as_int() >= 0);
				current->stack.push_back(value(script_type_manager::get_boolean_type(), bSkip));

				//current->stack.pop_back(2U);
				break;
			}
			case command_kind::pc_loop_count:
			{
				value* i = &current->stack.back();
				int64_t r = i->as_int();
				if (r > 0)
					i->set(script_type_manager::get_int_type(), r - 1);
				current->stack.push_back(value(script_type_manager::get_boolean_type(), r > 0));
				break;
			}
			case command_kind::pc_loop_foreach:
			{
				//Stack: .... [array] [counter]

				value* i = &current->stack.back();
				value* src_array = i - 1;

				std::vector<value>::iterator itrCur = src_array->array_get_begin() + i->as_int();
				std::vector<value>::iterator itrEnd = src_array->array_get_end();

				bool bSkip = false;
				if (src_array->get_type()->get_kind() != type_data::tk_array || itrCur >= itrEnd) {
					bSkip = true;
				}
				else {
					current->stack.push_back(value::new_from(*itrCur));
					i->set(script_type_manager::get_int_type(), i->as_int() + 1i64);
				}

				current->stack.push_back(value(script_type_manager::get_boolean_type(), bSkip));
				break;
			}

			case command_kind::pc_construct_array:
			{
				if (c->ip == 0U) {
					current->stack.push_back(value(script_type_manager::get_string_type(), 0.0));
					break;
				}

				std::vector<value> res_arr;
				res_arr.resize(c->ip);

				value* val_ptr = &current->stack.back() - c->ip + 1;

				type_data* type_arr = script_type_manager::get_instance()->get_array_type(val_ptr->get_type());

				value res;
				for (size_t iVal = 0U; iVal < c->ip; ++iVal, ++val_ptr) {
					BaseFunction::_append_check(this, iVal, type_arr, val_ptr->get_type());
					res_arr[iVal] = *val_ptr;
				}
				res.set(type_arr, res_arr);

				current->stack.pop_back(c->ip);
				current->stack.push_back(res);
				break;
			}

			case command_kind::pc_pop:
				current->stack.pop_back(c->ip);
				break;
			case command_kind::pc_push_value:
				current->stack.push_back(c->data);
				break;
			case command_kind::pc_push_variable:
			//case command_kind::pc_push_variable_unique:
			{
				value* var = find_variable_symbol(current.get(), c);
				if (var == nullptr) break;
				//if (c->command == command_kind::pc_push_variable_unique)
				//	var->unique();
				current->stack.push_back(*var);
				break;
			}

			//----------------------------------Inline operations----------------------------------
			case command_kind::pc_inline_top_inc:
			case command_kind::pc_inline_top_dec:
			{
				value* var = &(current->stack.back());
				value res = (c->command == command_kind::pc_inline_top_inc) ?
					BaseFunction::successor(this, 1, var) : BaseFunction::predecessor(this, 1, var);
				*var = res;
				break;
			}
			case command_kind::pc_inline_inc:
			case command_kind::pc_inline_dec:
			{
				//A hack for assign_writable's, since symbol::level can't normally go below 0 anyway
				if (c->level >= 0) {
					value* var = find_variable_symbol(current.get(), c);
					if (var == nullptr) break;

					value res = (c->command == command_kind::pc_inline_inc) ?
						BaseFunction::successor(this, 1, var) : BaseFunction::predecessor(this, 1, var);
					*var = res;
				}
				else {
					value* var = &(current->stack.back());

					value res = (c->command == command_kind::pc_inline_inc) ?
						BaseFunction::successor(this, 1, var) : BaseFunction::predecessor(this, 1, var);
					if (!res.has_data()) break;
					var->overwrite(res);
					current->stack.pop_back();
				}
				break;
			}
			case command_kind::pc_inline_add_asi:
			case command_kind::pc_inline_sub_asi:
			case command_kind::pc_inline_mul_asi:
			case command_kind::pc_inline_div_asi:
			case command_kind::pc_inline_mod_asi:
			case command_kind::pc_inline_pow_asi:
			case command_kind::pc_inline_cat_asi:
			{
				auto PerformFunction = [&](value* dest, command_kind cmd, value(&argv)[2]) {
#define DEF_CASE(_c, _fn) case _c: *dest = BaseFunction::_fn(this, 2, argv); break;
					switch (cmd) {
						DEF_CASE(command_kind::pc_inline_add_asi, add);
						DEF_CASE(command_kind::pc_inline_sub_asi, subtract);
						DEF_CASE(command_kind::pc_inline_mul_asi, multiply);
						DEF_CASE(command_kind::pc_inline_div_asi, divide);
						DEF_CASE(command_kind::pc_inline_mod_asi, remainder_);
						DEF_CASE(command_kind::pc_inline_pow_asi, power);
						DEF_CASE(command_kind::pc_inline_cat_asi, concatenate);
					}
#undef DEF_CASE
				};

				if (c->level >= 0) {
					value* var = find_variable_symbol(current.get(), c);
					if (var == nullptr) break;

					value arg[2] = { *var, current->stack.back() };
					value res;
					PerformFunction(&res, c->command, arg);

					current->stack.pop_back();
					*var = res;
				}
				else {
					value* dest = &(current->stack.back()) - 1;

					value arg[2] = { *dest, current->stack.back() };
					value res;
					PerformFunction(&res, c->command, arg);

					dest->overwrite(res);
					current->stack.pop_back(2U);
				}
				break;
			}
			case command_kind::pc_inline_neg:
			case command_kind::pc_inline_not:
			case command_kind::pc_inline_abs:
			{
				value res;

#define DEF_CASE(cmd, fn) case cmd: res = BaseFunction::fn(this, 1, &current->stack.back()); break;
				switch (c->command) {
					DEF_CASE(command_kind::pc_inline_neg, negative);
					DEF_CASE(command_kind::pc_inline_not, not_);
					DEF_CASE(command_kind::pc_inline_abs, absolute);
				}
#undef DEF_CASE

				current->stack.pop_back();
				current->stack.push_back(res);
				break;
			}
			case command_kind::pc_inline_add:
			case command_kind::pc_inline_sub:
			case command_kind::pc_inline_mul:
			case command_kind::pc_inline_div:
			case command_kind::pc_inline_mod:
			case command_kind::pc_inline_pow:
			case command_kind::pc_inline_app:
			case command_kind::pc_inline_cat:
			{
				value res;
				value* args = (value*)(&current->stack.back() - 1);

#define DEF_CASE(cmd, fn) case cmd: res = BaseFunction::fn(this, 2, args); break;
				switch (c->command) {
					DEF_CASE(command_kind::pc_inline_add, add);
					DEF_CASE(command_kind::pc_inline_sub, subtract);
					DEF_CASE(command_kind::pc_inline_mul, multiply);
					DEF_CASE(command_kind::pc_inline_div, divide);
					DEF_CASE(command_kind::pc_inline_mod, remainder_);
					DEF_CASE(command_kind::pc_inline_pow, power);
					DEF_CASE(command_kind::pc_inline_app, append);
					DEF_CASE(command_kind::pc_inline_cat, concatenate);
				}
#undef DEF_CASE

				current->stack.pop_back(2U);
				current->stack.push_back(res);
				break;
			}
			case command_kind::pc_inline_cmp_e:
			case command_kind::pc_inline_cmp_g:
			case command_kind::pc_inline_cmp_ge:
			case command_kind::pc_inline_cmp_l:
			case command_kind::pc_inline_cmp_le:
			case command_kind::pc_inline_cmp_ne:
			{
				value* args = (value*)(&current->stack.back() - 1);
				value cmp_res = BaseFunction::compare(this, 2, args);
				int cmp_r = cmp_res.as_int();

#define DEF_CASE(cmd, expr) case cmd: cmp_res.set(script_type_manager::get_boolean_type(), expr); break;
				switch (c->command) {
					DEF_CASE(command_kind::pc_inline_cmp_e, cmp_r == 0);
					DEF_CASE(command_kind::pc_inline_cmp_g, cmp_r > 0);
					DEF_CASE(command_kind::pc_inline_cmp_ge, cmp_r >= 0);
					DEF_CASE(command_kind::pc_inline_cmp_l, cmp_r < 0);
					DEF_CASE(command_kind::pc_inline_cmp_le, cmp_r <= 0);
					DEF_CASE(command_kind::pc_inline_cmp_ne, cmp_r != 0);
				}
#undef DEF_CASE

				current->stack.pop_back(2U);
				current->stack.push_back(cmp_res);
				break;
			}
			case command_kind::pc_inline_logic_and:
			case command_kind::pc_inline_logic_or:
			{
				value* var2 = &(current->stack.back());
				value* var1 = var2 - 1;

				bool b = c->command == command_kind::pc_inline_logic_and ?
					(var1->as_boolean() && var2->as_boolean()) : (var1->as_boolean() || var2->as_boolean());
				value res(script_type_manager::get_boolean_type(), b);

				current->stack.pop_back(2U);
				current->stack.push_back(res);
				break;
			}
			case command_kind::pc_inline_cast_var:
			{
				value* var = &(current->stack.back());
				BaseFunction::_value_cast(var, (type_data::type_kind)c->ip);
				break;
			}
			case command_kind::pc_inline_index_array:
			{
				value* var = &(current->stack.back()) - 1;
				const value& res = BaseFunction::index(this, 2, var);
				if (c->ip) res.unique();
				current->stack.pop_back(2U);
				current->stack.push_back(res);
				break;
			}
			case command_kind::pc_inline_length_array:
			{
				value* var = &(current->stack.back());
				size_t len = var->length_as_array();
				var->set(script_type_manager::get_int_type(), (int64_t)len);
				break;
			}
			//default:
			//	assert(false);
			}
		}
	}
}
value* script_machine::find_variable_symbol(environment* current_env, code* var_data) {
	/*
	auto err_uninitialized = [&]() {
#ifdef _DEBUG
		std::wstring error = L"Variable hasn't been initialized";
		error += StringUtility::FormatToWide(": %s\r\n", var_data->var_name.c_str());
		raise_error(error);
#else
		raise_error(L"Variable hasn't been initialized.\r\n");
#endif	
	};
	*/

	for (environment* i = current_env; i != nullptr; i = (i->parent).get()) {
		if (i->sub->level == var_data->level) {
			variables_t& vars = i->variables;

			if (vars[var_data->variable].has_data())
				return &vars[var_data->variable];
			else break;
		}
	}

	raise_error(L"Variable hasn't been initialized.\r\n");
	return nullptr;
}