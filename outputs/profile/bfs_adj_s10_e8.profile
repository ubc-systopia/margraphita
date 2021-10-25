Flat profile:

Each sample counts as 0.01 seconds.
  %   cumulative   self              self     total           
 time   seconds   seconds    calls  us/call  us/call  name    
 45.06      2.51     2.51                             __wt_hazard_clear
 35.55      4.49     1.98                             __wt_hazard_set
  5.75      4.81     0.32                             __global_once
  2.69      4.96     0.15                             __wt_row_search
  2.33      5.09     0.13                             bfs_info* bfs<AdjList>(AdjList&, int)
  1.62      5.18     0.09                             __wt_schema_project_out
  0.54      5.21     0.03                             __wt_page_in_func
  0.36      5.23     0.02  1615850     0.01     0.02  void std::__relocate_object_a<node, node, std::allocator<node> >(node*, node*, std::allocator<node>&)
  0.36      5.25     0.02   899520     0.02     0.02  void std::allocator_traits<std::allocator<int> >::construct<int, int const&>(std::allocator<int>&, int*, int const&)
  0.36      5.27     0.02                             __cursor_func_init.constprop.0
  0.36      5.29     0.02                             __wt_btcur_close
  0.36      5.31     0.02                             __wt_session_strerror
  0.18      5.32     0.01  2897160     0.00     0.00  void std::allocator_traits<std::allocator<node> >::construct<node, node>(std::allocator<node>&, node*, node&&)
  0.18      5.33     0.01  1635610     0.01     0.01  std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::compare(char const*) const
  0.18      5.34     0.01  1615850     0.01     0.01  void __gnu_cxx::new_allocator<node>::destroy<node>(node*)
  0.18      5.35     0.01  1281346     0.01     0.01  AdjList::__record_to_node(__wt_cursor*, int)
  0.18      5.36     0.01  1281310     0.01     0.02  int __gnu_cxx::__stoa<long, int, char, int>(long (*)(char const*, char**, int), char const*, char const*, unsigned long*, int)
  0.18      5.37     0.01  1281310     0.01     0.03  std::vector<int, std::allocator<int> >::push_back(int const&)
  0.18      5.38     0.01  1281310     0.01     0.01  __gnu_cxx::__stoa<long, int, char, int>(long (*)(char const*, char**, int), char const*, char const*, unsigned long*, int)::_Save_errno::_Save_errno()
  0.18      5.39     0.01  1281310     0.01     0.01  __gnu_cxx::__stoa<long, int, char, int>(long (*)(char const*, char**, int), char const*, char const*, unsigned long*, int)::_Save_errno::~_Save_errno()
  0.18      5.40     0.01   383550     0.03     0.03  void std::vector<int, std::allocator<int> >::_M_realloc_insert<int const&>(__gnu_cxx::__normal_iterator<int*, std::vector<int, std::allocator<int> > >, int const&)
  0.18      5.41     0.01   381790     0.03     0.13  void std::vector<node, std::allocator<node> >::_M_realloc_insert<node>(__gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >, node&&)
  0.18      5.42     0.01   354381     0.03     0.03  std::char_traits<char>::length(char const*)
  0.18      5.43     0.01   236238     0.04     0.04  void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char const*>(char const*, char const*, std::forward_iterator_tag)
  0.18      5.44     0.01                             __config_getraw
  0.18      5.45     0.01                             __curfile_insert
  0.18      5.46     0.01                             __curfile_search
  0.18      5.47     0.01                             __wt_btcur_search
  0.18      5.48     0.01                             __wt_calloc
  0.18      5.49     0.01                             __wt_cursor_set_keyv
  0.18      5.50     0.01                             __wt_curtable_get_value
  0.18      5.51     0.01                             __wt_curtable_set_key
  0.18      5.52     0.01                             __wt_readunlock
  0.18      5.53     0.01                             __wt_session_gen_leave
  0.18      5.54     0.01                             __wt_session_release_dhandle
  0.18      5.55     0.01                             __wt_struct_sizev
  0.18      5.56     0.01                             __wt_value_return_upd
  0.09      5.57     0.01  7457420     0.00     0.00  node&& std::forward<node>(std::remove_reference<node>::type&)
  0.09      5.57     0.01                             __gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > >::base() const
  0.00      5.57     0.00  4847550     0.00     0.00  node* std::__addressof<node>(node&)
  0.00      5.57     0.00  3796689     0.00     0.00  operator new(unsigned long, void*)
  0.00      5.57     0.00  3326200     0.00     0.00  __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >::__normal_iterator(node* const&)
  0.00      5.57     0.00  2897160     0.00     0.00  void __gnu_cxx::new_allocator<node>::construct<node, node>(node*, node&&)
  0.00      5.57     0.00  2897160     0.00     0.00  std::remove_reference<node&>::type&& std::move<node&>(node&)
  0.00      5.57     0.00  2798820     0.00     0.00  __gnu_cxx::__normal_iterator<int*, std::vector<int, std::allocator<int> > >::base() const
  0.00      5.57     0.00  2290740     0.00     0.00  node* std::__niter_base<node*>(node*)
  0.00      5.57     0.00  1799040     0.00     0.00  int const& std::forward<int const&>(std::remove_reference<int const&>::type&)
  0.00      5.57     0.00  1663100     0.00     0.00  std::vector<node, std::allocator<node> >::end()
  0.00      5.57     0.00  1635610     0.00     0.01  bool std::operator==<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&, char const*)
  0.00      5.57     0.00  1615850     0.00     0.01  void std::allocator_traits<std::allocator<node> >::destroy<node>(std::allocator<node>&, node*)
  0.00      5.57     0.00  1527160     0.00     0.00  __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >::base() const
  0.00      5.57     0.00  1527160     0.00     0.00  std::vector<node, std::allocator<node> >::size() const
  0.00      5.57     0.00  1517548     0.00     0.00  bool __gnu_cxx::__is_null_pointer<char const>(char const*)
  0.00      5.57     0.00  1517548     0.00     0.00  std::iterator_traits<char const*>::difference_type std::__distance<char const*>(char const*, char const*, std::random_access_iterator_tag)
  0.00      5.57     0.00  1517548     0.00     0.00  std::iterator_traits<char const*>::iterator_category std::__iterator_category<char const*>(char const* const&)
  0.00      5.57     0.00  1517548     0.00     0.00  std::iterator_traits<char const*>::difference_type std::distance<char const*>(char const*, char const*)
  0.00      5.57     0.00  1517510     0.00     0.01  bool std::operator!=<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&, char const*)
  0.00      5.57     0.00  1399410     0.00     0.00  bool __gnu_cxx::operator!=<int*, std::vector<int, std::allocator<int> > >(__gnu_cxx::__normal_iterator<int*, std::vector<int, std::allocator<int> > > const&, __gnu_cxx::__normal_iterator<int*, std::vector<int, std::allocator<int> > > const&)
  0.00      5.57     0.00  1281310     0.00     0.00  __gnu_cxx::__normal_iterator<int*, std::vector<int, std::allocator<int> > >::operator++()
  0.00      5.57     0.00  1281310     0.00     0.00  __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >::operator*() const
  0.00      5.57     0.00  1281310     0.00     0.00  __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >::operator-(long) const
  0.00      5.57     0.00  1281310     0.00     0.00  __gnu_cxx::__normal_iterator<int*, std::vector<int, std::allocator<int> > >::operator*() const
  0.00      5.57     0.00  1281310     0.00     0.04  node& std::vector<node, std::allocator<node> >::emplace_back<node>(node&&)
  0.00      5.57     0.00  1281310     0.00     0.00  std::vector<node, std::allocator<node> >::back()
  0.00      5.57     0.00  1281310     0.00     0.04  std::vector<node, std::allocator<node> >::push_back(node&&)
  0.00      5.57     0.00  1281310     0.00     0.02  std::__cxx11::stoi(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&, unsigned long*, int)
  0.00      5.57     0.00  1281310     0.00     0.00  __gnu_cxx::__stoa<long, int, char, int>(long (*)(char const*, char**, int), char const*, char const*, unsigned long*, int)::_Range_chk::_S_chk(long, std::integral_constant<bool, true>)
  0.00      5.57     0.00  1145370     0.00     0.00  __gnu_cxx::new_allocator<node>::max_size() const
  0.00      5.57     0.00   899520     0.00     0.00  void __gnu_cxx::new_allocator<int>::construct<int, int const&>(int*, int const&)
  0.00      5.57     0.00   826700     0.00     0.00  std::_Vector_base<int, std::allocator<int> >::_M_get_Tp_allocator()
  0.00      5.57     0.00   763584     0.00     0.00  unsigned long const& std::min<unsigned long>(unsigned long const&, unsigned long const&)
  0.00      5.57     0.00   763580     0.00     0.00  std::_Vector_base<node, std::allocator<node> >::_M_get_Tp_allocator() const
  0.00      5.57     0.00   763580     0.00     0.00  std::vector<node, std::allocator<node> >::max_size() const
  0.00      5.57     0.00   763580     0.00     0.00  std::_Vector_base<node, std::allocator<node> >::_M_get_Tp_allocator()
  0.00      5.57     0.00   763580     0.00     0.00  std::allocator_traits<std::allocator<node> >::max_size(std::allocator<node> const&)
  0.00      5.57     0.00   763580     0.00     0.00  std::vector<node, std::allocator<node> >::_S_max_size(std::allocator<node> const&)
  0.00      5.57     0.00   763580     0.00     0.05  std::vector<node, std::allocator<node> >::_S_relocate(node*, node*, node*, std::allocator<node>&)
  0.00      5.57     0.00   763580     0.00     0.05  std::vector<node, std::allocator<node> >::_S_do_relocate(node*, node*, node*, std::allocator<node>&, std::integral_constant<bool, true>)
  0.00      5.57     0.00   763580     0.00     0.05  node* std::__relocate_a<node*, node*, std::allocator<node> >(node*, node*, node*, std::allocator<node>&)
  0.00      5.57     0.00   763580     0.00     0.05  node* std::__relocate_a_1<node*, node*, std::allocator<node> >(node*, node*, node*, std::allocator<node>&)
  0.00      5.57     0.00   708600     0.00     0.00  __gnu_cxx::new_allocator<int>::~new_allocator()
  0.00      5.57     0.00   708600     0.00     0.00  std::allocator<int>::~allocator()
  0.00      5.57     0.00   708600     0.00     0.00  std::_Vector_base<int, std::allocator<int> >::_Vector_impl_data::_M_copy_data(std::_Vector_base<int, std::allocator<int> >::_Vector_impl_data const&)
  0.00      5.57     0.00   708600     0.00     0.00  std::_Vector_base<int, std::allocator<int> >::_Vector_impl_data::_Vector_impl_data()
  0.00      5.57     0.00   617990     0.00     0.00  __gnu_cxx::__normal_iterator<int*, std::vector<int, std::allocator<int> > >::__normal_iterator(int* const&)
  0.00      5.57     0.00   499890     0.00     0.00  std::vector<int, std::allocator<int> >::end()
  0.00      5.57     0.00   472400     0.00     0.00  __gnu_cxx::new_allocator<int>::new_allocator(__gnu_cxx::new_allocator<int> const&)
  0.00      5.57     0.00   472400     0.00     0.00  std::allocator<int>::allocator(std::allocator<int> const&)
  0.00      5.57     0.00   472400     0.00     0.00  void std::_Destroy_aux<true>::__destroy<int*>(int*, int*)
  0.00      5.57     0.00   472400     0.00     0.00  std::_Vector_base<int, std::allocator<int> >::_Vector_impl::~_Vector_impl()
  0.00      5.57     0.00   472400     0.00     0.00  std::_Vector_base<int, std::allocator<int> >::_M_deallocate(int*, unsigned long)
  0.00      5.57     0.00   472400     0.00     0.00  std::_Vector_base<int, std::allocator<int> >::~_Vector_base()
  0.00      5.57     0.00   472400     0.00     0.00  std::vector<int, std::allocator<int> >::~vector()
  0.00      5.57     0.00   472400     0.00     0.00  void std::_Destroy<int*>(int*, int*)
  0.00      5.57     0.00   472400     0.00     0.00  void std::_Destroy<int*, int>(int*, int*, std::allocator<int>&)
  0.00      5.57     0.00   381790     0.00     0.00  __gnu_cxx::new_allocator<node>::allocate(unsigned long, void const*)
  0.00      5.57     0.00   381790     0.00     0.00  __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >::difference_type __gnu_cxx::operator-<node*, std::vector<node, std::allocator<node> > >(__gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > > const&, __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > > const&)
  0.00      5.57     0.00   381790     0.00     0.00  std::vector<node, std::allocator<node> >::_M_check_len(unsigned long, char const*) const
  0.00      5.57     0.00   381790     0.00     0.00  std::_Vector_base<node, std::allocator<node> >::_M_allocate(unsigned long)
  0.00      5.57     0.00   381790     0.00     0.00  std::_Vector_base<node, std::allocator<node> >::_M_deallocate(node*, unsigned long)
  0.00      5.57     0.00   381790     0.00     0.00  std::allocator_traits<std::allocator<node> >::allocate(std::allocator<node>&, unsigned long)
  0.00      5.57     0.00   381790     0.00     0.00  std::vector<node, std::allocator<node> >::begin()
  0.00      5.57     0.00   381790     0.00     0.00  unsigned long const& std::max<unsigned long>(unsigned long const&, unsigned long const&)
  0.00      5.57     0.00   279050     0.00     0.00  __gnu_cxx::new_allocator<node>::deallocate(node*, unsigned long)
  0.00      5.57     0.00   279050     0.00     0.00  std::allocator_traits<std::allocator<node> >::deallocate(std::allocator<node>&, node*, unsigned long)
  0.00      5.57     0.00   236260     0.00     0.00  bool __gnu_cxx::__is_null_pointer<char>(char*)
  0.00      5.57     0.00   236260     0.00     0.00  void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char*>(char*, char*, std::forward_iterator_tag)
  0.00      5.57     0.00   236260     0.00     0.00  std::iterator_traits<char*>::difference_type std::__distance<char*>(char*, char*, std::random_access_iterator_tag)
  0.00      5.57     0.00   236260     0.00     0.00  std::iterator_traits<char*>::iterator_category std::__iterator_category<char*>(char* const&)
  0.00      5.57     0.00   236260     0.00     0.00  std::iterator_traits<char*>::difference_type std::distance<char*>(char*, char*)
  0.00      5.57     0.00   236238     0.00     0.04  void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char const*>(char const*, char const*)
  0.00      5.57     0.00   236238     0.00     0.04  void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct_aux<char const*>(char const*, char const*, std::__false_type)
  0.00      5.57     0.00   236238     0.00     0.07  std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::basic_string<std::allocator<char> >(char const*, std::allocator<char> const&)
  0.00      5.57     0.00   236200     0.00     0.00  __gnu_cxx::new_allocator<int>::new_allocator()
  0.00      5.57     0.00   236200     0.00     0.00  __gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >::__normal_iterator(int const* const&)
  0.00      5.57     0.00   236200     0.00     0.00  __gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >::base() const
  0.00      5.57     0.00   236200     0.00     0.00  std::_Vector_base<int, std::allocator<int> >::_M_get_Tp_allocator() const
  0.00      5.57     0.00   236200     0.00     0.00  std::vector<int, std::allocator<int> >::size() const
  0.00      5.57     0.00   236200     0.00     0.00  std::allocator<int>::allocator()
  0.00      5.57     0.00   236200     0.00     0.00  std::_Vector_base<int, std::allocator<int> >::_Vector_impl::_Vector_impl(std::allocator<int> const&)
  0.00      5.57     0.00   236200     0.00     0.00  std::_Vector_base<int, std::allocator<int> >::_Vector_impl::_Vector_impl()
  0.00      5.57     0.00   236200     0.00     0.00  std::_Vector_base<int, std::allocator<int> >::_Vector_impl_data::_M_swap_data(std::_Vector_base<int, std::allocator<int> >::_Vector_impl_data&)
  0.00      5.57     0.00   236200     0.00     0.00  std::_Vector_base<int, std::allocator<int> >::_Vector_base()
  0.00      5.57     0.00   236200     0.00     0.00  std::vector<int, std::allocator<int> >::vector()
  0.00      5.57     0.00   236200     0.00     0.00  __gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > > std::__miter_base<__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > > >(__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >)
  0.00      5.57     0.00   236200     0.00     0.00  int const* std::__niter_base<int const*, std::vector<int, std::allocator<int> > >(__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >)
  0.00      5.57     0.00   205480     0.00     0.00  __gnu_cxx::new_allocator<int>::deallocate(int*, unsigned long)
  0.00      5.57     0.00   205480     0.00     0.00  std::allocator_traits<std::allocator<int> >::deallocate(std::allocator<int>&, int*, unsigned long)
  0.00      5.57     0.00   118143     0.00     0.03  std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(char const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&)
  0.00      5.57     0.00   118140     0.00     0.03  AdjList::_get_table_cursor(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, __wt_cursor**, bool)
  0.00      5.57     0.00   118118     0.00     0.03  AdjList::get_node_cursor()
  0.00      5.57     0.00   118100     0.00     0.75  CommonUtil::unpack_int_vector_std(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >)
  0.00      5.57     0.00   118100     0.00     0.82  AdjList::get_adjlist(__wt_cursor*, int)
  0.00      5.57     0.00   118100     0.00     1.40  AdjList::get_out_nodes(int)
  0.00      5.57     0.00   118100     0.00     0.82  AdjList::__record_to_adjlist(__wt_cursor*, adjlist*)
  0.00      5.57     0.00   118100     0.00     0.00  AdjList::get_out_adjlist_cursor()
  0.00      5.57     0.00   118100     0.00     0.00  adjlist::adjlist()
  0.00      5.57     0.00   118100     0.00     0.00  adjlist::~adjlist()
  0.00      5.57     0.00   118100     0.00     0.00  __gnu_cxx::new_allocator<node>::new_allocator()
  0.00      5.57     0.00   118100     0.00     0.00  __gnu_cxx::__alloc_traits<std::allocator<int>, int>::_S_select_on_copy(std::allocator<int> const&)
  0.00      5.57     0.00   118100     0.00     0.00  std::_Vector_base<int, std::allocator<int> >::get_allocator() const
  0.00      5.57     0.00   118100     0.00     0.00  std::vector<int, std::allocator<int> >::end() const
  0.00      5.57     0.00   118100     0.00     0.00  std::vector<int, std::allocator<int> >::begin() const
  0.00      5.57     0.00   118100     0.00     0.00  std::allocator<node>::allocator()
  0.00      5.57     0.00   118100     0.00     0.00  int* std::__copy_move<false, true, std::random_access_iterator_tag>::__copy_m<int>(int const*, int const*, int*)
  0.00      5.57     0.00   118100     0.00     0.00  std::_Vector_base<node, std::allocator<node> >::_Vector_impl::_Vector_impl()
  0.00      5.57     0.00   118100     0.00     0.00  std::_Vector_base<node, std::allocator<node> >::_Vector_impl_data::_Vector_impl_data()
  0.00      5.57     0.00   118100     0.00     0.00  std::_Vector_base<node, std::allocator<node> >::_Vector_base()
  0.00      5.57     0.00   118100     0.00     0.00  std::_Vector_base<int, std::allocator<int> >::_M_allocate(unsigned long)
  0.00      5.57     0.00   118100     0.00     0.00  std::_Vector_base<int, std::allocator<int> >::_M_create_storage(unsigned long)
  0.00      5.57     0.00   118100     0.00     0.00  std::_Vector_base<int, std::allocator<int> >::_Vector_base(std::allocator<int> const&)
  0.00      5.57     0.00   118100     0.00     0.00  std::_Vector_base<int, std::allocator<int> >::_Vector_base(unsigned long, std::allocator<int> const&)
  0.00      5.57     0.00   118100     0.00     0.00  std::allocator_traits<std::allocator<int> >::select_on_container_copy_construction(std::allocator<int> const&)
  0.00      5.57     0.00   118100     0.00     0.00  int* std::__uninitialized_copy<true>::__uninit_copy<__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, int*>(__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, __gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, int*)
  0.00      5.57     0.00   118100     0.00     0.00  std::vector<node, std::allocator<node> >::vector()
  0.00      5.57     0.00   118100     0.00     0.00  std::vector<int, std::allocator<int> >::_M_move_assign(std::vector<int, std::allocator<int> >&&, std::integral_constant<bool, true>)
  0.00      5.57     0.00   118100     0.00     0.00  std::vector<int, std::allocator<int> >::begin()
  0.00      5.57     0.00   118100     0.00     0.00  std::vector<int, std::allocator<int> >::vector(std::allocator<int> const&)
  0.00      5.57     0.00   118100     0.00     0.00  std::vector<int, std::allocator<int> >::vector(std::vector<int, std::allocator<int> > const&)
  0.00      5.57     0.00   118100     0.00     0.00  std::vector<int, std::allocator<int> >::operator=(std::vector<int, std::allocator<int> >&&)
  0.00      5.57     0.00   118100     0.00     0.00  int* std::__niter_base<int*>(int*)
  0.00      5.57     0.00   118100     0.00     0.00  int* std::__niter_wrap<int*>(int* const&, int*)
  0.00      5.57     0.00   118100     0.00     0.00  int* std::__copy_move_a<false, int const*, int*>(int const*, int const*, int*)
  0.00      5.57     0.00   118100     0.00     0.00  int* std::__copy_move_a2<false, __gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, int*>(__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, __gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, int*)
  0.00      5.57     0.00   118100     0.00     0.00  void std::__alloc_on_move<std::allocator<int> >(std::allocator<int>&, std::allocator<int>&)
  0.00      5.57     0.00   118100     0.00     0.00  void std::__do_alloc_on_move<std::allocator<int> >(std::allocator<int>&, std::allocator<int>&, std::integral_constant<bool, true>)
  0.00      5.57     0.00   118100     0.00     0.00  int* std::uninitialized_copy<__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, int*>(__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, __gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, int*)
  0.00      5.57     0.00   118100     0.00     0.00  int* std::__uninitialized_copy_a<__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, int*, int>(__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, __gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, int*, std::allocator<int>&)
  0.00      5.57     0.00   118100     0.00     0.00  int* std::copy<__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, int*>(__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, __gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, int*)
  0.00      5.57     0.00   118100     0.00     0.00  std::remove_reference<std::allocator<int>&>::type&& std::move<std::allocator<int>&>(std::allocator<int>&)
  0.00      5.57     0.00   118100     0.00     0.00  std::remove_reference<std::vector<int, std::allocator<int> >&>::type&& std::move<std::vector<int, std::allocator<int> >&>(std::vector<int, std::allocator<int> >&)
  0.00      5.57     0.00   102740     0.00     0.00  __gnu_cxx::new_allocator<int>::allocate(unsigned long, void const*)
  0.00      5.57     0.00   102740     0.00     0.00  __gnu_cxx::new_allocator<int>::max_size() const
  0.00      5.57     0.00   102740     0.00     0.00  std::allocator_traits<std::allocator<int> >::allocate(std::allocator<int>&, unsigned long)
  0.00      5.57     0.00     1350     0.00     0.00  std::_Rb_tree<int, int, std::_Identity<int>, std::less<int>, std::allocator<int> >::_M_erase(std::_Rb_tree_node<int>*)
  0.00      5.57     0.00      800     0.00     0.00  void std::deque<int, std::allocator<int> >::_M_push_back_aux<int const&>(int const&)
  0.00      5.57     0.00      160     0.00     0.00  std::_Deque_base<int, std::allocator<int> >::_M_initialize_map(unsigned long)
  0.00      5.57     0.00       18     0.00     0.00  node::node()
  0.00      5.57     0.00       18     0.00     0.04  AdjList::get_out_degree(int)
  0.00      5.57     0.00       18     0.00     0.04  AdjList::get_random_node()
  0.00      5.57     0.00       18     0.00     0.00  std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_mutate(unsigned long, unsigned long, char const*, unsigned long)
  0.00      5.57     0.00        9     0.00     0.00  __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::~new_allocator()
  0.00      5.57     0.00        9     0.00     0.00  std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::~allocator()
  0.00      5.57     0.00        9     0.00     0.00  std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_get_Tp_allocator()
  0.00      5.57     0.00        9     0.00     0.00  void std::_Construct<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&)
  0.00      5.57     0.00        9     0.00     0.00  std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::__addressof<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&)
  0.00      5.57     0.00        9     0.00     0.00  std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const& std::forward<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&>(std::remove_reference<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&>::type&)
  0.00      5.57     0.00        8     0.00     0.00  __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::new_allocator(__gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&)
  0.00      5.57     0.00        8     0.00     0.00  __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::max_size() const
  0.00      5.57     0.00        8     0.00     0.00  std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::begin() const
  0.00      5.57     0.00        8     0.00     0.00  std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::allocator(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&)
  0.00      5.57     0.00        5     0.00     0.00  __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::new_allocator()
  0.00      5.57     0.00        5     0.00     0.00  std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::allocator()
  0.00      5.57     0.00        5     0.00     0.00  std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl_data::_Vector_impl_data()
  0.00      5.57     0.00        5     0.00     0.00  void std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_realloc_insert<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >(__gnu_cxx::__normal_iterator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > > >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&)
  0.00      5.57     0.00        4     0.00     0.00  __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::allocate(unsigned long, void const*)
  0.00      5.57     0.00        4     0.00     0.00  std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::end() const
  0.00      5.57     0.00        4     0.00     0.00  std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::size() const
  0.00      5.57     0.00        4     0.00     0.00  std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_allocate(unsigned long)
  0.00      5.57     0.00        4     0.00     0.00  std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl::_Vector_impl(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&)
  0.00      5.57     0.00        4     0.00     0.00  std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_base(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&)
  0.00      5.57     0.00        4     0.00     0.00  std::allocator_traits<std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::allocate(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >&, unsigned long)
  0.00      5.57     0.00        4     0.00     0.00  std::allocator_traits<std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::max_size(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&)
  0.00      5.57     0.00        4     0.00     0.00  std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::__uninitialized_copy<false>::__uninit_copy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*)
  0.00      5.57     0.00        4     0.00     0.00  std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_S_max_size(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&)
  0.00      5.57     0.00        4     0.00     0.00  std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_S_check_init_len(unsigned long, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&)
  0.00      5.57     0.00        4     0.00     0.00  void std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_range_initialize<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::forward_iterator_tag)
  0.00      5.57     0.00        4     0.00     0.00  std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::vector(std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&)
  0.00      5.57     0.00        4     0.00     0.00  std::iterator_traits<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>::difference_type std::__distance<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::random_access_iterator_tag)
  0.00      5.57     0.00        4     0.00     0.00  std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::uninitialized_copy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*)
  0.00      5.57     0.00        4     0.00     0.00  std::iterator_traits<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>::iterator_category std::__iterator_category<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const* const&)
  0.00      5.57     0.00        4     0.00     0.00  std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::__uninitialized_copy_a<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >&)
  0.00      5.57     0.00        4     0.00     0.00  std::remove_reference<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&>::type&& std::move<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&)
  0.00      5.57     0.00        4     0.00     0.00  std::iterator_traits<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>::difference_type std::distance<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*)
  0.00      5.57     0.00        3     0.00     0.00  std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&)
  0.00      5.57     0.00        2     0.00     0.00  std::filesystem::file_status::type() const
  0.00      5.57     0.00        2     0.00     0.00  std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&, char const*)
  0.00      5.57     0.00        1     0.00     0.00  _GLOBAL__sub_I__Z14print_csv_infoNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEiP8bfs_info
  0.00      5.57     0.00        1     0.00     2.20  _GLOBAL__sub_I__Z8METADATAB5cxx11
  0.00      5.57     0.00        1     0.00     0.07  _GLOBAL__sub_I__ZN13StandardGraphC2Ev
  0.00      5.57     0.00        1     0.00     0.07  _GLOBAL__sub_I__ZN7AdjListC2Ev
  0.00      5.57     0.00        1     0.00     0.07  _GLOBAL__sub_I__ZN7EdgeKeyC2Ev
  0.00      5.57     0.00        1     0.00     0.07  __static_initialization_and_destruction_0(int, int)
  0.00      5.57     0.00        1     0.00     2.20  __static_initialization_and_destruction_0(int, int)
  0.00      5.57     0.00        1     0.00     0.07  __static_initialization_and_destruction_0(int, int)
  0.00      5.57     0.00        1     0.00     0.07  __static_initialization_and_destruction_0(int, int)
  0.00      5.57     0.00        1     0.00     0.00  CommonUtil::open_session(__wt_connection*, __wt_session**)
  0.00      5.57     0.00        1     0.00     0.00  CommonUtil::open_connection(char*, __wt_connection**)
  0.00      5.57     0.00        1     0.00     0.00  CommonUtil::close_connection(__wt_connection*)
  0.00      5.57     0.00        1     0.00     0.00  CommonUtil::check_graph_params(graph_opts)
  0.00      5.57     0.00        1     0.00     0.00  graph_opts::graph_opts(graph_opts const&)
  0.00      5.57     0.00        1     0.00     0.00  graph_opts::~graph_opts()
  0.00      5.57     0.00        1     0.00     0.00  CmdLineBase::CmdLineBase(int, char**)
  0.00      5.57     0.00        1     0.00     0.08  AdjList::init_cursors()
  0.00      5.57     0.00        1     0.00     0.11  AdjList::__restore_from_db(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >)
  0.00      5.57     0.00        1     0.00     0.00  std::ctype<char>::do_widen(char) const
  0.00      5.57     0.00        1     0.00     0.00  std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::size() const
  0.00      5.57     0.00        1     0.00     0.00  std::_Head_base<0ul, std::filesystem::__cxx11::path::_List::_Impl*, false>::_M_head(std::_Head_base<0ul, std::filesystem::__cxx11::path::_List::_Impl*, false>&)
  0.00      5.57     0.00        1     0.00     0.00  std::_Head_base<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter, true>::_M_head(std::_Head_base<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter, true>&)
  0.00      5.57     0.00        1     0.00     0.00  std::filesystem::status_known(std::filesystem::file_status)
  0.00      5.57     0.00        1     0.00     0.00  std::filesystem::exists(std::filesystem::file_status)
  0.00      5.57     0.00        1     0.00     0.00  std::filesystem::exists(std::filesystem::__cxx11::path const&)
  0.00      5.57     0.00        1     0.00     0.00  std::filesystem::__cxx11::path::_List::~_List()
  0.00      5.57     0.00        1     0.00     0.00  std::filesystem::__cxx11::path::path(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&, std::filesystem::__cxx11::path::format)
  0.00      5.57     0.00        1     0.00     0.00  std::filesystem::__cxx11::path::~path()
  0.00      5.57     0.00        1     0.00     0.00  std::unique_ptr<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::get_deleter()
  0.00      5.57     0.00        1     0.00     0.00  std::unique_ptr<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::~unique_ptr()
  0.00      5.57     0.00        1     0.00     0.00  std::_Tuple_impl<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_head(std::_Tuple_impl<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&)
  0.00      5.57     0.00        1     0.00     0.00  std::_Tuple_impl<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_head(std::_Tuple_impl<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>&)
  0.00      5.57     0.00        1     0.00     0.00  void std::_Destroy_aux<false>::__destroy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*)
  0.00      5.57     0.00        1     0.00     0.00  std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl::_Vector_impl()
  0.00      5.57     0.00        1     0.00     0.00  std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl::~_Vector_impl()
  0.00      5.57     0.00        1     0.00     0.00  std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_deallocate(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, unsigned long)
  0.00      5.57     0.00        1     0.00     0.00  std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_base()
  0.00      5.57     0.00        1     0.00     0.00  std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::~_Vector_base()
  0.00      5.57     0.00        1     0.00     0.00  std::__uniq_ptr_impl<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_deleter()
  0.00      5.57     0.00        1     0.00     0.00  std::__uniq_ptr_impl<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_ptr()
  0.00      5.57     0.00        1     0.00     0.00  std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::vector()
  0.00      5.57     0.00        1     0.00     0.00  std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::~vector()
  0.00      5.57     0.00        1     0.00     0.00  std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_replace(unsigned long, unsigned long, char const*, unsigned long)
  0.00      5.57     0.00        1     0.00     0.00  std::filesystem::__cxx11::path::_List::_Impl*& std::__get_helper<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::_Tuple_impl<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&)
  0.00      5.57     0.00        1     0.00     0.00  std::filesystem::__cxx11::path::_List::_Impl_deleter& std::__get_helper<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::_Tuple_impl<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>&)
  0.00      5.57     0.00        1     0.00     0.00  std::tuple_element<0ul, std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter> >::type& std::get<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&)
  0.00      5.57     0.00        1     0.00     0.00  std::tuple_element<1ul, std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter> >::type& std::get<1ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&)
  0.00      5.57     0.00        1     0.00     0.00  std::remove_reference<std::filesystem::__cxx11::path::_List::_Impl*&>::type&& std::move<std::filesystem::__cxx11::path::_List::_Impl*&>(std::filesystem::__cxx11::path::_List::_Impl*&)
  0.00      5.57     0.00        1     0.00     0.00  void std::_Destroy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*)
  0.00      5.57     0.00        1     0.00     0.00  void std::_Destroy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >&)

 %         the percentage of the total running time of the
