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

	null_type = deref_itr(types.insert(type_data(type_data::tk_null)).first);
	int_type = deref_itr(types.insert(type_data(type_data::tk_int)).first);
	real_type = deref_itr(types.insert(type_data(type_data::tk_real)).first);
	char_type = deref_itr(types.insert(type_data(type_data::tk_char)).first);
	boolean_type = deref_itr(types.insert(type_data(type_data::tk_boolean)).first);
	ptr_type = deref_itr(types.insert(type_data(type_data::tk_pointer)).first);

	null_array_type = deref_itr(types.insert(type_data(type_data::tk_array)).first);

	string_type = deref_itr(types.insert(type_data(type_data::tk_array,
		char_type)).first);		//Char array (string)
	int_array_type = deref_itr(types.insert(type_data(type_data::tk_array,
		int_type)).first);		//Int array
	real_array_type = deref_itr(types.insert(type_data(type_data::tk_array,
		real_type)).first);		//Real array

	//Bool array
	types.insert(type_data(type_data::tk_array, boolean_type));
	//String array
	types.insert(type_data(type_data::tk_array, string_type));
}

type_data* script_type_manager::get_type(type_data* type) {
	auto itr = types.find(*type);
	if (itr == types.end()) {
		//No type found, insert and return the new type
		itr = types.insert(*type).first;
	}
	return deref_itr(itr);
}
type_data* script_type_manager::get_type(type_data::type_kind kind) {
	type_data target = type_data(kind);
	return get_type(&target);
}
type_data* script_type_manager::get_array_type(type_data* element) {
	type_data target = type_data(type_data::tk_array, element);
	return get_type(&target);
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

	reset();
}
script_machine::~script_machine() {
}

script_machine::environment::environment(ref_unsync_ptr<environment> parent, script_block* b) {
	this->parent = parent;
	sub = b;
	ip = 0;
	variables.clear();
	stack.clear();
	has_result = false;
	waitCount = 0;
}
script_machine::environment::~environment() {
	variables.clear();
	stack.clear();
}

bool script_machine::has_event(const std::string& event_name, std::map<std::string, script_block*>::iterator& res) {
	res = engine->events.find(event_name);
	return res != engine->events.end();
}
int script_machine::get_current_line() {
	ref_unsync_ptr<environment>& current = *current_thread_index;
	return (current->sub->codes[current->ip]).line;
}

