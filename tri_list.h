#ifndef TRI_LIST_H
#define TRI_LIST_H

#include <numeric>
#include <variant>
#include <vector>
#include <functional>

#include "tri_list_concepts.h"

// overloaded pattern; source: std::vist entry @ cppreference
template<class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template<class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

// ensure type T is exactly one of T1, T2, T3
template <typename T, typename T1, typename T2, typename T3>
concept OneOf =
  (std::same_as<T, T1> && !std::same_as<T, T2> && !std::same_as<T, T3>)
  || (!std::same_as<T, T1> && std::same_as<T, T2> && !std::same_as<T, T3>)
  || (!std::same_as<T, T1> && !std::same_as<T, T2> && std::same_as<T, T3>);

// główna klasa zadania
template <typename T1, typename T2, typename T3>
class tri_list : public std::ranges::view_interface<tri_list<T1, T2, T3>> {
  using elt_t = std::variant<T1, T2, T3>;
  std::vector<elt_t> buffer;

  std::vector<std::function<T1(T1)>> modifiers_t1;
  std::vector<std::function<T2(T2)>> modifiers_t2;
  std::vector<std::function<T3(T3)>> modifiers_t3;

  template <OneOf<T1, T2, T3> T>
  const std::vector<std::function<T(T)>>& get_modifiers() const {
    if constexpr (std::is_same<T, T1>::value) { return modifiers_t1; }
    else if constexpr (std::is_same<T, T2>::value) { return modifiers_t2; }
    else if constexpr (std::is_same<T, T3>::value) { return modifiers_t3; }
  }

  template <OneOf<T1, T2, T3> T>
  std::vector<std::function<T(T)>>& get_modifiers() {
    if constexpr (std::is_same<T, T1>::value) { return modifiers_t1; }
    else if constexpr (std::is_same<T, T2>::value) { return modifiers_t2; }
    else if constexpr (std::is_same<T, T3>::value) { return modifiers_t3; }
  }

  template <OneOf<T1, T2, T3> T>
  T apply_modifiers(T t) const {
    const std::vector<std::function<T(T)>>& modifiers = get_modifiers<T>();
    return std::accumulate(
      modifiers.begin(),
      modifiers.end(),
      t,
      [](T t, std::function<T(T)> f) { return std::invoke(f, t); }
    );
  }

  class iterator {
    using base_iterator = typename decltype(buffer)::const_iterator;
    const tri_list* tri_lst;
    base_iterator base;

  public:
    using iterator_category = typename base_iterator::iterator_category;
    using difference_type = typename base_iterator::difference_type;
    using value_type = elt_t;
    using pointer = elt_t*;
    using reference = elt_t&;
    
    iterator() = default;
    explicit iterator(const tri_list* tri_lst, const base_iterator& other) 
      : tri_lst(tri_lst), base(other) {}

    elt_t operator*() const {
      return std::visit(
        // ja bym wolal tak, ale to nie dziala
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
  tri_list() = default;
  tri_list(std::initializer_list<elt_t> elements) : buffer(elements) {};
  
  template <OneOf<T1, T2, T3> T>
  void push_back(const T& t) { buffer.push_back(t); }
  
  template <OneOf<T1, T2, T3> T, modifier<T> F>
  void modify_only(F m = F {}) {
    get_modifiers<T>().push_back(m);
  }

  template <OneOf<T1, T2, T3> T>
  void reset() {
      get_modifiers<T>().clear();
  }
  
  template <OneOf<T1, T2, T3> T>
  auto range_over() const {
    return buffer
      | std::ranges::views::filter(std::holds_alternative<T, T1, T2, T3>)
      // ja bym wolal tak, ale to nie dziala
      // | std::ranges::views::transform(std::get<T>)
      | std::ranges::views::transform([](elt_t x){ return std::get<T>(x); })
      | std::ranges::views::transform([&](T x){ return apply_modifiers<T>(x); });
  }

  iterator begin() const { return iterator(this, buffer.begin()); };
  iterator begin() { return iterator(this, buffer.begin()); };
  iterator end() const { return iterator(this, buffer.end()); }
  iterator end() { return iterator(this, buffer.end()); }
};

template <typename T>
T identity(T x) { return x; }
static_assert(modifier<decltype(identity<int>), int>);

template <typename T, modifier<T> F, modifier<T> G>
auto compose(const F& f, const G& g) {
  return [f, g](T t){ return f(g(std::forward<T>(t))); };
}


#endif // TRI_LIST_H