time       program used by this function.

cumulative a running sum of the number of seconds accounted
 seconds   for by this function and those listed above it.

 self      the number of seconds accounted for by this
seconds    function alone.  This is the major sort for this
           listing.

calls      the number of times this function was invoked, if
           this function is profiled, else blank.

 self      the average number of milliseconds spent in this
ms/call    function per call, if this function is profiled,
	   else blank.

 total     the average number of milliseconds spent in this
ms/call    function and its descendents per call, if this
	   function is profiled, else blank.

name       the name of the function.  This is the minor sort
           for this listing. The index shows the location of
	   the function in the gprof listing. If the index is
	   in parenthesis it shows where it would appear in
	   the gprof listing if it were to be printed.

Copyright (C) 2012-2020 Free Software Foundation, Inc.

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved.

		     Call graph (explanation follows)


granularity: each sample hit covers 2 byte(s) for 0.18% of 5.57 seconds

index % time    self  children    called     name
                                                 <spontaneous>
[1]     45.1    2.51    0.00                 __wt_hazard_clear [1]
-----------------------------------------------
                                                 <spontaneous>
[2]     35.5    1.98    0.00                 __wt_hazard_set [2]
-----------------------------------------------
                                                 <spontaneous>
[3]      5.7    0.32    0.00                 __global_once [3]
-----------------------------------------------
                                                 <spontaneous>
[4]      5.3    0.13    0.16                 bfs_info* bfs<AdjList>(AdjList&, int) [4]
                0.00    0.16  118100/118100      AdjList::get_out_nodes(int) [5]
                0.00    0.00    1760/383550      void std::vector<int, std::allocator<int> >::_M_realloc_insert<int const&>(__gnu_cxx::__normal_iterator<int*, std::vector<int, std::allocator<int> > >, int const&) [36]
                0.00    0.00    1350/1350        std::_Rb_tree<int, int, std::_Identity<int>, std::less<int>, std::allocator<int> >::_M_erase(std::_Rb_tree_node<int>*) [227]
                0.00    0.00     800/800         void std::deque<int, std::allocator<int> >::_M_push_back_aux<int const&>(int const&) [228]
                0.00    0.00     160/160         std::_Deque_base<int, std::allocator<int> >::_M_initialize_map(unsigned long) [229]
-----------------------------------------------
                0.00    0.16  118100/118100      bfs_info* bfs<AdjList>(AdjList&, int) [4]
[5]      3.0    0.00    0.16  118100         AdjList::get_out_nodes(int) [5]
                0.00    0.10  118100/118100      AdjList::get_adjlist(__wt_cursor*, int) [7]
                0.00    0.06 1281310/1281310     std::vector<node, std::allocator<node> >::push_back(node&&) [12]
                0.01    0.00 1281310/1281346     AdjList::__record_to_node(__wt_cursor*, int) [33]
                0.00    0.00  118100/118118      AdjList::get_node_cursor() [59]
                0.00    0.00 1399410/1399410     bool __gnu_cxx::operator!=<int*, std::vector<int, std::allocator<int> > >(__gnu_cxx::__normal_iterator<int*, std::vector<int, std::allocator<int> > > const&, __gnu_cxx::__normal_iterator<int*, std::vector<int, std::allocator<int> > > const&) [127]
                0.00    0.00 1281310/1281310     __gnu_cxx::__normal_iterator<int*, std::vector<int, std::allocator<int> > >::operator*() const [131]
                0.00    0.00 1281310/1281310     __gnu_cxx::__normal_iterator<int*, std::vector<int, std::allocator<int> > >::operator++() [128]
                0.00    0.00  118100/118100      std::vector<node, std::allocator<node> >::vector() [207]
                0.00    0.00  118100/118100      AdjList::get_out_adjlist_cursor() [188]
                0.00    0.00  118100/118100      std::vector<int, std::allocator<int> >::begin() [209]
                0.00    0.00  118100/499890      std::vector<int, std::allocator<int> >::end() [148]
                0.00    0.00  118100/472400      std::vector<int, std::allocator<int> >::~vector() [155]
-----------------------------------------------
                                                 <spontaneous>
[6]      2.7    0.15    0.00                 __wt_row_search [6]
-----------------------------------------------
                0.00    0.10  118100/118100      AdjList::get_out_nodes(int) [5]
[7]      1.7    0.00    0.10  118100         AdjList::get_adjlist(__wt_cursor*, int) [7]
                0.00    0.10  118100/118100      AdjList::__record_to_adjlist(__wt_cursor*, adjlist*) [8]
                0.00    0.00  118100/118100      adjlist::adjlist() [189]
                0.00    0.00  118100/118100      std::vector<int, std::allocator<int> >::vector(std::vector<int, std::allocator<int> > const&) [211]
                0.00    0.00  118100/118100      adjlist::~adjlist() [190]
-----------------------------------------------
                0.00    0.10  118100/118100      AdjList::get_adjlist(__wt_cursor*, int) [7]
[8]      1.7    0.00    0.10  118100         AdjList::__record_to_adjlist(__wt_cursor*, adjlist*) [8]
                0.00    0.09  118100/118100      CommonUtil::unpack_int_vector_std(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >) [10]
                0.00    0.01  118100/236238      std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::basic_string<std::allocator<char> >(char const*, std::allocator<char> const&) [27]
                0.00    0.00  118100/236260      void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char*>(char*, char*, std::forward_iterator_tag) [169]
                0.00    0.00  118100/118100      std::vector<int, std::allocator<int> >::operator=(std::vector<int, std::allocator<int> >&&) [212]
                0.00    0.00  118100/472400      std::vector<int, std::allocator<int> >::~vector() [155]
                0.00    0.00  118100/236200      std::vector<int, std::allocator<int> >::size() const [177]
-----------------------------------------------
                                                 <spontaneous>
[9]      1.6    0.09    0.00                 __wt_schema_project_out [9]
-----------------------------------------------
                0.00    0.09  118100/118100      AdjList::__record_to_adjlist(__wt_cursor*, adjlist*) [8]
[10]     1.6    0.00    0.09  118100         CommonUtil::unpack_int_vector_std(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >) [10]
                0.01    0.03 1281310/1281310     std::vector<int, std::allocator<int> >::push_back(int const&) [14]
                0.00    0.03 1281310/1281310     std::__cxx11::stoi(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&, unsigned long*, int) [21]
                0.00    0.01 1517510/1517510     bool std::operator!=<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&, char const*) [54]
                0.00    0.01  118100/236238      std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::basic_string<std::allocator<char> >(char const*, std::allocator<char> const&) [27]
                0.00    0.00  118100/1635610     bool std::operator==<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&, char const*) [30]
                0.00    0.00  118100/236200      std::vector<int, std::allocator<int> >::vector() [183]
-----------------------------------------------
                0.00    0.06 1281310/1281310     std::vector<node, std::allocator<node> >::push_back(node&&) [12]
[11]     1.0    0.00    0.06 1281310         node& std::vector<node, std::allocator<node> >::emplace_back<node>(node&&) [11]
                0.01    0.04  381790/381790      void std::vector<node, std::allocator<node> >::_M_realloc_insert<node>(__gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >, node&&) [13]
                0.00    0.00  899520/2897160     void std::allocator_traits<std::allocator<node> >::construct<node, node>(std::allocator<node>&, node*, node&&) [28]
                0.00    0.00 1281310/7457420     node&& std::forward<node>(std::remove_reference<node>::type&) [55]
                0.00    0.00 1281310/1281310     std::vector<node, std::allocator<node> >::back() [132]
                0.00    0.00  381790/1663100     std::vector<node, std::allocator<node> >::end() [120]
-----------------------------------------------
                0.00    0.06 1281310/1281310     AdjList::get_out_nodes(int) [5]
[12]     1.0    0.00    0.06 1281310         std::vector<node, std::allocator<node> >::push_back(node&&) [12]
                0.00    0.06 1281310/1281310     node& std::vector<node, std::allocator<node> >::emplace_back<node>(node&&) [11]
                0.00    0.00 1281310/2897160     std::remove_reference<node&>::type&& std::move<node&>(node&) [116]
-----------------------------------------------
                0.01    0.04  381790/381790      node& std::vector<node, std::allocator<node> >::emplace_back<node>(node&&) [11]
