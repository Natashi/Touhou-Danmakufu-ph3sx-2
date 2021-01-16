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

	null_array_type = deref_itr(types.insert(type_data(type_data::tk_array)).first);

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

script_engine::script_engine(const std::string& source, std::vector<function>* list_func, std::vector<constant>* list_const) {
	main_block = new_block(1, block_kind::bk_normal);

	const char* end = &source[0] + source.size();
	script_scanner s(source.c_str(), end);
	parser p(this, &s);

	if (list_func) p.load_functions(list_func);
	if (list_const) p.load_constants(list_const);
	p.begin_parse();

	events = p.events;

	error = p.error;
	error_message = p.error_message;
	error_line = p.error_line;
}

script_engine::script_engine(const std::vector<char>& source, std::vector<function>* list_func, std::vector<constant>* list_const) {
	main_block = new_block(1, block_kind::bk_normal);

	if (false) {
		wchar_t* pStart = (wchar_t*)&source[0];
		wchar_t* pEnd = (wchar_t*)(&source[0] + std::min(source.size(), 64U));
		std::wstring str = std::wstring(pStart, pEnd);
		//Logger::WriteTop(str);
	}
	const char* end = &source[0] + source.size();
	script_scanner s(&source[0], end);
	parser p(this, &s);

	if (list_func) p.load_functions(list_func);
	if (list_const) p.load_constants(list_const);
	p.begin_parse();

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

script_machine::environment::environment(ref_unsync_ptr<environment> parent, script_block* b) {
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
	ref_unsync_ptr<environment>& current = *current_thread_index;
	return (current->sub->codes[current->ip]).line;
}

void script_machine::run() {
	if (bTerminate) return;

	assert(!error);
	if (threads.size() == 0) {
		error_line = -1;

		threads.push_back(new environment(nullptr, engine->main_block));
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

		ref_unsync_ptr<environment>& env_first = *current_thread_index;
		env_first = new environment(env_first, event_itr->second);

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
	ref_unsync_ptr<environment> current = *current_thread_index;

	while (!finished && !bTerminate) {
		if (current->waitCount > 0) {
			yield();
			--current->waitCount;
			current = *current_thread_index;
			continue;
		}

		if (current->ip >= current->sub->codes.size()) {	//Routine finished
			ref_unsync_ptr<environment>& parent = current->parent;

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
				value* t = &stack.back();
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
				current->variables.resize(c->arg0);
				current->variables.length = c->arg0;
				break;
			case command_kind::pc_var_format:
			{
				for (size_t i = c->arg0; i < c->arg0 + c->arg1; ++i) {
					if (i >= current->variables.capacity) break;
					current->variables[i] = value();
				}
				break;
			}

			case command_kind::pc_pop:
				current->stack.pop_back(c->arg0);
				break;
			case command_kind::pc_push_value:
				current->stack.push_back(c->data);
				break;
			case command_kind::pc_push_variable:
			{
				value* var = find_variable_symbol(current.get(), c, 
					c->arg0, c->arg1, c->arg2);
				if (var == nullptr) break;
				current->stack.push_back(*var);
				break;
			}
			case command_kind::pc_dup_n:
			{
				stack_t& stack = current->stack;
				if (c->arg0 >= stack.size()) break;
				value* val = &stack.back() - c->arg0;
				stack.push_back(*val);
				break;
			}
			case command_kind::pc_swap:
			{
				size_t len = current->stack.size();
				if (len < 2) break;
				std::swap(current->stack[len - 1], current->stack[len - 2]);
				break;
			}
			case command_kind::pc_make_unique:
			{
				stack_t& stack = current->stack;
				if (c->arg0 >= stack.size()) break;
				value* val = &stack.back() - c->arg0;
				val->unique();
				break;
			}

			//case command_kind::_pc_jump_target:
			//	break;
			case command_kind::pc_jump:
				current->ip = c->arg0;
				break;
			case command_kind::pc_jump_if:
			case command_kind::pc_jump_if_not:
			{
				stack_t& stack = current->stack;
				value* top = &stack.back();
				bool bJE = c->command == command_kind::pc_jump_if;
				if ((bJE && top->as_boolean()) || (!bJE && !top->as_boolean()))
					current->ip = c->arg0;
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
					current->ip = c->arg0;
				break;
			}

			case command_kind::pc_copy_assign:
			case command_kind::pc_overwrite_assign:
			case command_kind::pc_ref_assign:
			{
				stack_t& stack = current->stack;

				if (c->command != command_kind::pc_ref_assign) {
					if (c->command == command_kind::pc_copy_assign) {
						value* dest = find_variable_symbol(current.get(), c, 
							c->arg0, c->arg1, true);
						value* src = &stack.back();

						if (dest != nullptr && src != nullptr) {
							//pc_copy_assign
							if (BaseFunction::_type_assign_check(this, src, dest)) {
								type_data* prev_type = dest->get_type();

								*dest = *src;
								dest->unique();

								if (prev_type && prev_type != src->get_type())
									BaseFunction::_value_cast(dest, prev_type->get_kind());
							}
						}
						stack.pop_back();
					}
					else {		//pc_overwrite_assign
						value* src = &stack.back();
						value* dest = src - 1;

						if (dest != nullptr && src != nullptr) {
							if (BaseFunction::_type_assign_check(this, src, dest)) {
								type_data* prev_type = dest->get_type();

								dest->overwrite(*src);

								if (prev_type && prev_type != src->get_type())
									BaseFunction::_value_cast(dest, prev_type->get_kind());
							}
						}
						stack.pop_back(2U);
					}
				}
				else {			//pc_ref_assign (unfinished)
					
				}

				break;
			}

			case command_kind::pc_sub_return:
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

				if (current_stack.size() < c->arg1) {
					std::string error = StringUtility::Format(
						"Unexpected script error: Stack size[%d] is less than the number of arguments[%d].\r\n",
						current_stack.size(), c->arg1);
					raise_error(error);
					break;
				}

				script_block* sub = c->block; //(script_block*)c->arg0
				if (sub->func) {
					//Default functions

					size_t sizePrev = current_stack.size();

					value* argv = nullptr;
					if (sizePrev > 0 && c->arg1 > 0)
						argv = current_stack.at + (sizePrev - c->arg1);
					value ret = sub->func(this, c->arg1, argv);

					if (stopped) {
						--(current->ip);
					}
					else {
						resuming = false;

						//Erase the passed arguments from the stack
						current_stack.pop_back(c->arg1);

						//Return value
						if (c->command == command_kind::pc_call_and_push_result)
							current_stack.push_back(ret);
					}
				}
				else if (sub->kind == block_kind::bk_microthread) {
					//Tasks
					ref_unsync_ptr<environment> e = new environment(current, sub);
					threads.insert(++current_thread_index, e);
					--current_thread_index;

					//Pass the arguments
					for (size_t i = 0; i < c->arg1; ++i) {
						e->stack.push_back(current_stack.back());
						current_stack.pop_back();
					}

					current = *current_thread_index;
				}
				else {
					//User-defined functions or internal blocks
					ref_unsync_ptr<environment> e = new environment(current, sub);
					e->has_result = c->command == command_kind::pc_call_and_push_result;
					*current_thread_index = e;

					//Pass the arguments
					for (size_t i = 0; i < c->arg1; ++i) {
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

			//Loop commands
			case command_kind::pc_loop_ascent:
			case command_kind::pc_loop_descent:
			{
				value* cmp_arg = &current->stack.back() - 1;
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
				if (c->arg0 == 0U) {
					current->stack.push_back(value(script_type_manager::get_null_array_type(), 0i64));
					break;
				}

				std::vector<value> res_arr;
				res_arr.resize(c->arg0);

				value* val_ptr = &current->stack.back() - c->arg0 + 1;

				type_data* type_elem = val_ptr->get_type();
				type_data* type_arr = script_type_manager::get_instance()->get_array_type(type_elem);

				value res;
				for (size_t iVal = 0U; iVal < c->arg0; ++iVal, ++val_ptr) {
					BaseFunction::_append_check(this, type_arr, val_ptr->get_type());
					{
						value appending = *val_ptr;
						if (appending.get_type()->get_kind() != type_elem->get_kind()) {
							appending.unique();
							BaseFunction::_value_cast(&appending, type_elem->get_kind());
						}
						res_arr[iVal] = appending;
					}
				}
				res.set(type_arr, res_arr);

				current->stack.pop_back(c->arg0);
				current->stack.push_back(res);
				break;
			}

			//----------------------------------Inline operations----------------------------------
			case command_kind::pc_inline_inc:
			case command_kind::pc_inline_dec:
			{
				if (c->arg0) {
					value* var = find_variable_symbol(current.get(), c, 
						c->arg1, c->arg2, false);
					if (var == nullptr) break;
					value res = (c->command == command_kind::pc_inline_inc) ?
						BaseFunction::successor(this, 1, var) : BaseFunction::predecessor(this, 1, var);
					*var = res;
				}
				else {
					value* var = &current->stack.back();
					value res = (c->command == command_kind::pc_inline_inc) ?
						BaseFunction::successor(this, 1, var) : BaseFunction::predecessor(this, 1, var);
					if (!var->has_data()) break;
					var->overwrite(res);
					if (c->arg1) current->stack.pop_back();
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
				auto PerformFunction = [&](value* dest, command_kind cmd, value* argv) {
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

				value res;
				if (c->arg0) {
					value* dest = find_variable_symbol(current.get(), c, 
						c->arg1, c->arg2, false);
					if (dest == nullptr) break;

					value arg[2] = { *dest, current->stack.back() };
					PerformFunction(&res, c->command, arg);

					*dest = res;
					current->stack.pop_back();
				}
				else {
					value* dest = &current->stack.back() - 1;

					PerformFunction(&res, c->command, dest);

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

				current->stack.back() = res;
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
				value* args = &current->stack.back() - 1;

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
				value* args = &current->stack.back() - 1;
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
				value* var2 = &current->stack.back();
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
				value* var = &current->stack.back();
				BaseFunction::_value_cast(var, (type_data::type_kind)c->arg0);
				break;
			}
			case command_kind::pc_inline_index_array:
			{
				value* var = &current->stack.back() - 1;
				value* res = (value*)BaseFunction::index(this, 2, var);
				if (res == nullptr) break;
				if (c->arg0)
					res->unique();
				value res_copy = *res;
				current->stack.pop_back(2U);
				current->stack.push_back(res_copy);
				break;
			}
			case command_kind::pc_inline_length_array:
			{
				value* var = &current->stack.back();
				size_t len = var->length_as_array();
				var->set(script_type_manager::get_int_type(), (int64_t)len);
				break;
			}
			}
		}
	}
}

value* script_machine::find_variable_symbol(environment* current_env, code* c, 
	uint32_t level, uint32_t variable, bool allow_null) 
{
	for (environment* i = current_env; i != nullptr; i = (i->parent).get()) {
		if (i->sub->level == level) {
			value& res = i->variables[variable];

			if (allow_null || res.has_data())
				return &res;
			else {
#ifdef _DEBUG
				raise_error(StringUtility::Format("Variable hasn't been initialized: %s\r\n", 
					c->var_name.c_str()));
#else
				raise_error("Variable hasn't been initialized.\r\n");
#endif
				return nullptr;
			}
		}
	}

#ifdef _DEBUG
	raise_error(StringUtility::Format("Variable not found: %s (level=%u,id=%u)\r\n", 
		c->var_name.c_str(), level, variable));
#else
	raise_error(StringUtility::Format("Variable not found. (level=%u,id=%u)\r\n", 
		level, variable));
#endif
	return nullptr;
}