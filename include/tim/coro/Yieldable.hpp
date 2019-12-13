#ifndef TIM_CORO_BASIC_CORTOUTINE_HPP
#define TIM_CORO_BASIC_CORTOUTINE_HPP

#include <csetjmp>

namespace tim::coro {

inline namespace basic_coroutine {

enum class ResumptionKind: int {
	Continue=1,
	End
};

class NoResultAvailableException: std::exception {
	NoResultAvailableException() = default;
	
	virtual const char* what() const override {
		return "No result available";
	}

};

struct ExecutionContext {
	ExecutionContext() = default;
	ExecutionContext(const ExecutionContext&) = default;
	ExecutionContext(ExecutionContext&&) = default;
	
	ExecutionContext& operator=(const ExecutionContext&) = default;
	ExecutionContext& operator=(ExecutionContext&&) = default;

	ResumptionKind yield_to(ExecutionContext& other, ResumptionKind kind) {
		switch(setjmp(ctx_)) {
		default:
			assert(false);
		case 0:
			std::longjmp(&other.ctx_, static_cast<int>(kind));
			assert(false);
			break;
		case (int)ResumptionKind::Continue:
			return ResumptionKind::Continue;
		case (int)ResumptionKind::End:
			return ResumptionKind::End;
		}
	}

private:
	std::jmp_buf ctx_;
};

[[noreturn]]
void coroutine_entry_point(void (*start_function)(void*, ExecutionContext*), void* start_function_args, ExecutionContext* caller_context);

[[noreturn]]
void force_end_execution(ExecutionContext& current_);

template <class T>
struct YieldResult: ExecutionContext {
	using result_builder_type = T (*)(YieldResult<T>*);
protected:
	YieldResult() = default;
	YieldResult(const YieldResult&) = delete;
	constexpr YieldResult(YieldResult&& other) noexcept:
		ExecutionContext(static_cast<ExecutionContext&&>(other)),
		type_erased_result_builder_(other.type_erased_result_builder_.exchange(nullptr))
	{
		
	}

	YieldResult& operator=(const YieldResult&) = delete;
	YieldResult& operator=(YieldResult&& other) = delete; 

	template <class ... Args>
	constexpr YieldResult(result_builder_type builder):
		ExecutionContext(),
		type_erased_result_builder_(builder)
	{
		
	}

public:
	struct Handle {
		Handle() = default;

		constexpr Handle(YieldResult& res) noexcept:
			builder_(res.type_erased_result_builder_.exchange(nullptr)),
			yield_result_(&res)
		{

		}

		constexpr operator bool() const {
			return static_cast<bool>(builder_);
		}

		constexpr T operator() {
			assert(*this);
			return builder_(yield_result_);
		}

	private:
		result_builder_type builder_ = nullptr;
		YieldResult *yield_result_ = nullptr;
	};

	constexpr Handle get_result_handle() noexcept {
		return Handle(*this);
	}

private:
	std::atomic<result_builder_type> type_erased_result_builder_;
};

template <class T, class Fn>
struct YieldResultBuilder: YieldResult<T> {
	static_assert(std::is_invocable_r<T, Fn&>::value, "");
	

	YieldResultBuilder() = default;

	template <class ... Args, std::enable_if_t<std::is_constructible_v<Fn, Args&&...>, bool> = false>
	constexpr YieldResultBuilder(std::in_place_t, Args&& ... args) noexcept(std::is_nothrow_constructible_v<Fn, Args&&...>):
		YieldResult<T>(+[](YieldResult<T>* self) -> T {
			return static_cast<YieldResultBuilder*>(self)->builder_();
		})
	{
		
	}

	
private:
	Fn builder_;
};

template <class T, class Fn>
constexpr auto make_yield_result_builder(Fn&& fn)
	-> YieldResultBuilder<T, std::decay_t<Fn>>
{
	return YieldResultBuilder<T, std::decay_t<Fn>>(std::in_place, std::forward<Fn>(fn));
}


struct YieldableBase {
protected:
	struct State {
		State() = default;
		explicit constexpr State(std::nullptr_t): State() {}
		explicit constexpr State(ExecutionContext* c): context_or_caller_{c} {}
		explicit constexpr State(YieldableBase* c): context_or_caller_{c} {}

		constexpr YieldableBase* caller() const {
			return static_cast<YieldableBase*>(context_or_caller_);
		}

		constexpr ExecutionContext* suspended_context() const {
			return static_cast<ExecutionContext*>(context_or_caller_);
		}

		constexpr operator bool() const {
			return static_cast<bool>(context_or_caller_);
		}

	private:
		void* result_or_caller_ = nullptr;
	};

public:
	YieldableBase() = delete;

	explicit YieldableBase(std::nullptr_t):
		state_(State{nullptr})
	{
		
	}

	YieldableBase(const YieldableBase&) = delete:
	YieldableBase(YieldableBase&&) = delete:

	YieldableBase& operator=(const YieldableBase&) = delete:
	YieldableBase& operator=(YieldableBase&&) = delete:

protected:

	ResumptionKind suspend_execution_and_resume_caller(ExecutionContext& suspend_point, ResumptionKind kind) {
		// Yield back to whoever called us.
		YieldableBase* yield_target = state_.exchange(State{&suspend_point}).caller();
		assert(yield_target);
		ExecutionContext* resume_context = yield_target->state_.exchange(State{this}).suspended_context();
		assert(resume_context);
		return suspend_point.yield_to(*resume_target, kind);
	}

	template <class T, class ... Args>
	ResumptionKind do_emplace_yield(Args&& ... args) {
		auto result = make_yield_result_builder<T>([&](){ return PushType(std::forward<Args>(args)...); });
		return this->suspend_execution_and_resume_caller(result, ResumptionKind::Continue);
	}

	template <class T, class E>
	ResumptionKind do_emplace_raise(Args&& ... args) {
		auto result = make_yield_result_builder<T>([&]() -> T {
			throw E(std::forward<Args>(args)...);
		});
		return this->suspend_execution_and_resume_caller(result, ResumptionKind::Continue);
	}

	std::atomic<State> state_;
};

template <class PushType, class PullType=void>
struct Yieldable: YieldableBaseMethods<PushType> {

	template <class ... Args>
	PullType emplace_yield(Args&& ... args) {
		switch(do_emplace_yield<PushType>(std::forward<Args>(args)...)) {
		case ResumptionKind::Continue: {
			auto result_handle = static_cast<YieldResult<PullType>*>(state_)->get_result_handle();
			if(!result_handle) {
				throw NoResultAvailableException();
			}
			return result_handle();
		}
		case ResumptionKind::End:
			force_end_execution();
			break;
		default:
			assert(false);
			break;
		}
	}

	template <class T, std::enable_if_t<std::is_constructible_v<PushType, T&&>, bool> = false>
	PullType yield(T&& yield_result) {
		return emplace_yield(std::forward<T>(yield_result));
	}

	template <class E, class ... Args, std::enable_if_t<std::is_constructible_v<E, Args&&...>, bool> = false>
	PullType emplace_raise(Args&& ... args) {
		switch(do_emplace_raise<PushType, E>(std::forward<Args>(args)...)) {
		case ResumptionKind::Continue: {
			auto result_handle = static_cast<YieldResult<PullType>*>(state_)->get_result_handle();
			if(!result_handle) {
				throw NoResultAvailableException();
			}
			return result_handle();
		}
		case ResumptionKind::End:
			force_end_execution();
			break;
		default:
			assert(false);
			break;
		}
	}

	template <class E, std::enable_if_t<std::is_constructible_v<std::decay_t<E>, E&&>, bool> = false>
	PullType raise(E&& exc) {
		return emplace_yield(std::forward<E>(exc));
	}

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


} /* inline namespace basic_coroutine */

} /* namespace tim::coro */


#endif /* TIM_CORO_BASIC_CORTOUTINE_HPP */