[13]     0.9    0.01    0.04  381790         void std::vector<node, std::allocator<node> >::_M_realloc_insert<node>(__gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >, node&&) [13]
                0.00    0.04  763580/763580      std::vector<node, std::allocator<node> >::_S_relocate(node*, node*, node*, std::allocator<node>&) [16]
                0.00    0.00  381790/2897160     void std::allocator_traits<std::allocator<node> >::construct<node, node>(std::allocator<node>&, node*, node&&) [28]
                0.00    0.00  381790/7457420     node&& std::forward<node>(std::remove_reference<node>::type&) [55]
                0.00    0.00  763580/1527160     __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >::base() const [121]
                0.00    0.00  763580/763580      std::_Vector_base<node, std::allocator<node> >::_M_get_Tp_allocator() [140]
                0.00    0.00  381790/381790      std::vector<node, std::allocator<node> >::_M_check_len(unsigned long, char const*) const [160]
                0.00    0.00  381790/381790      std::vector<node, std::allocator<node> >::begin() [164]
                0.00    0.00  381790/381790      __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >::difference_type __gnu_cxx::operator-<node*, std::vector<node, std::allocator<node> > >(__gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > > const&, __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > > const&) [159]
                0.00    0.00  381790/381790      std::_Vector_base<node, std::allocator<node> >::_M_allocate(unsigned long) [161]
                0.00    0.00  381790/381790      std::_Vector_base<node, std::allocator<node> >::_M_deallocate(node*, unsigned long) [162]
-----------------------------------------------
                0.01    0.03 1281310/1281310     CommonUtil::unpack_int_vector_std(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >) [10]
[14]     0.7    0.01    0.03 1281310         std::vector<int, std::allocator<int> >::push_back(int const&) [14]
                0.02    0.00  899520/899520      void std::allocator_traits<std::allocator<int> >::construct<int, int const&>(std::allocator<int>&, int*, int const&) [23]
                0.01    0.00  381790/383550      void std::vector<int, std::allocator<int> >::_M_realloc_insert<int const&>(__gnu_cxx::__normal_iterator<int*, std::vector<int, std::allocator<int> > >, int const&) [36]
                0.00    0.00  381790/499890      std::vector<int, std::allocator<int> >::end() [148]
-----------------------------------------------
                0.02    0.02 1615850/1615850     node* std::__relocate_a_1<node*, node*, std::allocator<node> >(node*, node*, node*, std::allocator<node>&) [19]
[15]     0.7    0.02    0.02 1615850         void std::__relocate_object_a<node, node, std::allocator<node> >(node*, node*, std::allocator<node>&) [15]
                0.00    0.01 1615850/1615850     void std::allocator_traits<std::allocator<node> >::destroy<node>(std::allocator<node>&, node*) [32]
                0.01    0.00 1615850/2897160     void std::allocator_traits<std::allocator<node> >::construct<node, node>(std::allocator<node>&, node*, node&&) [28]
                0.00    0.00 1615850/2897160     std::remove_reference<node&>::type&& std::move<node&>(node&) [116]
                0.00    0.00 1615850/4847550     node* std::__addressof<node>(node&) [113]
-----------------------------------------------
                0.00    0.04  763580/763580      void std::vector<node, std::allocator<node> >::_M_realloc_insert<node>(__gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >, node&&) [13]
[16]     0.7    0.00    0.04  763580         std::vector<node, std::allocator<node> >::_S_relocate(node*, node*, node*, std::allocator<node>&) [16]
                0.00    0.04  763580/763580      std::vector<node, std::allocator<node> >::_S_do_relocate(node*, node*, node*, std::allocator<node>&, std::integral_constant<bool, true>) [17]
-----------------------------------------------
                0.00    0.04  763580/763580      std::vector<node, std::allocator<node> >::_S_relocate(node*, node*, node*, std::allocator<node>&) [16]
[17]     0.7    0.00    0.04  763580         std::vector<node, std::allocator<node> >::_S_do_relocate(node*, node*, node*, std::allocator<node>&, std::integral_constant<bool, true>) [17]
                0.00    0.04  763580/763580      node* std::__relocate_a<node*, node*, std::allocator<node> >(node*, node*, node*, std::allocator<node>&) [18]
-----------------------------------------------
                0.00    0.04  763580/763580      std::vector<node, std::allocator<node> >::_S_do_relocate(node*, node*, node*, std::allocator<node>&, std::integral_constant<bool, true>) [17]
[18]     0.7    0.00    0.04  763580         node* std::__relocate_a<node*, node*, std::allocator<node> >(node*, node*, node*, std::allocator<node>&) [18]
                0.00    0.04  763580/763580      node* std::__relocate_a_1<node*, node*, std::allocator<node> >(node*, node*, node*, std::allocator<node>&) [19]
                0.00    0.00 2290740/2290740     node* std::__niter_base<node*>(node*) [118]
-----------------------------------------------
                0.00    0.04  763580/763580      node* std::__relocate_a<node*, node*, std::allocator<node> >(node*, node*, node*, std::allocator<node>&) [18]
[19]     0.7    0.00    0.04  763580         node* std::__relocate_a_1<node*, node*, std::allocator<node> >(node*, node*, node*, std::allocator<node>&) [19]
                0.02    0.02 1615850/1615850     void std::__relocate_object_a<node, node, std::allocator<node> >(node*, node*, std::allocator<node>&) [15]
                0.00    0.00 3231700/4847550     node* std::__addressof<node>(node&) [113]
-----------------------------------------------
                0.01    0.02 1281310/1281310     std::__cxx11::stoi(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&, unsigned long*, int) [21]
[20]     0.5    0.01    0.02 1281310         int __gnu_cxx::__stoa<long, int, char, int>(long (*)(char const*, char**, int), char const*, char const*, unsigned long*, int) [20]
                0.01    0.00 1281310/1281310     __gnu_cxx::__stoa<long, int, char, int>(long (*)(char const*, char**, int), char const*, char const*, unsigned long*, int)::_Save_errno::_Save_errno() [34]
                0.01    0.00 1281310/1281310     __gnu_cxx::__stoa<long, int, char, int>(long (*)(char const*, char**, int), char const*, char const*, unsigned long*, int)::_Save_errno::~_Save_errno() [35]
                0.00    0.00 1281310/1281310     __gnu_cxx::__stoa<long, int, char, int>(long (*)(char const*, char**, int), char const*, char const*, unsigned long*, int)::_Range_chk::_S_chk(long, std::integral_constant<bool, true>) [133]
-----------------------------------------------
                0.00    0.03 1281310/1281310     CommonUtil::unpack_int_vector_std(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >) [10]
[21]     0.5    0.00    0.03 1281310         std::__cxx11::stoi(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&, unsigned long*, int) [21]
                0.01    0.02 1281310/1281310     int __gnu_cxx::__stoa<long, int, char, int>(long (*)(char const*, char**, int), char const*, char const*, unsigned long*, int) [20]
-----------------------------------------------
                                                 <spontaneous>
[22]     0.5    0.03    0.00                 __wt_page_in_func [22]
-----------------------------------------------
                0.02    0.00  899520/899520      std::vector<int, std::allocator<int> >::push_back(int const&) [14]
[23]     0.4    0.02    0.00  899520         void std::allocator_traits<std::allocator<int> >::construct<int, int const&>(std::allocator<int>&, int*, int const&) [23]
                0.00    0.00  899520/1799040     int const& std::forward<int const&>(std::remove_reference<int const&>::type&) [119]
                0.00    0.00  899520/899520      void __gnu_cxx::new_allocator<int>::construct<int, int const&>(int*, int const&) [135]
-----------------------------------------------
                                                 <spontaneous>
[24]     0.4    0.02    0.00                 __cursor_func_init.constprop.0 [24]
-----------------------------------------------
                                                 <spontaneous>
[25]     0.4    0.02    0.00                 __wt_btcur_close [25]
-----------------------------------------------
                                                 <spontaneous>
[26]     0.4    0.02    0.00                 __wt_session_strerror [26]
-----------------------------------------------
                0.00    0.00       1/236238      __static_initialization_and_destruction_0(int, int) [73]
                0.00    0.00       1/236238      __static_initialization_and_destruction_0(int, int) [74]
                0.00    0.00       1/236238      __static_initialization_and_destruction_0(int, int) [75]
                0.00    0.00       5/236238      AdjList::AdjList(graph_opts) [67]
                0.00    0.00      30/236238      __static_initialization_and_destruction_0(int, int) [63]
                0.00    0.01  118100/236238      AdjList::__record_to_adjlist(__wt_cursor*, adjlist*) [8]
                0.00    0.01  118100/236238      CommonUtil::unpack_int_vector_std(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >) [10]
[27]     0.3    0.00    0.02  236238         std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::basic_string<std::allocator<char> >(char const*, std::allocator<char> const&) [27]
                0.00    0.01  236238/236238      void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char const*>(char const*, char const*) [38]
                0.01    0.00  236238/354381      std::char_traits<char>::length(char const*) [37]
-----------------------------------------------
                0.00    0.00  381790/2897160     void std::vector<node, std::allocator<node> >::_M_realloc_insert<node>(__gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >, node&&) [13]
                0.00    0.00  899520/2897160     node& std::vector<node, std::allocator<node> >::emplace_back<node>(node&&) [11]
                0.01    0.00 1615850/2897160     void std::__relocate_object_a<node, node, std::allocator<node> >(node*, node*, std::allocator<node>&) [15]
[28]     0.2    0.01    0.00 2897160         void std::allocator_traits<std::allocator<node> >::construct<node, node>(std::allocator<node>&, node*, node&&) [28]
                0.00    0.00 2897160/7457420     node&& std::forward<node>(std::remove_reference<node>::type&) [55]
                0.00    0.00 2897160/2897160     void __gnu_cxx::new_allocator<node>::construct<node, node>(node*, node&&) [60]
-----------------------------------------------
                0.01    0.00 1635610/1635610     bool std::operator==<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&, char const*) [30]
[29]     0.2    0.01    0.00 1635610         std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::compare(char const*) const [29]
-----------------------------------------------
                0.00    0.00  118100/1635610     CommonUtil::unpack_int_vector_std(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >) [10]
                0.00    0.01 1517510/1635610     bool std::operator!=<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&, char const*) [54]
[30]     0.2    0.00    0.01 1635610         bool std::operator==<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&, char const*) [30]
                0.01    0.00 1635610/1635610     std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::compare(char const*) const [29]
-----------------------------------------------
                0.01    0.00 1615850/1615850     void std::allocator_traits<std::allocator<node> >::destroy<node>(std::allocator<node>&, node*) [32]
[31]     0.2    0.01    0.00 1615850         void __gnu_cxx::new_allocator<node>::destroy<node>(node*) [31]
-----------------------------------------------
                0.00    0.01 1615850/1615850     void std::__relocate_object_a<node, node, std::allocator<node> >(node*, node*, std::allocator<node>&) [15]
[32]     0.2    0.00    0.01 1615850         void std::allocator_traits<std::allocator<node> >::destroy<node>(std::allocator<node>&, node*) [32]
                0.01    0.00 1615850/1615850     void __gnu_cxx::new_allocator<node>::destroy<node>(node*) [31]
-----------------------------------------------
                0.00    0.00      18/1281346     AdjList::get_random_node() [66]
                0.00    0.00      18/1281346     AdjList::get_out_degree(int) [65]
                0.01    0.00 1281310/1281346     AdjList::get_out_nodes(int) [5]
[33]     0.2    0.01    0.00 1281346         AdjList::__record_to_node(__wt_cursor*, int) [33]
-----------------------------------------------
                0.01    0.00 1281310/1281310     int __gnu_cxx::__stoa<long, int, char, int>(long (*)(char const*, char**, int), char const*, char const*, unsigned long*, int) [20]
[34]     0.2    0.01    0.00 1281310         __gnu_cxx::__stoa<long, int, char, int>(long (*)(char const*, char**, int), char const*, char const*, unsigned long*, int)::_Save_errno::_Save_errno() [34]
-----------------------------------------------
                0.01    0.00 1281310/1281310     int __gnu_cxx::__stoa<long, int, char, int>(long (*)(char const*, char**, int), char const*, char const*, unsigned long*, int) [20]
[35]     0.2    0.01    0.00 1281310         __gnu_cxx::__stoa<long, int, char, int>(long (*)(char const*, char**, int), char const*, char const*, unsigned long*, int)::_Save_errno::~_Save_errno() [35]
-----------------------------------------------
                0.00    0.00    1760/383550      bfs_info* bfs<AdjList>(AdjList&, int) [4]
                0.01    0.00  381790/383550      std::vector<int, std::allocator<int> >::push_back(int const&) [14]
[36]     0.2    0.01    0.00  383550         void std::vector<int, std::allocator<int> >::_M_realloc_insert<int const&>(__gnu_cxx::__normal_iterator<int*, std::vector<int, std::allocator<int> > >, int const&) [36]
-----------------------------------------------
                0.00    0.00  118143/354381      std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(char const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&) [57]
                0.01    0.00  236238/354381      std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::basic_string<std::allocator<char> >(char const*, std::allocator<char> const&) [27]
[37]     0.2    0.01    0.00  354381         std::char_traits<char>::length(char const*) [37]
-----------------------------------------------
                0.00    0.01  236238/236238      std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::basic_string<std::allocator<char> >(char const*, std::allocator<char> const&) [27]
[38]     0.2    0.00    0.01  236238         void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char const*>(char const*, char const*) [38]
                0.00    0.01  236238/236238      void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct_aux<char const*>(char const*, char const*, std::__false_type) [40]
-----------------------------------------------
                0.01    0.00  236238/236238      void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct_aux<char const*>(char const*, char const*, std::__false_type) [40]
[39]     0.2    0.01    0.00  236238         void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char const*>(char const*, char const*, std::forward_iterator_tag) [39]
                0.00    0.00 1517548/1517548     bool __gnu_cxx::__is_null_pointer<char const>(char const*) [123]
                0.00    0.00 1517548/1517548     std::iterator_traits<char const*>::difference_type std::distance<char const*>(char const*, char const*) [126]
-----------------------------------------------
                0.00    0.01  236238/236238      void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char const*>(char const*, char const*) [38]
[40]     0.2    0.00    0.01  236238         void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct_aux<char const*>(char const*, char const*, std::__false_type) [40]
                0.01    0.00  236238/236238      void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char const*>(char const*, char const*, std::forward_iterator_tag) [39]
-----------------------------------------------
                                                 <spontaneous>
[41]     0.2    0.01    0.00                 __config_getraw [41]
-----------------------------------------------
                                                 <spontaneous>
[42]     0.2    0.01    0.00                 __curfile_insert [42]
-----------------------------------------------
                                                 <spontaneous>
[43]     0.2    0.01    0.00                 __curfile_search [43]
-----------------------------------------------
                                                 <spontaneous>
[44]     0.2    0.01    0.00                 __wt_btcur_search [44]
-----------------------------------------------
                                                 <spontaneous>
[45]     0.2    0.01    0.00                 __wt_calloc [45]
-----------------------------------------------
                                                 <spontaneous>
[46]     0.2    0.01    0.00                 __wt_cursor_set_keyv [46]
-----------------------------------------------
                                                 <spontaneous>
[47]     0.2    0.01    0.00                 __wt_curtable_get_value [47]
-----------------------------------------------
                                                 <spontaneous>
[48]     0.2    0.01    0.00                 __wt_curtable_set_key [48]
-----------------------------------------------
                                                 <spontaneous>
[49]     0.2    0.01    0.00                 __wt_readunlock [49]
-----------------------------------------------
                                                 <spontaneous>
[50]     0.2    0.01    0.00                 __wt_session_gen_leave [50]
-----------------------------------------------
                                                 <spontaneous>
[51]     0.2    0.01    0.00                 __wt_session_release_dhandle [51]
-----------------------------------------------
                                                 <spontaneous>
[52]     0.2    0.01    0.00                 __wt_struct_sizev [52]
-----------------------------------------------
                                                 <spontaneous>
[53]     0.2    0.01    0.00                 __wt_value_return_upd [53]
-----------------------------------------------
                0.00    0.01 1517510/1517510     CommonUtil::unpack_int_vector_std(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >) [10]
[54]     0.2    0.00    0.01 1517510         bool std::operator!=<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&, char const*) [54]
                0.00    0.01 1517510/1635610     bool std::operator==<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&, char const*) [30]
-----------------------------------------------
                0.00    0.00  381790/7457420     void std::vector<node, std::allocator<node> >::_M_realloc_insert<node>(__gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >, node&&) [13]
                0.00    0.00 1281310/7457420     node& std::vector<node, std::allocator<node> >::emplace_back<node>(node&&) [11]
                0.00    0.00 2897160/7457420     void std::allocator_traits<std::allocator<node> >::construct<node, node>(std::allocator<node>&, node*, node&&) [28]
                0.00    0.00 2897160/7457420     void __gnu_cxx::new_allocator<node>::construct<node, node>(node*, node&&) [60]
[55]     0.1    0.01    0.00 7457420         node&& std::forward<node>(std::remove_reference<node>::type&) [55]
-----------------------------------------------
                                                 <spontaneous>
[56]     0.1    0.01    0.00                 __gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > >::base() const [56]
-----------------------------------------------
                0.00    0.00       3/118143      __static_initialization_and_destruction_0(int, int) [63]
                0.00    0.00  118140/118143      AdjList::_get_table_cursor(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, __wt_cursor**, bool) [58]
[57]     0.1    0.00    0.00  118143         std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(char const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&) [57]
                0.00    0.00  118143/354381      std::char_traits<char>::length(char const*) [37]
-----------------------------------------------
                0.00    0.00       1/118140      AdjList::__restore_from_db(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >) [68]
                0.00    0.00       3/118140      AdjList::init_cursors() [69]
                0.00    0.00      18/118140      AdjList::get_random_node() [66]
                0.00    0.00  118118/118140      AdjList::get_node_cursor() [59]
[58]     0.1    0.00    0.00  118140         AdjList::_get_table_cursor(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, __wt_cursor**, bool) [58]
                0.00    0.00  118140/118143      std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(char const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&) [57]
-----------------------------------------------
                0.00    0.00      18/118118      AdjList::get_out_degree(int) [65]
                0.00    0.00  118100/118118      AdjList::get_out_nodes(int) [5]
[59]     0.1    0.00    0.00  118118         AdjList::get_node_cursor() [59]
                0.00    0.00  118118/118140      AdjList::_get_table_cursor(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, __wt_cursor**, bool) [58]
                0.00    0.00  118118/236260      void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char*>(char*, char*, std::forward_iterator_tag) [169]
-----------------------------------------------
                0.00    0.00 2897160/2897160     void std::allocator_traits<std::allocator<node> >::construct<node, node>(std::allocator<node>&, node*, node&&) [28]
[60]     0.0    0.00    0.00 2897160         void __gnu_cxx::new_allocator<node>::construct<node, node>(node*, node&&) [60]
                0.00    0.00 2897160/7457420     node&& std::forward<node>(std::remove_reference<node>::type&) [55]
                0.00    0.00 2897160/3796689     operator new(unsigned long, void*) [114]
-----------------------------------------------
                                                 <spontaneous>
[61]     0.0    0.00    0.00                 __libc_csu_init [61]
                0.00    0.00       1/1           _GLOBAL__sub_I__Z8METADATAB5cxx11 [62]
                0.00    0.00       1/1           _GLOBAL__sub_I__ZN13StandardGraphC2Ev [70]
                0.00    0.00       1/1           _GLOBAL__sub_I__ZN7EdgeKeyC2Ev [72]
                0.00    0.00       1/1           _GLOBAL__sub_I__ZN7AdjListC2Ev [71]
                0.00    0.00       1/1           _GLOBAL__sub_I__Z14print_csv_infoNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEiP8bfs_info [268]
-----------------------------------------------
                0.00    0.00       1/1           __libc_csu_init [61]
[62]     0.0    0.00    0.00       1         _GLOBAL__sub_I__Z8METADATAB5cxx11 [62]
                0.00    0.00       1/1           __static_initialization_and_destruction_0(int, int) [63]
-----------------------------------------------
                0.00    0.00       1/1           _GLOBAL__sub_I__Z8METADATAB5cxx11 [62]
[63]     0.0    0.00    0.00       1         __static_initialization_and_destruction_0(int, int) [63]
                0.00    0.00      30/236238      std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::basic_string<std::allocator<char> >(char const*, std::allocator<char> const&) [27]
                0.00    0.00       3/118143      std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(char const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&) [57]
                0.00    0.00       1/3           std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&) [265]
-----------------------------------------------
                                                 <spontaneous>
[64]     0.0    0.00    0.00                 int find_random_start<AdjList>(AdjList&) [64]
                0.00    0.00      18/18          AdjList::get_out_degree(int) [65]
                0.00    0.00      18/18          AdjList::get_random_node() [66]
