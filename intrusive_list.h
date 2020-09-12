#pragma once

#include <type_traits>
#include <iterator>

namespace intrusive {
    /*
    Тег по-умолчанию чтобы пользователям не нужно было
    придумывать теги, если они используют лишь одну базу
    list_element.
    */
    struct default_tag;

    template<typename Tag = default_tag>
    struct list_element {
        list_element *next;
        list_element *prev;

        list_element() {
            prev = next = this;
        }

        void link(list_element *prev, list_element *next) {
            this->next = next;
            this->prev = prev;

            next->prev = this;
            prev->next = this;
        }

        void unlink() {
            this->next->prev = prev;
            this->prev->next = next;

            this->prev = this->next = this;
        }
    };

    template<typename T, typename Tag = default_tag>
    struct list {
    private:
        mutable list_element<Tag> pointer;
    public:
        template<bool is_const>
        struct list_iterator {
            friend class list;
        private:
            list_element<Tag> *iter;
            explicit list_iterator(list_element<Tag> *pointer) : iter(pointer) {}
        private:
            template<class TT>
            using add_constness = std::conditional_t<is_const, std::add_const_t<TT>, std::remove_const_t<TT>>;
            using underlying_iterator =
            std::conditional_t<is_const,
                    const list_element<Tag>*,
                    list_element<Tag>*>;
        public:

            using iterator_category = std::bidirectional_iterator_tag;
            using value_type = add_constness<T>;
            using difference_type = std::ptrdiff_t;
            using pointer   = value_type *;
            using reference = value_type &;

            list_iterator() = default;

            template<bool Const=true>
            list_iterator(const list_iterator<Const> &other) : iter(other.iter) {}


            reference operator* () const noexcept { return static_cast<reference>(*iter); }

            pointer   operator->() const noexcept { return static_cast<pointer   >(iter); }

            list_iterator &operator++() & noexcept {
                iter = iter->next;
                return *this;
            }

            list_iterator &operator--() & noexcept {
                iter = iter->prev;
                return *this;
            }

            list_iterator operator++(int) & noexcept {
                auto tmp = *this;
                operator++();
                return tmp;
            }

            list_iterator operator--(int) & noexcept {
                auto tmp = *this;
                operator--();
                return tmp;
            }

            friend bool operator==(const list_iterator &lhs, const list_iterator &rhs) {
                return lhs.iter == rhs.iter;
            }

            friend bool operator!=(const list_iterator &lhs, const list_iterator &rhs) {
                return !(lhs == rhs);
            }
        };

        using iterator       = list_iterator<false>;
        using const_iterator = list_iterator<true>;

        static_assert(std::is_convertible_v<T &, list_element<Tag> &>,
                      "value type is not convertible to list_element");

        list() noexcept {
            pointer.prev = pointer.next = &pointer;
        }

        list(list const &) = delete;

        list(list &&other) noexcept { operator=(std::move(other)); }

        ~list() = default;

        list &operator=(list const &) = delete;

        list &operator=(list &&other) noexcept {
            clear();
            if (!other.empty()) {
                pointer.link(other.pointer.prev, other.pointer.next);
            }
            other.clear();
            return *this;
        }

        void clear() noexcept {
            pointer.prev = pointer.next = &pointer;
        }

        /*
        Поскольку вставка изменяет данные в list_element
        мы принимаем неконстантный T&.
        */
        void push_back(T &elem) noexcept {
            static_cast<list_element<Tag> &>(elem).link(pointer.prev, &pointer);
        }

        void pop_back() noexcept {
            pointer.prev->unlink();
        }

        T &back() noexcept {
            return static_cast<T &>(*pointer.prev);
        }

        T const &back() const noexcept {
            return static_cast<T const &>(*pointer.prev);
        }

        void push_front(T &elem) noexcept {
            static_cast<list_element<Tag> &>(elem).link(&pointer, pointer.next);
        }

        void pop_front() noexcept {
            pointer.next->unlink();
        }

        T &front() noexcept {
            return static_cast<T &>(*pointer.next);
        }

        T const &front() const noexcept {
            return static_cast<T const &>(*pointer.next);
        }

        [[nodiscard]] bool empty() const noexcept {
            return pointer.prev == &pointer;
        }

        iterator begin() noexcept {
            return iterator(pointer.next);
        }

        const_iterator begin() const noexcept {
            return const_iterator(pointer.next);
        }

        iterator end() noexcept {
            return iterator(&pointer);
        }

        const_iterator end() const noexcept {
            return const_iterator(&pointer);
        }

        iterator insert(const_iterator pos, T &elem) noexcept {
            elem.link(pos.iter->prev, pos.iter);
            return iterator(&elem);
        }

        iterator erase(const_iterator pos) noexcept {
            (++pos).iter->prev->unlink();
            return iterator(pos);
        }

        void splice(const_iterator pos, list &, const_iterator first, const_iterator last) noexcept {
            if (first == last) {
                return;
            }
            //prev(first) -> last
            first.iter->prev->next = last.iter;
            auto swap_last = last.iter->prev;
            last.iter->prev = first.iter->prev;

            //prev(pos) -> first -> ... -> prev(last) -> pos
            pos.iter->prev->next = first.iter;
            first.iter->prev = pos.iter->prev;
            swap_last->next = pos.iter;
            pos.iter->prev = swap_last;
        }
    };
}
