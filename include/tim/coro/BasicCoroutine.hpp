#ifndef TIM_CORO_BASIC_CORTOUTINE_HPP
#define TIM_CORO_BASIC_CORTOUTINE_HPP

#include <csetjmp>

namespace tim::coro {

inline namespace basic_coroutine {

struct ExecutionContext;

enum class ResumptionKind: int {
	Continue,
	Start,
	End
};

ResumptionKind resume_execution(ExecutionContext&, ResumptionKind);

[[noreturn]]
void coroutine_entry_point(void (*start_function)(void*, ExecutionContext*), void* start_function_args, ExecutionContext* caller_context);

template <class YieldResultType, class ResumeArgType = void>
struct Yieldable;
template <class YieldResultType, class ResumeArgType = void>
struct Resumable;


template <class YieldResultType, class ResumeArgType>
struct Yieldable {
	using sibling_type = Resumable<YieldResultType, ResumeArgType>;
	using resume_arg_maker_type = MakeResumeArg;

	Yieldable() = delete;
	Yieldable(const Yieldable&) = delete;
	Yieldable(Yieldable&&) = delete;

	Yieldable& operator=(const Yieldable&) = delete;
	Yieldable& operator=(Yieldable&&) = delete;

	template <class T, std::enable_if_t<std::is_constructible_v<YieldResultType, T&&>, bool> = false>
	ResumeArgType yield(T&& value) noexcept(std::is_nothrow_constructible_v<YieldResultType, T&&>) {
		si
		return 
	}
	
private:
	ResumeArgType yield_to_sibling() noexcept;

	template <>
	friend class Resumable<YieldResultType, ResumeArgType>;

	BasicYieldable(sigbling_type* sibling):
		sibling_(sibling)
	{

	}

	sibling_type* sibling_;
	YieldResultType (*make_resume_arg_)(
};



template <class YieldResultType, class ResumeArgType = void>
struct Yieldable {
	
	Yieldable() = delete;
	Yieldable(const Yieldable&) = delete;
	Yieldable(Yieldable&&) = delete;

	Yieldable& operator=(const Yieldable&) = delete;
	Yieldable& operator=(Yieldable&&) = delete;

	template <class T, std::enable_if_t<std::is_constructible_v<YieldResultType, T&&>, bool> = false>
	ResumeArgType yield(T&& value) noexcept(std::is_nothrow_constructible_v<YieldResultType, T&&>) {
		return 
	}
	
	
private:
	Generator<T>* 
	ExecutionContext* caller_context_;
};

template <class T, class OnResume>
struct Generator {
	using yieldable_type = Yieldable<T, OnResume>;
	struct YieldableDeleter {
		constexpr void operator()(yieldable_type* yieldable) {
			yieldable->destroy();
		}
	};

	

private:
	std::unique_ptr<Yieldable
	OnResume on_resume_;
};

template <class Func, class ... Args>
Generator<T> make_generator(Func fn, Args&& ... args) {
	
}


struct BasicCoroutineHandle {

	
private:
	std::jmp_buf
};
	
template <class Resum

} /* inline namespace basic_coroutine */

} /* namespace tim::coro */


#endif /* TIM_CORO_BASIC_CORTOUTINE_HPP */