-----------------------------------------------
                0.00    0.00      18/18          int find_random_start<AdjList>(AdjList&) [64]
[65]     0.0    0.00    0.00      18         AdjList::get_out_degree(int) [65]
                0.00    0.00      18/118118      AdjList::get_node_cursor() [59]
                0.00    0.00      18/1281346     AdjList::__record_to_node(__wt_cursor*, int) [33]
-----------------------------------------------
                0.00    0.00      18/18          int find_random_start<AdjList>(AdjList&) [64]
[66]     0.0    0.00    0.00      18         AdjList::get_random_node() [66]
                0.00    0.00      18/118140      AdjList::_get_table_cursor(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, __wt_cursor**, bool) [58]
                0.00    0.00      18/1281346     AdjList::__record_to_node(__wt_cursor*, int) [33]
                0.00    0.00      18/18          node::node() [230]
                0.00    0.00      18/236260      void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char*>(char*, char*, std::forward_iterator_tag) [169]
-----------------------------------------------
                                                 <spontaneous>
[67]     0.0    0.00    0.00                 AdjList::AdjList(graph_opts) [67]
                0.00    0.00       5/236238      std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::basic_string<std::allocator<char> >(char const*, std::allocator<char> const&) [27]
                0.00    0.00       1/1           AdjList::__restore_from_db(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >) [68]
                0.00    0.00       9/236260      void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char*>(char*, char*, std::forward_iterator_tag) [169]
                0.00    0.00       4/5           std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::allocator() [243]
                0.00    0.00       4/4           std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::vector(std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [258]
                0.00    0.00       4/9           std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::~allocator() [233]
                0.00    0.00       2/2           std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&, char const*) [267]
                0.00    0.00       2/3           std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&) [265]
                0.00    0.00       1/1           graph_opts::graph_opts(graph_opts const&) [273]
                0.00    0.00       1/1           CommonUtil::check_graph_params(graph_opts) [272]
                0.00    0.00       1/1           graph_opts::~graph_opts() [274]
                0.00    0.00       1/1           std::filesystem::__cxx11::path::path(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&, std::filesystem::__cxx11::path::format) [284]
                0.00    0.00       1/1           std::filesystem::exists(std::filesystem::__cxx11::path const&) [282]
                0.00    0.00       1/1           std::filesystem::__cxx11::path::~path() [285]
-----------------------------------------------
                0.00    0.00       1/1           AdjList::AdjList(graph_opts) [67]
[68]     0.0    0.00    0.00       1         AdjList::__restore_from_db(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >) [68]
                0.00    0.00       1/1           AdjList::init_cursors() [69]
                0.00    0.00       1/118140      AdjList::_get_table_cursor(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, __wt_cursor**, bool) [58]
                0.00    0.00       1/1           CommonUtil::open_connection(char*, __wt_connection**) [270]
                0.00    0.00       1/1           CommonUtil::open_session(__wt_connection*, __wt_session**) [269]
                0.00    0.00       1/236260      void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char*>(char*, char*, std::forward_iterator_tag) [169]
-----------------------------------------------
                0.00    0.00       1/1           AdjList::__restore_from_db(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >) [68]
[69]     0.0    0.00    0.00       1         AdjList::init_cursors() [69]
                0.00    0.00       3/118140      AdjList::_get_table_cursor(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, __wt_cursor**, bool) [58]
                0.00    0.00       3/236260      void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char*>(char*, char*, std::forward_iterator_tag) [169]
-----------------------------------------------
                0.00    0.00       1/1           __libc_csu_init [61]
[70]     0.0    0.00    0.00       1         _GLOBAL__sub_I__ZN13StandardGraphC2Ev [70]
                0.00    0.00       1/1           __static_initialization_and_destruction_0(int, int) [75]
-----------------------------------------------
                0.00    0.00       1/1           __libc_csu_init [61]
[71]     0.0    0.00    0.00       1         _GLOBAL__sub_I__ZN7AdjListC2Ev [71]
                0.00    0.00       1/1           __static_initialization_and_destruction_0(int, int) [73]
-----------------------------------------------
                0.00    0.00       1/1           __libc_csu_init [61]
[72]     0.0    0.00    0.00       1         _GLOBAL__sub_I__ZN7EdgeKeyC2Ev [72]
                0.00    0.00       1/1           __static_initialization_and_destruction_0(int, int) [74]
-----------------------------------------------
                0.00    0.00       1/1           _GLOBAL__sub_I__ZN7AdjListC2Ev [71]
[73]     0.0    0.00    0.00       1         __static_initialization_and_destruction_0(int, int) [73]
                0.00    0.00       1/236238      std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::basic_string<std::allocator<char> >(char const*, std::allocator<char> const&) [27]
-----------------------------------------------
                0.00    0.00       1/1           _GLOBAL__sub_I__ZN7EdgeKeyC2Ev [72]
[74]     0.0    0.00    0.00       1         __static_initialization_and_destruction_0(int, int) [74]
                0.00    0.00       1/236238      std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::basic_string<std::allocator<char> >(char const*, std::allocator<char> const&) [27]
-----------------------------------------------
                0.00    0.00       1/1           _GLOBAL__sub_I__ZN13StandardGraphC2Ev [70]
[75]     0.0    0.00    0.00       1         __static_initialization_and_destruction_0(int, int) [75]
                0.00    0.00       1/236238      std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::basic_string<std::allocator<char> >(char const*, std::allocator<char> const&) [27]
-----------------------------------------------
                0.00    0.00 1615850/4847550     void std::__relocate_object_a<node, node, std::allocator<node> >(node*, node*, std::allocator<node>&) [15]
                0.00    0.00 3231700/4847550     node* std::__relocate_a_1<node*, node*, std::allocator<node> >(node*, node*, node*, std::allocator<node>&) [19]
[113]    0.0    0.00    0.00 4847550         node* std::__addressof<node>(node&) [113]
-----------------------------------------------
                0.00    0.00       9/3796689     void std::_Construct<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&) [235]
                0.00    0.00  899520/3796689     void __gnu_cxx::new_allocator<int>::construct<int, int const&>(int*, int const&) [135]
                0.00    0.00 2897160/3796689     void __gnu_cxx::new_allocator<node>::construct<node, node>(node*, node&&) [60]
[114]    0.0    0.00    0.00 3796689         operator new(unsigned long, void*) [114]
-----------------------------------------------
                0.00    0.00  381790/3326200     std::vector<node, std::allocator<node> >::begin() [164]
                0.00    0.00 1281310/3326200     __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >::operator-(long) const [130]
                0.00    0.00 1663100/3326200     std::vector<node, std::allocator<node> >::end() [120]
[115]    0.0    0.00    0.00 3326200         __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >::__normal_iterator(node* const&) [115]
-----------------------------------------------
                0.00    0.00 1281310/2897160     std::vector<node, std::allocator<node> >::push_back(node&&) [12]
                0.00    0.00 1615850/2897160     void std::__relocate_object_a<node, node, std::allocator<node> >(node*, node*, std::allocator<node>&) [15]
[116]    0.0    0.00    0.00 2897160         std::remove_reference<node&>::type&& std::move<node&>(node&) [116]
-----------------------------------------------
                0.00    0.00 2798820/2798820     bool __gnu_cxx::operator!=<int*, std::vector<int, std::allocator<int> > >(__gnu_cxx::__normal_iterator<int*, std::vector<int, std::allocator<int> > > const&, __gnu_cxx::__normal_iterator<int*, std::vector<int, std::allocator<int> > > const&) [127]
[117]    0.0    0.00    0.00 2798820         __gnu_cxx::__normal_iterator<int*, std::vector<int, std::allocator<int> > >::base() const [117]
-----------------------------------------------
                0.00    0.00 2290740/2290740     node* std::__relocate_a<node*, node*, std::allocator<node> >(node*, node*, node*, std::allocator<node>&) [18]
[118]    0.0    0.00    0.00 2290740         node* std::__niter_base<node*>(node*) [118]
-----------------------------------------------
                0.00    0.00  899520/1799040     void std::allocator_traits<std::allocator<int> >::construct<int, int const&>(std::allocator<int>&, int*, int const&) [23]
                0.00    0.00  899520/1799040     void __gnu_cxx::new_allocator<int>::construct<int, int const&>(int*, int const&) [135]
[119]    0.0    0.00    0.00 1799040         int const& std::forward<int const&>(std::remove_reference<int const&>::type&) [119]
-----------------------------------------------
                0.00    0.00  381790/1663100     node& std::vector<node, std::allocator<node> >::emplace_back<node>(node&&) [11]
                0.00    0.00 1281310/1663100     std::vector<node, std::allocator<node> >::back() [132]
[120]    0.0    0.00    0.00 1663100         std::vector<node, std::allocator<node> >::end() [120]
                0.00    0.00 1663100/3326200     __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >::__normal_iterator(node* const&) [115]
-----------------------------------------------
                0.00    0.00  763580/1527160     __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >::difference_type __gnu_cxx::operator-<node*, std::vector<node, std::allocator<node> > >(__gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > > const&, __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > > const&) [159]
                0.00    0.00  763580/1527160     void std::vector<node, std::allocator<node> >::_M_realloc_insert<node>(__gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >, node&&) [13]
[121]    0.0    0.00    0.00 1527160         __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >::base() const [121]
-----------------------------------------------
                0.00    0.00 1527160/1527160     std::vector<node, std::allocator<node> >::_M_check_len(unsigned long, char const*) const [160]
[122]    0.0    0.00    0.00 1527160         std::vector<node, std::allocator<node> >::size() const [122]
-----------------------------------------------
                0.00    0.00 1517548/1517548     void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char const*>(char const*, char const*, std::forward_iterator_tag) [39]
[123]    0.0    0.00    0.00 1517548         bool __gnu_cxx::__is_null_pointer<char const>(char const*) [123]
-----------------------------------------------
                0.00    0.00 1517548/1517548     std::iterator_traits<char const*>::difference_type std::distance<char const*>(char const*, char const*) [126]
[124]    0.0    0.00    0.00 1517548         std::iterator_traits<char const*>::difference_type std::__distance<char const*>(char const*, char const*, std::random_access_iterator_tag) [124]
-----------------------------------------------
                0.00    0.00 1517548/1517548     std::iterator_traits<char const*>::difference_type std::distance<char const*>(char const*, char const*) [126]
[125]    0.0    0.00    0.00 1517548         std::iterator_traits<char const*>::iterator_category std::__iterator_category<char const*>(char const* const&) [125]
-----------------------------------------------
                0.00    0.00 1517548/1517548     void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char const*>(char const*, char const*, std::forward_iterator_tag) [39]
[126]    0.0    0.00    0.00 1517548         std::iterator_traits<char const*>::difference_type std::distance<char const*>(char const*, char const*) [126]
                0.00    0.00 1517548/1517548     std::iterator_traits<char const*>::iterator_category std::__iterator_category<char const*>(char const* const&) [125]
                0.00    0.00 1517548/1517548     std::iterator_traits<char const*>::difference_type std::__distance<char const*>(char const*, char const*, std::random_access_iterator_tag) [124]
-----------------------------------------------
                0.00    0.00 1399410/1399410     AdjList::get_out_nodes(int) [5]
[127]    0.0    0.00    0.00 1399410         bool __gnu_cxx::operator!=<int*, std::vector<int, std::allocator<int> > >(__gnu_cxx::__normal_iterator<int*, std::vector<int, std::allocator<int> > > const&, __gnu_cxx::__normal_iterator<int*, std::vector<int, std::allocator<int> > > const&) [127]
                0.00    0.00 2798820/2798820     __gnu_cxx::__normal_iterator<int*, std::vector<int, std::allocator<int> > >::base() const [117]
-----------------------------------------------
                0.00    0.00 1281310/1281310     AdjList::get_out_nodes(int) [5]
[128]    0.0    0.00    0.00 1281310         __gnu_cxx::__normal_iterator<int*, std::vector<int, std::allocator<int> > >::operator++() [128]
-----------------------------------------------
                0.00    0.00 1281310/1281310     std::vector<node, std::allocator<node> >::back() [132]
[129]    0.0    0.00    0.00 1281310         __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >::operator*() const [129]
-----------------------------------------------
                0.00    0.00 1281310/1281310     std::vector<node, std::allocator<node> >::back() [132]
[130]    0.0    0.00    0.00 1281310         __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >::operator-(long) const [130]
                0.00    0.00 1281310/3326200     __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >::__normal_iterator(node* const&) [115]
-----------------------------------------------
                0.00    0.00 1281310/1281310     AdjList::get_out_nodes(int) [5]
[131]    0.0    0.00    0.00 1281310         __gnu_cxx::__normal_iterator<int*, std::vector<int, std::allocator<int> > >::operator*() const [131]
-----------------------------------------------
                0.00    0.00 1281310/1281310     node& std::vector<node, std::allocator<node> >::emplace_back<node>(node&&) [11]
[132]    0.0    0.00    0.00 1281310         std::vector<node, std::allocator<node> >::back() [132]
                0.00    0.00 1281310/1663100     std::vector<node, std::allocator<node> >::end() [120]
                0.00    0.00 1281310/1281310     __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >::operator-(long) const [130]
                0.00    0.00 1281310/1281310     __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >::operator*() const [129]
-----------------------------------------------
                0.00    0.00 1281310/1281310     int __gnu_cxx::__stoa<long, int, char, int>(long (*)(char const*, char**, int), char const*, char const*, unsigned long*, int) [20]
[133]    0.0    0.00    0.00 1281310         __gnu_cxx::__stoa<long, int, char, int>(long (*)(char const*, char**, int), char const*, char const*, unsigned long*, int)::_Range_chk::_S_chk(long, std::integral_constant<bool, true>) [133]
-----------------------------------------------
                0.00    0.00  381790/1145370     __gnu_cxx::new_allocator<node>::allocate(unsigned long, void const*) [158]
                0.00    0.00  763580/1145370     std::allocator_traits<std::allocator<node> >::max_size(std::allocator<node> const&) [141]
[134]    0.0    0.00    0.00 1145370         __gnu_cxx::new_allocator<node>::max_size() const [134]
-----------------------------------------------
                0.00    0.00  899520/899520      void std::allocator_traits<std::allocator<int> >::construct<int, int const&>(std::allocator<int>&, int*, int const&) [23]
[135]    0.0    0.00    0.00  899520         void __gnu_cxx::new_allocator<int>::construct<int, int const&>(int*, int const&) [135]
                0.00    0.00  899520/1799040     int const& std::forward<int const&>(std::remove_reference<int const&>::type&) [119]
                0.00    0.00  899520/3796689     operator new(unsigned long, void*) [114]
-----------------------------------------------
                0.00    0.00  118100/826700      std::vector<int, std::allocator<int> >::vector(std::vector<int, std::allocator<int> > const&) [211]
                0.00    0.00  236200/826700      std::vector<int, std::allocator<int> >::_M_move_assign(std::vector<int, std::allocator<int> >&&, std::integral_constant<bool, true>) [208]
                0.00    0.00  472400/826700      std::vector<int, std::allocator<int> >::~vector() [155]
