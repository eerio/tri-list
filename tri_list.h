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

  template <typename T>
  using mod_t = std::function<T(const T&)>;

  template <typename T>
  using mods_t = std::vector<mod_t<T>>;

  std::vector<elt_t> buffer;

  // a) istotna decyzja projektowa: nie bedziemy uzywac tutaj funkcji compose
  // i sprytnie komponowac kolejnych modyfikacji - to powoduje duzo alokacji
  // zamiast tego bedziemy trzymac wektory tych modyfikacji
  // (na ok0_example.cc daje to 31 alokacji, a to mniej niż 4 miliony)
  // b) uzywamy tupli, zeby moc potem uzyc std::get<T>(tuple), co
  // jest zdecydowanie najładniejszym sposobem na wyciągnięcie zmiennej po typie
  std::tuple<mods_t<T1>, mods_t<T2>, mods_t<T3>> mods_containers;

  // zwroc kontener trzymający kolejne modyfikacje dla typu T
  template <OneOf<T1, T2, T3> T>
  const std::vector<mod_t<T>>& get_modifiers() const {
    return std::get<mods_t<T>>(mods_containers);
  }

  // niestety zrobienie tego overloadu "sprytnie", tj. jakimś const-castem
  // skutkuje u mnie segfaultem - nie jestem pewien jak to zrobić bez kopiowana kodu
  template <OneOf<T1, T2, T3> T>
  std::vector<mod_t<T>>& get_modifiers() {
    return std::get<mods_t<T>>(mods_containers);
  }

  template <OneOf<T1, T2, T3> T>
  T apply_modifiers(T t) const {
    const std::vector<mod_t<T>>& modifiers = get_modifiers<T>();
    return std::accumulate(
      modifiers.begin(),
      modifiers.end(),
      t,
      [](T t, mod_t<T> f) { return std::invoke(f, t); }
    );
  }

  // ta klase bym wolal zdefiniowac na zewnatrz, ale nie umiem
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