void script_machine::reset() {
	error = false;
	error_message = L"";
	error_line = -1;

	bTerminate = false;

	finished = false;
	stopped = false;
	resuming = false;

	list_parent_environment.clear();
	threads.clear();
	current_thread_index = std::list<ref_unsync_ptr<environment>>::iterator();
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
			script_value_vector& stack = current->stack;
			script_value_vector& variables = current->variables;

			code* c = &(current->sub->codes[current->ip]);
			error_line = c->line;
			++(current->ip);

			switch (c->command) {
			case command_kind::pc_wait:
			{
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
				variables.resize(c->arg0);
				variables.length = c->arg0;
				break;
			case command_kind::pc_var_format:
			{
				for (size_t i = c->arg0; i < c->arg0 + c->arg1; ++i) {
					if (i >= variables.capacity) break;
					variables[i] = value();
				}
				break;
			}

			case command_kind::pc_pop:
				stack.pop_back(c->arg0);
				break;
			case command_kind::pc_push_value:
				stack.push_back(c->data);
				//stack.back().make_unique();
				break;
			case command_kind::pc_push_variable:
			case command_kind::pc_push_variable2:
			{
				value* var = find_variable_symbol(current.get(), c, 
					c->arg0, c->arg1, c->arg2);
				if (var == nullptr) break;

				if (c->command == command_kind::pc_push_variable)
					stack.push_back(*var);
				else
					stack.push_back(value(script_type_manager::get_ptr_type(), var));

				break;
			}
			case command_kind::pc_dup_n:
			{
				if (c->arg0 >= stack.size()) break;
				value* val = &stack.back() - c->arg0;
				stack.push_back(*val);
				//stack.back().make_unique();
				break;
			}
			case command_kind::pc_swap:
			{
				size_t len = stack.size();
				if (len < 2) break;
				std::swap(stack[len - 1], stack[len - 2]);
				break;
			}
			case command_kind::pc_load_ptr:
			{
				if (c->arg0 >= stack.size()) break;
				value* val = &stack.back() - c->arg0;
				stack.push_back(value(script_type_manager::get_ptr_type(), val));
				break;
			}
			case command_kind::pc_unload_ptr:
			{
				value* val = &stack.back();
				value* valAtPtr = val->as_ptr();
				*val = *valAtPtr;
				break;
			}
			case command_kind::pc_make_unique:
			{
				if (c->arg0 >= stack.size()) break;
				value* val = &stack.back() - c->arg0;
				val->make_unique();
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
				value* top = &stack.back();
				bool bJE = c->command == command_kind::pc_jump_if_nopop;
				if ((bJE && top->as_boolean()) || (!bJE && !top->as_boolean()))
					current->ip = c->arg0;
				break;
			}

			case command_kind::pc_copy_assign:
			case command_kind::pc_ref_assign:
			{
				if (c->command == command_kind::pc_copy_assign) {
					value* dest = find_variable_symbol(current.get(), c,
						c->arg0, c->arg1, true);
					value* src = &stack.back();

					if (dest != nullptr && src != nullptr) {
						//pc_copy_assign
						if (BaseFunction::_type_assign_check(this, src, dest)) {
							type_data* prev_type = dest->get_type();

							*dest = *src;
							dest->make_unique();

							if (prev_type && prev_type != src->get_type())
								BaseFunction::_value_cast(dest, prev_type);
						}
					}
					stack.pop_back();
				}
				else {		//pc_ref_assign
					value* src = &stack.back();
					value* dest = src[-1].as_ptr();

					if (dest != nullptr && src != nullptr) {
						if (BaseFunction::_type_assign_check(this, src, dest)) {
							type_data* prev_type = dest->get_type();

							*dest = *src;

							if (prev_type && prev_type != src->get_type())
								BaseFunction::_value_cast(dest, prev_type);
						}
					}
					stack.pop_back(2U);
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
				//assert(current_stack.size() >= c->arguments);

				if (stack.size() < c->arg1) {
					std::string error = StringUtility::Format(
						"Unexpected script error: Stack size[%d] is less than the number of arguments[%d].\r\n",
						stack.size(), c->arg1);
					raise_error(error);
					break;
				}

				script_block* sub = c->block; //(script_block*)c->arg0
				if (sub->func) {
					//Default functions

					size_t sizePrev = stack.size();

					value* argv = nullptr;
					if (sizePrev > 0 && c->arg1 > 0)
						argv = stack.at + (sizePrev - c->arg1);
					value ret = sub->func(this, c->arg1, argv);

					if (stopped) {
						--(current->ip);
					}
					else {
						resuming = false;

						//Erase the passed arguments from the stack
						stack.pop_back(c->arg1);

						//Return value
						if (c->command == command_kind::pc_call_and_push_result)
							stack.push_back(ret);
					}
				}
				else if (sub->kind == block_kind::bk_microthread) {
					//Tasks
					ref_unsync_ptr<environment> e = new environment(current, sub);
					threads.insert(++current_thread_index, e);
					--current_thread_index;

					//Pass the arguments
					for (size_t i = 0; i < c->arg1; ++i) {
						e->stack.push_back(stack.back());
						stack.pop_back();
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
						e->stack.push_back(stack.back());
						stack.pop_back();
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
				value* t = &stack.back();
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
				t->reset(script_type_manager::get_boolean_type(), b);
				break;
			}

			//Loop commands
			case command_kind::pc_loop_ascent:
			case command_kind::pc_loop_descent:
			{
				value* cmp_arg = &stack.back() - 1;
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
				stack.push_back(value(script_type_manager::get_boolean_type(), bSkip));

				//stack.pop_back(2U);
				break;
			}
			case command_kind::pc_loop_count:
			{
				value* i = &stack.back();
				int64_t r = i->as_int();
				if (r > 0)
					i->reset(script_type_manager::get_int_type(), r - 1);
				stack.push_back(value(script_type_manager::get_boolean_type(), r > 0));
				break;
			}
			case command_kind::pc_loop_foreach:
			{
				//Stack: .... [array] [counter]
				value* i = &stack.back();
				value* src_array = i - 1;

				std::vector<value>::iterator itrCur = src_array->array_get_begin() + i->as_int();
				std::vector<value>::iterator itrEnd = src_array->array_get_end();

				bool bSkip = false;
				if (src_array->get_type()->get_kind() != type_data::tk_array || itrCur >= itrEnd) {
					bSkip = true;
				}
				else {
					stack.push_back(*itrCur);
					//stack.back().make_unique();
					i->set(i->get_type(), i->as_int() + 1i64);
				}

				stack.push_back(value(script_type_manager::get_boolean_type(), bSkip));
				break;
			}

			case command_kind::pc_construct_array:
			{
				if (c->arg0 == 0U) {
					stack.push_back(BaseFunction::_create_empty(script_type_manager::get_null_array_type()));
					break;
				}

				std::vector<value> res_arr;
				res_arr.resize(c->arg0);

				value* val_ptr = &stack.back() - c->arg0 + 1;

				type_data* type_elem = val_ptr->get_type();
				type_data* type_arr = script_type_manager::get_instance()->get_array_type(type_elem);

				value res;
				for (size_t iVal = 0U; iVal < c->arg0; ++iVal, ++val_ptr) {
					BaseFunction::_append_check(this, type_arr, val_ptr->get_type());
					{
						value appending = *val_ptr;
						if (appending.get_type()->get_kind() != type_elem->get_kind()) {
							appending.make_unique();
							BaseFunction::_value_cast(&appending, type_elem);
						}
						res_arr[iVal] = appending;
					}
				}
				res.reset(type_arr, res_arr);

				stack.pop_back(c->arg0);
				stack.push_back(res);
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
					value* var = stack.back().as_ptr();
					if (!var->has_data()) break;
					value res = (c->command == command_kind::pc_inline_inc) ?
						BaseFunction::successor(this, 1, var) : BaseFunction::predecessor(this, 1, var);
					*var = res;
					if (c->arg1)
						stack.pop_back();
				}
				break;
			}
			case command_kind::pc_inline_add_asi:
			case command_kind::pc_inline_sub_asi:
			case command_kind::pc_inline_mul_asi:
			case command_kind::pc_inline_div_asi:
			case command_kind::pc_inline_fdiv_asi:
			case command_kind::pc_inline_mod_asi:
			case command_kind::pc_inline_pow_asi:
			//case command_kind::pc_inline_cat_asi:
			{
				auto PerformFunction = [&](value* dest, command_kind cmd, value* argv) {
#define DEF_CASE(_c, _fn) case _c: *dest = BaseFunction::_fn(this, 2, argv); break;
					switch (cmd) {
						DEF_CASE(command_kind::pc_inline_add_asi, add);
						DEF_CASE(command_kind::pc_inline_sub_asi, subtract);
						DEF_CASE(command_kind::pc_inline_mul_asi, multiply);
						DEF_CASE(command_kind::pc_inline_div_asi, divide);
						DEF_CASE(command_kind::pc_inline_fdiv_asi, fdivide);
						DEF_CASE(command_kind::pc_inline_mod_asi, remainder_);
						DEF_CASE(command_kind::pc_inline_pow_asi, power);
						//DEF_CASE(command_kind::pc_inline_cat_asi, concatenate);
					}
#undef DEF_CASE
				};

				value res;
				if (c->arg0) {
					value* dest = find_variable_symbol(current.get(), c, 
						c->arg1, c->arg2, false);
					if (dest == nullptr) break;

					value arg[2] = { *dest, stack.back() };
					PerformFunction(&res, c->command, arg);

					BaseFunction::_value_cast(&res, dest->get_type());
					*dest = res;

					stack.pop_back();
				}
				else {
					value* pArg = &stack.back() - 1;
					value& pDest = *(pArg->as_ptr());

					value arg[2] = { pDest, pArg[1] };
					PerformFunction(&res, c->command, arg);

					BaseFunction::_value_cast(&res, pDest.get_type());
					pDest = res;

					stack.pop_back(2U);
				}
				break;
			}
			case command_kind::pc_inline_cat_asi:
			{
				if (c->arg0) {
					value* dest = find_variable_symbol(current.get(), c,
						c->arg1, c->arg2, false);
					if (dest == nullptr) break;

					value arg[2] = { *dest, stack.back() };
					BaseFunction::concatenate_direct(this, 2, arg);

					stack.pop_back();
				}
				else {
					value* pArg = &stack.back() - 1;

					value arg[2] = { *(pArg->as_ptr()), pArg[1] };
					BaseFunction::concatenate_direct(this, 2, arg);

					stack.pop_back(2U);
				}
				break;
			}
			case command_kind::pc_inline_neg:
			case command_kind::pc_inline_not:
			case command_kind::pc_inline_abs:
			{
				value res;
				value* arg = &stack.back();

#define DEF_CASE(cmd, fn) case cmd: res = BaseFunction::fn(this, 1, arg); break;
				switch (c->command) {
					DEF_CASE(command_kind::pc_inline_neg, negative);
					DEF_CASE(command_kind::pc_inline_not, not_);
					DEF_CASE(command_kind::pc_inline_abs, absolute);
				}
#undef DEF_CASE

				arg->make_unique();
				*arg = res;
				break;
			}
			case command_kind::pc_inline_add:
			case command_kind::pc_inline_sub:
			case command_kind::pc_inline_mul:
			case command_kind::pc_inline_div:
			case command_kind::pc_inline_fdiv:
			case command_kind::pc_inline_mod:
			case command_kind::pc_inline_pow:
			case command_kind::pc_inline_app:
			case command_kind::pc_inline_cat:
			{
				value res;
				value* args = &stack.back() - 1;

#define DEF_CASE(cmd, fn) case cmd: res = BaseFunction::fn(this, 2, args); break;
				switch (c->command) {
					DEF_CASE(command_kind::pc_inline_add, add);
					DEF_CASE(command_kind::pc_inline_sub, subtract);
					DEF_CASE(command_kind::pc_inline_mul, multiply);
					DEF_CASE(command_kind::pc_inline_div, divide);
					DEF_CASE(command_kind::pc_inline_fdiv, fdivide);
					DEF_CASE(command_kind::pc_inline_mod, remainder_);
					DEF_CASE(command_kind::pc_inline_pow, power);
					DEF_CASE(command_kind::pc_inline_app, append);
					DEF_CASE(command_kind::pc_inline_cat, concatenate);
				}
#undef DEF_CASE

				stack.pop_back(2U);
				stack.push_back(res);
				break;
			}
			case command_kind::pc_inline_cmp_e:
			case command_kind::pc_inline_cmp_g:
			case command_kind::pc_inline_cmp_ge:
			case command_kind::pc_inline_cmp_l:
			case command_kind::pc_inline_cmp_le:
			case command_kind::pc_inline_cmp_ne:
			{
				value* args = &stack.back() - 1;
				value cmp_res = BaseFunction::compare(this, 2, args);
				int cmp_r = cmp_res.as_int();

#define DEF_CASE(cmd, expr) case cmd: cmp_res.reset(script_type_manager::get_boolean_type(), expr); break;
				switch (c->command) {
					DEF_CASE(command_kind::pc_inline_cmp_e, cmp_r == 0);
					DEF_CASE(command_kind::pc_inline_cmp_g, cmp_r > 0);
					DEF_CASE(command_kind::pc_inline_cmp_ge, cmp_r >= 0);
					DEF_CASE(command_kind::pc_inline_cmp_l, cmp_r < 0);
					DEF_CASE(command_kind::pc_inline_cmp_le, cmp_r <= 0);
					DEF_CASE(command_kind::pc_inline_cmp_ne, cmp_r != 0);
				}
#undef DEF_CASE

				stack.pop_back(2U);
				stack.push_back(cmp_res);
				break;
			}
			case command_kind::pc_inline_logic_and:
			case command_kind::pc_inline_logic_or:
			{
				value* var2 = &stack.back();
				value* var1 = var2 - 1;

				bool b = c->command == command_kind::pc_inline_logic_and ?
					(var1->as_boolean() && var2->as_boolean()) : (var1->as_boolean() || var2->as_boolean());
				value res(script_type_manager::get_boolean_type(), b);

				stack.pop_back(2U);
				stack.push_back(res);
				break;
			}
			case command_kind::pc_inline_cast_var:
			{
				value* var = &stack.back();

				type_data* castFrom = var->get_type();
				type_data* castTo = (type_data*)c->arg0;

				if (c->arg1) {
					if (castTo && castFrom != castTo) {
						if (BaseFunction::_type_assign_check(this, castFrom, castTo)) {
							BaseFunction::_value_cast(var, castTo);
						}
					}
				}
				else BaseFunction::_value_cast(var, castTo);

				break;
			}
			case command_kind::pc_inline_index_array:
			{
				value* arr = &stack.back() - 1;
				value* idx = arr + 1;

				value* pRes = (value*)BaseFunction::index(this, 2, arr->as_ptr(), idx);
				if (pRes == nullptr) break;
				
				*arr = value(script_type_manager::get_ptr_type(), pRes);
				stack.pop_back(1U);	//pop idx
				break;
			}
			case command_kind::pc_inline_index_array2:
			{
				value* arr = &stack.back() - 1;
				value* idx = arr + 1;

				value* pRes = (value*)BaseFunction::index(this, 2, arr, idx);
				if (pRes == nullptr) break;
				value res = *pRes;

				stack.pop_back(2U);
				stack.push_back(res);
				break;
			}
			case command_kind::pc_inline_length_array:
			{
				value* var = &stack.back();
				size_t len = var->length_as_array();
				var->reset(script_type_manager::get_int_type(), (int64_t)len);
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
			value* res = &(i->variables[variable]);

			if (allow_null || res->has_data())
				return res;
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
	raise_error(StringUtility::Format("Variable not found (level=%u,id=%u)\r\n", 
		level, variable));
#endif
	return nullptr;
}