[136]    0.0    0.00    0.00  826700         std::_Vector_base<int, std::allocator<int> >::_M_get_Tp_allocator() [136]
-----------------------------------------------
                0.00    0.00       4/763584      std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_S_max_size(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [255]
                0.00    0.00  763580/763584      std::vector<node, std::allocator<node> >::_S_max_size(std::allocator<node> const&) [142]
[137]    0.0    0.00    0.00  763584         unsigned long const& std::min<unsigned long>(unsigned long const&, unsigned long const&) [137]
-----------------------------------------------
                0.00    0.00  763580/763580      std::vector<node, std::allocator<node> >::max_size() const [139]
[138]    0.0    0.00    0.00  763580         std::_Vector_base<node, std::allocator<node> >::_M_get_Tp_allocator() const [138]
-----------------------------------------------
                0.00    0.00  763580/763580      std::vector<node, std::allocator<node> >::_M_check_len(unsigned long, char const*) const [160]
[139]    0.0    0.00    0.00  763580         std::vector<node, std::allocator<node> >::max_size() const [139]
                0.00    0.00  763580/763580      std::vector<node, std::allocator<node> >::_S_max_size(std::allocator<node> const&) [142]
                0.00    0.00  763580/763580      std::_Vector_base<node, std::allocator<node> >::_M_get_Tp_allocator() const [138]
-----------------------------------------------
                0.00    0.00  763580/763580      void std::vector<node, std::allocator<node> >::_M_realloc_insert<node>(__gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >, node&&) [13]
[140]    0.0    0.00    0.00  763580         std::_Vector_base<node, std::allocator<node> >::_M_get_Tp_allocator() [140]
-----------------------------------------------
                0.00    0.00  763580/763580      std::vector<node, std::allocator<node> >::_S_max_size(std::allocator<node> const&) [142]
[141]    0.0    0.00    0.00  763580         std::allocator_traits<std::allocator<node> >::max_size(std::allocator<node> const&) [141]
                0.00    0.00  763580/1145370     __gnu_cxx::new_allocator<node>::max_size() const [134]
-----------------------------------------------
                0.00    0.00  763580/763580      std::vector<node, std::allocator<node> >::max_size() const [139]
[142]    0.0    0.00    0.00  763580         std::vector<node, std::allocator<node> >::_S_max_size(std::allocator<node> const&) [142]
                0.00    0.00  763580/763580      std::allocator_traits<std::allocator<node> >::max_size(std::allocator<node> const&) [141]
                0.00    0.00  763580/763584      unsigned long const& std::min<unsigned long>(unsigned long const&, unsigned long const&) [137]
-----------------------------------------------
                0.00    0.00  708600/708600      std::allocator<int>::~allocator() [144]
[143]    0.0    0.00    0.00  708600         __gnu_cxx::new_allocator<int>::~new_allocator() [143]
-----------------------------------------------
                0.00    0.00  118100/708600      std::vector<int, std::allocator<int> >::vector(std::vector<int, std::allocator<int> > const&) [211]
                0.00    0.00  118100/708600      std::vector<int, std::allocator<int> >::_M_move_assign(std::vector<int, std::allocator<int> >&&, std::integral_constant<bool, true>) [208]
                0.00    0.00  472400/708600      std::_Vector_base<int, std::allocator<int> >::_Vector_impl::~_Vector_impl() [152]
[144]    0.0    0.00    0.00  708600         std::allocator<int>::~allocator() [144]
                0.00    0.00  708600/708600      __gnu_cxx::new_allocator<int>::~new_allocator() [143]
-----------------------------------------------
                0.00    0.00  708600/708600      std::_Vector_base<int, std::allocator<int> >::_Vector_impl_data::_M_swap_data(std::_Vector_base<int, std::allocator<int> >::_Vector_impl_data&) [181]
[145]    0.0    0.00    0.00  708600         std::_Vector_base<int, std::allocator<int> >::_Vector_impl_data::_M_copy_data(std::_Vector_base<int, std::allocator<int> >::_Vector_impl_data const&) [145]
-----------------------------------------------
                0.00    0.00  236200/708600      std::_Vector_base<int, std::allocator<int> >::_Vector_impl::_Vector_impl() [180]
                0.00    0.00  236200/708600      std::_Vector_base<int, std::allocator<int> >::_Vector_impl_data::_M_swap_data(std::_Vector_base<int, std::allocator<int> >::_Vector_impl_data&) [181]
                0.00    0.00  236200/708600      std::_Vector_base<int, std::allocator<int> >::_Vector_impl::_Vector_impl(std::allocator<int> const&) [179]
[146]    0.0    0.00    0.00  708600         std::_Vector_base<int, std::allocator<int> >::_Vector_impl_data::_Vector_impl_data() [146]
-----------------------------------------------
                0.00    0.00  118100/617990      std::vector<int, std::allocator<int> >::begin() [209]
                0.00    0.00  499890/617990      std::vector<int, std::allocator<int> >::end() [148]
[147]    0.0    0.00    0.00  617990         __gnu_cxx::__normal_iterator<int*, std::vector<int, std::allocator<int> > >::__normal_iterator(int* const&) [147]
-----------------------------------------------
                0.00    0.00  118100/499890      AdjList::get_out_nodes(int) [5]
                0.00    0.00  381790/499890      std::vector<int, std::allocator<int> >::push_back(int const&) [14]
[148]    0.0    0.00    0.00  499890         std::vector<int, std::allocator<int> >::end() [148]
                0.00    0.00  499890/617990      __gnu_cxx::__normal_iterator<int*, std::vector<int, std::allocator<int> > >::__normal_iterator(int* const&) [147]
-----------------------------------------------
                0.00    0.00  472400/472400      std::allocator<int>::allocator(std::allocator<int> const&) [150]
[149]    0.0    0.00    0.00  472400         __gnu_cxx::new_allocator<int>::new_allocator(__gnu_cxx::new_allocator<int> const&) [149]
-----------------------------------------------
                0.00    0.00  118100/472400      std::_Vector_base<int, std::allocator<int> >::get_allocator() const [193]
                0.00    0.00  118100/472400      std::allocator_traits<std::allocator<int> >::select_on_container_copy_construction(std::allocator<int> const&) [205]
                0.00    0.00  236200/472400      std::_Vector_base<int, std::allocator<int> >::_Vector_impl::_Vector_impl(std::allocator<int> const&) [179]
[150]    0.0    0.00    0.00  472400         std::allocator<int>::allocator(std::allocator<int> const&) [150]
                0.00    0.00  472400/472400      __gnu_cxx::new_allocator<int>::new_allocator(__gnu_cxx::new_allocator<int> const&) [149]
-----------------------------------------------
                0.00    0.00  472400/472400      void std::_Destroy<int*>(int*, int*) [156]
[151]    0.0    0.00    0.00  472400         void std::_Destroy_aux<true>::__destroy<int*>(int*, int*) [151]
-----------------------------------------------
                0.00    0.00  472400/472400      std::_Vector_base<int, std::allocator<int> >::~_Vector_base() [154]
[152]    0.0    0.00    0.00  472400         std::_Vector_base<int, std::allocator<int> >::_Vector_impl::~_Vector_impl() [152]
                0.00    0.00  472400/708600      std::allocator<int>::~allocator() [144]
-----------------------------------------------
                0.00    0.00  472400/472400      std::_Vector_base<int, std::allocator<int> >::~_Vector_base() [154]
[153]    0.0    0.00    0.00  472400         std::_Vector_base<int, std::allocator<int> >::_M_deallocate(int*, unsigned long) [153]
                0.00    0.00  205480/205480      std::allocator_traits<std::allocator<int> >::deallocate(std::allocator<int>&, int*, unsigned long) [187]
-----------------------------------------------
                0.00    0.00  472400/472400      std::vector<int, std::allocator<int> >::~vector() [155]
[154]    0.0    0.00    0.00  472400         std::_Vector_base<int, std::allocator<int> >::~_Vector_base() [154]
                0.00    0.00  472400/472400      std::_Vector_base<int, std::allocator<int> >::_M_deallocate(int*, unsigned long) [153]
                0.00    0.00  472400/472400      std::_Vector_base<int, std::allocator<int> >::_Vector_impl::~_Vector_impl() [152]
-----------------------------------------------
                0.00    0.00  118100/472400      AdjList::__record_to_adjlist(__wt_cursor*, adjlist*) [8]
                0.00    0.00  118100/472400      AdjList::get_out_nodes(int) [5]
                0.00    0.00  118100/472400      adjlist::~adjlist() [190]
                0.00    0.00  118100/472400      std::vector<int, std::allocator<int> >::_M_move_assign(std::vector<int, std::allocator<int> >&&, std::integral_constant<bool, true>) [208]
[155]    0.0    0.00    0.00  472400         std::vector<int, std::allocator<int> >::~vector() [155]
                0.00    0.00  472400/826700      std::_Vector_base<int, std::allocator<int> >::_M_get_Tp_allocator() [136]
                0.00    0.00  472400/472400      void std::_Destroy<int*, int>(int*, int*, std::allocator<int>&) [157]
                0.00    0.00  472400/472400      std::_Vector_base<int, std::allocator<int> >::~_Vector_base() [154]
-----------------------------------------------
                0.00    0.00  472400/472400      void std::_Destroy<int*, int>(int*, int*, std::allocator<int>&) [157]
[156]    0.0    0.00    0.00  472400         void std::_Destroy<int*>(int*, int*) [156]
                0.00    0.00  472400/472400      void std::_Destroy_aux<true>::__destroy<int*>(int*, int*) [151]
-----------------------------------------------
                0.00    0.00  472400/472400      std::vector<int, std::allocator<int> >::~vector() [155]
[157]    0.0    0.00    0.00  472400         void std::_Destroy<int*, int>(int*, int*, std::allocator<int>&) [157]
                0.00    0.00  472400/472400      void std::_Destroy<int*>(int*, int*) [156]
-----------------------------------------------
                0.00    0.00  381790/381790      std::allocator_traits<std::allocator<node> >::allocate(std::allocator<node>&, unsigned long) [163]
[158]    0.0    0.00    0.00  381790         __gnu_cxx::new_allocator<node>::allocate(unsigned long, void const*) [158]
                0.00    0.00  381790/1145370     __gnu_cxx::new_allocator<node>::max_size() const [134]
-----------------------------------------------
                0.00    0.00  381790/381790      void std::vector<node, std::allocator<node> >::_M_realloc_insert<node>(__gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >, node&&) [13]
[159]    0.0    0.00    0.00  381790         __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >::difference_type __gnu_cxx::operator-<node*, std::vector<node, std::allocator<node> > >(__gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > > const&, __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > > const&) [159]
                0.00    0.00  763580/1527160     __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >::base() const [121]
-----------------------------------------------
                0.00    0.00  381790/381790      void std::vector<node, std::allocator<node> >::_M_realloc_insert<node>(__gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >, node&&) [13]
[160]    0.0    0.00    0.00  381790         std::vector<node, std::allocator<node> >::_M_check_len(unsigned long, char const*) const [160]
                0.00    0.00 1527160/1527160     std::vector<node, std::allocator<node> >::size() const [122]
                0.00    0.00  763580/763580      std::vector<node, std::allocator<node> >::max_size() const [139]
                0.00    0.00  381790/381790      unsigned long const& std::max<unsigned long>(unsigned long const&, unsigned long const&) [165]
-----------------------------------------------
                0.00    0.00  381790/381790      void std::vector<node, std::allocator<node> >::_M_realloc_insert<node>(__gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >, node&&) [13]
[161]    0.0    0.00    0.00  381790         std::_Vector_base<node, std::allocator<node> >::_M_allocate(unsigned long) [161]
                0.00    0.00  381790/381790      std::allocator_traits<std::allocator<node> >::allocate(std::allocator<node>&, unsigned long) [163]
-----------------------------------------------
                0.00    0.00  381790/381790      void std::vector<node, std::allocator<node> >::_M_realloc_insert<node>(__gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >, node&&) [13]
[162]    0.0    0.00    0.00  381790         std::_Vector_base<node, std::allocator<node> >::_M_deallocate(node*, unsigned long) [162]
                0.00    0.00  279050/279050      std::allocator_traits<std::allocator<node> >::deallocate(std::allocator<node>&, node*, unsigned long) [167]
-----------------------------------------------
                0.00    0.00  381790/381790      std::_Vector_base<node, std::allocator<node> >::_M_allocate(unsigned long) [161]
[163]    0.0    0.00    0.00  381790         std::allocator_traits<std::allocator<node> >::allocate(std::allocator<node>&, unsigned long) [163]
                0.00    0.00  381790/381790      __gnu_cxx::new_allocator<node>::allocate(unsigned long, void const*) [158]
-----------------------------------------------
                0.00    0.00  381790/381790      void std::vector<node, std::allocator<node> >::_M_realloc_insert<node>(__gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >, node&&) [13]
[164]    0.0    0.00    0.00  381790         std::vector<node, std::allocator<node> >::begin() [164]
                0.00    0.00  381790/3326200     __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >::__normal_iterator(node* const&) [115]
-----------------------------------------------
                0.00    0.00  381790/381790      std::vector<node, std::allocator<node> >::_M_check_len(unsigned long, char const*) const [160]
[165]    0.0    0.00    0.00  381790         unsigned long const& std::max<unsigned long>(unsigned long const&, unsigned long const&) [165]
-----------------------------------------------
                0.00    0.00  279050/279050      std::allocator_traits<std::allocator<node> >::deallocate(std::allocator<node>&, node*, unsigned long) [167]
[166]    0.0    0.00    0.00  279050         __gnu_cxx::new_allocator<node>::deallocate(node*, unsigned long) [166]
-----------------------------------------------
                0.00    0.00  279050/279050      std::_Vector_base<node, std::allocator<node> >::_M_deallocate(node*, unsigned long) [162]
[167]    0.0    0.00    0.00  279050         std::allocator_traits<std::allocator<node> >::deallocate(std::allocator<node>&, node*, unsigned long) [167]
                0.00    0.00  279050/279050      __gnu_cxx::new_allocator<node>::deallocate(node*, unsigned long) [166]
-----------------------------------------------
                0.00    0.00  236260/236260      void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char*>(char*, char*, std::forward_iterator_tag) [169]
[168]    0.0    0.00    0.00  236260         bool __gnu_cxx::__is_null_pointer<char>(char*) [168]
-----------------------------------------------
                0.00    0.00       1/236260      AdjList::__restore_from_db(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >) [68]
                0.00    0.00       2/236260      std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&, char const*) [267]
                0.00    0.00       3/236260      AdjList::init_cursors() [69]
                0.00    0.00       9/236260      AdjList::AdjList(graph_opts) [67]
                0.00    0.00       9/236260      void std::_Construct<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&) [235]
                0.00    0.00      18/236260      AdjList::get_random_node() [66]
                0.00    0.00  118100/236260      AdjList::__record_to_adjlist(__wt_cursor*, adjlist*) [8]
                0.00    0.00  118118/236260      AdjList::get_node_cursor() [59]
[169]    0.0    0.00    0.00  236260         void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char*>(char*, char*, std::forward_iterator_tag) [169]
                0.00    0.00  236260/236260      bool __gnu_cxx::__is_null_pointer<char>(char*) [168]
                0.00    0.00  236260/236260      std::iterator_traits<char*>::difference_type std::distance<char*>(char*, char*) [172]
-----------------------------------------------
                0.00    0.00  236260/236260      std::iterator_traits<char*>::difference_type std::distance<char*>(char*, char*) [172]
[170]    0.0    0.00    0.00  236260         std::iterator_traits<char*>::difference_type std::__distance<char*>(char*, char*, std::random_access_iterator_tag) [170]
-----------------------------------------------
                0.00    0.00  236260/236260      std::iterator_traits<char*>::difference_type std::distance<char*>(char*, char*) [172]
[171]    0.0    0.00    0.00  236260         std::iterator_traits<char*>::iterator_category std::__iterator_category<char*>(char* const&) [171]
-----------------------------------------------
                0.00    0.00  236260/236260      void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char*>(char*, char*, std::forward_iterator_tag) [169]
[172]    0.0    0.00    0.00  236260         std::iterator_traits<char*>::difference_type std::distance<char*>(char*, char*) [172]
                0.00    0.00  236260/236260      std::iterator_traits<char*>::iterator_category std::__iterator_category<char*>(char* const&) [171]
                0.00    0.00  236260/236260      std::iterator_traits<char*>::difference_type std::__distance<char*>(char*, char*, std::random_access_iterator_tag) [170]
-----------------------------------------------
                0.00    0.00  236200/236200      std::allocator<int>::allocator() [178]
[173]    0.0    0.00    0.00  236200         __gnu_cxx::new_allocator<int>::new_allocator() [173]
-----------------------------------------------
                0.00    0.00  118100/236200      std::vector<int, std::allocator<int> >::begin() const [195]
                0.00    0.00  118100/236200      std::vector<int, std::allocator<int> >::end() const [194]
[174]    0.0    0.00    0.00  236200         __gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >::__normal_iterator(int const* const&) [174]
-----------------------------------------------
                0.00    0.00  236200/236200      int const* std::__niter_base<int const*, std::vector<int, std::allocator<int> > >(__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >) [185]
[175]    0.0    0.00    0.00  236200         __gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >::base() const [175]
-----------------------------------------------
                0.00    0.00  118100/236200      std::vector<int, std::allocator<int> >::vector(std::vector<int, std::allocator<int> > const&) [211]
                0.00    0.00  118100/236200      std::_Vector_base<int, std::allocator<int> >::get_allocator() const [193]
[176]    0.0    0.00    0.00  236200         std::_Vector_base<int, std::allocator<int> >::_M_get_Tp_allocator() const [176]
-----------------------------------------------
                0.00    0.00  118100/236200      AdjList::__record_to_adjlist(__wt_cursor*, adjlist*) [8]
                0.00    0.00  118100/236200      std::vector<int, std::allocator<int> >::vector(std::vector<int, std::allocator<int> > const&) [211]
[177]    0.0    0.00    0.00  236200         std::vector<int, std::allocator<int> >::size() const [177]
-----------------------------------------------
                0.00    0.00  236200/236200      std::_Vector_base<int, std::allocator<int> >::_Vector_impl::_Vector_impl() [180]
[178]    0.0    0.00    0.00  236200         std::allocator<int>::allocator() [178]
                0.00    0.00  236200/236200      __gnu_cxx::new_allocator<int>::new_allocator() [173]
-----------------------------------------------
                0.00    0.00  118100/236200      std::_Vector_base<int, std::allocator<int> >::_Vector_base(unsigned long, std::allocator<int> const&) [204]
                0.00    0.00  118100/236200      std::_Vector_base<int, std::allocator<int> >::_Vector_base(std::allocator<int> const&) [203]
[179]    0.0    0.00    0.00  236200         std::_Vector_base<int, std::allocator<int> >::_Vector_impl::_Vector_impl(std::allocator<int> const&) [179]
                0.00    0.00  236200/472400      std::allocator<int>::allocator(std::allocator<int> const&) [150]
                0.00    0.00  236200/708600      std::_Vector_base<int, std::allocator<int> >::_Vector_impl_data::_Vector_impl_data() [146]
-----------------------------------------------
                0.00    0.00  236200/236200      std::_Vector_base<int, std::allocator<int> >::_Vector_base() [182]
[180]    0.0    0.00    0.00  236200         std::_Vector_base<int, std::allocator<int> >::_Vector_impl::_Vector_impl() [180]
                0.00    0.00  236200/236200      std::allocator<int>::allocator() [178]
                0.00    0.00  236200/708600      std::_Vector_base<int, std::allocator<int> >::_Vector_impl_data::_Vector_impl_data() [146]
-----------------------------------------------
                0.00    0.00  236200/236200      std::vector<int, std::allocator<int> >::_M_move_assign(std::vector<int, std::allocator<int> >&&, std::integral_constant<bool, true>) [208]
[181]    0.0    0.00    0.00  236200         std::_Vector_base<int, std::allocator<int> >::_Vector_impl_data::_M_swap_data(std::_Vector_base<int, std::allocator<int> >::_Vector_impl_data&) [181]
                0.00    0.00  708600/708600      std::_Vector_base<int, std::allocator<int> >::_Vector_impl_data::_M_copy_data(std::_Vector_base<int, std::allocator<int> >::_Vector_impl_data const&) [145]
                0.00    0.00  236200/708600      std::_Vector_base<int, std::allocator<int> >::_Vector_impl_data::_Vector_impl_data() [146]
-----------------------------------------------
                0.00    0.00  236200/236200      std::vector<int, std::allocator<int> >::vector() [183]
[182]    0.0    0.00    0.00  236200         std::_Vector_base<int, std::allocator<int> >::_Vector_base() [182]
                0.00    0.00  236200/236200      std::_Vector_base<int, std::allocator<int> >::_Vector_impl::_Vector_impl() [180]
-----------------------------------------------
                0.00    0.00  118100/236200      adjlist::adjlist() [189]
                0.00    0.00  118100/236200      CommonUtil::unpack_int_vector_std(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >) [10]
[183]    0.0    0.00    0.00  236200         std::vector<int, std::allocator<int> >::vector() [183]
                0.00    0.00  236200/236200      std::_Vector_base<int, std::allocator<int> >::_Vector_base() [182]
-----------------------------------------------
                0.00    0.00  236200/236200      int* std::copy<__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, int*>(__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, __gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, int*) [221]
[184]    0.0    0.00    0.00  236200         __gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > > std::__miter_base<__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > > >(__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >) [184]
-----------------------------------------------
                0.00    0.00  236200/236200      int* std::__copy_move_a2<false, __gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, int*>(__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, __gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, int*) [216]
[185]    0.0    0.00    0.00  236200         int const* std::__niter_base<int const*, std::vector<int, std::allocator<int> > >(__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >) [185]
                0.00    0.00  236200/236200      __gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >::base() const [175]
-----------------------------------------------
                0.00    0.00  205480/205480      std::allocator_traits<std::allocator<int> >::deallocate(std::allocator<int>&, int*, unsigned long) [187]
[186]    0.0    0.00    0.00  205480         __gnu_cxx::new_allocator<int>::deallocate(int*, unsigned long) [186]
-----------------------------------------------
                0.00    0.00  205480/205480      std::_Vector_base<int, std::allocator<int> >::_M_deallocate(int*, unsigned long) [153]
[187]    0.0    0.00    0.00  205480         std::allocator_traits<std::allocator<int> >::deallocate(std::allocator<int>&, int*, unsigned long) [187]
                0.00    0.00  205480/205480      __gnu_cxx::new_allocator<int>::deallocate(int*, unsigned long) [186]
-----------------------------------------------
                0.00    0.00  118100/118100      AdjList::get_out_nodes(int) [5]
[188]    0.0    0.00    0.00  118100         AdjList::get_out_adjlist_cursor() [188]
-----------------------------------------------
                0.00    0.00  118100/118100      AdjList::get_adjlist(__wt_cursor*, int) [7]
[189]    0.0    0.00    0.00  118100         adjlist::adjlist() [189]
                0.00    0.00  118100/236200      std::vector<int, std::allocator<int> >::vector() [183]
-----------------------------------------------
                0.00    0.00  118100/118100      AdjList::get_adjlist(__wt_cursor*, int) [7]
[190]    0.0    0.00    0.00  118100         adjlist::~adjlist() [190]
                0.00    0.00  118100/472400      std::vector<int, std::allocator<int> >::~vector() [155]
-----------------------------------------------
                0.00    0.00  118100/118100      std::allocator<node>::allocator() [196]
[191]    0.0    0.00    0.00  118100         __gnu_cxx::new_allocator<node>::new_allocator() [191]
-----------------------------------------------
                0.00    0.00  118100/118100      std::vector<int, std::allocator<int> >::vector(std::vector<int, std::allocator<int> > const&) [211]
[192]    0.0    0.00    0.00  118100         __gnu_cxx::__alloc_traits<std::allocator<int>, int>::_S_select_on_copy(std::allocator<int> const&) [192]
                0.00    0.00  118100/118100      std::allocator_traits<std::allocator<int> >::select_on_container_copy_construction(std::allocator<int> const&) [205]
-----------------------------------------------
                0.00    0.00  118100/118100      std::vector<int, std::allocator<int> >::_M_move_assign(std::vector<int, std::allocator<int> >&&, std::integral_constant<bool, true>) [208]
[193]    0.0    0.00    0.00  118100         std::_Vector_base<int, std::allocator<int> >::get_allocator() const [193]
                0.00    0.00  118100/236200      std::_Vector_base<int, std::allocator<int> >::_M_get_Tp_allocator() const [176]
                0.00    0.00  118100/472400      std::allocator<int>::allocator(std::allocator<int> const&) [150]
-----------------------------------------------
                0.00    0.00  118100/118100      std::vector<int, std::allocator<int> >::vector(std::vector<int, std::allocator<int> > const&) [211]
[194]    0.0    0.00    0.00  118100         std::vector<int, std::allocator<int> >::end() const [194]
                0.00    0.00  118100/236200      __gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >::__normal_iterator(int const* const&) [174]
-----------------------------------------------
                0.00    0.00  118100/118100      std::vector<int, std::allocator<int> >::vector(std::vector<int, std::allocator<int> > const&) [211]
[195]    0.0    0.00    0.00  118100         std::vector<int, std::allocator<int> >::begin() const [195]
                0.00    0.00  118100/236200      __gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >::__normal_iterator(int const* const&) [174]
-----------------------------------------------
                0.00    0.00  118100/118100      std::_Vector_base<node, std::allocator<node> >::_Vector_impl::_Vector_impl() [198]
[196]    0.0    0.00    0.00  118100         std::allocator<node>::allocator() [196]
                0.00    0.00  118100/118100      __gnu_cxx::new_allocator<node>::new_allocator() [191]
-----------------------------------------------
                0.00    0.00  118100/118100      int* std::__copy_move_a<false, int const*, int*>(int const*, int const*, int*) [215]
[197]    0.0    0.00    0.00  118100         int* std::__copy_move<false, true, std::random_access_iterator_tag>::__copy_m<int>(int const*, int const*, int*) [197]
-----------------------------------------------
                0.00    0.00  118100/118100      std::_Vector_base<node, std::allocator<node> >::_Vector_base() [200]
[198]    0.0    0.00    0.00  118100         std::_Vector_base<node, std::allocator<node> >::_Vector_impl::_Vector_impl() [198]
                0.00    0.00  118100/118100      std::allocator<node>::allocator() [196]
                0.00    0.00  118100/118100      std::_Vector_base<node, std::allocator<node> >::_Vector_impl_data::_Vector_impl_data() [199]
