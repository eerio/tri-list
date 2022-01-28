#ifndef TRI_LIST_H
#define TRI_LIST_H

#include <numeric>
#include <variant>
#include <vector>
#include <functional>

#include "tri_list_concepts.h"

template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;


// const correctness?
// constexpr?


template <typename T, typename T1, typename T2, typename T3>
concept OneOf =
  (std::same_as<T, T1> && !std::same_as<T, T2> && !std::same_as<T, T3>)
  || (!std::same_as<T, T1> && std::same_as<T, T2> && !std::same_as<T, T3>)
  || (!std::same_as<T, T1> && !std::same_as<T, T2> && std::same_as<T, T3>);

template <typename T>
T identity(T x) { return x; }
static_assert(modifier<decltype(identity<int>), int>);

template <typename T, modifier<T> F, modifier<T> G>
auto compose(F&& f, G&& g) {
  return [f, g](T t){ return f(g(t)); };
}

template <typename T1, typename T2, typename T3>
class tri_list : public std::ranges::view_interface<tri_list<T1, T2, T3>> {
  using elt_t = std::variant<T1, T2, T3>;
  std::vector<elt_t> buffer;

  std::vector<std::function<T1(T1)>> modifiers_t1;
  std::vector<std::function<T2(T2)>> modifiers_t2;
  std::vector<std::function<T3(T3)>> modifiers_t3;

  template <typename T>
  std::vector<std::function<T(T)>>& get_modifiers() {
    if constexpr (std::is_same<T, T1>::value) { return modifiers_t1; }
    else if constexpr (std::is_same<T, T2>::value) { return modifiers_t2; }
    else if constexpr (std::is_same<T, T3>::value) { return modifiers_t3; }
    else { abort(); }
  }

  template <typename T>
  T apply_modifiers(T t) {
    auto modifiers = get_modifiers<T>();
    for (auto mod : modifiers) {
      t = mod(t);
    }
    return t;
    // return std::accumulate(
    //   modifiers.begin(),
    //   modifiers.end(),
    //   t,
    //   [](auto f, T t) { return std::invoke(f, t); }
    // );
  }

  class iterator {
    using base_iterator = typename decltype(buffer)::iterator;
    tri_list* tri_lst;
    base_iterator base;

  public:
    using iterator_category = typename base_iterator::iterator_category;
    using difference_type = typename base_iterator::difference_type;
    using value_type = elt_t;
    using pointer = elt_t*;
    using reference = elt_t;
    
    iterator() = default;
    explicit iterator(tri_list* tri_lst, const base_iterator& other) 
      : tri_lst(tri_lst), base(other) {}

    elt_t operator*() const {
      // return tri_lst->apply_modifiers(*base);
      // using t = decltype(*base);
      return std::visit(
        // tri_lst->apply_modifiers,
        overloaded {
          [&](T1 t) -> elt_t {return tri_lst->apply_modifiers(t); },
          [&](T2 t) -> elt_t {return tri_lst->apply_modifiers(t); },
          [&](T3 t) -> elt_t {return tri_lst->apply_modifiers(t); }
        },
        *base
      );
    }

    iterator& operator++() { base++; return *this; }
    iterator& operator--() { base--; return *this; }
    iterator operator++(int) { iterator result {*this}; ++this; return result; }
    iterator operator--(int) { iterator result {*this}; --this; return result; }
    friend bool operator==(const iterator& a, const iterator& b) { return a.base == b.base; }
  };

public:
  tri_list() : tri_list({}) {}
  tri_list(std::initializer_list<elt_t> elements) : buffer(elements) {
  };
  
  template <OneOf<T1, T2, T3> T>
  // template <typename T>
  void push_back(const T& t) { buffer.push_back(t); }
  
  template <OneOf<T1, T2, T3> T, modifier<T> F>
  // template <typename T, modifier<T> F>
  void modify_only([[maybe_unused]] F m = F {}) {
    get_modifiers<T>().push_back(m);
  }

  template <OneOf<T1, T2, T3> T>
  // template <typename T>
  void reset() {
      get_modifiers<T>().clear();
  }
  
  template <OneOf<T1, T2, T3> T>
  // template <typename T>
  auto range_over() {
    return buffer
      | std::ranges::views::filter(std::holds_alternative<T, T1, T2, T3>)
      | std::ranges::views::transform([&](auto x){ return apply_modifiers(std::get<T>(x));});
      // | std::ranges::views::transform(std::get<T, T1, T2, T3>)
      // | std::ranges::views::transform(apply_modifiers);
  }

  iterator begin() { return iterator(this, buffer.begin()); };
  iterator end() { return iterator(this, buffer.end()); }
};


#endif // TRI_LIST_H
