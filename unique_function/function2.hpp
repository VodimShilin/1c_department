#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

namespace fu2 {
namespace detail {

}namespace invocation_table {
#if !defined(FU2_HAS_DISABLED_EXCEPTIONS)
#if defined(FU2_HAS_NO_FUNCTIONAL_HEADER)
        struct bad_function_call : std::exception {
            bad_function_call() noexcept {
            }

            char const* what() const noexcept override {
                return "bad function call";
            }
        };
#else
        using std::bad_function_call;
#endif
#endif

#ifdef FU2_HAS_CXX17_NOEXCEPT_FUNCTION_TYPE
#define FU2_DETAIL_EXPAND_QUALIFIERS_NOEXCEPT(F) \
  F(, , noexcept, , &)                           \
  F(const, , noexcept, , &)                      \
  F(, volatile, noexcept, , &)                   \
  F(const, volatile, noexcept, , &)              \
  F(, , noexcept, &, &)                          \
  F(const, , noexcept, &, &)                     \
  F(, volatile, noexcept, &, &)                  \
  F(const, volatile, noexcept, &, &)             \
  F(, , noexcept, &&, &&)                        \
  F(const, , noexcept, &&, &&)                   \
  F(, volatile, noexcept, &&, &&)                \
  F(const, volatile, noexcept, &&, &&)
#define FU2_DETAIL_EXPAND_CV_NOEXCEPT(F) \
  F(, , noexcept)                        \
  F(const, , noexcept)                   \
  F(, volatile, noexcept)                \
  F(const, volatile, noexcept)
#else  // FU2_HAS_CXX17_NOEXCEPT_FUNCTION_TYPE
#define FU2_DETAIL_EXPAND_QUALIFIERS_NOEXCEPT(F)
#define FU2_DETAIL_EXPAND_CV_NOEXCEPT(F)
#endif  // FU2_HAS_CXX17_NOEXCEPT_FUNCTION_TYPE

#define FU2_DETAIL_EXPAND_QUALIFIERS(F) \
  F(, , , , &)                          \
  F(const, , , , &)                     \
  F(, volatile, , , &)                  \
  F(const, volatile, , , &)             \
  F(, , , &, &)                         \
  F(const, , , &, &)                    \
  F(, volatile, , &, &)                 \
  F(const, volatile, , &, &)            \
  F(, , , &&, &&)                       \
  F(const, , , &&, &&)                  \
  F(, volatile, , &&, &&)               \
  F(const, volatile, , &&, &&)          \
  FU2_DETAIL_EXPAND_QUALIFIERS_NOEXCEPT(F)
#define FU2_DETAIL_EXPAND_CV(F) \
  F(, , )                       \
  F(const, , )                  \
  F(, volatile, )               \
  F(const, volatile, )          \
  FU2_DETAIL_EXPAND_CV_NOEXCEPT(F)

        /// If the function is qualified as noexcept, the call will never throw
        template <bool IsNoexcept>
        [[noreturn]] void throw_or_abortnoexcept(
                std::integral_constant<bool, IsNoexcept> /*is_throwing*/) noexcept {
            std::abort();
        }
        /// Calls std::abort on empty function calls
        [[noreturn]] inline void throw_or_abort(
                std::false_type /*is_throwing*/) noexcept {
            std::abort();
        }
        /// Throws bad_function_call on empty funciton calls
        [[noreturn]] inline void throw_or_abort(std::true_type /*is_throwing*/) {
#ifdef FU2_HAS_DISABLED_EXCEPTIONS
            throw_or_abort(std::false_type{});
#else
            throw bad_function_call{};
#endif
        }

template<typename Config, typename Property>
class function;

template<bool Owning, bool Copyable, typename Capacity>
struct config {
    // Is true if the function is owning.
    static constexpr auto const is_owning = Owning;

    // Is true if the function is copyable.
    static constexpr auto const is_copyable = Copyable;

    // The internal capacity of the function
    // used in small functor optimization.
    // The object shall expose the real capacity through Capacity::capacity
    // and the intended alignment through Capacity::alignment.
    using capacity = Capacity;
};

template<bool Throws, bool HasStrongExceptGuarantee, typename... Args>
struct property {
    // Is true when the function throws an exception on empty invocation.
    static constexpr auto const is_throwing = Throws;

    // Is true when the function throws an exception on empty invocation.
    static constexpr auto const is_strong_exception_guaranteed =
            HasStrongExceptGuarantee;
};

namespace type_erasure {
    /// Specialization to work with addresses of callable objects
    template <typename T, typename = void>
    struct address_taker {
        template <typename O>
        static void* take(O&& obj) {
            return std::addressof(obj);
        }
        static T& restore(void* ptr) {
            return *static_cast<T*>(ptr);
        }
        static T const& restore(void const* ptr) {
            return *static_cast<T const*>(ptr);
        }
        static T volatile& restore(void volatile* ptr) {
            return *static_cast<T volatile*>(ptr);
        }
        static T const volatile& restore(void const volatile* ptr) {
            return *static_cast<T const volatile*>(ptr);
        }
    };

template <typename Config, bool IsThrowing, bool HasStrongExceptGuarantee,
         typename... Args>
class function<Config, property<IsThrowing, HasStrongExceptGuarantee, Args...>>
    : type_erasure::invocation_table::operator_impl<
              0U,
              function<Config,
                       property<IsThrowing, HasStrongExceptGuarantee, Args...>>,
              Args...> {
    template <typename, typename>
    friend class function;

    template <std::size_t, typename, typename...>
    friend class type_erasure::invocation_table::operator_impl;

    using property_t = property<IsThrowing, HasStrongExceptGuarantee, Args...>;
    using erasure_t =
            type_erasure::erasure<Config::is_owning, Config, property_t>;

    template <typename T>
    using enable_if_can_accept_all_t =
            std::enable_if_t<accepts_all<std::decay_t<T>, identity<Args...>>::value>;

    template <typename Function, typename = void>
    struct is_convertible_to_this : std::false_type {};
    template <typename RightConfig>
    struct is_convertible_to_this<
            function<RightConfig, property_t>,
            void_t<enable_if_copyable_correct_t<Config, RightConfig>,
                   enable_if_owning_correct_t<Config, RightConfig>>>
        : std::true_type {};

    template <typename T>
    using enable_if_not_convertible_to_this =
            std::enable_if_t<!is_convertible_to_this<std::decay_t<T>>::value>;

    template <typename T>
    using enable_if_owning_t =
            std::enable_if_t<std::is_same<T, T>::value && Config::is_owning>;

    template <typename T>
    using assert_wrong_copy_assign_t =
            typename assert_wrong_copy_assign<Config, std::decay_t<T>>::type;

    template <typename T>
    using assert_no_strong_except_guarantee_t =
            typename assert_no_strong_except_guarantee<HasStrongExceptGuarantee,
                                                       std::decay_t<T>>::type;

    erasure_t erasure_;

public:
    /// Default constructor which empty constructs the function
    function() = default;
    ~function() = default;

    explicit constexpr function(function const& /*right*/) = default;
    explicit constexpr function(function&& /*right*/) = default;

    /// Copy construction from another copyable function
    template <typename RightConfig,
             std::enable_if_t<RightConfig::is_copyable>* = nullptr,
             enable_if_copyable_correct_t<Config, RightConfig>* = nullptr,
             enable_if_owning_correct_t<Config, RightConfig>* = nullptr>
    constexpr function(function<RightConfig, property_t> const& right)
        : erasure_(right.erasure_) {
    }

    /// Move construction from another function
    template <typename RightConfig,
             enable_if_copyable_correct_t<Config, RightConfig>* = nullptr,
             enable_if_owning_correct_t<Config, RightConfig>* = nullptr>
    constexpr function(function<RightConfig, property_t>&& right)
        : erasure_(std::move(right.erasure_)) {
    }

    /// Construction from a callable object which overloads the `()` operator
    template <typename T,  //
             enable_if_not_convertible_to_this<T>* = nullptr,
             enable_if_can_accept_all_t<T>* = nullptr,
             assert_wrong_copy_assign_t<T>* = nullptr,
             assert_no_strong_except_guarantee_t<T>* = nullptr>
    constexpr function(T&& callable)
        : erasure_(use_bool_op<unrefcv_t<T>>{}, std::forward<T>(callable)) {
    }
    template <typename T, typename Allocator,  //
             enable_if_not_convertible_to_this<T>* = nullptr,
             enable_if_can_accept_all_t<T>* = nullptr,
             enable_if_owning_t<T>* = nullptr,
             assert_wrong_copy_assign_t<T>* = nullptr,
             assert_no_strong_except_guarantee_t<T>* = nullptr>
    constexpr function(T&& callable, Allocator&& allocator)
        : erasure_(use_bool_op<unrefcv_t<T>>{}, std::forward<T>(callable),
                   std::forward<Allocator>(allocator)) {
    }

    /// Empty constructs the function
    constexpr function(std::nullptr_t np) : erasure_(np) {
    }

    function& operator=(function const& /*right*/) = default;
    function& operator=(function&& /*right*/) = default;

    /// Copy assigning from another copyable function
    template <typename RightConfig,
             std::enable_if_t<RightConfig::is_copyable>* = nullptr,
             enable_if_copyable_correct_t<Config, RightConfig>* = nullptr,
             enable_if_owning_correct_t<Config, RightConfig>* = nullptr>
    function& operator=(function<RightConfig, property_t> const& right) {
        erasure_ = right.erasure_;
        return *this;
    }

    /// Move assigning from another function
    template <typename RightConfig,
             enable_if_copyable_correct_t<Config, RightConfig>* = nullptr,
             enable_if_owning_correct_t<Config, RightConfig>* = nullptr>
    function& operator=(function<RightConfig, property_t>&& right) {
        erasure_ = std::move(right.erasure_);
        return *this;
    }

    /// Move assigning from a callable object
    template <typename T,  // ...
             enable_if_not_convertible_to_this<T>* = nullptr,
             enable_if_can_accept_all_t<T>* = nullptr,
             assert_wrong_copy_assign_t<T>* = nullptr,
             assert_no_strong_except_guarantee_t<T>* = nullptr>
    function& operator=(T&& callable) {
        erasure_.assign(use_bool_op<unrefcv_t<T>>{}, std::forward<T>(callable));
        return *this;
    }

    /// Clears the function
    function& operator=(std::nullptr_t np) {
        erasure_ = np;
        return *this;
    }

    /// Returns true when the function is empty
    bool empty() const noexcept {
        return erasure_.empty();
    }

    /// Returns true when the function isn't empty
    explicit operator bool() const noexcept {
        return !empty();
    }

    /// Assigns a new target with an optional allocator
    template <typename T, typename Allocator = std::allocator<std::decay_t<T>>,
             enable_if_not_convertible_to_this<T>* = nullptr,
             enable_if_can_accept_all_t<T>* = nullptr,
             assert_wrong_copy_assign_t<T>* = nullptr,
             assert_no_strong_except_guarantee_t<T>* = nullptr>
    void assign(T&& callable, Allocator&& allocator = Allocator{}) {
        erasure_.assign(use_bool_op<unrefcv_t<T>>{}, std::forward<T>(callable),
                        std::forward<Allocator>(allocator));
    }

    /// Swaps this function with the given function
    void swap(function& other) noexcept(HasStrongExceptGuarantee) {
        if (&other == this) {
            return;
        }

        function cache = std::move(other);
        other = std::move(*this);
        *this = std::move(cache);
    }

    /// Swaps the left function with the right one
    friend void swap(function& left,
                     function& right) noexcept(HasStrongExceptGuarantee) {
        left.swap(right);
    }

    /// Calls the wrapped callable object
    using type_erasure::invocation_table::operator_impl<
            0U, function<Config, property_t>, Args...>::operator();
};

using object_size = std::integral_constant<std::size_t, 32U>;
} // namespace detail



template <std::size_t Capacity,
         std::size_t Alignment = alignof(std::max_align_t)>
struct capacity_fixed {
    static constexpr std::size_t capacity = Capacity;
    static constexpr std::size_t alignment = Alignment;
};

struct capacity_default
    : capacity_fixed<detail::object_size::value - (2 * sizeof(void*))> {};


template <bool IsOwning, bool IsCopyable, typename Capacity, bool IsThrowing,
         bool HasStrongExceptGuarantee, typename... Signatures>
using function_base = detail::function<
        detail::config<IsOwning, IsCopyable, Capacity>,
        detail::property<IsThrowing, HasStrongExceptGuarantee, Signatures...>>;

template <typename... Signatures>
using function = function_base<true, true, capacity_default,  //
                               true, false, Signatures...>;

template <typename... Signatures>
using unique_function = function_base<true, false, capacity_default,  //
                                      true, false, Signatures...>;

template <typename... Signatures>
using function_view = function_base<false, true, capacity_default,  //
                                    true, false, Signatures...>;

using detail::type_erasure::invocation_table::bad_function_call;

} // namespace fu2