-----------------------------------------------
                0.00    0.00  118100/118100      std::_Vector_base<node, std::allocator<node> >::_Vector_impl::_Vector_impl() [198]
[199]    0.0    0.00    0.00  118100         std::_Vector_base<node, std::allocator<node> >::_Vector_impl_data::_Vector_impl_data() [199]
-----------------------------------------------
                0.00    0.00  118100/118100      std::vector<node, std::allocator<node> >::vector() [207]
[200]    0.0    0.00    0.00  118100         std::_Vector_base<node, std::allocator<node> >::_Vector_base() [200]
                0.00    0.00  118100/118100      std::_Vector_base<node, std::allocator<node> >::_Vector_impl::_Vector_impl() [198]
-----------------------------------------------
                0.00    0.00  118100/118100      std::_Vector_base<int, std::allocator<int> >::_M_create_storage(unsigned long) [202]
[201]    0.0    0.00    0.00  118100         std::_Vector_base<int, std::allocator<int> >::_M_allocate(unsigned long) [201]
                0.00    0.00  102740/102740      std::allocator_traits<std::allocator<int> >::allocate(std::allocator<int>&, unsigned long) [226]
-----------------------------------------------
                0.00    0.00  118100/118100      std::_Vector_base<int, std::allocator<int> >::_Vector_base(unsigned long, std::allocator<int> const&) [204]
[202]    0.0    0.00    0.00  118100         std::_Vector_base<int, std::allocator<int> >::_M_create_storage(unsigned long) [202]
                0.00    0.00  118100/118100      std::_Vector_base<int, std::allocator<int> >::_M_allocate(unsigned long) [201]
-----------------------------------------------
                0.00    0.00  118100/118100      std::vector<int, std::allocator<int> >::vector(std::allocator<int> const&) [210]
[203]    0.0    0.00    0.00  118100         std::_Vector_base<int, std::allocator<int> >::_Vector_base(std::allocator<int> const&) [203]
                0.00    0.00  118100/236200      std::_Vector_base<int, std::allocator<int> >::_Vector_impl::_Vector_impl(std::allocator<int> const&) [179]
-----------------------------------------------
                0.00    0.00  118100/118100      std::vector<int, std::allocator<int> >::vector(std::vector<int, std::allocator<int> > const&) [211]
[204]    0.0    0.00    0.00  118100         std::_Vector_base<int, std::allocator<int> >::_Vector_base(unsigned long, std::allocator<int> const&) [204]
                0.00    0.00  118100/236200      std::_Vector_base<int, std::allocator<int> >::_Vector_impl::_Vector_impl(std::allocator<int> const&) [179]
                0.00    0.00  118100/118100      std::_Vector_base<int, std::allocator<int> >::_M_create_storage(unsigned long) [202]
-----------------------------------------------
                0.00    0.00  118100/118100      __gnu_cxx::__alloc_traits<std::allocator<int>, int>::_S_select_on_copy(std::allocator<int> const&) [192]
[205]    0.0    0.00    0.00  118100         std::allocator_traits<std::allocator<int> >::select_on_container_copy_construction(std::allocator<int> const&) [205]
                0.00    0.00  118100/472400      std::allocator<int>::allocator(std::allocator<int> const&) [150]
-----------------------------------------------
                0.00    0.00  118100/118100      int* std::uninitialized_copy<__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, int*>(__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, __gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, int*) [219]
[206]    0.0    0.00    0.00  118100         int* std::__uninitialized_copy<true>::__uninit_copy<__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, int*>(__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, __gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, int*) [206]
                0.00    0.00  118100/118100      int* std::copy<__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, int*>(__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, __gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, int*) [221]
-----------------------------------------------
                0.00    0.00  118100/118100      AdjList::get_out_nodes(int) [5]
[207]    0.0    0.00    0.00  118100         std::vector<node, std::allocator<node> >::vector() [207]
                0.00    0.00  118100/118100      std::_Vector_base<node, std::allocator<node> >::_Vector_base() [200]
-----------------------------------------------
                0.00    0.00  118100/118100      std::vector<int, std::allocator<int> >::operator=(std::vector<int, std::allocator<int> >&&) [212]
[208]    0.0    0.00    0.00  118100         std::vector<int, std::allocator<int> >::_M_move_assign(std::vector<int, std::allocator<int> >&&, std::integral_constant<bool, true>) [208]
                0.00    0.00  236200/236200      std::_Vector_base<int, std::allocator<int> >::_Vector_impl_data::_M_swap_data(std::_Vector_base<int, std::allocator<int> >::_Vector_impl_data&) [181]
                0.00    0.00  236200/826700      std::_Vector_base<int, std::allocator<int> >::_M_get_Tp_allocator() [136]
                0.00    0.00  118100/118100      std::_Vector_base<int, std::allocator<int> >::get_allocator() const [193]
                0.00    0.00  118100/118100      std::vector<int, std::allocator<int> >::vector(std::allocator<int> const&) [210]
                0.00    0.00  118100/708600      std::allocator<int>::~allocator() [144]
                0.00    0.00  118100/472400      std::vector<int, std::allocator<int> >::~vector() [155]
                0.00    0.00  118100/118100      void std::__alloc_on_move<std::allocator<int> >(std::allocator<int>&, std::allocator<int>&) [217]
-----------------------------------------------
                0.00    0.00  118100/118100      AdjList::get_out_nodes(int) [5]
[209]    0.0    0.00    0.00  118100         std::vector<int, std::allocator<int> >::begin() [209]
                0.00    0.00  118100/617990      __gnu_cxx::__normal_iterator<int*, std::vector<int, std::allocator<int> > >::__normal_iterator(int* const&) [147]
-----------------------------------------------
                0.00    0.00  118100/118100      std::vector<int, std::allocator<int> >::_M_move_assign(std::vector<int, std::allocator<int> >&&, std::integral_constant<bool, true>) [208]
[210]    0.0    0.00    0.00  118100         std::vector<int, std::allocator<int> >::vector(std::allocator<int> const&) [210]
                0.00    0.00  118100/118100      std::_Vector_base<int, std::allocator<int> >::_Vector_base(std::allocator<int> const&) [203]
-----------------------------------------------
                0.00    0.00  118100/118100      AdjList::get_adjlist(__wt_cursor*, int) [7]
[211]    0.0    0.00    0.00  118100         std::vector<int, std::allocator<int> >::vector(std::vector<int, std::allocator<int> > const&) [211]
                0.00    0.00  118100/236200      std::_Vector_base<int, std::allocator<int> >::_M_get_Tp_allocator() const [176]
                0.00    0.00  118100/236200      std::vector<int, std::allocator<int> >::size() const [177]
                0.00    0.00  118100/118100      __gnu_cxx::__alloc_traits<std::allocator<int>, int>::_S_select_on_copy(std::allocator<int> const&) [192]
                0.00    0.00  118100/708600      std::allocator<int>::~allocator() [144]
                0.00    0.00  118100/118100      std::_Vector_base<int, std::allocator<int> >::_Vector_base(unsigned long, std::allocator<int> const&) [204]
                0.00    0.00  118100/826700      std::_Vector_base<int, std::allocator<int> >::_M_get_Tp_allocator() [136]
                0.00    0.00  118100/118100      std::vector<int, std::allocator<int> >::end() const [194]
                0.00    0.00  118100/118100      std::vector<int, std::allocator<int> >::begin() const [195]
                0.00    0.00  118100/118100      int* std::__uninitialized_copy_a<__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, int*, int>(__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, __gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, int*, std::allocator<int>&) [220]
-----------------------------------------------
                0.00    0.00  118100/118100      AdjList::__record_to_adjlist(__wt_cursor*, adjlist*) [8]
[212]    0.0    0.00    0.00  118100         std::vector<int, std::allocator<int> >::operator=(std::vector<int, std::allocator<int> >&&) [212]
                0.00    0.00  118100/118100      std::remove_reference<std::vector<int, std::allocator<int> >&>::type&& std::move<std::vector<int, std::allocator<int> >&>(std::vector<int, std::allocator<int> >&) [223]
                0.00    0.00  118100/118100      std::vector<int, std::allocator<int> >::_M_move_assign(std::vector<int, std::allocator<int> >&&, std::integral_constant<bool, true>) [208]
-----------------------------------------------
                0.00    0.00  118100/118100      int* std::__copy_move_a2<false, __gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, int*>(__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, __gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, int*) [216]
[213]    0.0    0.00    0.00  118100         int* std::__niter_base<int*>(int*) [213]
-----------------------------------------------
                0.00    0.00  118100/118100      int* std::__copy_move_a2<false, __gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, int*>(__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, __gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, int*) [216]
[214]    0.0    0.00    0.00  118100         int* std::__niter_wrap<int*>(int* const&, int*) [214]
-----------------------------------------------
                0.00    0.00  118100/118100      int* std::__copy_move_a2<false, __gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, int*>(__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, __gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, int*) [216]
[215]    0.0    0.00    0.00  118100         int* std::__copy_move_a<false, int const*, int*>(int const*, int const*, int*) [215]
                0.00    0.00  118100/118100      int* std::__copy_move<false, true, std::random_access_iterator_tag>::__copy_m<int>(int const*, int const*, int*) [197]
-----------------------------------------------
                0.00    0.00  118100/118100      int* std::copy<__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, int*>(__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, __gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, int*) [221]
[216]    0.0    0.00    0.00  118100         int* std::__copy_move_a2<false, __gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, int*>(__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, __gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, int*) [216]
                0.00    0.00  236200/236200      int const* std::__niter_base<int const*, std::vector<int, std::allocator<int> > >(__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >) [185]
                0.00    0.00  118100/118100      int* std::__niter_base<int*>(int*) [213]
                0.00    0.00  118100/118100      int* std::__copy_move_a<false, int const*, int*>(int const*, int const*, int*) [215]
                0.00    0.00  118100/118100      int* std::__niter_wrap<int*>(int* const&, int*) [214]
-----------------------------------------------
                0.00    0.00  118100/118100      std::vector<int, std::allocator<int> >::_M_move_assign(std::vector<int, std::allocator<int> >&&, std::integral_constant<bool, true>) [208]
[217]    0.0    0.00    0.00  118100         void std::__alloc_on_move<std::allocator<int> >(std::allocator<int>&, std::allocator<int>&) [217]
                0.00    0.00  118100/118100      void std::__do_alloc_on_move<std::allocator<int> >(std::allocator<int>&, std::allocator<int>&, std::integral_constant<bool, true>) [218]
-----------------------------------------------
                0.00    0.00  118100/118100      void std::__alloc_on_move<std::allocator<int> >(std::allocator<int>&, std::allocator<int>&) [217]
[218]    0.0    0.00    0.00  118100         void std::__do_alloc_on_move<std::allocator<int> >(std::allocator<int>&, std::allocator<int>&, std::integral_constant<bool, true>) [218]
                0.00    0.00  118100/118100      std::remove_reference<std::allocator<int>&>::type&& std::move<std::allocator<int>&>(std::allocator<int>&) [222]
-----------------------------------------------
                0.00    0.00  118100/118100      int* std::__uninitialized_copy_a<__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, int*, int>(__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, __gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, int*, std::allocator<int>&) [220]
[219]    0.0    0.00    0.00  118100         int* std::uninitialized_copy<__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, int*>(__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, __gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, int*) [219]
                0.00    0.00  118100/118100      int* std::__uninitialized_copy<true>::__uninit_copy<__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, int*>(__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, __gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, int*) [206]
-----------------------------------------------
                0.00    0.00  118100/118100      std::vector<int, std::allocator<int> >::vector(std::vector<int, std::allocator<int> > const&) [211]
[220]    0.0    0.00    0.00  118100         int* std::__uninitialized_copy_a<__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, int*, int>(__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, __gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, int*, std::allocator<int>&) [220]
                0.00    0.00  118100/118100      int* std::uninitialized_copy<__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, int*>(__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, __gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, int*) [219]
-----------------------------------------------
                0.00    0.00  118100/118100      int* std::__uninitialized_copy<true>::__uninit_copy<__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, int*>(__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, __gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, int*) [206]
[221]    0.0    0.00    0.00  118100         int* std::copy<__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, int*>(__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, __gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, int*) [221]
                0.00    0.00  236200/236200      __gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > > std::__miter_base<__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > > >(__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >) [184]
                0.00    0.00  118100/118100      int* std::__copy_move_a2<false, __gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, int*>(__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, __gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, int*) [216]
-----------------------------------------------
                0.00    0.00  118100/118100      void std::__do_alloc_on_move<std::allocator<int> >(std::allocator<int>&, std::allocator<int>&, std::integral_constant<bool, true>) [218]
[222]    0.0    0.00    0.00  118100         std::remove_reference<std::allocator<int>&>::type&& std::move<std::allocator<int>&>(std::allocator<int>&) [222]
-----------------------------------------------
                0.00    0.00  118100/118100      std::vector<int, std::allocator<int> >::operator=(std::vector<int, std::allocator<int> >&&) [212]
[223]    0.0    0.00    0.00  118100         std::remove_reference<std::vector<int, std::allocator<int> >&>::type&& std::move<std::vector<int, std::allocator<int> >&>(std::vector<int, std::allocator<int> >&) [223]
-----------------------------------------------
                0.00    0.00  102740/102740      std::allocator_traits<std::allocator<int> >::allocate(std::allocator<int>&, unsigned long) [226]
[224]    0.0    0.00    0.00  102740         __gnu_cxx::new_allocator<int>::allocate(unsigned long, void const*) [224]
                0.00    0.00  102740/102740      __gnu_cxx::new_allocator<int>::max_size() const [225]
-----------------------------------------------
                0.00    0.00  102740/102740      __gnu_cxx::new_allocator<int>::allocate(unsigned long, void const*) [224]
[225]    0.0    0.00    0.00  102740         __gnu_cxx::new_allocator<int>::max_size() const [225]
-----------------------------------------------
                0.00    0.00  102740/102740      std::_Vector_base<int, std::allocator<int> >::_M_allocate(unsigned long) [201]
[226]    0.0    0.00    0.00  102740         std::allocator_traits<std::allocator<int> >::allocate(std::allocator<int>&, unsigned long) [226]
                0.00    0.00  102740/102740      __gnu_cxx::new_allocator<int>::allocate(unsigned long, void const*) [224]
-----------------------------------------------
                              116750             std::_Rb_tree<int, int, std::_Identity<int>, std::less<int>, std::allocator<int> >::_M_erase(std::_Rb_tree_node<int>*) [227]
                0.00    0.00    1350/1350        bfs_info* bfs<AdjList>(AdjList&, int) [4]
[227]    0.0    0.00    0.00    1350+116750  std::_Rb_tree<int, int, std::_Identity<int>, std::less<int>, std::allocator<int> >::_M_erase(std::_Rb_tree_node<int>*) [227]
                              116750             std::_Rb_tree<int, int, std::_Identity<int>, std::less<int>, std::allocator<int> >::_M_erase(std::_Rb_tree_node<int>*) [227]
-----------------------------------------------
                0.00    0.00     800/800         bfs_info* bfs<AdjList>(AdjList&, int) [4]
[228]    0.0    0.00    0.00     800         void std::deque<int, std::allocator<int> >::_M_push_back_aux<int const&>(int const&) [228]
-----------------------------------------------
                0.00    0.00     160/160         bfs_info* bfs<AdjList>(AdjList&, int) [4]
[229]    0.0    0.00    0.00     160         std::_Deque_base<int, std::allocator<int> >::_M_initialize_map(unsigned long) [229]
-----------------------------------------------
                0.00    0.00      18/18          AdjList::get_random_node() [66]
[230]    0.0    0.00    0.00      18         node::node() [230]
-----------------------------------------------
                0.00    0.00       1/18          std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_replace(unsigned long, unsigned long, char const*, unsigned long) [300]
                0.00    0.00       1/18          CmdLineApp::CmdLineApp(int, char**) [314]
                0.00    0.00      16/18          print_csv_info(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, int, bfs_info*) [308]
[231]    0.0    0.00    0.00      18         std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_mutate(unsigned long, unsigned long, char const*, unsigned long) [231]
-----------------------------------------------
                0.00    0.00       9/9           std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::~allocator() [233]
[232]    0.0    0.00    0.00       9         __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::~new_allocator() [232]
-----------------------------------------------
                0.00    0.00       1/9           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl::~_Vector_impl() [292]
                0.00    0.00       4/9           AdjList::AdjList(graph_opts) [67]
                0.00    0.00       4/9           std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_S_check_init_len(unsigned long, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [256]
[233]    0.0    0.00    0.00       9         std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::~allocator() [233]
                0.00    0.00       9/9           __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::~new_allocator() [232]
-----------------------------------------------
                0.00    0.00       1/9           std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::~vector() [299]
                0.00    0.00       8/9           void std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_range_initialize<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::forward_iterator_tag) [257]
[234]    0.0    0.00    0.00       9         std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_get_Tp_allocator() [234]
-----------------------------------------------
                0.00    0.00       9/9           std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::__uninitialized_copy<false>::__uninit_copy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*) [254]
[235]    0.0    0.00    0.00       9         void std::_Construct<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&) [235]
                0.00    0.00       9/9           std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const& std::forward<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&>(std::remove_reference<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&>::type&) [237]
                0.00    0.00       9/3796689     operator new(unsigned long, void*) [114]
                0.00    0.00       9/236260      void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char*>(char*, char*, std::forward_iterator_tag) [169]
-----------------------------------------------
                0.00    0.00       9/9           std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::__uninitialized_copy<false>::__uninit_copy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*) [254]
[236]    0.0    0.00    0.00       9         std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::__addressof<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&) [236]
-----------------------------------------------
                0.00    0.00       9/9           void std::_Construct<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&) [235]
