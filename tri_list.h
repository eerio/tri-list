#ifndef TRI_LIST_H
#define TRI_LIST_H

#include <variant>
#include <vector>

#include "tri_list_concepts.h"

// template <typename T, typename T1, typename T2, typename T3>
// concept OneOf =
//   (std::same_as<T, T1> && !std::same_as<T, T2> && !std::same_as<T, T3>)
//   || (!std::same_as<T, T1> && std::same_as<T, T2> && !std::same_as<T, T3>)
//   || (!std::same_as<T, T1> && !std::same_as<T, T2> && std::same_as<T, T3>);

template <typename T>
T identity(T x) { return x; }
static_assert(modifier<decltype(identity<int>), int>);

template <typename T, modifier<T> F, modifier<T> G>
auto compose(F f, G g) {
  return [f, g](T t){ return f(g(t)); };
}

template <typename T1, typename T2, typename T3>
class tri_list : public std::ranges::view_interface<tri_list<T1, T2, T3>> {
  using elt_t = std::variant<T1, T2, T3>;
  std::vector<elt_t> buffer;

  template<typename T>
  std::function<T(T)> elt_modifier;

  class iterator {
    using base_iterator = typename decltype(buffer)::iterator;
    base_iterator base;

  public:
    using iterator_category = typename base_iterator::iterator_category;
    using difference_type = typename base_iterator::difference_type;
    using value_type = elt_t;
    using pointer = elt_t*;
    using reference = elt_t;
    
    iterator() = default;
    explicit iterator(const base_iterator& other) : base(other) {}

    // TUTAJ MUSI BYC EWALUACJA
    reference operator*() const { return (*base); }
    iterator& operator++() { base++; return *this; }
    iterator& operator--() { base--; return *this; }
    iterator operator++(int) { iterator result {*this}; ++this; return result; }
    iterator operator--(int) { iterator result {*this}; --this; return result; }
    friend bool operator==(const iterator& a, const iterator& b) { return a.base == b.base; }
  };

public:
  tri_list() : tri_list({}) { assert(false); }
  tri_list(std::initializer_list<elt_t> elements) : buffer(elements) {
    elt_modifier<T1> = identity<T1>;
    elt_modifier<T2> = identity<T2>;
    elt_modifier<T3> = identity<T3>;
  };
  
  // template <OneOf<T1, T2, T3> T>
  template <typename T>
  void push_back(const T& t) { buffer.push_back(t); }
  
  // template <OneOf<T1, T2, T3> T, modifier<T> F>
  template <typename T, modifier<T> F>
  void modify_only([[maybe_unused]] F m = F {}) {
    elt_modifier<T> = compose<T>(m, elt_modifier<T>);
  }

  // template <OneOf<T1, T2, T3> T>
  template <typename T>
  void reset() {
      elt_modifier<T> = identity<T>;
  }
  
  // template <OneOf<T1, T2, T3> T>
  template <typename T>
  auto range_over() {
    return buffer
      | std::ranges::views::filter(std::holds_alternative<T, T1, T2, T3>)
      | std::ranges::views::transform([](std::variant<T1, T2, T3> x){ return std::get<T>(x);});
  }

  iterator begin() { return iterator(buffer.begin()); };
  iterator end() { return iterator(buffer.end()); }
};

#endif // TRI_LIST_H
