/* type_list.h -- type list
 * 
 * Implementation based on TypeLists, Modern C++ Design, Andrei Alexandrescu, 2001
 *
 *			Ryan McDougall -- 2010
 */

#ifndef _TYPE_LIST_H_
#define _TYPE_LIST_H_

#include <typeinfo>
#include <tr1/type_traits>

namespace type_list
{
    //=============================================================================
    // type list data type and null type

    struct null_type {};
    
    template <typename T, typename U>
        struct type_list
        {
            typedef T Head;
            typedef U Tail;
        };

    //=============================================================================
    // compute length of the list

    template <typename TypeList> struct length;

    template <>
        struct length <null_type>
        {
            enum { result = 0 };
        };

    template <typename Head, typename Tail>
        struct length <type_list <Head, Tail> >
        {
            enum { result = 1 + length <Tail>::result };
        };

    //=============================================================================
    // get the type at the given index

    template <typename TypeList, size_t i> struct type_at;

    template <typename Head, typename Tail>
        struct type_at <type_list <Head, Tail>, 0>
        {
            typedef Head result;
        };

    template <typename Head, typename Tail, size_t i>
        struct type_at <type_list <Head, Tail>, i>
        {
            typedef typename type_at <Tail, i-1>::result result;
        };

    //=============================================================================
    // linear search for the given type

    template <typename TypeList, class T> struct index_of;

    template <typename T>
        struct index_of <null_type, T>
        {
            enum { result = -1 };
        };

    template <typename T, typename Tail>
        struct index_of <type_list <T, Tail>, T>
        {
            enum { result = 0 };
        };

    template <typename Head, typename Tail, typename T>
        struct index_of <type_list <Head, Tail>, T>
        {
            private:
                enum { temp = index_of <Tail, T>::result };
            public:
                enum { result = (temp < 0)? -1 : temp + 1 };
        };
    
    //=============================================================================
    // create new list with appended type

    template <typename TypeList, typename T> struct append;

    template <> 
        struct append <null_type, null_type>
        {
            typedef null_type result;
        };

    template <typename T>
        struct append <null_type, T>
        {
            typedef type_list <T, null_type> result;
        };

    template <typename Head, typename Tail>
        struct append <null_type, type_list <Head, Tail> >
        {
            typedef type_list <Head, Tail> result;
        };

    template <typename Head, typename Tail, typename T>
        struct append <type_list <Head, Tail>, T>
        {
            typedef type_list <Head, typename append <Tail, T>::result> result;
        };
    
    //=============================================================================
    // tell if second list is an extension of the first

    template <typename TypeList1, typename TypeList2> struct extends;
    
    template <>
        struct extends <null_type, null_type>
        {
            enum { result = 1 };
        };
    
    template <typename TypeList>
        struct extends <TypeList, null_type>
        {
            enum { result = 1 };
        };
    
    template <typename TypeList>
        struct extends <null_type, TypeList>
        {
            enum { result = 0 };
        };
    
    template <typename Head1, typename Tail1, typename Head2, typename Tail2>
        struct extends <type_list <Head1, Tail1>, type_list <Head2, Tail2> >
        {
                enum { result = (extends <Tail1, Tail2>::result && 
                        std::tr1::is_same <Head1, Head2>::value)? 1 : 0 };
        };

    //=============================================================================
    // allocate a series of objects using the type as template parameter

    template <typename TypeList, template <class> class T, typename Base> struct allocate;

    template <template <class> class T, typename Base>
        struct allocate <null_type, T, Base>
        {
            allocate (Base **array) {}
        };

    template <typename Head, typename Tail, template <class> class T, typename Base>
        struct allocate <type_list <Head, Tail>, T, Base>
        {
            allocate (Base **array)
            {
                *array = static_cast <Base *> (new T <Head>);
                allocate <Tail, T, Base> (array + 1);
            }
        };
}

#define TYPE_LIST_1(t1) type_list::type_list<t1,type_list::null_type>

#define TYPE_LIST_2(t1,t2) type_list::type_list<t1,type_list::type_list<t2,type_list::null_type> >

#define TYPE_LIST_3(t1,t2,t3) type_list::type_list<t1,type_list::type_list<t2, type_list::type_list<t3,type_list::null_type> > >

#define TYPE_LIST_4(t1,t2,t3,t4) type_list::type_list<t1,type_list::type_list<t2,type_list::type_list<t3,type_list::type_list<t4,type_list::null_type> > > >

#define TYPE_LIST_5(t1,t2,t3,t4,t5) type_list::type_list<t1,type_list::type_list<t2,type_list::type_list<t3,type_list::type_list<t4,type_list::type_list<t5,type_list::null_type> > > > >

#define TYPE_LIST_6(t1,t2,t3,t4,t5,t6) type_list::type_list<t1,type_list::type_list<t2,type_list::type_list<t3,type_list::type_list<t4,type_list::type_list<t5,type_list::type_list<t6,type_list::null_type> > > > > >

#define TYPE_LIST_7(t1,t2,t3,t4,t5,t6,t7) type_list::type_list<t1,type_list::type_list<t2,type_list::type_list<t3,type_list::type_list<t4,type_list::type_list<t5,type_list::type_list<t6,type_list::type_list<t7,type_list::null_type> > > > > > >

#define TYPE_LIST_8(t1,t2,t3,t4,t5,t6,t7,t8) type_list::type_list<t1,type_list::type_list<t2,type_list::type_list<t3,type_list::type_list<t4,type_list::type_list<t5,type_list::type_list<t6,type_list::type_list<t7,type_list::type_list<t8,type_list::null_type> > > > > > > >

#define TYPE_LIST_9(t1,t2,t3,t4,t5,t6,t7,t8,t9) type_list::type_list<t1,type_list::type_list<t2,type_list::type_list<t3,type_list::type_list<t4,type_list::type_list<t5,type_list::type_list<t6,type_list::type_list<t7,type_list::type_list<t8,type_list::type_list<t9,type_list::null_type> > > > > > > > >

#endif