[237]    0.0    0.00    0.00       9         std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const& std::forward<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&>(std::remove_reference<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&>::type&) [237]
-----------------------------------------------
                0.00    0.00       8/8           std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::allocator(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [241]
[238]    0.0    0.00    0.00       8         __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::new_allocator(__gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [238]
-----------------------------------------------
                0.00    0.00       4/8           std::allocator_traits<std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::max_size(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [253]
                0.00    0.00       4/8           __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::allocate(unsigned long, void const*) [246]
[239]    0.0    0.00    0.00       8         __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::max_size() const [239]
-----------------------------------------------
                0.00    0.00       4/8           std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::vector(std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [258]
                0.00    0.00       4/8           std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::end() const [247]
[240]    0.0    0.00    0.00       8         std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::begin() const [240]
-----------------------------------------------
                0.00    0.00       4/8           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl::_Vector_impl(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [250]
                0.00    0.00       4/8           std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_S_check_init_len(unsigned long, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [256]
[241]    0.0    0.00    0.00       8         std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::allocator(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [241]
                0.00    0.00       8/8           __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::new_allocator(__gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [238]
-----------------------------------------------
                0.00    0.00       5/5           std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::allocator() [243]
[242]    0.0    0.00    0.00       5         __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::new_allocator() [242]
-----------------------------------------------
                0.00    0.00       1/5           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl::_Vector_impl() [291]
                0.00    0.00       4/5           AdjList::AdjList(graph_opts) [67]
[243]    0.0    0.00    0.00       5         std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::allocator() [243]
                0.00    0.00       5/5           __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::new_allocator() [242]
-----------------------------------------------
                0.00    0.00       1/5           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl::_Vector_impl() [291]
                0.00    0.00       4/5           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl::_Vector_impl(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [250]
[244]    0.0    0.00    0.00       5         std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl_data::_Vector_impl_data() [244]
-----------------------------------------------
                0.00    0.00       5/5           CmdLineBase::CmdLineBase(int, char**) [275]
[245]    0.0    0.00    0.00       5         void std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_realloc_insert<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >(__gnu_cxx::__normal_iterator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > > >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&) [245]
-----------------------------------------------
                0.00    0.00       4/4           std::allocator_traits<std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::allocate(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >&, unsigned long) [252]
[246]    0.0    0.00    0.00       4         __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::allocate(unsigned long, void const*) [246]
                0.00    0.00       4/8           __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::max_size() const [239]
-----------------------------------------------
                0.00    0.00       4/4           std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::vector(std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [258]
[247]    0.0    0.00    0.00       4         std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::end() const [247]
                0.00    0.00       4/8           std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::begin() const [240]
                0.00    0.00       4/4           std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::size() const [248]
-----------------------------------------------
                0.00    0.00       4/4           std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::end() const [247]
[248]    0.0    0.00    0.00       4         std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::size() const [248]
-----------------------------------------------
                0.00    0.00       4/4           void std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_range_initialize<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::forward_iterator_tag) [257]
[249]    0.0    0.00    0.00       4         std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_allocate(unsigned long) [249]
                0.00    0.00       4/4           std::allocator_traits<std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::allocate(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >&, unsigned long) [252]
-----------------------------------------------
                0.00    0.00       4/4           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_base(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [251]
[250]    0.0    0.00    0.00       4         std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl::_Vector_impl(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [250]
                0.00    0.00       4/8           std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::allocator(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [241]
                0.00    0.00       4/5           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl_data::_Vector_impl_data() [244]
-----------------------------------------------
                0.00    0.00       4/4           std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::vector(std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [258]
[251]    0.0    0.00    0.00       4         std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_base(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [251]
                0.00    0.00       4/4           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl::_Vector_impl(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [250]
-----------------------------------------------
                0.00    0.00       4/4           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_allocate(unsigned long) [249]
[252]    0.0    0.00    0.00       4         std::allocator_traits<std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::allocate(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >&, unsigned long) [252]
                0.00    0.00       4/4           __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::allocate(unsigned long, void const*) [246]
-----------------------------------------------
                0.00    0.00       4/4           std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_S_max_size(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [255]
[253]    0.0    0.00    0.00       4         std::allocator_traits<std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::max_size(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [253]
                0.00    0.00       4/8           __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::max_size() const [239]
-----------------------------------------------
                0.00    0.00       4/4           std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::uninitialized_copy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*) [260]
[254]    0.0    0.00    0.00       4         std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::__uninitialized_copy<false>::__uninit_copy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*) [254]
                0.00    0.00       9/9           std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::__addressof<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&) [236]
                0.00    0.00       9/9           void std::_Construct<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&) [235]
-----------------------------------------------
                0.00    0.00       4/4           std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_S_check_init_len(unsigned long, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [256]
[255]    0.0    0.00    0.00       4         std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_S_max_size(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [255]
                0.00    0.00       4/4           std::allocator_traits<std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::max_size(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [253]
                0.00    0.00       4/763584      unsigned long const& std::min<unsigned long>(unsigned long const&, unsigned long const&) [137]
-----------------------------------------------
                0.00    0.00       4/4           void std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_range_initialize<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::forward_iterator_tag) [257]
[256]    0.0    0.00    0.00       4         std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_S_check_init_len(unsigned long, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [256]
                0.00    0.00       4/8           std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::allocator(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [241]
                0.00    0.00       4/4           std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_S_max_size(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [255]
                0.00    0.00       4/9           std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::~allocator() [233]
-----------------------------------------------
                0.00    0.00       4/4           std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::vector(std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [258]
[257]    0.0    0.00    0.00       4         void std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_range_initialize<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::forward_iterator_tag) [257]
                0.00    0.00       8/9           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_get_Tp_allocator() [234]
                0.00    0.00       4/4           std::iterator_traits<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>::difference_type std::distance<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*) [264]
                0.00    0.00       4/4           std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_S_check_init_len(unsigned long, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [256]
                0.00    0.00       4/4           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_allocate(unsigned long) [249]
                0.00    0.00       4/4           std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::__uninitialized_copy_a<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >&) [262]
-----------------------------------------------
                0.00    0.00       4/4           AdjList::AdjList(graph_opts) [67]
[258]    0.0    0.00    0.00       4         std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::vector(std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [258]
                0.00    0.00       4/4           std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::end() const [247]
                0.00    0.00       4/4           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_base(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [251]
                0.00    0.00       4/8           std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::begin() const [240]
                0.00    0.00       4/4           void std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_range_initialize<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::forward_iterator_tag) [257]
-----------------------------------------------
                0.00    0.00       4/4           std::iterator_traits<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>::difference_type std::distance<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*) [264]
[259]    0.0    0.00    0.00       4         std::iterator_traits<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>::difference_type std::__distance<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::random_access_iterator_tag) [259]
-----------------------------------------------
                0.00    0.00       4/4           std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::__uninitialized_copy_a<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >&) [262]
[260]    0.0    0.00    0.00       4         std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::uninitialized_copy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*) [260]
                0.00    0.00       4/4           std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::__uninitialized_copy<false>::__uninit_copy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*) [254]
-----------------------------------------------
                0.00    0.00       4/4           std::iterator_traits<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>::difference_type std::distance<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*) [264]
[261]    0.0    0.00    0.00       4         std::iterator_traits<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>::iterator_category std::__iterator_category<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const* const&) [261]
-----------------------------------------------
                0.00    0.00       4/4           void std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_range_initialize<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::forward_iterator_tag) [257]
[262]    0.0    0.00    0.00       4         std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::__uninitialized_copy_a<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >&) [262]
                0.00    0.00       4/4           std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::uninitialized_copy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*) [260]
-----------------------------------------------
                0.00    0.00       1/4           std::filesystem::__cxx11::path::path(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&, std::filesystem::__cxx11::path::format) [284]
                0.00    0.00       3/4           std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&) [265]
[263]    0.0    0.00    0.00       4         std::remove_reference<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&>::type&& std::move<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&) [263]
-----------------------------------------------
                0.00    0.00       4/4           void std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_range_initialize<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::forward_iterator_tag) [257]
[264]    0.0    0.00    0.00       4         std::iterator_traits<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>::difference_type std::distance<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*) [264]
                0.00    0.00       4/4           std::iterator_traits<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>::iterator_category std::__iterator_category<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const* const&) [261]
                0.00    0.00       4/4           std::iterator_traits<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>::difference_type std::__distance<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::random_access_iterator_tag) [259]
-----------------------------------------------
                0.00    0.00       1/3           __static_initialization_and_destruction_0(int, int) [63]
                0.00    0.00       2/3           AdjList::AdjList(graph_opts) [67]
[265]    0.0    0.00    0.00       3         std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&) [265]
                0.00    0.00       3/4           std::remove_reference<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&>::type&& std::move<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&) [263]
-----------------------------------------------
                0.00    0.00       1/2           std::filesystem::exists(std::filesystem::file_status) [281]
                0.00    0.00       1/2           std::filesystem::status_known(std::filesystem::file_status) [280]
[266]    0.0    0.00    0.00       2         std::filesystem::file_status::type() const [266]
-----------------------------------------------
                0.00    0.00       2/2           AdjList::AdjList(graph_opts) [67]
[267]    0.0    0.00    0.00       2         std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&, char const*) [267]
                0.00    0.00       2/236260      void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char*>(char*, char*, std::forward_iterator_tag) [169]
-----------------------------------------------
                0.00    0.00       1/1           __libc_csu_init [61]
[268]    0.0    0.00    0.00       1         _GLOBAL__sub_I__Z14print_csv_infoNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEiP8bfs_info [268]
-----------------------------------------------
                0.00    0.00       1/1           AdjList::__restore_from_db(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >) [68]
[269]    0.0    0.00    0.00       1         CommonUtil::open_session(__wt_connection*, __wt_session**) [269]
-----------------------------------------------
                0.00    0.00       1/1           AdjList::__restore_from_db(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >) [68]
[270]    0.0    0.00    0.00       1         CommonUtil::open_connection(char*, __wt_connection**) [270]
-----------------------------------------------
                0.00    0.00       1/1           AdjList::close() [426]
[271]    0.0    0.00    0.00       1         CommonUtil::close_connection(__wt_connection*) [271]
-----------------------------------------------
                0.00    0.00       1/1           AdjList::AdjList(graph_opts) [67]
[272]    0.0    0.00    0.00       1         CommonUtil::check_graph_params(graph_opts) [272]
                0.00    0.00       1/1           std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::vector() [298]
                0.00    0.00       1/1           std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::size() const [277]
                0.00    0.00       1/1           std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::~vector() [299]
-----------------------------------------------
                0.00    0.00       1/1           AdjList::AdjList(graph_opts) [67]
[273]    0.0    0.00    0.00       1         graph_opts::graph_opts(graph_opts const&) [273]
-----------------------------------------------
                0.00    0.00       1/1           AdjList::AdjList(graph_opts) [67]
[274]    0.0    0.00    0.00       1         graph_opts::~graph_opts() [274]
-----------------------------------------------
                0.00    0.00       1/1           CmdLineApp::CmdLineApp(int, char**) [314]
[275]    0.0    0.00    0.00       1         CmdLineBase::CmdLineBase(int, char**) [275]
                0.00    0.00       5/5           void std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_realloc_insert<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >(__gnu_cxx::__normal_iterator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > > >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&) [245]
-----------------------------------------------
                0.00    0.00       1/1           CmdLineApp::CmdLineApp(int, char**) [314]
[276]    0.0    0.00    0.00       1         std::ctype<char>::do_widen(char) const [276]
-----------------------------------------------
                0.00    0.00       1/1           CommonUtil::check_graph_params(graph_opts) [272]
[277]    0.0    0.00    0.00       1         std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::size() const [277]
-----------------------------------------------
                0.00    0.00       1/1           std::_Tuple_impl<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_head(std::_Tuple_impl<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [288]
[278]    0.0    0.00    0.00       1         std::_Head_base<0ul, std::filesystem::__cxx11::path::_List::_Impl*, false>::_M_head(std::_Head_base<0ul, std::filesystem::__cxx11::path::_List::_Impl*, false>&) [278]
-----------------------------------------------
                0.00    0.00       1/1           std::_Tuple_impl<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_head(std::_Tuple_impl<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [289]
[279]    0.0    0.00    0.00       1         std::_Head_base<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter, true>::_M_head(std::_Head_base<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter, true>&) [279]
-----------------------------------------------
                0.00    0.00       1/1           std::filesystem::exists(std::filesystem::file_status) [281]
[280]    0.0    0.00    0.00       1         std::filesystem::status_known(std::filesystem::file_status) [280]
                0.00    0.00       1/2           std::filesystem::file_status::type() const [266]
-----------------------------------------------
                0.00    0.00       1/1           std::filesystem::exists(std::filesystem::__cxx11::path const&) [282]
[281]    0.0    0.00    0.00       1         std::filesystem::exists(std::filesystem::file_status) [281]
                0.00    0.00       1/1           std::filesystem::status_known(std::filesystem::file_status) [280]
                0.00    0.00       1/2           std::filesystem::file_status::type() const [266]
-----------------------------------------------
                0.00    0.00       1/1           AdjList::AdjList(graph_opts) [67]
[282]    0.0    0.00    0.00       1         std::filesystem::exists(std::filesystem::__cxx11::path const&) [282]
                0.00    0.00       1/1           std::filesystem::exists(std::filesystem::file_status) [281]
-----------------------------------------------
                0.00    0.00       1/1           std::filesystem::__cxx11::path::~path() [285]
[283]    0.0    0.00    0.00       1         std::filesystem::__cxx11::path::_List::~_List() [283]
                0.00    0.00       1/1           std::unique_ptr<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::~unique_ptr() [287]
-----------------------------------------------
                0.00    0.00       1/1           AdjList::AdjList(graph_opts) [67]
[284]    0.0    0.00    0.00       1         std::filesystem::__cxx11::path::path(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&, std::filesystem::__cxx11::path::format) [284]
                0.00    0.00       1/4           std::remove_reference<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&>::type&& std::move<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&) [263]
-----------------------------------------------
                0.00    0.00       1/1           AdjList::AdjList(graph_opts) [67]
[285]    0.0    0.00    0.00       1         std::filesystem::__cxx11::path::~path() [285]
                0.00    0.00       1/1           std::filesystem::__cxx11::path::_List::~_List() [283]
-----------------------------------------------
                0.00    0.00       1/1           std::unique_ptr<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::~unique_ptr() [287]
[286]    0.0    0.00    0.00       1         std::unique_ptr<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::get_deleter() [286]
                0.00    0.00       1/1           std::__uniq_ptr_impl<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_deleter() [296]
-----------------------------------------------
                0.00    0.00       1/1           std::filesystem::__cxx11::path::_List::~_List() [283]
[287]    0.0    0.00    0.00       1         std::unique_ptr<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::~unique_ptr() [287]
                0.00    0.00       1/1           std::__uniq_ptr_impl<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_ptr() [297]
                0.00    0.00       1/1           std::unique_ptr<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::get_deleter() [286]
                0.00    0.00       1/1           std::remove_reference<std::filesystem::__cxx11::path::_List::_Impl*&>::type&& std::move<std::filesystem::__cxx11::path::_List::_Impl*&>(std::filesystem::__cxx11::path::_List::_Impl*&) [305]
-----------------------------------------------
                0.00    0.00       1/1           std::filesystem::__cxx11::path::_List::_Impl*& std::__get_helper<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::_Tuple_impl<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [301]
[288]    0.0    0.00    0.00       1         std::_Tuple_impl<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_head(std::_Tuple_impl<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [288]
                0.00    0.00       1/1           std::_Head_base<0ul, std::filesystem::__cxx11::path::_List::_Impl*, false>::_M_head(std::_Head_base<0ul, std::filesystem::__cxx11::path::_List::_Impl*, false>&) [278]
-----------------------------------------------
                0.00    0.00       1/1           std::filesystem::__cxx11::path::_List::_Impl_deleter& std::__get_helper<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::_Tuple_impl<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [302]
[289]    0.0    0.00    0.00       1         std::_Tuple_impl<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_head(std::_Tuple_impl<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [289]
                0.00    0.00       1/1           std::_Head_base<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter, true>::_M_head(std::_Head_base<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter, true>&) [279]
-----------------------------------------------
                0.00    0.00       1/1           void std::_Destroy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*) [306]
[290]    0.0    0.00    0.00       1         void std::_Destroy_aux<false>::__destroy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*) [290]
-----------------------------------------------
                0.00    0.00       1/1           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_base() [294]
[291]    0.0    0.00    0.00       1         std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl::_Vector_impl() [291]
                0.00    0.00       1/5           std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::allocator() [243]
                0.00    0.00       1/5           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl_data::_Vector_impl_data() [244]
-----------------------------------------------
                0.00    0.00       1/1           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::~_Vector_base() [295]
[292]    0.0    0.00    0.00       1         std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl::~_Vector_impl() [292]
                0.00    0.00       1/9           std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::~allocator() [233]
-----------------------------------------------
                0.00    0.00       1/1           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::~_Vector_base() [295]
[293]    0.0    0.00    0.00       1         std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_deallocate(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, unsigned long) [293]
-----------------------------------------------
                0.00    0.00       1/1           std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::vector() [298]
[294]    0.0    0.00    0.00       1         std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_base() [294]
                0.00    0.00       1/1           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl::_Vector_impl() [291]
-----------------------------------------------
                0.00    0.00       1/1           std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::~vector() [299]
[295]    0.0    0.00    0.00       1         std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::~_Vector_base() [295]
                0.00    0.00       1/1           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_deallocate(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, unsigned long) [293]
                0.00    0.00       1/1           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl::~_Vector_impl() [292]
-----------------------------------------------
                0.00    0.00       1/1           std::unique_ptr<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::get_deleter() [286]
[296]    0.0    0.00    0.00       1         std::__uniq_ptr_impl<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_deleter() [296]
                0.00    0.00       1/1           std::tuple_element<1ul, std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter> >::type& std::get<1ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [304]
-----------------------------------------------
                0.00    0.00       1/1           std::unique_ptr<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::~unique_ptr() [287]
[297]    0.0    0.00    0.00       1         std::__uniq_ptr_impl<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_ptr() [297]
                0.00    0.00       1/1           std::tuple_element<0ul, std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter> >::type& std::get<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [303]
-----------------------------------------------
                0.00    0.00       1/1           CommonUtil::check_graph_params(graph_opts) [272]
[298]    0.0    0.00    0.00       1         std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::vector() [298]
                0.00    0.00       1/1           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_base() [294]
-----------------------------------------------
                0.00    0.00       1/1           CommonUtil::check_graph_params(graph_opts) [272]
[299]    0.0    0.00    0.00       1         std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::~vector() [299]
                0.00    0.00       1/9           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_get_Tp_allocator() [234]
                0.00    0.00       1/1           void std::_Destroy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >&) [307]
                0.00    0.00       1/1           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::~_Vector_base() [295]
-----------------------------------------------
                0.00    0.00       1/1           CmdLineApp::CmdLineApp(int, char**) [314]
[300]    0.0    0.00    0.00       1         std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_replace(unsigned long, unsigned long, char const*, unsigned long) [300]
                0.00    0.00       1/18          std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_mutate(unsigned long, unsigned long, char const*, unsigned long) [231]
-----------------------------------------------
                0.00    0.00       1/1           std::tuple_element<0ul, std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter> >::type& std::get<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [303]
[301]    0.0    0.00    0.00       1         std::filesystem::__cxx11::path::_List::_Impl*& std::__get_helper<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::_Tuple_impl<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [301]
                0.00    0.00       1/1           std::_Tuple_impl<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_head(std::_Tuple_impl<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [288]
-----------------------------------------------
                0.00    0.00       1/1           std::tuple_element<1ul, std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter> >::type& std::get<1ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [304]
[302]    0.0    0.00    0.00       1         std::filesystem::__cxx11::path::_List::_Impl_deleter& std::__get_helper<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::_Tuple_impl<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [302]
                0.00    0.00       1/1           std::_Tuple_impl<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_head(std::_Tuple_impl<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [289]
-----------------------------------------------
                0.00    0.00       1/1           std::__uniq_ptr_impl<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_ptr() [297]
[303]    0.0    0.00    0.00       1         std::tuple_element<0ul, std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter> >::type& std::get<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [303]
                0.00    0.00       1/1           std::filesystem::__cxx11::path::_List::_Impl*& std::__get_helper<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::_Tuple_impl<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [301]
-----------------------------------------------
                0.00    0.00       1/1           std::__uniq_ptr_impl<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_deleter() [296]
[304]    0.0    0.00    0.00       1         std::tuple_element<1ul, std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter> >::type& std::get<1ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [304]
                0.00    0.00       1/1           std::filesystem::__cxx11::path::_List::_Impl_deleter& std::__get_helper<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::_Tuple_impl<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [302]
-----------------------------------------------
                0.00    0.00       1/1           std::unique_ptr<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::~unique_ptr() [287]
[305]    0.0    0.00    0.00       1         std::remove_reference<std::filesystem::__cxx11::path::_List::_Impl*&>::type&& std::move<std::filesystem::__cxx11::path::_List::_Impl*&>(std::filesystem::__cxx11::path::_List::_Impl*&) [305]
-----------------------------------------------
                0.00    0.00       1/1           void std::_Destroy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >&) [307]
[306]    0.0    0.00    0.00       1         void std::_Destroy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*) [306]
                0.00    0.00       1/1           void std::_Destroy_aux<false>::__destroy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*) [290]
-----------------------------------------------
                0.00    0.00       1/1           std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::~vector() [299]
[307]    0.0    0.00    0.00       1         void std::_Destroy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >&) [307]
                0.00    0.00       1/1           void std::_Destroy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*) [306]
-----------------------------------------------

 This table describes the call tree of the program, and was sorted by
 the total amount of time spent in each function and its children.

 Each entry in this table consists of several lines.  The line with the
 index number at the left hand margin lists the current function.
 The lines above it list the functions that called this function,
 and the lines below it list the functions this one called.
 This line lists:
     index	A unique number given to each element of the table.
		Index numbers are sorted numerically.
		The index number is printed next to every function name so
		it is easier to look up where the function is in the table.

     % time	This is the percentage of the `total' time that was spent
		in this function and its children.  Note that due to
		different viewpoints, functions excluded by options, etc,
		these numbers will NOT add up to 100%.

     self	This is the total amount of time spent in this function.

     children	This is the total amount of time propagated into this
		function by its children.

     called	This is the number of times the function was called.
		If the function called itself recursively, the number
		only includes non-recursive calls, and is followed by
		a `+' and the number of recursive calls.

     name	The name of the current function.  The index number is
		printed after it.  If the function is a member of a
		cycle, the cycle number is printed between the
		function's name and the index number.


 For the function's parents, the fields have the following meanings:

     self	This is the amount of time that was propagated directly
		from the function into this parent.

     children	This is the amount of time that was propagated from
		the function's children into this parent.

     called	This is the number of times this parent called the
		function `/' the total number of times the function
		was called.  Recursive calls to the function are not
		included in the number after the `/'.

     name	This is the name of the parent.  The parent's index
		number is printed after it.  If the parent is a
		member of a cycle, the cycle number is printed between
		the name and the index number.

 If the parents of the function cannot be determined, the word
 `<spontaneous>' is printed in the `name' field, and all the other
 fields are blank.

 For the function's children, the fields have the following meanings:

     self	This is the amount of time that was propagated directly
		from the child into the function.

     children	This is the amount of time that was propagated from the
		child's children to the function.

     called	This is the number of times the function called
		this child `/' the total number of times the child
		was called.  Recursive calls by the child are not
		listed in the number after the `/'.

     name	This is the name of the child.  The child's index
		number is printed after it.  If the child is a
		member of a cycle, the cycle number is printed
		between the name and the index number.

 If there are any cycles (circles) in the call graph, there is an
 entry for the cycle-as-a-whole.  This entry shows who called the
 cycle (as parents) and the members of the cycle (as children.)
 The `+' recursive calls entry shows the number of function calls that
 were internal to the cycle, and the calls entry for each member shows,
 for that member, how many times it was called from other members of
 the cycle.

Copyright (C) 2012-2020 Free Software Foundation, Inc.

Copying and distribution of this file, with or without modification,
are permitted in any medium without royalty provided the copyright
notice and this notice are preserved.

Index by function name

 [268] _GLOBAL__sub_I__Z14print_csv_infoNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEiP8bfs_info (bfs.cpp) [144] std::allocator<int>::~allocator() [155] std::vector<int, std::allocator<int> >::~vector()
  [62] _GLOBAL__sub_I__Z8METADATAB5cxx11 (common.cpp) [278] std::_Head_base<0ul, std::filesystem::__cxx11::path::_List::_Impl*, false>::_M_head(std::_Head_base<0ul, std::filesystem::__cxx11::path::_List::_Impl*, false>&) [212] std::vector<int, std::allocator<int> >::operator=(std::vector<int, std::allocator<int> >&&)
  [70] _GLOBAL__sub_I__ZN13StandardGraphC2Ev (standard_graph.cpp) [279] std::_Head_base<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter, true>::_M_head(std::_Head_base<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter, true>&) [300] std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_replace(unsigned long, unsigned long, char const*, unsigned long)
  [71] _GLOBAL__sub_I__ZN7AdjListC2Ev (adj_list.cpp) [280] std::filesystem::status_known(std::filesystem::file_status) [38] void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char const*>(char const*, char const*)
  [72] _GLOBAL__sub_I__ZN7EdgeKeyC2Ev (edgekey.cpp) [281] std::filesystem::exists(std::filesystem::file_status) [39] void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char const*>(char const*, char const*, std::forward_iterator_tag)
   [4] bfs_info* bfs<AdjList>(AdjList&, int) [282] std::filesystem::exists(std::filesystem::__cxx11::path const&) [169] void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char*>(char*, char*, std::forward_iterator_tag)
  [73] __static_initialization_and_destruction_0(int, int) (adj_list.cpp) [283] std::filesystem::__cxx11::path::_List::~_List() [40] void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct_aux<char const*>(char const*, char const*, std::__false_type)
  [63] __static_initialization_and_destruction_0(int, int) (common.cpp) [284] std::filesystem::__cxx11::path::path(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&, std::filesystem::__cxx11::path::format) [231] std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_mutate(unsigned long, unsigned long, char const*, unsigned long)
  [74] __static_initialization_and_destruction_0(int, int) (edgekey.cpp) [285] std::filesystem::__cxx11::path::~path() [27] std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::basic_string<std::allocator<char> >(char const*, std::allocator<char> const&)
  [75] __static_initialization_and_destruction_0(int, int) (standard_graph.cpp) [286] std::unique_ptr<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::get_deleter() [21] std::__cxx11::stoi(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&, unsigned long*, int)
 [269] CommonUtil::open_session(__wt_connection*, __wt_session**) [287] std::unique_ptr<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::~unique_ptr() [227] std::_Rb_tree<int, int, std::_Identity<int>, std::less<int>, std::allocator<int> >::_M_erase(std::_Rb_tree_node<int>*)
 [270] CommonUtil::open_connection(char*, __wt_connection**) [229] std::_Deque_base<int, std::allocator<int> >::_M_initialize_map(unsigned long) [235] void std::_Construct<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&)
 [271] CommonUtil::close_connection(__wt_connection*) [288] std::_Tuple_impl<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_head(std::_Tuple_impl<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [259] std::iterator_traits<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>::difference_type std::__distance<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::random_access_iterator_tag)
 [272] CommonUtil::check_graph_params(graph_opts) [289] std::_Tuple_impl<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_head(std::_Tuple_impl<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [124] std::iterator_traits<char const*>::difference_type std::__distance<char const*>(char const*, char const*, std::random_access_iterator_tag)
  [10] CommonUtil::unpack_int_vector_std(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >) [197] int* std::__copy_move<false, true, std::random_access_iterator_tag>::__copy_m<int>(int const*, int const*, int*) [170] std::iterator_traits<char*>::difference_type std::__distance<char*>(char*, char*, std::random_access_iterator_tag)
 [273] graph_opts::graph_opts(graph_opts const&) [37] std::char_traits<char>::length(char const*) [113] node* std::__addressof<node>(node&)
 [274] graph_opts::~graph_opts() [290] void std::_Destroy_aux<false>::__destroy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*) [236] std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::__addressof<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&)
 [275] CmdLineBase::CmdLineBase(int, char**) [151] void std::_Destroy_aux<true>::__destroy<int*>(int*, int*) [301] std::filesystem::__cxx11::path::_List::_Impl*& std::__get_helper<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::_Tuple_impl<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&)
 [230] node::node()          [161] std::_Vector_base<node, std::allocator<node> >::_M_allocate(unsigned long) [302] std::filesystem::__cxx11::path::_List::_Impl_deleter& std::__get_helper<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::_Tuple_impl<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>&)
   [7] AdjList::get_adjlist(__wt_cursor*, int) [198] std::_Vector_base<node, std::allocator<node> >::_Vector_impl::_Vector_impl() [184] __gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > > std::__miter_base<__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > > >(__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >)
  [69] AdjList::init_cursors() [162] std::_Vector_base<node, std::allocator<node> >::_M_deallocate(node*, unsigned long) [118] node* std::__niter_base<node*>(node*)
   [5] AdjList::get_out_nodes(int) [199] std::_Vector_base<node, std::allocator<node> >::_Vector_impl_data::_Vector_impl_data() [185] int const* std::__niter_base<int const*, std::vector<int, std::allocator<int> > >(__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >)
  [65] AdjList::get_out_degree(int) [140] std::_Vector_base<node, std::allocator<node> >::_M_get_Tp_allocator() [213] int* std::__niter_base<int*>(int*)
  [59] AdjList::get_node_cursor() [200] std::_Vector_base<node, std::allocator<node> >::_Vector_base() [214] int* std::__niter_wrap<int*>(int* const&, int*)
  [66] AdjList::get_random_node() [249] std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_allocate(unsigned long) [18] node* std::__relocate_a<node*, node*, std::allocator<node> >(node*, node*, node*, std::allocator<node>&)
  [33] AdjList::__record_to_node(__wt_cursor*, int) [250] std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl::_Vector_impl(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [215] int* std::__copy_move_a<false, int const*, int*>(int const*, int const*, int*)
  [68] AdjList::__restore_from_db(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >) [291] std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl::_Vector_impl() [216] int* std::__copy_move_a2<false, __gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, int*>(__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, __gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, int*)
  [58] AdjList::_get_table_cursor(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, __wt_cursor**, bool) [292] std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl::~_Vector_impl() [19] node* std::__relocate_a_1<node*, node*, std::allocator<node> >(node*, node*, node*, std::allocator<node>&)
   [8] AdjList::__record_to_adjlist(__wt_cursor*, adjlist*) [293] std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_deallocate(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, unsigned long) [217] void std::__alloc_on_move<std::allocator<int> >(std::allocator<int>&, std::allocator<int>&)
 [188] AdjList::get_out_adjlist_cursor() [244] std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl_data::_Vector_impl_data() [218] void std::__do_alloc_on_move<std::allocator<int> >(std::allocator<int>&, std::allocator<int>&, std::integral_constant<bool, true>)
 [189] adjlist::adjlist()    [234] std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_get_Tp_allocator() [219] int* std::uninitialized_copy<__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, int*>(__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, __gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, int*)
 [190] adjlist::~adjlist()   [294] std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_base() [260] std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::uninitialized_copy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*)
 [166] __gnu_cxx::new_allocator<node>::deallocate(node*, unsigned long) [251] std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_base(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [261] std::iterator_traits<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>::iterator_category std::__iterator_category<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const* const&)
  [31] void __gnu_cxx::new_allocator<node>::destroy<node>(node*) [295] std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::~_Vector_base() [125] std::iterator_traits<char const*>::iterator_category std::__iterator_category<char const*>(char const* const&)
 [158] __gnu_cxx::new_allocator<node>::allocate(unsigned long, void const*) [201] std::_Vector_base<int, std::allocator<int> >::_M_allocate(unsigned long) [171] std::iterator_traits<char*>::iterator_category std::__iterator_category<char*>(char* const&)
  [60] void __gnu_cxx::new_allocator<node>::construct<node, node>(node*, node&&) [179] std::_Vector_base<int, std::allocator<int> >::_Vector_impl::_Vector_impl(std::allocator<int> const&) [15] void std::__relocate_object_a<node, node, std::allocator<node> >(node*, node*, std::allocator<node>&)
 [191] __gnu_cxx::new_allocator<node>::new_allocator() [180] std::_Vector_base<int, std::allocator<int> >::_Vector_impl::_Vector_impl() [220] int* std::__uninitialized_copy_a<__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, int*, int>(__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, __gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, int*, std::allocator<int>&)
 [246] __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::allocate(unsigned long, void const*) [152] std::_Vector_base<int, std::allocator<int> >::_Vector_impl::~_Vector_impl() [262] std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::__uninitialized_copy_a<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >&)
 [238] __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::new_allocator(__gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [153] std::_Vector_base<int, std::allocator<int> >::_M_deallocate(int*, unsigned long) [303] std::tuple_element<0ul, std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter> >::type& std::get<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&)
 [242] __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::new_allocator() [202] std::_Vector_base<int, std::allocator<int> >::_M_create_storage(unsigned long) [304] std::tuple_element<1ul, std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter> >::type& std::get<1ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&)
 [232] __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::~new_allocator() [145] std::_Vector_base<int, std::allocator<int> >::_Vector_impl_data::_M_copy_data(std::_Vector_base<int, std::allocator<int> >::_Vector_impl_data const&) [165] unsigned long const& std::max<unsigned long>(unsigned long const&, unsigned long const&)
 [186] __gnu_cxx::new_allocator<int>::deallocate(int*, unsigned long) [181] std::_Vector_base<int, std::allocator<int> >::_Vector_impl_data::_M_swap_data(std::_Vector_base<int, std::allocator<int> >::_Vector_impl_data&) [137] unsigned long const& std::min<unsigned long>(unsigned long const&, unsigned long const&)
 [224] __gnu_cxx::new_allocator<int>::allocate(unsigned long, void const*) [146] std::_Vector_base<int, std::allocator<int> >::_Vector_impl_data::_Vector_impl_data() [221] int* std::copy<__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, int*>(__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, __gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, int*)
 [135] void __gnu_cxx::new_allocator<int>::construct<int, int const&>(int*, int const&) [136] std::_Vector_base<int, std::allocator<int> >::_M_get_Tp_allocator() [116] std::remove_reference<node&>::type&& std::move<node&>(node&)
 [173] __gnu_cxx::new_allocator<int>::new_allocator() [203] std::_Vector_base<int, std::allocator<int> >::_Vector_base(std::allocator<int> const&) [263] std::remove_reference<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&>::type&& std::move<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&)
 [149] __gnu_cxx::new_allocator<int>::new_allocator(__gnu_cxx::new_allocator<int> const&) [182] std::_Vector_base<int, std::allocator<int> >::_Vector_base() [305] std::remove_reference<std::filesystem::__cxx11::path::_List::_Impl*&>::type&& std::move<std::filesystem::__cxx11::path::_List::_Impl*&>(std::filesystem::__cxx11::path::_List::_Impl*&)
 [143] __gnu_cxx::new_allocator<int>::~new_allocator() [204] std::_Vector_base<int, std::allocator<int> >::_Vector_base(unsigned long, std::allocator<int> const&) [222] std::remove_reference<std::allocator<int>&>::type&& std::move<std::allocator<int>&>(std::allocator<int>&)
 [192] __gnu_cxx::__alloc_traits<std::allocator<int>, int>::_S_select_on_copy(std::allocator<int> const&) [154] std::_Vector_base<int, std::allocator<int> >::~_Vector_base() [223] std::remove_reference<std::vector<int, std::allocator<int> >&>::type&& std::move<std::vector<int, std::allocator<int> >&>(std::vector<int, std::allocator<int> >&)
 [123] bool __gnu_cxx::__is_null_pointer<char const>(char const*) [296] std::__uniq_ptr_impl<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_deleter() [55] node&& std::forward<node>(std::remove_reference<node>::type&)
 [168] bool __gnu_cxx::__is_null_pointer<char>(char*) [297] std::__uniq_ptr_impl<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_ptr() [237] std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const& std::forward<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&>(std::remove_reference<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&>::type&)
 [115] __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >::__normal_iterator(node* const&) [167] std::allocator_traits<std::allocator<node> >::deallocate(std::allocator<node>&, node*, unsigned long) [119] int const& std::forward<int const&>(std::remove_reference<int const&>::type&)
 [174] __gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >::__normal_iterator(int const* const&) [32] void std::allocator_traits<std::allocator<node> >::destroy<node>(std::allocator<node>&, node*) [306] void std::_Destroy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*)
 [147] __gnu_cxx::__normal_iterator<int*, std::vector<int, std::allocator<int> > >::__normal_iterator(int* const&) [163] std::allocator_traits<std::allocator<node> >::allocate(std::allocator<node>&, unsigned long) [307] void std::_Destroy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >&)
 [128] __gnu_cxx::__normal_iterator<int*, std::vector<int, std::allocator<int> > >::operator++() [141] std::allocator_traits<std::allocator<node> >::max_size(std::allocator<node> const&) [156] void std::_Destroy<int*>(int*, int*)
  [20] int __gnu_cxx::__stoa<long, int, char, int>(long (*)(char const*, char**, int), char const*, char const*, unsigned long*, int) [28] void std::allocator_traits<std::allocator<node> >::construct<node, node>(std::allocator<node>&, node*, node&&) [157] void std::_Destroy<int*, int>(int*, int*, std::allocator<int>&)
 [159] __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >::difference_type __gnu_cxx::operator-<node*, std::vector<node, std::allocator<node> > >(__gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > > const&, __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > > const&) [252] std::allocator_traits<std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::allocate(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >&, unsigned long) [264] std::iterator_traits<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>::difference_type std::distance<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*)
 [127] bool __gnu_cxx::operator!=<int*, std::vector<int, std::allocator<int> > >(__gnu_cxx::__normal_iterator<int*, std::vector<int, std::allocator<int> > > const&, __gnu_cxx::__normal_iterator<int*, std::vector<int, std::allocator<int> > > const&) [253] std::allocator_traits<std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::max_size(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [126] std::iterator_traits<char const*>::difference_type std::distance<char const*>(char const*, char const*)
 [134] __gnu_cxx::new_allocator<node>::max_size() const [187] std::allocator_traits<std::allocator<int> >::deallocate(std::allocator<int>&, int*, unsigned long) [172] std::iterator_traits<char*>::difference_type std::distance<char*>(char*, char*)
 [239] __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::max_size() const [205] std::allocator_traits<std::allocator<int> >::select_on_container_copy_construction(std::allocator<int> const&) [30] bool std::operator==<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&, char const*)
 [225] __gnu_cxx::new_allocator<int>::max_size() const [226] std::allocator_traits<std::allocator<int> >::allocate(std::allocator<int>&, unsigned long) [54] bool std::operator!=<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&, char const*)
  [56] __gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > >::base() const [23] void std::allocator_traits<std::allocator<int> >::construct<int, int const&>(std::allocator<int>&, int*, int const&) [265] std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&)
 [121] __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >::base() const [254] std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::__uninitialized_copy<false>::__uninit_copy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*) [57] std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(char const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&)
 [129] __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >::operator*() const [206] int* std::__uninitialized_copy<true>::__uninit_copy<__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, int*>(__gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, __gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >, int*) [267] std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&, char const*)
 [130] __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >::operator-(long) const [228] void std::deque<int, std::allocator<int> >::_M_push_back_aux<int const&>(int const&) [133] __gnu_cxx::__stoa<long, int, char, int>(long (*)(char const*, char**, int), char const*, char const*, unsigned long*, int)::_Range_chk::_S_chk(long, std::integral_constant<bool, true>)
 [175] __gnu_cxx::__normal_iterator<int const*, std::vector<int, std::allocator<int> > >::base() const [142] std::vector<node, std::allocator<node> >::_S_max_size(std::allocator<node> const&) [34] __gnu_cxx::__stoa<long, int, char, int>(long (*)(char const*, char**, int), char const*, char const*, unsigned long*, int)::_Save_errno::_Save_errno()
 [117] __gnu_cxx::__normal_iterator<int*, std::vector<int, std::allocator<int> > >::base() const [16] std::vector<node, std::allocator<node> >::_S_relocate(node*, node*, node*, std::allocator<node>&) [35] __gnu_cxx::__stoa<long, int, char, int>(long (*)(char const*, char**, int), char const*, char const*, unsigned long*, int)::_Save_errno::~_Save_errno()
 [131] __gnu_cxx::__normal_iterator<int*, std::vector<int, std::allocator<int> > >::operator*() const [11] node& std::vector<node, std::allocator<node> >::emplace_back<node>(node&&) [114] operator new(unsigned long, void*)
 [266] std::filesystem::file_status::type() const [17] std::vector<node, std::allocator<node> >::_S_do_relocate(node*, node*, node*, std::allocator<node>&, std::integral_constant<bool, true>) [41] __config_getraw (config.c)
 [138] std::_Vector_base<node, std::allocator<node> >::_M_get_Tp_allocator() const [13] void std::vector<node, std::allocator<node> >::_M_realloc_insert<node>(__gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >, node&&) [42] __curfile_insert (cur_file.c)
 [193] std::_Vector_base<int, std::allocator<int> >::get_allocator() const [120] std::vector<node, std::allocator<node> >::end() [43] __curfile_search (cur_file.c)
 [176] std::_Vector_base<int, std::allocator<int> >::_M_get_Tp_allocator() const [132] std::vector<node, std::allocator<node> >::back() [24] __cursor_func_init.constprop.0 (cursor.i)
 [247] std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::end() const [164] std::vector<node, std::allocator<node> >::begin() [3] __global_once (global.c)
 [248] std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::size() const [12] std::vector<node, std::allocator<node> >::push_back(node&&) [25] __wt_btcur_close
 [240] std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::begin() const [207] std::vector<node, std::allocator<node> >::vector() [44] __wt_btcur_search
 [276] std::ctype<char>::do_widen(char) const [255] std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_S_max_size(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [45] __wt_calloc
 [160] std::vector<node, std::allocator<node> >::_M_check_len(unsigned long, char const*) const [245] void std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_realloc_insert<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >(__gnu_cxx::__normal_iterator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > > >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&) [46] __wt_cursor_set_keyv
 [122] std::vector<node, std::allocator<node> >::size() const [256] std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_S_check_init_len(unsigned long, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [47] __wt_curtable_get_value
 [139] std::vector<node, std::allocator<node> >::max_size() const [257] void std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_range_initialize<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::forward_iterator_tag) [48] __wt_curtable_set_key
 [277] std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::size() const [298] std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::vector() [1] __wt_hazard_clear
 [194] std::vector<int, std::allocator<int> >::end() const [258] std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::vector(std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [2] __wt_hazard_set
 [177] std::vector<int, std::allocator<int> >::size() const [299] std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::~vector() [22] __wt_page_in_func
 [195] std::vector<int, std::allocator<int> >::begin() const [208] std::vector<int, std::allocator<int> >::_M_move_assign(std::vector<int, std::allocator<int> >&&, std::integral_constant<bool, true>) [49] __wt_readunlock
  [29] std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::compare(char const*) const [36] void std::vector<int, std::allocator<int> >::_M_realloc_insert<int const&>(__gnu_cxx::__normal_iterator<int*, std::vector<int, std::allocator<int> > >, int const&) [6] __wt_row_search
 [196] std::allocator<node>::allocator() [148] std::vector<int, std::allocator<int> >::end() [9] __wt_schema_project_out
 [241] std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::allocator(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [209] std::vector<int, std::allocator<int> >::begin() [50] __wt_session_gen_leave
 [243] std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::allocator() [14] std::vector<int, std::allocator<int> >::push_back(int const&) [51] __wt_session_release_dhandle
 [233] std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::~allocator() [210] std::vector<int, std::allocator<int> >::vector(std::allocator<int> const&) [26] __wt_session_strerror
 [150] std::allocator<int>::allocator(std::allocator<int> const&) [211] std::vector<int, std::allocator<int> >::vector(std::vector<int, std::allocator<int> > const&) [52] __wt_struct_sizev (packing.i)
 [178] std::allocator<int>::allocator() [183] std::vector<int, std::allocator<int> >::vector() [53] __wt_value_return_upd
