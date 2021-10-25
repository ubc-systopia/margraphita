Flat profile:

Each sample counts as 0.01 seconds.
  %   cumulative   self              self     total           
 time   seconds   seconds    calls  us/call  us/call  name    
 52.09      1.87     1.87                             __wt_hazard_clear
  8.91      2.19     0.32                             __global_once
  4.18      2.34     0.15                             __wt_row_search
  3.06      2.45     0.11                             __wt_schema_project_slice
  2.79      2.55     0.10                             __wt_schema_project_out
  1.67      2.61     0.06                             bfs_info* bfs<EdgeKey>(EdgeKey&, int)
  1.67      2.67     0.06                             __wt_config_next
  1.39      2.72     0.05  3035193     0.02     0.02  std::char_traits<char>::length(char const*)
  1.11      2.76     0.04                             __wt_btcur_close
  1.11      2.80     0.04                             __wt_btcur_next
  1.11      2.84     0.04                             __wt_hazard_set
  1.11      2.88     0.04                             __wt_struct_packv
  1.11      2.92     0.04                             __wt_struct_sizev
  0.84      2.95     0.03                             __wt_curfile_open
  0.84      2.98     0.03                             __wt_cursor_close
  0.84      3.01     0.03                             __wt_cursor_set_keyv
  0.84      3.04     0.03                             __wt_schema_open_table
  0.56      3.06     0.02  2562751     0.01     0.03  std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::basic_string<std::allocator<char> >(char const*, std::allocator<char> const&)
  0.56      3.08     0.02  1281358     0.02     0.08  EdgeKey::__record_to_node(__wt_cursor*, node*)
  0.56      3.10     0.02                             __curfile_search
  0.56      3.12     0.02                             __curindex_next
  0.56      3.14     0.02                             __session_open_cursor_int
  0.56      3.16     0.02                             __wt_btcur_search
  0.56      3.18     0.02                             __wt_config_gets_def
  0.56      3.20     0.02                             __wt_cursor_cache_get
  0.56      3.22     0.02                             __wt_malloc
  0.56      3.24     0.02                             __wt_metadata_cursor
  0.56      3.26     0.02                             __wt_page_in_func
  0.28      3.27     0.01  2562751     0.00     0.00  std::iterator_traits<char const*>::difference_type std::__distance<char const*>(char const*, char const*, std::random_access_iterator_tag)
  0.28      3.28     0.01  1615870     0.01     0.01  void __gnu_cxx::new_allocator<node>::construct<node, node>(node*, node&&)
  0.28      3.29     0.01  1615870     0.01     0.01  void std::allocator_traits<std::allocator<node> >::construct<node, node>(std::allocator<node>&, node*, node&&)
  0.28      3.30     0.01  1615870     0.01     0.02  void std::__relocate_object_a<node, node, std::allocator<node> >(node*, node*, std::allocator<node>&)
  0.28      3.31     0.01  1615870     0.01     0.01  std::remove_reference<node&>::type&& std::move<node&>(node&)
  0.28      3.32     0.01  1281358     0.01     0.01  EdgeKey::extract_from_string(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, int*, int*)
  0.28      3.33     0.01  1281320     0.01     0.01  void __gnu_cxx::new_allocator<node>::construct<node, node const&>(node*, node const&)
  0.28      3.34     0.01   763600     0.01     0.08  std::vector<node, std::allocator<node> >::_S_relocate(node*, node*, node*, std::allocator<node>&)
  0.28      3.35     0.01   763600     0.01     0.07  node* std::__relocate_a_1<node*, node*, std::allocator<node> >(node*, node*, node*, std::allocator<node>&)
  0.28      3.36     0.01   236219     0.04     0.06  EdgeKey::get_edge_cursor()
  0.28      3.37     0.01   118100     0.08     1.69  EdgeKey::get_out_nodes(int)
  0.28      3.38     0.01                             std::vector<node, std::allocator<node> >::~vector()
  0.28      3.39     0.01                             __curindex_get_value
  0.28      3.40     0.01                             __curindex_move
  0.28      3.41     0.01                             __curtable_search
  0.28      3.42     0.01                             __pack_write
  0.28      3.43     0.01                             __schema_open_index
  0.28      3.44     0.01                             __unpack_read
  0.28      3.45     0.01                             __wt_btcur_iterate_setup
  0.28      3.46     0.01                             __wt_btcur_search_near
  0.28      3.47     0.01                             __wt_clock_to_nsec
  0.28      3.48     0.01                             __wt_curtable_get_key
  0.28      3.49     0.01                             __wt_curtable_get_value
  0.28      3.50     0.01                             __wt_curtable_open
  0.28      3.51     0.01                             __wt_curtable_set_key
  0.28      3.52     0.01                             __wt_readlock
  0.28      3.53     0.01                             __wt_readunlock
  0.28      3.54     0.01                             __wt_realloc_noclear
  0.28      3.55     0.01                             __wt_schema_open_index
  0.28      3.56     0.01                             __wt_session_gen_leave
  0.28      3.57     0.01                             __wt_strndup
  0.28      3.58     0.01                             __wt_writelock
  0.28      3.59     0.01                             wiredtiger_crc32c_func
  0.00      3.59     0.00  4847610     0.00     0.00  node* std::__addressof<node>(node&)
  0.00      3.59     0.00  3231740     0.00     0.00  node&& std::forward<node>(std::remove_reference<node>::type&)
  0.00      3.59     0.00  2944440     0.00     0.00  node const& std::forward<node const&>(std::remove_reference<node const&>::type&)
  0.00      3.59     0.00  2897193     0.00     0.00  operator new(unsigned long, void*)
  0.00      3.59     0.00  2562751     0.00     0.00  bool __gnu_cxx::__is_null_pointer<char const>(char const*)
  0.00      3.59     0.00  2562751     0.00     0.00  void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char const*>(char const*, char const*)
  0.00      3.59     0.00  2562751     0.00     0.00  void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char const*>(char const*, char const*, std::forward_iterator_tag)
  0.00      3.59     0.00  2562751     0.00     0.00  void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct_aux<char const*>(char const*, char const*, std::__false_type)
  0.00      3.59     0.00  2562751     0.00     0.00  std::iterator_traits<char const*>::iterator_category std::__iterator_category<char const*>(char const* const&)
  0.00      3.59     0.00  2562751     0.00     0.00  std::iterator_traits<char const*>::difference_type std::distance<char const*>(char const*, char const*)
  0.00      3.59     0.00  2290800     0.00     0.00  node* std::__niter_base<node*>(node*)
  0.00      3.59     0.00  1615870     0.00     0.00  void __gnu_cxx::new_allocator<node>::destroy<node>(node*)
  0.00      3.59     0.00  1615870     0.00     0.00  void std::allocator_traits<std::allocator<node> >::destroy<node>(std::allocator<node>&, node*)
  0.00      3.59     0.00  1527200     0.00     0.00  __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >::base() const
  0.00      3.59     0.00  1527200     0.00     0.00  std::vector<node, std::allocator<node> >::size() const
  0.00      3.59     0.00  1281358     0.00     0.00  std::operator|(std::_Ios_Openmode, std::_Ios_Openmode)
  0.00      3.59     0.00  1281339     0.00     0.00  node::node()
  0.00      3.59     0.00  1281320     0.00     0.01  void std::allocator_traits<std::allocator<node> >::construct<node, node const&>(std::allocator<node>&, node*, node const&)
  0.00      3.59     0.00  1281320     0.00     0.05  std::vector<node, std::allocator<node> >::push_back(node const&)
  0.00      3.59     0.00  1145400     0.00     0.00  __gnu_cxx::new_allocator<node>::max_size() const
  0.00      3.59     0.00   763601     0.00     0.00  unsigned long const& std::min<unsigned long>(unsigned long const&, unsigned long const&)
  0.00      3.59     0.00   763600     0.00     0.00  __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >::__normal_iterator(node* const&)
  0.00      3.59     0.00   763600     0.00     0.00  std::_Vector_base<node, std::allocator<node> >::_M_get_Tp_allocator() const
  0.00      3.59     0.00   763600     0.00     0.00  std::vector<node, std::allocator<node> >::max_size() const
  0.00      3.59     0.00   763600     0.00     0.00  std::_Vector_base<node, std::allocator<node> >::_M_get_Tp_allocator()
  0.00      3.59     0.00   763600     0.00     0.00  std::allocator_traits<std::allocator<node> >::max_size(std::allocator<node> const&)
  0.00      3.59     0.00   763600     0.00     0.00  std::vector<node, std::allocator<node> >::_S_max_size(std::allocator<node> const&)
  0.00      3.59     0.00   763600     0.00     0.07  std::vector<node, std::allocator<node> >::_S_do_relocate(node*, node*, node*, std::allocator<node>&, std::integral_constant<bool, true>)
  0.00      3.59     0.00   763600     0.00     0.07  node* std::__relocate_a<node*, node*, std::allocator<node> >(node*, node*, node*, std::allocator<node>&)
  0.00      3.59     0.00   708604     0.00     0.00  std::remove_reference<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&>::type&& std::move<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&)
  0.00      3.59     0.00   590547     0.00     0.00  bool __gnu_cxx::__is_null_pointer<char>(char*)
  0.00      3.59     0.00   590547     0.00     0.00  void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char*>(char*, char*, std::forward_iterator_tag)
  0.00      3.59     0.00   590547     0.00     0.00  std::iterator_traits<char*>::difference_type std::__distance<char*>(char*, char*, std::random_access_iterator_tag)
  0.00      3.59     0.00   590547     0.00     0.00  std::iterator_traits<char*>::iterator_category std::__iterator_category<char*>(char* const&)
  0.00      3.59     0.00   590547     0.00     0.00  std::iterator_traits<char*>::difference_type std::distance<char*>(char*, char*)
  0.00      3.59     0.00   472442     0.00     0.02  std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(char const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&)
  0.00      3.59     0.00   381800     0.00     0.00  __gnu_cxx::new_allocator<node>::allocate(unsigned long, void const*)
  0.00      3.59     0.00   381800     0.00     0.00  __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >::difference_type __gnu_cxx::operator-<node*, std::vector<node, std::allocator<node> > >(__gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > > const&, __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > > const&)
  0.00      3.59     0.00   381800     0.00     0.00  std::vector<node, std::allocator<node> >::_M_check_len(unsigned long, char const*) const
  0.00      3.59     0.00   381800     0.00     0.00  std::_Vector_base<node, std::allocator<node> >::_M_allocate(unsigned long)
  0.00      3.59     0.00   381800     0.00     0.00  std::_Vector_base<node, std::allocator<node> >::_M_deallocate(node*, unsigned long)
  0.00      3.59     0.00   381800     0.00     0.00  std::allocator_traits<std::allocator<node> >::allocate(std::allocator<node>&, unsigned long)
  0.00      3.59     0.00   381800     0.00     0.16  void std::vector<node, std::allocator<node> >::_M_realloc_insert<node const&>(__gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >, node const&)
  0.00      3.59     0.00   381800     0.00     0.00  std::vector<node, std::allocator<node> >::end()
  0.00      3.59     0.00   381800     0.00     0.00  std::vector<node, std::allocator<node> >::begin()
  0.00      3.59     0.00   381800     0.00     0.00  unsigned long const& std::max<unsigned long>(unsigned long const&, unsigned long const&)
  0.00      3.59     0.00   354303     0.00     0.00  std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&)
  0.00      3.59     0.00   354300     0.00     0.00  std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&, char const*)
  0.00      3.59     0.00   279060     0.00     0.00  __gnu_cxx::new_allocator<node>::deallocate(node*, unsigned long)
  0.00      3.59     0.00   279060     0.00     0.00  std::allocator_traits<std::allocator<node> >::deallocate(std::allocator<node>&, node*, unsigned long)
  0.00      3.59     0.00   236239     0.00     0.02  EdgeKey::_get_table_cursor(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, __wt_cursor**, bool)
  0.00      3.59     0.00   118100     0.00     0.03  EdgeKey::get_src_idx_cur()
  0.00      3.59     0.00   118100     0.00     0.02  EdgeKey::_get_index_cursor(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, __wt_cursor**)
  0.00      3.59     0.00   118100     0.00     0.06  EdgeKey::has_node(int)
  0.00      3.59     0.00   118100     0.00     0.00  __gnu_cxx::new_allocator<node>::new_allocator()
  0.00      3.59     0.00   118100     0.00     0.00  std::allocator<node>::allocator()
  0.00      3.59     0.00   118100     0.00     0.00  std::_Vector_base<node, std::allocator<node> >::_Vector_impl::_Vector_impl()
  0.00      3.59     0.00   118100     0.00     0.00  std::_Vector_base<node, std::allocator<node> >::_Vector_impl_data::_Vector_impl_data()
  0.00      3.59     0.00   118100     0.00     0.00  std::_Vector_base<node, std::allocator<node> >::_Vector_base()
  0.00      3.59     0.00   118100     0.00     0.00  std::vector<node, std::allocator<node> >::vector()
  0.00      3.59     0.00     1760     0.00     0.00  void std::vector<int, std::allocator<int> >::_M_realloc_insert<int const&>(__gnu_cxx::__normal_iterator<int*, std::vector<int, std::allocator<int> > >, int const&)
  0.00      3.59     0.00     1170     0.00     0.00  std::_Rb_tree<int, int, std::_Identity<int>, std::less<int>, std::allocator<int> >::_M_erase(std::_Rb_tree_node<int>*)
  0.00      3.59     0.00      800     0.00     0.00  void std::deque<int, std::allocator<int> >::_M_push_back_aux<int const&>(int const&)
  0.00      3.59     0.00      160     0.00     0.00  std::_Deque_base<int, std::allocator<int> >::_M_initialize_map(unsigned long)
  0.00      3.59     0.00       19     0.00     0.14  EdgeKey::get_out_degree(int)
  0.00      3.59     0.00       19     0.00     0.10  EdgeKey::get_random_node()
  0.00      3.59     0.00       18     0.00     0.00  std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_mutate(unsigned long, unsigned long, char const*, unsigned long)
  0.00      3.59     0.00        5     0.00     0.00  void std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_realloc_insert<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >(__gnu_cxx::__normal_iterator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > > >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&)
  0.00      3.59     0.00        3     0.00     0.00  __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::~new_allocator()
  0.00      3.59     0.00        3     0.00     0.00  std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::~allocator()
  0.00      3.59     0.00        3     0.00     0.00  std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_get_Tp_allocator()
  0.00      3.59     0.00        3     0.00     0.00  void std::_Construct<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&)
  0.00      3.59     0.00        3     0.00     0.00  std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::__addressof<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&)
  0.00      3.59     0.00        3     0.00     0.00  std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const& std::forward<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&>(std::remove_reference<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&>::type&)
  0.00      3.59     0.00        2     0.00     0.00  __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::new_allocator(__gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&)
  0.00      3.59     0.00        2     0.00     0.00  __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::new_allocator()
  0.00      3.59     0.00        2     0.00     0.00  __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::max_size() const
  0.00      3.59     0.00        2     0.00     0.00  std::filesystem::file_status::type() const
  0.00      3.59     0.00        2     0.00     0.00  std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::begin() const
  0.00      3.59     0.00        2     0.00     0.00  std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::allocator(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&)
  0.00      3.59     0.00        2     0.00     0.00  std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::allocator()
  0.00      3.59     0.00        2     0.00     0.00  std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl_data::_Vector_impl_data()
  0.00      3.59     0.00        2     0.00     0.00  std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&, char const*)
  0.00      3.59     0.00        1     0.00     0.00  _GLOBAL__sub_I__Z14print_csv_infoNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEiP8bfs_info
  0.00      3.59     0.00        1     0.00     0.89  _GLOBAL__sub_I__Z8METADATAB5cxx11
  0.00      3.59     0.00        1     0.00     0.03  _GLOBAL__sub_I__ZN13StandardGraphC2Ev
  0.00      3.59     0.00        1     0.00     0.03  _GLOBAL__sub_I__ZN7AdjListC2Ev
  0.00      3.59     0.00        1     0.00     0.03  _GLOBAL__sub_I__ZN7EdgeKeyC2Ev
  0.00      3.59     0.00        1     0.00     0.03  __static_initialization_and_destruction_0(int, int)
  0.00      3.59     0.00        1     0.00     0.89  __static_initialization_and_destruction_0(int, int)
  0.00      3.59     0.00        1     0.00     0.03  __static_initialization_and_destruction_0(int, int)
  0.00      3.59     0.00        1     0.00     0.03  __static_initialization_and_destruction_0(int, int)
  0.00      3.59     0.00        1     0.00     0.00  CommonUtil::open_session(__wt_connection*, __wt_session**)
  0.00      3.59     0.00        1     0.00     0.00  CommonUtil::close_session(__wt_session*)
  0.00      3.59     0.00        1     0.00     0.00  CommonUtil::open_connection(char*, __wt_connection**)
  0.00      3.59     0.00        1     0.00     0.00  CommonUtil::close_connection(__wt_connection*)
  0.00      3.59     0.00        1     0.00     0.00  CommonUtil::check_graph_params(graph_opts)
  0.00      3.59     0.00        1     0.00     0.00  graph_opts::graph_opts(graph_opts const&)
  0.00      3.59     0.00        1     0.00     0.00  graph_opts::~graph_opts()
  0.00      3.59     0.00        1     0.00     0.00  CmdLineBase::CmdLineBase(int, char**)
  0.00      3.59     0.00        1     0.00     0.02  EdgeKey::__restore_from_db(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >)
  0.00      3.59     0.00        1     0.00     0.00  __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::allocate(unsigned long, void const*)
  0.00      3.59     0.00        1     0.00     0.00  std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::end() const
  0.00      3.59     0.00        1     0.00     0.00  std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::size() const
  0.00      3.59     0.00        1     0.00     0.00  std::ctype<char>::do_widen(char) const
  0.00      3.59     0.00        1     0.00     0.00  std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::size() const
  0.00      3.59     0.00        1     0.00     0.00  std::_Head_base<0ul, std::filesystem::__cxx11::path::_List::_Impl*, false>::_M_head(std::_Head_base<0ul, std::filesystem::__cxx11::path::_List::_Impl*, false>&)
  0.00      3.59     0.00        1     0.00     0.00  std::_Head_base<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter, true>::_M_head(std::_Head_base<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter, true>&)
  0.00      3.59     0.00        1     0.00     0.00  std::filesystem::status_known(std::filesystem::file_status)
  0.00      3.59     0.00        1     0.00     0.00  std::filesystem::exists(std::filesystem::file_status)
  0.00      3.59     0.00        1     0.00     0.00  std::filesystem::exists(std::filesystem::__cxx11::path const&)
  0.00      3.59     0.00        1     0.00     0.00  std::filesystem::__cxx11::path::_List::~_List()
  0.00      3.59     0.00        1     0.00     0.00  std::filesystem::__cxx11::path::path(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&, std::filesystem::__cxx11::path::format)
  0.00      3.59     0.00        1     0.00     0.00  std::filesystem::__cxx11::path::~path()
  0.00      3.59     0.00        1     0.00     0.00  std::unique_ptr<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::get_deleter()
  0.00      3.59     0.00        1     0.00     0.00  std::unique_ptr<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::~unique_ptr()
  0.00      3.59     0.00        1     0.00     0.00  std::_Tuple_impl<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_head(std::_Tuple_impl<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&)
  0.00      3.59     0.00        1     0.00     0.00  std::_Tuple_impl<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_head(std::_Tuple_impl<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>&)
  0.00      3.59     0.00        1     0.00     0.00  void std::_Destroy_aux<false>::__destroy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*)
  0.00      3.59     0.00        1     0.00     0.00  std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_allocate(unsigned long)
  0.00      3.59     0.00        1     0.00     0.00  std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl::_Vector_impl(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&)
  0.00      3.59     0.00        1     0.00     0.00  std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl::_Vector_impl()
  0.00      3.59     0.00        1     0.00     0.00  std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl::~_Vector_impl()
  0.00      3.59     0.00        1     0.00     0.00  std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_deallocate(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, unsigned long)
  0.00      3.59     0.00        1     0.00     0.00  std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_base()
  0.00      3.59     0.00        1     0.00     0.00  std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_base(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&)
  0.00      3.59     0.00        1     0.00     0.00  std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::~_Vector_base()
  0.00      3.59     0.00        1     0.00     0.00  std::__uniq_ptr_impl<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_deleter()
  0.00      3.59     0.00        1     0.00     0.00  std::__uniq_ptr_impl<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_ptr()
  0.00      3.59     0.00        1     0.00     0.00  std::allocator_traits<std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::allocate(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >&, unsigned long)
  0.00      3.59     0.00        1     0.00     0.00  std::allocator_traits<std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::max_size(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&)
  0.00      3.59     0.00        1     0.00     0.00  std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::__uninitialized_copy<false>::__uninit_copy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*)
  0.00      3.59     0.00        1     0.00     0.00  std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_S_max_size(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&)
  0.00      3.59     0.00        1     0.00     0.00  std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_S_check_init_len(unsigned long, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&)
  0.00      3.59     0.00        1     0.00     0.00  void std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_range_initialize<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::forward_iterator_tag)
  0.00      3.59     0.00        1     0.00     0.00  std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::vector()
  0.00      3.59     0.00        1     0.00     0.00  std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::vector(std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&)
  0.00      3.59     0.00        1     0.00     0.00  std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::~vector()
  0.00      3.59     0.00        1     0.00     0.00  std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_replace(unsigned long, unsigned long, char const*, unsigned long)
  0.00      3.59     0.00        1     0.00     0.00  std::iterator_traits<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>::difference_type std::__distance<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::random_access_iterator_tag)
  0.00      3.59     0.00        1     0.00     0.00  std::filesystem::__cxx11::path::_List::_Impl*& std::__get_helper<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::_Tuple_impl<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&)
  0.00      3.59     0.00        1     0.00     0.00  std::filesystem::__cxx11::path::_List::_Impl_deleter& std::__get_helper<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::_Tuple_impl<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>&)
  0.00      3.59     0.00        1     0.00     0.00  std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::uninitialized_copy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*)
  0.00      3.59     0.00        1     0.00     0.00  std::iterator_traits<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>::iterator_category std::__iterator_category<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const* const&)
  0.00      3.59     0.00        1     0.00     0.00  std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::__uninitialized_copy_a<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >&)
  0.00      3.59     0.00        1     0.00     0.00  std::tuple_element<0ul, std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter> >::type& std::get<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&)
  0.00      3.59     0.00        1     0.00     0.00  std::tuple_element<1ul, std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter> >::type& std::get<1ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&)
  0.00      3.59     0.00        1     0.00     0.00  std::remove_reference<std::filesystem::__cxx11::path::_List::_Impl*&>::type&& std::move<std::filesystem::__cxx11::path::_List::_Impl*&>(std::filesystem::__cxx11::path::_List::_Impl*&)
  0.00      3.59     0.00        1     0.00     0.00  void std::_Destroy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*)
  0.00      3.59     0.00        1     0.00     0.00  void std::_Destroy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >&)
  0.00      3.59     0.00        1     0.00     0.00  std::iterator_traits<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>::difference_type std::distance<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*)

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


granularity: each sample hit covers 2 byte(s) for 0.28% of 3.59 seconds

index % time    self  children    called     name
                                                 <spontaneous>
[1]     52.1    1.87    0.00                 __wt_hazard_clear [1]
-----------------------------------------------
                                                 <spontaneous>
[2]      8.9    0.32    0.00                 __global_once [2]
-----------------------------------------------
                                                 <spontaneous>
[3]      7.2    0.06    0.20                 bfs_info* bfs<EdgeKey>(EdgeKey&, int) [3]
                0.01    0.19  118100/118100      EdgeKey::get_out_nodes(int) [4]
                0.00    0.00    1760/1760        void std::vector<int, std::allocator<int> >::_M_realloc_insert<int const&>(__gnu_cxx::__normal_iterator<int*, std::vector<int, std::allocator<int> > >, int const&) [172]
                0.00    0.00    1170/1170        std::_Rb_tree<int, int, std::_Identity<int>, std::less<int>, std::allocator<int> >::_M_erase(std::_Rb_tree_node<int>*) [173]
                0.00    0.00     800/800         void std::deque<int, std::allocator<int> >::_M_push_back_aux<int const&>(int const&) [174]
                0.00    0.00     160/160         std::_Deque_base<int, std::allocator<int> >::_M_initialize_map(unsigned long) [175]
-----------------------------------------------
                0.01    0.19  118100/118100      bfs_info* bfs<EdgeKey>(EdgeKey&, int) [3]
[4]      5.6    0.01    0.19  118100         EdgeKey::get_out_nodes(int) [4]
                0.02    0.08 1281320/1281358     EdgeKey::__record_to_node(__wt_cursor*, node*) [7]
                0.00    0.07 1281320/1281320     std::vector<node, std::allocator<node> >::push_back(node const&) [10]
                0.00    0.00  118100/236219      EdgeKey::get_edge_cursor() [38]
                0.00    0.01  118100/118100      EdgeKey::has_node(int) [72]
                0.00    0.00  118100/118100      EdgeKey::get_src_idx_cur() [74]
                0.00    0.00 1281320/1281339     node::node() [138]
                0.00    0.00  118100/118100      std::vector<node, std::allocator<node> >::vector() [171]
-----------------------------------------------
                                                 <spontaneous>
[5]      4.2    0.15    0.00                 __wt_row_search [5]
-----------------------------------------------
                                                 <spontaneous>
[6]      3.1    0.11    0.00                 __wt_schema_project_slice [6]
-----------------------------------------------
                0.00    0.00      19/1281358     EdgeKey::get_random_node() [78]
                0.00    0.00      19/1281358     EdgeKey::get_out_degree(int) [77]
                0.02    0.08 1281320/1281358     EdgeKey::get_out_nodes(int) [4]
[7]      2.8    0.02    0.08 1281358         EdgeKey::__record_to_node(__wt_cursor*, node*) [7]
                0.02    0.05 2562716/2562751     std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::basic_string<std::allocator<char> >(char const*, std::allocator<char> const&) [9]
                0.01    0.00 1281358/1281358     EdgeKey::extract_from_string(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, int*, int*) [47]
-----------------------------------------------
                                                 <spontaneous>
[8]      2.8    0.10    0.00                 __wt_schema_project_out [8]
-----------------------------------------------
                0.00    0.00       1/2562751     __static_initialization_and_destruction_0(int, int) [86]
                0.00    0.00       1/2562751     __static_initialization_and_destruction_0(int, int) [87]
                0.00    0.00       1/2562751     __static_initialization_and_destruction_0(int, int) [88]
                0.00    0.00       2/2562751     EdgeKey::EdgeKey(graph_opts) [82]
                0.00    0.00      30/2562751     __static_initialization_and_destruction_0(int, int) [81]
                0.02    0.05 2562716/2562751     EdgeKey::__record_to_node(__wt_cursor*, node*) [7]
[9]      2.0    0.02    0.05 2562751         std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::basic_string<std::allocator<char> >(char const*, std::allocator<char> const&) [9]
                0.04    0.00 2562751/3035193     std::char_traits<char>::length(char const*) [14]
                0.00    0.01 2562751/2562751     void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char const*>(char const*, char const*) [40]
-----------------------------------------------
                0.00    0.07 1281320/1281320     EdgeKey::get_out_nodes(int) [4]
[10]     1.9    0.00    0.07 1281320         std::vector<node, std::allocator<node> >::push_back(node const&) [10]
                0.00    0.06  381800/381800      void std::vector<node, std::allocator<node> >::_M_realloc_insert<node const&>(__gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >, node const&) [11]
                0.00    0.01  899520/1281320     void std::allocator_traits<std::allocator<node> >::construct<node, node const&>(std::allocator<node>&, node*, node const&) [49]
                0.00    0.00  381800/381800      std::vector<node, std::allocator<node> >::end() [159]
-----------------------------------------------
                0.00    0.06  381800/381800      std::vector<node, std::allocator<node> >::push_back(node const&) [10]
[11]     1.8    0.00    0.06  381800         void std::vector<node, std::allocator<node> >::_M_realloc_insert<node const&>(__gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >, node const&) [11]
                0.01    0.05  763600/763600      std::vector<node, std::allocator<node> >::_S_relocate(node*, node*, node*, std::allocator<node>&) [12]
                0.00    0.00  381800/1281320     void std::allocator_traits<std::allocator<node> >::construct<node, node const&>(std::allocator<node>&, node*, node const&) [49]
                0.00    0.00  763600/763600      std::_Vector_base<node, std::allocator<node> >::_M_get_Tp_allocator() [144]
                0.00    0.00  763600/1527200     __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >::base() const [135]
                0.00    0.00  381800/381800      std::vector<node, std::allocator<node> >::_M_check_len(unsigned long, char const*) const [155]
                0.00    0.00  381800/381800      std::vector<node, std::allocator<node> >::begin() [160]
                0.00    0.00  381800/381800      __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >::difference_type __gnu_cxx::operator-<node*, std::vector<node, std::allocator<node> > >(__gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > > const&, __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > > const&) [154]
                0.00    0.00  381800/381800      std::_Vector_base<node, std::allocator<node> >::_M_allocate(unsigned long) [156]
                0.00    0.00  381800/2944440     node const& std::forward<node const&>(std::remove_reference<node const&>::type&) [128]
                0.00    0.00  381800/381800      std::_Vector_base<node, std::allocator<node> >::_M_deallocate(node*, unsigned long) [157]
-----------------------------------------------
                0.01    0.05  763600/763600      void std::vector<node, std::allocator<node> >::_M_realloc_insert<node const&>(__gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >, node const&) [11]
[12]     1.7    0.01    0.05  763600         std::vector<node, std::allocator<node> >::_S_relocate(node*, node*, node*, std::allocator<node>&) [12]
                0.00    0.05  763600/763600      std::vector<node, std::allocator<node> >::_S_do_relocate(node*, node*, node*, std::allocator<node>&, std::integral_constant<bool, true>) [15]
-----------------------------------------------
                                                 <spontaneous>
[13]     1.7    0.06    0.00                 __wt_config_next [13]
-----------------------------------------------
                0.01    0.00  472442/3035193     std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(char const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&) [71]
                0.04    0.00 2562751/3035193     std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::basic_string<std::allocator<char> >(char const*, std::allocator<char> const&) [9]
[14]     1.4    0.05    0.00 3035193         std::char_traits<char>::length(char const*) [14]
-----------------------------------------------
                0.00    0.05  763600/763600      std::vector<node, std::allocator<node> >::_S_relocate(node*, node*, node*, std::allocator<node>&) [12]
[15]     1.4    0.00    0.05  763600         std::vector<node, std::allocator<node> >::_S_do_relocate(node*, node*, node*, std::allocator<node>&, std::integral_constant<bool, true>) [15]
                0.00    0.05  763600/763600      node* std::__relocate_a<node*, node*, std::allocator<node> >(node*, node*, node*, std::allocator<node>&) [16]
-----------------------------------------------
                0.00    0.05  763600/763600      std::vector<node, std::allocator<node> >::_S_do_relocate(node*, node*, node*, std::allocator<node>&, std::integral_constant<bool, true>) [15]
[16]     1.4    0.00    0.05  763600         node* std::__relocate_a<node*, node*, std::allocator<node> >(node*, node*, node*, std::allocator<node>&) [16]
                0.01    0.04  763600/763600      node* std::__relocate_a_1<node*, node*, std::allocator<node> >(node*, node*, node*, std::allocator<node>&) [17]
                0.00    0.00 2290800/2290800     node* std::__niter_base<node*>(node*) [132]
-----------------------------------------------
                0.01    0.04  763600/763600      node* std::__relocate_a<node*, node*, std::allocator<node> >(node*, node*, node*, std::allocator<node>&) [16]
[17]     1.4    0.01    0.04  763600         node* std::__relocate_a_1<node*, node*, std::allocator<node> >(node*, node*, node*, std::allocator<node>&) [17]
                0.01    0.03 1615870/1615870     void std::__relocate_object_a<node, node, std::allocator<node> >(node*, node*, std::allocator<node>&) [18]
                0.00    0.00 3231740/4847610     node* std::__addressof<node>(node&) [126]
-----------------------------------------------
                0.01    0.03 1615870/1615870     node* std::__relocate_a_1<node*, node*, std::allocator<node> >(node*, node*, node*, std::allocator<node>&) [17]
[18]     1.1    0.01    0.03 1615870         void std::__relocate_object_a<node, node, std::allocator<node> >(node*, node*, std::allocator<node>&) [18]
                0.01    0.01 1615870/1615870     void std::allocator_traits<std::allocator<node> >::construct<node, node>(std::allocator<node>&, node*, node&&) [28]
                0.01    0.00 1615870/1615870     std::remove_reference<node&>::type&& std::move<node&>(node&) [46]
                0.00    0.00 1615870/4847610     node* std::__addressof<node>(node&) [126]
                0.00    0.00 1615870/1615870     void std::allocator_traits<std::allocator<node> >::destroy<node>(std::allocator<node>&, node*) [134]
-----------------------------------------------
                                                 <spontaneous>
[19]     1.1    0.04    0.00                 __wt_btcur_close [19]
-----------------------------------------------
                                                 <spontaneous>
[20]     1.1    0.04    0.00                 __wt_btcur_next [20]
-----------------------------------------------
                                                 <spontaneous>
[21]     1.1    0.04    0.00                 __wt_hazard_set [21]
-----------------------------------------------
                                                 <spontaneous>
[22]     1.1    0.04    0.00                 __wt_struct_packv [22]
-----------------------------------------------
                                                 <spontaneous>
[23]     1.1    0.04    0.00                 __wt_struct_sizev [23]
-----------------------------------------------
                                                 <spontaneous>
[24]     0.8    0.03    0.00                 __wt_curfile_open [24]
-----------------------------------------------
                                                 <spontaneous>
[25]     0.8    0.03    0.00                 __wt_cursor_close [25]
-----------------------------------------------
                                                 <spontaneous>
[26]     0.8    0.03    0.00                 __wt_cursor_set_keyv [26]
-----------------------------------------------
                                                 <spontaneous>
[27]     0.8    0.03    0.00                 __wt_schema_open_table [27]
-----------------------------------------------
                0.01    0.01 1615870/1615870     void std::__relocate_object_a<node, node, std::allocator<node> >(node*, node*, std::allocator<node>&) [18]
[28]     0.6    0.01    0.01 1615870         void std::allocator_traits<std::allocator<node> >::construct<node, node>(std::allocator<node>&, node*, node&&) [28]
                0.01    0.00 1615870/1615870     void __gnu_cxx::new_allocator<node>::construct<node, node>(node*, node&&) [45]
                0.00    0.00 1615870/3231740     node&& std::forward<node>(std::remove_reference<node>::type&) [127]
-----------------------------------------------
                                                 <spontaneous>
[29]     0.6    0.02    0.00                 __curfile_search [29]
-----------------------------------------------
                                                 <spontaneous>
[30]     0.6    0.02    0.00                 __curindex_next [30]
-----------------------------------------------
                                                 <spontaneous>
[31]     0.6    0.02    0.00                 __session_open_cursor_int [31]
-----------------------------------------------
                                                 <spontaneous>
[32]     0.6    0.02    0.00                 __wt_btcur_search [32]
-----------------------------------------------
                                                 <spontaneous>
[33]     0.6    0.02    0.00                 __wt_config_gets_def [33]
-----------------------------------------------
                                                 <spontaneous>
[34]     0.6    0.02    0.00                 __wt_cursor_cache_get [34]
-----------------------------------------------
                                                 <spontaneous>
[35]     0.6    0.02    0.00                 __wt_malloc [35]
-----------------------------------------------
                                                 <spontaneous>
[36]     0.6    0.02    0.00                 __wt_metadata_cursor [36]
-----------------------------------------------
                                                 <spontaneous>
[37]     0.6    0.02    0.00                 __wt_page_in_func [37]
-----------------------------------------------
                0.00    0.00      19/236219      EdgeKey::get_out_degree(int) [77]
                0.00    0.00  118100/236219      EdgeKey::has_node(int) [72]
                0.00    0.00  118100/236219      EdgeKey::get_out_nodes(int) [4]
[38]     0.4    0.01    0.00  236219         EdgeKey::get_edge_cursor() [38]
                0.00    0.00  236219/236239      EdgeKey::_get_table_cursor(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, __wt_cursor**, bool) [73]
                0.00    0.00  236219/590547      void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char*>(char*, char*, std::forward_iterator_tag) [149]
-----------------------------------------------
                                                 <spontaneous>
[39]     0.3    0.01    0.00                 wiredtiger_crc32c_func [39]
-----------------------------------------------
                0.00    0.01 2562751/2562751     std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::basic_string<std::allocator<char> >(char const*, std::allocator<char> const&) [9]
[40]     0.3    0.00    0.01 2562751         void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char const*>(char const*, char const*) [40]
                0.00    0.01 2562751/2562751     void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct_aux<char const*>(char const*, char const*, std::__false_type) [42]
-----------------------------------------------
                0.00    0.01 2562751/2562751     void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct_aux<char const*>(char const*, char const*, std::__false_type) [42]
[41]     0.3    0.00    0.01 2562751         void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char const*>(char const*, char const*, std::forward_iterator_tag) [41]
                0.00    0.01 2562751/2562751     std::iterator_traits<char const*>::difference_type std::distance<char const*>(char const*, char const*) [44]
                0.00    0.00 2562751/2562751     bool __gnu_cxx::__is_null_pointer<char const>(char const*) [130]
-----------------------------------------------
                0.00    0.01 2562751/2562751     void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char const*>(char const*, char const*) [40]
[42]     0.3    0.00    0.01 2562751         void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct_aux<char const*>(char const*, char const*, std::__false_type) [42]
                0.00    0.01 2562751/2562751     void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char const*>(char const*, char const*, std::forward_iterator_tag) [41]
-----------------------------------------------
                0.01    0.00 2562751/2562751     std::iterator_traits<char const*>::difference_type std::distance<char const*>(char const*, char const*) [44]
[43]     0.3    0.01    0.00 2562751         std::iterator_traits<char const*>::difference_type std::__distance<char const*>(char const*, char const*, std::random_access_iterator_tag) [43]
-----------------------------------------------
                0.00    0.01 2562751/2562751     void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char const*>(char const*, char const*, std::forward_iterator_tag) [41]
[44]     0.3    0.00    0.01 2562751         std::iterator_traits<char const*>::difference_type std::distance<char const*>(char const*, char const*) [44]
                0.01    0.00 2562751/2562751     std::iterator_traits<char const*>::difference_type std::__distance<char const*>(char const*, char const*, std::random_access_iterator_tag) [43]
                0.00    0.00 2562751/2562751     std::iterator_traits<char const*>::iterator_category std::__iterator_category<char const*>(char const* const&) [131]
-----------------------------------------------
                0.01    0.00 1615870/1615870     void std::allocator_traits<std::allocator<node> >::construct<node, node>(std::allocator<node>&, node*, node&&) [28]
[45]     0.3    0.01    0.00 1615870         void __gnu_cxx::new_allocator<node>::construct<node, node>(node*, node&&) [45]
                0.00    0.00 1615870/3231740     node&& std::forward<node>(std::remove_reference<node>::type&) [127]
                0.00    0.00 1615870/2897193     operator new(unsigned long, void*) [129]
-----------------------------------------------
                0.01    0.00 1615870/1615870     void std::__relocate_object_a<node, node, std::allocator<node> >(node*, node*, std::allocator<node>&) [18]
[46]     0.3    0.01    0.00 1615870         std::remove_reference<node&>::type&& std::move<node&>(node&) [46]
-----------------------------------------------
                0.01    0.00 1281358/1281358     EdgeKey::__record_to_node(__wt_cursor*, node*) [7]
[47]     0.3    0.01    0.00 1281358         EdgeKey::extract_from_string(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, int*, int*) [47]
                0.00    0.00 1281358/1281358     std::operator|(std::_Ios_Openmode, std::_Ios_Openmode) [137]
-----------------------------------------------
                0.01    0.00 1281320/1281320     void std::allocator_traits<std::allocator<node> >::construct<node, node const&>(std::allocator<node>&, node*, node const&) [49]
[48]     0.3    0.01    0.00 1281320         void __gnu_cxx::new_allocator<node>::construct<node, node const&>(node*, node const&) [48]
                0.00    0.00 1281320/2944440     node const& std::forward<node const&>(std::remove_reference<node const&>::type&) [128]
                0.00    0.00 1281320/2897193     operator new(unsigned long, void*) [129]
-----------------------------------------------
                0.00    0.00  381800/1281320     void std::vector<node, std::allocator<node> >::_M_realloc_insert<node const&>(__gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >, node const&) [11]
                0.00    0.01  899520/1281320     std::vector<node, std::allocator<node> >::push_back(node const&) [10]
[49]     0.3    0.00    0.01 1281320         void std::allocator_traits<std::allocator<node> >::construct<node, node const&>(std::allocator<node>&, node*, node const&) [49]
                0.01    0.00 1281320/1281320     void __gnu_cxx::new_allocator<node>::construct<node, node const&>(node*, node const&) [48]
                0.00    0.00 1281320/2944440     node const& std::forward<node const&>(std::remove_reference<node const&>::type&) [128]
-----------------------------------------------
                                                 <spontaneous>
[50]     0.3    0.01    0.00                 std::vector<node, std::allocator<node> >::~vector() [50]
-----------------------------------------------
                                                 <spontaneous>
[51]     0.3    0.01    0.00                 __curindex_get_value [51]
-----------------------------------------------
                                                 <spontaneous>
[52]     0.3    0.01    0.00                 __curindex_move [52]
-----------------------------------------------
                                                 <spontaneous>
[53]     0.3    0.01    0.00                 __curtable_search [53]
-----------------------------------------------
                                                 <spontaneous>
[54]     0.3    0.01    0.00                 __pack_write [54]
-----------------------------------------------
                                                 <spontaneous>
[55]     0.3    0.01    0.00                 __schema_open_index [55]
-----------------------------------------------
                                                 <spontaneous>
[56]     0.3    0.01    0.00                 __unpack_read [56]
-----------------------------------------------
                                                 <spontaneous>
[57]     0.3    0.01    0.00                 __wt_btcur_iterate_setup [57]
-----------------------------------------------
                                                 <spontaneous>
[58]     0.3    0.01    0.00                 __wt_btcur_search_near [58]
-----------------------------------------------
                                                 <spontaneous>
[59]     0.3    0.01    0.00                 __wt_clock_to_nsec [59]
-----------------------------------------------
                                                 <spontaneous>
[60]     0.3    0.01    0.00                 __wt_curtable_get_key [60]
-----------------------------------------------
                                                 <spontaneous>
[61]     0.3    0.01    0.00                 __wt_curtable_get_value [61]
-----------------------------------------------
                                                 <spontaneous>
[62]     0.3    0.01    0.00                 __wt_curtable_open [62]
-----------------------------------------------
                                                 <spontaneous>
[63]     0.3    0.01    0.00                 __wt_curtable_set_key [63]
-----------------------------------------------
                                                 <spontaneous>
[64]     0.3    0.01    0.00                 __wt_readlock [64]
-----------------------------------------------
                                                 <spontaneous>
[65]     0.3    0.01    0.00                 __wt_readunlock [65]
-----------------------------------------------
                                                 <spontaneous>
[66]     0.3    0.01    0.00                 __wt_realloc_noclear [66]
-----------------------------------------------
                                                 <spontaneous>
[67]     0.3    0.01    0.00                 __wt_schema_open_index [67]
-----------------------------------------------
                                                 <spontaneous>
[68]     0.3    0.01    0.00                 __wt_session_gen_leave [68]
-----------------------------------------------
                                                 <spontaneous>
[69]     0.3    0.01    0.00                 __wt_strndup [69]
-----------------------------------------------
                                                 <spontaneous>
[70]     0.3    0.01    0.00                 __wt_writelock [70]
-----------------------------------------------
                0.00    0.00       3/472442      __static_initialization_and_destruction_0(int, int) [81]
                0.00    0.00  118100/472442      EdgeKey::_get_index_cursor(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, __wt_cursor**) [75]
                0.00    0.00  118100/472442      EdgeKey::get_src_idx_cur() [74]
                0.00    0.00  236239/472442      EdgeKey::_get_table_cursor(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, __wt_cursor**, bool) [73]
[71]     0.2    0.00    0.01  472442         std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(char const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&) [71]
                0.01    0.00  472442/3035193     std::char_traits<char>::length(char const*) [14]
-----------------------------------------------
                0.00    0.01  118100/118100      EdgeKey::get_out_nodes(int) [4]
[72]     0.2    0.00    0.01  118100         EdgeKey::has_node(int) [72]
                0.00    0.00  118100/236219      EdgeKey::get_edge_cursor() [38]
-----------------------------------------------
                0.00    0.00       1/236239      EdgeKey::__restore_from_db(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >) [89]
                0.00    0.00      19/236239      EdgeKey::get_random_node() [78]
                0.00    0.00  236219/236239      EdgeKey::get_edge_cursor() [38]
[73]     0.1    0.00    0.00  236239         EdgeKey::_get_table_cursor(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, __wt_cursor**, bool) [73]
                0.00    0.00  236239/472442      std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(char const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&) [71]
-----------------------------------------------
                0.00    0.00  118100/118100      EdgeKey::get_out_nodes(int) [4]
[74]     0.1    0.00    0.00  118100         EdgeKey::get_src_idx_cur() [74]
                0.00    0.00  118100/472442      std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(char const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&) [71]
                0.00    0.00  118100/118100      EdgeKey::_get_index_cursor(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, __wt_cursor**) [75]
                0.00    0.00  354300/590547      void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char*>(char*, char*, std::forward_iterator_tag) [149]
                0.00    0.00  236200/354300      std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&, char const*) [163]
                0.00    0.00  118100/354303      std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&) [162]
-----------------------------------------------
                0.00    0.00  118100/118100      EdgeKey::get_src_idx_cur() [74]
[75]     0.1    0.00    0.00  118100         EdgeKey::_get_index_cursor(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, __wt_cursor**) [75]
                0.00    0.00  118100/472442      std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(char const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&) [71]
                0.00    0.00  236200/354303      std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&) [162]
                0.00    0.00  118100/354300      std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&, char const*) [163]
-----------------------------------------------
                                                 <spontaneous>
[76]     0.0    0.00    0.00                 int find_random_start<EdgeKey>(EdgeKey&) [76]
                0.00    0.00      19/19          EdgeKey::get_out_degree(int) [77]
                0.00    0.00      19/19          EdgeKey::get_random_node() [78]
-----------------------------------------------
                0.00    0.00      19/19          int find_random_start<EdgeKey>(EdgeKey&) [76]
[77]     0.0    0.00    0.00      19         EdgeKey::get_out_degree(int) [77]
                0.00    0.00      19/1281358     EdgeKey::__record_to_node(__wt_cursor*, node*) [7]
                0.00    0.00      19/236219      EdgeKey::get_edge_cursor() [38]
                0.00    0.00      19/1281339     node::node() [138]
-----------------------------------------------
                0.00    0.00      19/19          int find_random_start<EdgeKey>(EdgeKey&) [76]
[78]     0.0    0.00    0.00      19         EdgeKey::get_random_node() [78]
                0.00    0.00      19/1281358     EdgeKey::__record_to_node(__wt_cursor*, node*) [7]
                0.00    0.00      19/236239      EdgeKey::_get_table_cursor(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, __wt_cursor**, bool) [73]
                0.00    0.00      19/590547      void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char*>(char*, char*, std::forward_iterator_tag) [149]
-----------------------------------------------
                                                 <spontaneous>
[79]     0.0    0.00    0.00                 __libc_csu_init [79]
                0.00    0.00       1/1           _GLOBAL__sub_I__Z8METADATAB5cxx11 [80]
                0.00    0.00       1/1           _GLOBAL__sub_I__ZN13StandardGraphC2Ev [83]
                0.00    0.00       1/1           _GLOBAL__sub_I__ZN7EdgeKeyC2Ev [85]
                0.00    0.00       1/1           _GLOBAL__sub_I__ZN7AdjListC2Ev [84]
                0.00    0.00       1/1           _GLOBAL__sub_I__Z14print_csv_infoNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEiP8bfs_info [193]
-----------------------------------------------
                0.00    0.00       1/1           __libc_csu_init [79]
[80]     0.0    0.00    0.00       1         _GLOBAL__sub_I__Z8METADATAB5cxx11 [80]
                0.00    0.00       1/1           __static_initialization_and_destruction_0(int, int) [81]
-----------------------------------------------
                0.00    0.00       1/1           _GLOBAL__sub_I__Z8METADATAB5cxx11 [80]
[81]     0.0    0.00    0.00       1         __static_initialization_and_destruction_0(int, int) [81]
                0.00    0.00      30/2562751     std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::basic_string<std::allocator<char> >(char const*, std::allocator<char> const&) [9]
                0.00    0.00       3/472442      std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(char const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&) [71]
                0.00    0.00       1/354303      std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&) [162]
-----------------------------------------------
                                                 <spontaneous>
[82]     0.0    0.00    0.00                 EdgeKey::EdgeKey(graph_opts) [82]
                0.00    0.00       2/2562751     std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::basic_string<std::allocator<char> >(char const*, std::allocator<char> const&) [9]
                0.00    0.00       1/1           EdgeKey::__restore_from_db(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >) [89]
                0.00    0.00       3/590547      void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char*>(char*, char*, std::forward_iterator_tag) [149]
                0.00    0.00       2/2           std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&, char const*) [192]
                0.00    0.00       2/354303      std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&) [162]
                0.00    0.00       1/2           std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::allocator() [190]
                0.00    0.00       1/1           std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::vector(std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [237]
                0.00    0.00       1/3           std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::~allocator() [179]
                0.00    0.00       1/1           CommonUtil::check_graph_params(graph_opts) [198]
                0.00    0.00       1/1           graph_opts::graph_opts(graph_opts const&) [199]
                0.00    0.00       1/1           graph_opts::~graph_opts() [200]
                0.00    0.00       1/1           std::filesystem::__cxx11::path::path(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&, std::filesystem::__cxx11::path::format) [213]
                0.00    0.00       1/1           std::filesystem::exists(std::filesystem::__cxx11::path const&) [211]
                0.00    0.00       1/1           std::filesystem::__cxx11::path::~path() [214]
-----------------------------------------------
                0.00    0.00       1/1           __libc_csu_init [79]
[83]     0.0    0.00    0.00       1         _GLOBAL__sub_I__ZN13StandardGraphC2Ev [83]
                0.00    0.00       1/1           __static_initialization_and_destruction_0(int, int) [88]
-----------------------------------------------
                0.00    0.00       1/1           __libc_csu_init [79]
[84]     0.0    0.00    0.00       1         _GLOBAL__sub_I__ZN7AdjListC2Ev [84]
                0.00    0.00       1/1           __static_initialization_and_destruction_0(int, int) [86]
-----------------------------------------------
                0.00    0.00       1/1           __libc_csu_init [79]
[85]     0.0    0.00    0.00       1         _GLOBAL__sub_I__ZN7EdgeKeyC2Ev [85]
                0.00    0.00       1/1           __static_initialization_and_destruction_0(int, int) [87]
-----------------------------------------------
                0.00    0.00       1/1           _GLOBAL__sub_I__ZN7AdjListC2Ev [84]
[86]     0.0    0.00    0.00       1         __static_initialization_and_destruction_0(int, int) [86]
                0.00    0.00       1/2562751     std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::basic_string<std::allocator<char> >(char const*, std::allocator<char> const&) [9]
-----------------------------------------------
                0.00    0.00       1/1           _GLOBAL__sub_I__ZN7EdgeKeyC2Ev [85]
[87]     0.0    0.00    0.00       1         __static_initialization_and_destruction_0(int, int) [87]
                0.00    0.00       1/2562751     std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::basic_string<std::allocator<char> >(char const*, std::allocator<char> const&) [9]
-----------------------------------------------
                0.00    0.00       1/1           _GLOBAL__sub_I__ZN13StandardGraphC2Ev [83]
[88]     0.0    0.00    0.00       1         __static_initialization_and_destruction_0(int, int) [88]
                0.00    0.00       1/2562751     std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::basic_string<std::allocator<char> >(char const*, std::allocator<char> const&) [9]
-----------------------------------------------
                0.00    0.00       1/1           EdgeKey::EdgeKey(graph_opts) [82]
[89]     0.0    0.00    0.00       1         EdgeKey::__restore_from_db(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >) [89]
                0.00    0.00       1/236239      EdgeKey::_get_table_cursor(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, __wt_cursor**, bool) [73]
                0.00    0.00       1/1           CommonUtil::open_connection(char*, __wt_connection**) [196]
                0.00    0.00       1/1           CommonUtil::open_session(__wt_connection*, __wt_session**) [194]
                0.00    0.00       1/590547      void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char*>(char*, char*, std::forward_iterator_tag) [149]
-----------------------------------------------
                0.00    0.00 1615870/4847610     void std::__relocate_object_a<node, node, std::allocator<node> >(node*, node*, std::allocator<node>&) [18]
                0.00    0.00 3231740/4847610     node* std::__relocate_a_1<node*, node*, std::allocator<node> >(node*, node*, node*, std::allocator<node>&) [17]
[126]    0.0    0.00    0.00 4847610         node* std::__addressof<node>(node&) [126]
-----------------------------------------------
                0.00    0.00 1615870/3231740     void std::allocator_traits<std::allocator<node> >::construct<node, node>(std::allocator<node>&, node*, node&&) [28]
                0.00    0.00 1615870/3231740     void __gnu_cxx::new_allocator<node>::construct<node, node>(node*, node&&) [45]
[127]    0.0    0.00    0.00 3231740         node&& std::forward<node>(std::remove_reference<node>::type&) [127]
-----------------------------------------------
                0.00    0.00  381800/2944440     void std::vector<node, std::allocator<node> >::_M_realloc_insert<node const&>(__gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >, node const&) [11]
                0.00    0.00 1281320/2944440     void std::allocator_traits<std::allocator<node> >::construct<node, node const&>(std::allocator<node>&, node*, node const&) [49]
                0.00    0.00 1281320/2944440     void __gnu_cxx::new_allocator<node>::construct<node, node const&>(node*, node const&) [48]
[128]    0.0    0.00    0.00 2944440         node const& std::forward<node const&>(std::remove_reference<node const&>::type&) [128]
-----------------------------------------------
                0.00    0.00       3/2897193     void std::_Construct<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&) [181]
                0.00    0.00 1281320/2897193     void __gnu_cxx::new_allocator<node>::construct<node, node const&>(node*, node const&) [48]
                0.00    0.00 1615870/2897193     void __gnu_cxx::new_allocator<node>::construct<node, node>(node*, node&&) [45]
[129]    0.0    0.00    0.00 2897193         operator new(unsigned long, void*) [129]
-----------------------------------------------
                0.00    0.00 2562751/2562751     void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char const*>(char const*, char const*, std::forward_iterator_tag) [41]
[130]    0.0    0.00    0.00 2562751         bool __gnu_cxx::__is_null_pointer<char const>(char const*) [130]
-----------------------------------------------
                0.00    0.00 2562751/2562751     std::iterator_traits<char const*>::difference_type std::distance<char const*>(char const*, char const*) [44]
[131]    0.0    0.00    0.00 2562751         std::iterator_traits<char const*>::iterator_category std::__iterator_category<char const*>(char const* const&) [131]
-----------------------------------------------
                0.00    0.00 2290800/2290800     node* std::__relocate_a<node*, node*, std::allocator<node> >(node*, node*, node*, std::allocator<node>&) [16]
[132]    0.0    0.00    0.00 2290800         node* std::__niter_base<node*>(node*) [132]
-----------------------------------------------
                0.00    0.00 1615870/1615870     void std::allocator_traits<std::allocator<node> >::destroy<node>(std::allocator<node>&, node*) [134]
[133]    0.0    0.00    0.00 1615870         void __gnu_cxx::new_allocator<node>::destroy<node>(node*) [133]
-----------------------------------------------
                0.00    0.00 1615870/1615870     void std::__relocate_object_a<node, node, std::allocator<node> >(node*, node*, std::allocator<node>&) [18]
[134]    0.0    0.00    0.00 1615870         void std::allocator_traits<std::allocator<node> >::destroy<node>(std::allocator<node>&, node*) [134]
                0.00    0.00 1615870/1615870     void __gnu_cxx::new_allocator<node>::destroy<node>(node*) [133]
-----------------------------------------------
                0.00    0.00  763600/1527200     void std::vector<node, std::allocator<node> >::_M_realloc_insert<node const&>(__gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >, node const&) [11]
                0.00    0.00  763600/1527200     __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >::difference_type __gnu_cxx::operator-<node*, std::vector<node, std::allocator<node> > >(__gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > > const&, __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > > const&) [154]
[135]    0.0    0.00    0.00 1527200         __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >::base() const [135]
-----------------------------------------------
                0.00    0.00 1527200/1527200     std::vector<node, std::allocator<node> >::_M_check_len(unsigned long, char const*) const [155]
[136]    0.0    0.00    0.00 1527200         std::vector<node, std::allocator<node> >::size() const [136]
-----------------------------------------------
                0.00    0.00 1281358/1281358     EdgeKey::extract_from_string(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, int*, int*) [47]
[137]    0.0    0.00    0.00 1281358         std::operator|(std::_Ios_Openmode, std::_Ios_Openmode) [137]
-----------------------------------------------
                0.00    0.00      19/1281339     EdgeKey::get_out_degree(int) [77]
                0.00    0.00 1281320/1281339     EdgeKey::get_out_nodes(int) [4]
[138]    0.0    0.00    0.00 1281339         node::node() [138]
-----------------------------------------------
                0.00    0.00  381800/1145400     __gnu_cxx::new_allocator<node>::allocate(unsigned long, void const*) [153]
                0.00    0.00  763600/1145400     std::allocator_traits<std::allocator<node> >::max_size(std::allocator<node> const&) [145]
[139]    0.0    0.00    0.00 1145400         __gnu_cxx::new_allocator<node>::max_size() const [139]
-----------------------------------------------
                0.00    0.00       1/763601      std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_S_max_size(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [233]
                0.00    0.00  763600/763601      std::vector<node, std::allocator<node> >::_S_max_size(std::allocator<node> const&) [146]
[140]    0.0    0.00    0.00  763601         unsigned long const& std::min<unsigned long>(unsigned long const&, unsigned long const&) [140]
-----------------------------------------------
                0.00    0.00  381800/763600      std::vector<node, std::allocator<node> >::end() [159]
                0.00    0.00  381800/763600      std::vector<node, std::allocator<node> >::begin() [160]
[141]    0.0    0.00    0.00  763600         __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >::__normal_iterator(node* const&) [141]
-----------------------------------------------
                0.00    0.00  763600/763600      std::vector<node, std::allocator<node> >::max_size() const [143]
[142]    0.0    0.00    0.00  763600         std::_Vector_base<node, std::allocator<node> >::_M_get_Tp_allocator() const [142]
-----------------------------------------------
                0.00    0.00  763600/763600      std::vector<node, std::allocator<node> >::_M_check_len(unsigned long, char const*) const [155]
[143]    0.0    0.00    0.00  763600         std::vector<node, std::allocator<node> >::max_size() const [143]
                0.00    0.00  763600/763600      std::vector<node, std::allocator<node> >::_S_max_size(std::allocator<node> const&) [146]
                0.00    0.00  763600/763600      std::_Vector_base<node, std::allocator<node> >::_M_get_Tp_allocator() const [142]
-----------------------------------------------
                0.00    0.00  763600/763600      void std::vector<node, std::allocator<node> >::_M_realloc_insert<node const&>(__gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >, node const&) [11]
[144]    0.0    0.00    0.00  763600         std::_Vector_base<node, std::allocator<node> >::_M_get_Tp_allocator() [144]
-----------------------------------------------
                0.00    0.00  763600/763600      std::vector<node, std::allocator<node> >::_S_max_size(std::allocator<node> const&) [146]
[145]    0.0    0.00    0.00  763600         std::allocator_traits<std::allocator<node> >::max_size(std::allocator<node> const&) [145]
                0.00    0.00  763600/1145400     __gnu_cxx::new_allocator<node>::max_size() const [139]
-----------------------------------------------
                0.00    0.00  763600/763600      std::vector<node, std::allocator<node> >::max_size() const [143]
[146]    0.0    0.00    0.00  763600         std::vector<node, std::allocator<node> >::_S_max_size(std::allocator<node> const&) [146]
                0.00    0.00  763600/763600      std::allocator_traits<std::allocator<node> >::max_size(std::allocator<node> const&) [145]
                0.00    0.00  763600/763601      unsigned long const& std::min<unsigned long>(unsigned long const&, unsigned long const&) [140]
-----------------------------------------------
                0.00    0.00       1/708604      std::filesystem::__cxx11::path::path(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&, std::filesystem::__cxx11::path::format) [213]
                0.00    0.00  354300/708604      std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&, char const*) [163]
                0.00    0.00  354303/708604      std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&) [162]
[147]    0.0    0.00    0.00  708604         std::remove_reference<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&>::type&& std::move<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&) [147]
-----------------------------------------------
                0.00    0.00  590547/590547      void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char*>(char*, char*, std::forward_iterator_tag) [149]
[148]    0.0    0.00    0.00  590547         bool __gnu_cxx::__is_null_pointer<char>(char*) [148]
-----------------------------------------------
                0.00    0.00       1/590547      EdgeKey::__restore_from_db(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >) [89]
                0.00    0.00       2/590547      std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&, char const*) [192]
                0.00    0.00       3/590547      void std::_Construct<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&) [181]
                0.00    0.00       3/590547      EdgeKey::EdgeKey(graph_opts) [82]
                0.00    0.00      19/590547      EdgeKey::get_random_node() [78]
                0.00    0.00  236219/590547      EdgeKey::get_edge_cursor() [38]
                0.00    0.00  354300/590547      EdgeKey::get_src_idx_cur() [74]
[149]    0.0    0.00    0.00  590547         void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char*>(char*, char*, std::forward_iterator_tag) [149]
                0.00    0.00  590547/590547      bool __gnu_cxx::__is_null_pointer<char>(char*) [148]
                0.00    0.00  590547/590547      std::iterator_traits<char*>::difference_type std::distance<char*>(char*, char*) [152]
-----------------------------------------------
                0.00    0.00  590547/590547      std::iterator_traits<char*>::difference_type std::distance<char*>(char*, char*) [152]
[150]    0.0    0.00    0.00  590547         std::iterator_traits<char*>::difference_type std::__distance<char*>(char*, char*, std::random_access_iterator_tag) [150]
-----------------------------------------------
                0.00    0.00  590547/590547      std::iterator_traits<char*>::difference_type std::distance<char*>(char*, char*) [152]
[151]    0.0    0.00    0.00  590547         std::iterator_traits<char*>::iterator_category std::__iterator_category<char*>(char* const&) [151]
-----------------------------------------------
                0.00    0.00  590547/590547      void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char*>(char*, char*, std::forward_iterator_tag) [149]
[152]    0.0    0.00    0.00  590547         std::iterator_traits<char*>::difference_type std::distance<char*>(char*, char*) [152]
                0.00    0.00  590547/590547      std::iterator_traits<char*>::iterator_category std::__iterator_category<char*>(char* const&) [151]
                0.00    0.00  590547/590547      std::iterator_traits<char*>::difference_type std::__distance<char*>(char*, char*, std::random_access_iterator_tag) [150]
-----------------------------------------------
                0.00    0.00  381800/381800      std::allocator_traits<std::allocator<node> >::allocate(std::allocator<node>&, unsigned long) [158]
[153]    0.0    0.00    0.00  381800         __gnu_cxx::new_allocator<node>::allocate(unsigned long, void const*) [153]
                0.00    0.00  381800/1145400     __gnu_cxx::new_allocator<node>::max_size() const [139]
-----------------------------------------------
                0.00    0.00  381800/381800      void std::vector<node, std::allocator<node> >::_M_realloc_insert<node const&>(__gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >, node const&) [11]
[154]    0.0    0.00    0.00  381800         __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >::difference_type __gnu_cxx::operator-<node*, std::vector<node, std::allocator<node> > >(__gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > > const&, __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > > const&) [154]
                0.00    0.00  763600/1527200     __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >::base() const [135]
-----------------------------------------------
                0.00    0.00  381800/381800      void std::vector<node, std::allocator<node> >::_M_realloc_insert<node const&>(__gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >, node const&) [11]
[155]    0.0    0.00    0.00  381800         std::vector<node, std::allocator<node> >::_M_check_len(unsigned long, char const*) const [155]
                0.00    0.00 1527200/1527200     std::vector<node, std::allocator<node> >::size() const [136]
                0.00    0.00  763600/763600      std::vector<node, std::allocator<node> >::max_size() const [143]
                0.00    0.00  381800/381800      unsigned long const& std::max<unsigned long>(unsigned long const&, unsigned long const&) [161]
-----------------------------------------------
                0.00    0.00  381800/381800      void std::vector<node, std::allocator<node> >::_M_realloc_insert<node const&>(__gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >, node const&) [11]
[156]    0.0    0.00    0.00  381800         std::_Vector_base<node, std::allocator<node> >::_M_allocate(unsigned long) [156]
                0.00    0.00  381800/381800      std::allocator_traits<std::allocator<node> >::allocate(std::allocator<node>&, unsigned long) [158]
-----------------------------------------------
                0.00    0.00  381800/381800      void std::vector<node, std::allocator<node> >::_M_realloc_insert<node const&>(__gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >, node const&) [11]
[157]    0.0    0.00    0.00  381800         std::_Vector_base<node, std::allocator<node> >::_M_deallocate(node*, unsigned long) [157]
                0.00    0.00  279060/279060      std::allocator_traits<std::allocator<node> >::deallocate(std::allocator<node>&, node*, unsigned long) [165]
-----------------------------------------------
                0.00    0.00  381800/381800      std::_Vector_base<node, std::allocator<node> >::_M_allocate(unsigned long) [156]
[158]    0.0    0.00    0.00  381800         std::allocator_traits<std::allocator<node> >::allocate(std::allocator<node>&, unsigned long) [158]
                0.00    0.00  381800/381800      __gnu_cxx::new_allocator<node>::allocate(unsigned long, void const*) [153]
-----------------------------------------------
                0.00    0.00  381800/381800      std::vector<node, std::allocator<node> >::push_back(node const&) [10]
[159]    0.0    0.00    0.00  381800         std::vector<node, std::allocator<node> >::end() [159]
                0.00    0.00  381800/763600      __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >::__normal_iterator(node* const&) [141]
-----------------------------------------------
                0.00    0.00  381800/381800      void std::vector<node, std::allocator<node> >::_M_realloc_insert<node const&>(__gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >, node const&) [11]
[160]    0.0    0.00    0.00  381800         std::vector<node, std::allocator<node> >::begin() [160]
                0.00    0.00  381800/763600      __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >::__normal_iterator(node* const&) [141]
-----------------------------------------------
                0.00    0.00  381800/381800      std::vector<node, std::allocator<node> >::_M_check_len(unsigned long, char const*) const [155]
[161]    0.0    0.00    0.00  381800         unsigned long const& std::max<unsigned long>(unsigned long const&, unsigned long const&) [161]
-----------------------------------------------
                0.00    0.00       1/354303      __static_initialization_and_destruction_0(int, int) [81]
                0.00    0.00       2/354303      EdgeKey::EdgeKey(graph_opts) [82]
                0.00    0.00  118100/354303      EdgeKey::get_src_idx_cur() [74]
                0.00    0.00  236200/354303      EdgeKey::_get_index_cursor(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, __wt_cursor**) [75]
[162]    0.0    0.00    0.00  354303         std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&) [162]
                0.00    0.00  354303/708604      std::remove_reference<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&>::type&& std::move<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&) [147]
-----------------------------------------------
                0.00    0.00  118100/354300      EdgeKey::_get_index_cursor(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, __wt_cursor**) [75]
                0.00    0.00  236200/354300      EdgeKey::get_src_idx_cur() [74]
[163]    0.0    0.00    0.00  354300         std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&, char const*) [163]
                0.00    0.00  354300/708604      std::remove_reference<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&>::type&& std::move<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&) [147]
-----------------------------------------------
                0.00    0.00  279060/279060      std::allocator_traits<std::allocator<node> >::deallocate(std::allocator<node>&, node*, unsigned long) [165]
[164]    0.0    0.00    0.00  279060         __gnu_cxx::new_allocator<node>::deallocate(node*, unsigned long) [164]
-----------------------------------------------
                0.00    0.00  279060/279060      std::_Vector_base<node, std::allocator<node> >::_M_deallocate(node*, unsigned long) [157]
[165]    0.0    0.00    0.00  279060         std::allocator_traits<std::allocator<node> >::deallocate(std::allocator<node>&, node*, unsigned long) [165]
                0.00    0.00  279060/279060      __gnu_cxx::new_allocator<node>::deallocate(node*, unsigned long) [164]
-----------------------------------------------
                0.00    0.00  118100/118100      std::allocator<node>::allocator() [167]
[166]    0.0    0.00    0.00  118100         __gnu_cxx::new_allocator<node>::new_allocator() [166]
-----------------------------------------------
                0.00    0.00  118100/118100      std::_Vector_base<node, std::allocator<node> >::_Vector_impl::_Vector_impl() [168]
[167]    0.0    0.00    0.00  118100         std::allocator<node>::allocator() [167]
                0.00    0.00  118100/118100      __gnu_cxx::new_allocator<node>::new_allocator() [166]
-----------------------------------------------
                0.00    0.00  118100/118100      std::_Vector_base<node, std::allocator<node> >::_Vector_base() [170]
[168]    0.0    0.00    0.00  118100         std::_Vector_base<node, std::allocator<node> >::_Vector_impl::_Vector_impl() [168]
                0.00    0.00  118100/118100      std::_Vector_base<node, std::allocator<node> >::_Vector_impl_data::_Vector_impl_data() [169]
                0.00    0.00  118100/118100      std::allocator<node>::allocator() [167]
-----------------------------------------------
                0.00    0.00  118100/118100      std::_Vector_base<node, std::allocator<node> >::_Vector_impl::_Vector_impl() [168]
[169]    0.0    0.00    0.00  118100         std::_Vector_base<node, std::allocator<node> >::_Vector_impl_data::_Vector_impl_data() [169]
-----------------------------------------------
                0.00    0.00  118100/118100      std::vector<node, std::allocator<node> >::vector() [171]
[170]    0.0    0.00    0.00  118100         std::_Vector_base<node, std::allocator<node> >::_Vector_base() [170]
                0.00    0.00  118100/118100      std::_Vector_base<node, std::allocator<node> >::_Vector_impl::_Vector_impl() [168]
-----------------------------------------------
                0.00    0.00  118100/118100      EdgeKey::get_out_nodes(int) [4]
[171]    0.0    0.00    0.00  118100         std::vector<node, std::allocator<node> >::vector() [171]
                0.00    0.00  118100/118100      std::_Vector_base<node, std::allocator<node> >::_Vector_base() [170]
-----------------------------------------------
                0.00    0.00    1760/1760        bfs_info* bfs<EdgeKey>(EdgeKey&, int) [3]
[172]    0.0    0.00    0.00    1760         void std::vector<int, std::allocator<int> >::_M_realloc_insert<int const&>(__gnu_cxx::__normal_iterator<int*, std::vector<int, std::allocator<int> > >, int const&) [172]
-----------------------------------------------
                              116930             std::_Rb_tree<int, int, std::_Identity<int>, std::less<int>, std::allocator<int> >::_M_erase(std::_Rb_tree_node<int>*) [173]
                0.00    0.00    1170/1170        bfs_info* bfs<EdgeKey>(EdgeKey&, int) [3]
[173]    0.0    0.00    0.00    1170+116930  std::_Rb_tree<int, int, std::_Identity<int>, std::less<int>, std::allocator<int> >::_M_erase(std::_Rb_tree_node<int>*) [173]
                              116930             std::_Rb_tree<int, int, std::_Identity<int>, std::less<int>, std::allocator<int> >::_M_erase(std::_Rb_tree_node<int>*) [173]
-----------------------------------------------
                0.00    0.00     800/800         bfs_info* bfs<EdgeKey>(EdgeKey&, int) [3]
[174]    0.0    0.00    0.00     800         void std::deque<int, std::allocator<int> >::_M_push_back_aux<int const&>(int const&) [174]
-----------------------------------------------
                0.00    0.00     160/160         bfs_info* bfs<EdgeKey>(EdgeKey&, int) [3]
[175]    0.0    0.00    0.00     160         std::_Deque_base<int, std::allocator<int> >::_M_initialize_map(unsigned long) [175]
-----------------------------------------------
                0.00    0.00       1/18          std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_replace(unsigned long, unsigned long, char const*, unsigned long) [239]
                0.00    0.00       1/18          CmdLineApp::CmdLineApp(int, char**) [258]
                0.00    0.00      16/18          print_csv_info(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, int, bfs_info*) [252]
[176]    0.0    0.00    0.00      18         std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_mutate(unsigned long, unsigned long, char const*, unsigned long) [176]
-----------------------------------------------
                0.00    0.00       5/5           CmdLineBase::CmdLineBase(int, char**) [201]
[177]    0.0    0.00    0.00       5         void std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_realloc_insert<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >(__gnu_cxx::__normal_iterator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > > >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&) [177]
-----------------------------------------------
                0.00    0.00       3/3           std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::~allocator() [179]
[178]    0.0    0.00    0.00       3         __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::~new_allocator() [178]
-----------------------------------------------
                0.00    0.00       1/3           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl::~_Vector_impl() [223]
                0.00    0.00       1/3           std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_S_check_init_len(unsigned long, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [234]
                0.00    0.00       1/3           EdgeKey::EdgeKey(graph_opts) [82]
[179]    0.0    0.00    0.00       3         std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::~allocator() [179]
                0.00    0.00       3/3           __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::~new_allocator() [178]
-----------------------------------------------
                0.00    0.00       1/3           std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::~vector() [238]
                0.00    0.00       2/3           void std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_range_initialize<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::forward_iterator_tag) [235]
[180]    0.0    0.00    0.00       3         std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_get_Tp_allocator() [180]
-----------------------------------------------
                0.00    0.00       3/3           std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::__uninitialized_copy<false>::__uninit_copy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*) [232]
[181]    0.0    0.00    0.00       3         void std::_Construct<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&) [181]
                0.00    0.00       3/3           std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const& std::forward<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&>(std::remove_reference<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&>::type&) [183]
                0.00    0.00       3/2897193     operator new(unsigned long, void*) [129]
                0.00    0.00       3/590547      void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char*>(char*, char*, std::forward_iterator_tag) [149]
-----------------------------------------------
                0.00    0.00       3/3           std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::__uninitialized_copy<false>::__uninit_copy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*) [232]
[182]    0.0    0.00    0.00       3         std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::__addressof<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&) [182]
-----------------------------------------------
                0.00    0.00       3/3           void std::_Construct<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&) [181]
[183]    0.0    0.00    0.00       3         std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const& std::forward<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&>(std::remove_reference<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&>::type&) [183]
-----------------------------------------------
                0.00    0.00       2/2           std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::allocator(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [189]
[184]    0.0    0.00    0.00       2         __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::new_allocator(__gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [184]
-----------------------------------------------
                0.00    0.00       2/2           std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::allocator() [190]
[185]    0.0    0.00    0.00       2         __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::new_allocator() [185]
-----------------------------------------------
                0.00    0.00       1/2           std::allocator_traits<std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::max_size(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [231]
                0.00    0.00       1/2           __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::allocate(unsigned long, void const*) [202]
[186]    0.0    0.00    0.00       2         __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::max_size() const [186]
-----------------------------------------------
                0.00    0.00       1/2           std::filesystem::exists(std::filesystem::file_status) [210]
                0.00    0.00       1/2           std::filesystem::status_known(std::filesystem::file_status) [209]
[187]    0.0    0.00    0.00       2         std::filesystem::file_status::type() const [187]
-----------------------------------------------
                0.00    0.00       1/2           std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::vector(std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [237]
                0.00    0.00       1/2           std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::end() const [203]
[188]    0.0    0.00    0.00       2         std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::begin() const [188]
-----------------------------------------------
                0.00    0.00       1/2           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl::_Vector_impl(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [221]
                0.00    0.00       1/2           std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_S_check_init_len(unsigned long, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [234]
[189]    0.0    0.00    0.00       2         std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::allocator(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [189]
                0.00    0.00       2/2           __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::new_allocator(__gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [184]
-----------------------------------------------
                0.00    0.00       1/2           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl::_Vector_impl() [222]
                0.00    0.00       1/2           EdgeKey::EdgeKey(graph_opts) [82]
[190]    0.0    0.00    0.00       2         std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::allocator() [190]
                0.00    0.00       2/2           __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::new_allocator() [185]
-----------------------------------------------
                0.00    0.00       1/2           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl::_Vector_impl(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [221]
                0.00    0.00       1/2           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl::_Vector_impl() [222]
[191]    0.0    0.00    0.00       2         std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl_data::_Vector_impl_data() [191]
-----------------------------------------------
                0.00    0.00       2/2           EdgeKey::EdgeKey(graph_opts) [82]
[192]    0.0    0.00    0.00       2         std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&, char const*) [192]
                0.00    0.00       2/590547      void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char*>(char*, char*, std::forward_iterator_tag) [149]
-----------------------------------------------
                0.00    0.00       1/1           __libc_csu_init [79]
[193]    0.0    0.00    0.00       1         _GLOBAL__sub_I__Z14print_csv_infoNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEiP8bfs_info [193]
-----------------------------------------------
                0.00    0.00       1/1           EdgeKey::__restore_from_db(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >) [89]
[194]    0.0    0.00    0.00       1         CommonUtil::open_session(__wt_connection*, __wt_session**) [194]
-----------------------------------------------
                0.00    0.00       1/1           EdgeKey::close() [415]
[195]    0.0    0.00    0.00       1         CommonUtil::close_session(__wt_session*) [195]
-----------------------------------------------
                0.00    0.00       1/1           EdgeKey::__restore_from_db(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >) [89]
[196]    0.0    0.00    0.00       1         CommonUtil::open_connection(char*, __wt_connection**) [196]
-----------------------------------------------
                0.00    0.00       1/1           EdgeKey::close() [415]
[197]    0.0    0.00    0.00       1         CommonUtil::close_connection(__wt_connection*) [197]
-----------------------------------------------
                0.00    0.00       1/1           EdgeKey::EdgeKey(graph_opts) [82]
[198]    0.0    0.00    0.00       1         CommonUtil::check_graph_params(graph_opts) [198]
                0.00    0.00       1/1           std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::vector() [236]
                0.00    0.00       1/1           std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::size() const [206]
                0.00    0.00       1/1           std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::~vector() [238]
-----------------------------------------------
                0.00    0.00       1/1           EdgeKey::EdgeKey(graph_opts) [82]
[199]    0.0    0.00    0.00       1         graph_opts::graph_opts(graph_opts const&) [199]
-----------------------------------------------
                0.00    0.00       1/1           EdgeKey::EdgeKey(graph_opts) [82]
[200]    0.0    0.00    0.00       1         graph_opts::~graph_opts() [200]
-----------------------------------------------
                0.00    0.00       1/1           CmdLineApp::CmdLineApp(int, char**) [258]
[201]    0.0    0.00    0.00       1         CmdLineBase::CmdLineBase(int, char**) [201]
                0.00    0.00       5/5           void std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_realloc_insert<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >(__gnu_cxx::__normal_iterator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > > >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&) [177]
-----------------------------------------------
                0.00    0.00       1/1           std::allocator_traits<std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::allocate(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >&, unsigned long) [230]
[202]    0.0    0.00    0.00       1         __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::allocate(unsigned long, void const*) [202]
                0.00    0.00       1/2           __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::max_size() const [186]
-----------------------------------------------
                0.00    0.00       1/1           std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::vector(std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [237]
[203]    0.0    0.00    0.00       1         std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::end() const [203]
                0.00    0.00       1/2           std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::begin() const [188]
                0.00    0.00       1/1           std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::size() const [204]
-----------------------------------------------
                0.00    0.00       1/1           std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::end() const [203]
[204]    0.0    0.00    0.00       1         std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::size() const [204]
-----------------------------------------------
                0.00    0.00       1/1           CmdLineApp::CmdLineApp(int, char**) [258]
[205]    0.0    0.00    0.00       1         std::ctype<char>::do_widen(char) const [205]
-----------------------------------------------
                0.00    0.00       1/1           CommonUtil::check_graph_params(graph_opts) [198]
[206]    0.0    0.00    0.00       1         std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::size() const [206]
-----------------------------------------------
                0.00    0.00       1/1           std::_Tuple_impl<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_head(std::_Tuple_impl<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [217]
[207]    0.0    0.00    0.00       1         std::_Head_base<0ul, std::filesystem::__cxx11::path::_List::_Impl*, false>::_M_head(std::_Head_base<0ul, std::filesystem::__cxx11::path::_List::_Impl*, false>&) [207]
-----------------------------------------------
                0.00    0.00       1/1           std::_Tuple_impl<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_head(std::_Tuple_impl<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [218]
[208]    0.0    0.00    0.00       1         std::_Head_base<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter, true>::_M_head(std::_Head_base<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter, true>&) [208]
-----------------------------------------------
                0.00    0.00       1/1           std::filesystem::exists(std::filesystem::file_status) [210]
[209]    0.0    0.00    0.00       1         std::filesystem::status_known(std::filesystem::file_status) [209]
                0.00    0.00       1/2           std::filesystem::file_status::type() const [187]
-----------------------------------------------
                0.00    0.00       1/1           std::filesystem::exists(std::filesystem::__cxx11::path const&) [211]
[210]    0.0    0.00    0.00       1         std::filesystem::exists(std::filesystem::file_status) [210]
                0.00    0.00       1/1           std::filesystem::status_known(std::filesystem::file_status) [209]
                0.00    0.00       1/2           std::filesystem::file_status::type() const [187]
-----------------------------------------------
                0.00    0.00       1/1           EdgeKey::EdgeKey(graph_opts) [82]
[211]    0.0    0.00    0.00       1         std::filesystem::exists(std::filesystem::__cxx11::path const&) [211]
                0.00    0.00       1/1           std::filesystem::exists(std::filesystem::file_status) [210]
-----------------------------------------------
                0.00    0.00       1/1           std::filesystem::__cxx11::path::~path() [214]
[212]    0.0    0.00    0.00       1         std::filesystem::__cxx11::path::_List::~_List() [212]
                0.00    0.00       1/1           std::unique_ptr<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::~unique_ptr() [216]
-----------------------------------------------
                0.00    0.00       1/1           EdgeKey::EdgeKey(graph_opts) [82]
[213]    0.0    0.00    0.00       1         std::filesystem::__cxx11::path::path(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&, std::filesystem::__cxx11::path::format) [213]
                0.00    0.00       1/708604      std::remove_reference<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&>::type&& std::move<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&) [147]
-----------------------------------------------
                0.00    0.00       1/1           EdgeKey::EdgeKey(graph_opts) [82]
[214]    0.0    0.00    0.00       1         std::filesystem::__cxx11::path::~path() [214]
                0.00    0.00       1/1           std::filesystem::__cxx11::path::_List::~_List() [212]
-----------------------------------------------
                0.00    0.00       1/1           std::unique_ptr<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::~unique_ptr() [216]
[215]    0.0    0.00    0.00       1         std::unique_ptr<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::get_deleter() [215]
                0.00    0.00       1/1           std::__uniq_ptr_impl<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_deleter() [228]
-----------------------------------------------
                0.00    0.00       1/1           std::filesystem::__cxx11::path::_List::~_List() [212]
[216]    0.0    0.00    0.00       1         std::unique_ptr<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::~unique_ptr() [216]
                0.00    0.00       1/1           std::__uniq_ptr_impl<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_ptr() [229]
                0.00    0.00       1/1           std::unique_ptr<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::get_deleter() [215]
                0.00    0.00       1/1           std::remove_reference<std::filesystem::__cxx11::path::_List::_Impl*&>::type&& std::move<std::filesystem::__cxx11::path::_List::_Impl*&>(std::filesystem::__cxx11::path::_List::_Impl*&) [248]
-----------------------------------------------
                0.00    0.00       1/1           std::filesystem::__cxx11::path::_List::_Impl*& std::__get_helper<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::_Tuple_impl<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [241]
[217]    0.0    0.00    0.00       1         std::_Tuple_impl<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_head(std::_Tuple_impl<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [217]
                0.00    0.00       1/1           std::_Head_base<0ul, std::filesystem::__cxx11::path::_List::_Impl*, false>::_M_head(std::_Head_base<0ul, std::filesystem::__cxx11::path::_List::_Impl*, false>&) [207]
-----------------------------------------------
                0.00    0.00       1/1           std::filesystem::__cxx11::path::_List::_Impl_deleter& std::__get_helper<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::_Tuple_impl<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [242]
[218]    0.0    0.00    0.00       1         std::_Tuple_impl<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_head(std::_Tuple_impl<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [218]
                0.00    0.00       1/1           std::_Head_base<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter, true>::_M_head(std::_Head_base<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter, true>&) [208]
-----------------------------------------------
                0.00    0.00       1/1           void std::_Destroy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*) [249]
[219]    0.0    0.00    0.00       1         void std::_Destroy_aux<false>::__destroy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*) [219]
-----------------------------------------------
                0.00    0.00       1/1           void std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_range_initialize<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::forward_iterator_tag) [235]
[220]    0.0    0.00    0.00       1         std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_allocate(unsigned long) [220]
                0.00    0.00       1/1           std::allocator_traits<std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::allocate(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >&, unsigned long) [230]
-----------------------------------------------
                0.00    0.00       1/1           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_base(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [226]
[221]    0.0    0.00    0.00       1         std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl::_Vector_impl(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [221]
                0.00    0.00       1/2           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl_data::_Vector_impl_data() [191]
                0.00    0.00       1/2           std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::allocator(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [189]
-----------------------------------------------
                0.00    0.00       1/1           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_base() [225]
[222]    0.0    0.00    0.00       1         std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl::_Vector_impl() [222]
                0.00    0.00       1/2           std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::allocator() [190]
                0.00    0.00       1/2           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl_data::_Vector_impl_data() [191]
-----------------------------------------------
                0.00    0.00       1/1           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::~_Vector_base() [227]
[223]    0.0    0.00    0.00       1         std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl::~_Vector_impl() [223]
                0.00    0.00       1/3           std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::~allocator() [179]
-----------------------------------------------
                0.00    0.00       1/1           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::~_Vector_base() [227]
[224]    0.0    0.00    0.00       1         std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_deallocate(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, unsigned long) [224]
-----------------------------------------------
                0.00    0.00       1/1           std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::vector() [236]
[225]    0.0    0.00    0.00       1         std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_base() [225]
                0.00    0.00       1/1           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl::_Vector_impl() [222]
-----------------------------------------------
                0.00    0.00       1/1           std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::vector(std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [237]
[226]    0.0    0.00    0.00       1         std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_base(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [226]
                0.00    0.00       1/1           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl::_Vector_impl(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [221]
-----------------------------------------------
                0.00    0.00       1/1           std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::~vector() [238]
[227]    0.0    0.00    0.00       1         std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::~_Vector_base() [227]
                0.00    0.00       1/1           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl::~_Vector_impl() [223]
                0.00    0.00       1/1           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_deallocate(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, unsigned long) [224]
-----------------------------------------------
                0.00    0.00       1/1           std::unique_ptr<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::get_deleter() [215]
[228]    0.0    0.00    0.00       1         std::__uniq_ptr_impl<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_deleter() [228]
                0.00    0.00       1/1           std::tuple_element<1ul, std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter> >::type& std::get<1ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [247]
-----------------------------------------------
                0.00    0.00       1/1           std::unique_ptr<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::~unique_ptr() [216]
[229]    0.0    0.00    0.00       1         std::__uniq_ptr_impl<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_ptr() [229]
                0.00    0.00       1/1           std::tuple_element<0ul, std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter> >::type& std::get<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [246]
-----------------------------------------------
                0.00    0.00       1/1           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_allocate(unsigned long) [220]
[230]    0.0    0.00    0.00       1         std::allocator_traits<std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::allocate(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >&, unsigned long) [230]
                0.00    0.00       1/1           __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::allocate(unsigned long, void const*) [202]
-----------------------------------------------
                0.00    0.00       1/1           std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_S_max_size(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [233]
[231]    0.0    0.00    0.00       1         std::allocator_traits<std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::max_size(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [231]
                0.00    0.00       1/2           __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::max_size() const [186]
-----------------------------------------------
                0.00    0.00       1/1           std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::uninitialized_copy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*) [243]
[232]    0.0    0.00    0.00       1         std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::__uninitialized_copy<false>::__uninit_copy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*) [232]
                0.00    0.00       3/3           std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::__addressof<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&) [182]
                0.00    0.00       3/3           void std::_Construct<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&) [181]
-----------------------------------------------
                0.00    0.00       1/1           std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_S_check_init_len(unsigned long, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [234]
[233]    0.0    0.00    0.00       1         std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_S_max_size(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [233]
                0.00    0.00       1/1           std::allocator_traits<std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::max_size(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [231]
                0.00    0.00       1/763601      unsigned long const& std::min<unsigned long>(unsigned long const&, unsigned long const&) [140]
-----------------------------------------------
                0.00    0.00       1/1           void std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_range_initialize<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::forward_iterator_tag) [235]
[234]    0.0    0.00    0.00       1         std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_S_check_init_len(unsigned long, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [234]
                0.00    0.00       1/2           std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::allocator(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [189]
                0.00    0.00       1/1           std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_S_max_size(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [233]
                0.00    0.00       1/3           std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::~allocator() [179]
-----------------------------------------------
                0.00    0.00       1/1           std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::vector(std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [237]
[235]    0.0    0.00    0.00       1         void std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_range_initialize<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::forward_iterator_tag) [235]
                0.00    0.00       2/3           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_get_Tp_allocator() [180]
                0.00    0.00       1/1           std::iterator_traits<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>::difference_type std::distance<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*) [251]
                0.00    0.00       1/1           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_allocate(unsigned long) [220]
                0.00    0.00       1/1           std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_S_check_init_len(unsigned long, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [234]
                0.00    0.00       1/1           std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::__uninitialized_copy_a<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >&) [245]
-----------------------------------------------
                0.00    0.00       1/1           CommonUtil::check_graph_params(graph_opts) [198]
[236]    0.0    0.00    0.00       1         std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::vector() [236]
                0.00    0.00       1/1           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_base() [225]
-----------------------------------------------
                0.00    0.00       1/1           EdgeKey::EdgeKey(graph_opts) [82]
[237]    0.0    0.00    0.00       1         std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::vector(std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [237]
                0.00    0.00       1/1           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_base(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [226]
                0.00    0.00       1/1           std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::end() const [203]
                0.00    0.00       1/2           std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::begin() const [188]
                0.00    0.00       1/1           void std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_range_initialize<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::forward_iterator_tag) [235]
-----------------------------------------------
                0.00    0.00       1/1           CommonUtil::check_graph_params(graph_opts) [198]
[238]    0.0    0.00    0.00       1         std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::~vector() [238]
                0.00    0.00       1/3           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_get_Tp_allocator() [180]
                0.00    0.00       1/1           void std::_Destroy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >&) [250]
                0.00    0.00       1/1           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::~_Vector_base() [227]
-----------------------------------------------
                0.00    0.00       1/1           CmdLineApp::CmdLineApp(int, char**) [258]
[239]    0.0    0.00    0.00       1         std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_replace(unsigned long, unsigned long, char const*, unsigned long) [239]
                0.00    0.00       1/18          std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_mutate(unsigned long, unsigned long, char const*, unsigned long) [176]
-----------------------------------------------
                0.00    0.00       1/1           std::iterator_traits<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>::difference_type std::distance<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*) [251]
[240]    0.0    0.00    0.00       1         std::iterator_traits<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>::difference_type std::__distance<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::random_access_iterator_tag) [240]
-----------------------------------------------
                0.00    0.00       1/1           std::tuple_element<0ul, std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter> >::type& std::get<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [246]
[241]    0.0    0.00    0.00       1         std::filesystem::__cxx11::path::_List::_Impl*& std::__get_helper<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::_Tuple_impl<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [241]
                0.00    0.00       1/1           std::_Tuple_impl<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_head(std::_Tuple_impl<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [217]
-----------------------------------------------
                0.00    0.00       1/1           std::tuple_element<1ul, std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter> >::type& std::get<1ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [247]
[242]    0.0    0.00    0.00       1         std::filesystem::__cxx11::path::_List::_Impl_deleter& std::__get_helper<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::_Tuple_impl<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [242]
                0.00    0.00       1/1           std::_Tuple_impl<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_head(std::_Tuple_impl<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [218]
-----------------------------------------------
                0.00    0.00       1/1           std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::__uninitialized_copy_a<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >&) [245]
[243]    0.0    0.00    0.00       1         std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::uninitialized_copy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*) [243]
                0.00    0.00       1/1           std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::__uninitialized_copy<false>::__uninit_copy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*) [232]
-----------------------------------------------
                0.00    0.00       1/1           std::iterator_traits<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>::difference_type std::distance<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*) [251]
[244]    0.0    0.00    0.00       1         std::iterator_traits<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>::iterator_category std::__iterator_category<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const* const&) [244]
-----------------------------------------------
                0.00    0.00       1/1           void std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_range_initialize<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::forward_iterator_tag) [235]
[245]    0.0    0.00    0.00       1         std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::__uninitialized_copy_a<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >&) [245]
                0.00    0.00       1/1           std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::uninitialized_copy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*) [243]
-----------------------------------------------
                0.00    0.00       1/1           std::__uniq_ptr_impl<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_ptr() [229]
[246]    0.0    0.00    0.00       1         std::tuple_element<0ul, std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter> >::type& std::get<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [246]
                0.00    0.00       1/1           std::filesystem::__cxx11::path::_List::_Impl*& std::__get_helper<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::_Tuple_impl<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [241]
-----------------------------------------------
                0.00    0.00       1/1           std::__uniq_ptr_impl<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_deleter() [228]
[247]    0.0    0.00    0.00       1         std::tuple_element<1ul, std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter> >::type& std::get<1ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [247]
                0.00    0.00       1/1           std::filesystem::__cxx11::path::_List::_Impl_deleter& std::__get_helper<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::_Tuple_impl<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [242]
-----------------------------------------------
                0.00    0.00       1/1           std::unique_ptr<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::~unique_ptr() [216]
[248]    0.0    0.00    0.00       1         std::remove_reference<std::filesystem::__cxx11::path::_List::_Impl*&>::type&& std::move<std::filesystem::__cxx11::path::_List::_Impl*&>(std::filesystem::__cxx11::path::_List::_Impl*&) [248]
-----------------------------------------------
                0.00    0.00       1/1           void std::_Destroy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >&) [250]
[249]    0.0    0.00    0.00       1         void std::_Destroy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*) [249]
                0.00    0.00       1/1           void std::_Destroy_aux<false>::__destroy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*) [219]
-----------------------------------------------
                0.00    0.00       1/1           std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::~vector() [238]
[250]    0.0    0.00    0.00       1         void std::_Destroy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >&) [250]
                0.00    0.00       1/1           void std::_Destroy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*) [249]
-----------------------------------------------
                0.00    0.00       1/1           void std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_range_initialize<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::forward_iterator_tag) [235]
[251]    0.0    0.00    0.00       1         std::iterator_traits<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>::difference_type std::distance<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*) [251]
                0.00    0.00       1/1           std::iterator_traits<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>::iterator_category std::__iterator_category<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const* const&) [244]
                0.00    0.00       1/1           std::iterator_traits<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>::difference_type std::__distance<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::random_access_iterator_tag) [240]
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

 [193] _GLOBAL__sub_I__Z14print_csv_infoNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEiP8bfs_info (bfs.cpp) [175] std::_Deque_base<int, std::allocator<int> >::_M_initialize_map(unsigned long) [131] std::iterator_traits<char const*>::iterator_category std::__iterator_category<char const*>(char const* const&)
  [80] _GLOBAL__sub_I__Z8METADATAB5cxx11 (common.cpp) [217] std::_Tuple_impl<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_head(std::_Tuple_impl<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [151] std::iterator_traits<char*>::iterator_category std::__iterator_category<char*>(char* const&)
  [83] _GLOBAL__sub_I__ZN13StandardGraphC2Ev (standard_graph.cpp) [218] std::_Tuple_impl<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_head(std::_Tuple_impl<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [18] void std::__relocate_object_a<node, node, std::allocator<node> >(node*, node*, std::allocator<node>&)
  [84] _GLOBAL__sub_I__ZN7AdjListC2Ev (adj_list.cpp) [14] std::char_traits<char>::length(char const*) [245] std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::__uninitialized_copy_a<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >&)
  [85] _GLOBAL__sub_I__ZN7EdgeKeyC2Ev (edgekey.cpp) [219] void std::_Destroy_aux<false>::__destroy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*) [246] std::tuple_element<0ul, std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter> >::type& std::get<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&)
   [3] bfs_info* bfs<EdgeKey>(EdgeKey&, int) [156] std::_Vector_base<node, std::allocator<node> >::_M_allocate(unsigned long) [247] std::tuple_element<1ul, std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter> >::type& std::get<1ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&)
  [86] __static_initialization_and_destruction_0(int, int) (adj_list.cpp) [168] std::_Vector_base<node, std::allocator<node> >::_Vector_impl::_Vector_impl() [161] unsigned long const& std::max<unsigned long>(unsigned long const&, unsigned long const&)
  [81] __static_initialization_and_destruction_0(int, int) (common.cpp) [157] std::_Vector_base<node, std::allocator<node> >::_M_deallocate(node*, unsigned long) [140] unsigned long const& std::min<unsigned long>(unsigned long const&, unsigned long const&)
  [87] __static_initialization_and_destruction_0(int, int) (edgekey.cpp) [169] std::_Vector_base<node, std::allocator<node> >::_Vector_impl_data::_Vector_impl_data() [46] std::remove_reference<node&>::type&& std::move<node&>(node&)
  [88] __static_initialization_and_destruction_0(int, int) (standard_graph.cpp) [144] std::_Vector_base<node, std::allocator<node> >::_M_get_Tp_allocator() [147] std::remove_reference<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&>::type&& std::move<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&)
 [194] CommonUtil::open_session(__wt_connection*, __wt_session**) [170] std::_Vector_base<node, std::allocator<node> >::_Vector_base() [248] std::remove_reference<std::filesystem::__cxx11::path::_List::_Impl*&>::type&& std::move<std::filesystem::__cxx11::path::_List::_Impl*&>(std::filesystem::__cxx11::path::_List::_Impl*&)
 [195] CommonUtil::close_session(__wt_session*) [220] std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_allocate(unsigned long) [127] node&& std::forward<node>(std::remove_reference<node>::type&)
 [196] CommonUtil::open_connection(char*, __wt_connection**) [221] std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl::_Vector_impl(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [128] node const& std::forward<node const&>(std::remove_reference<node const&>::type&)
 [197] CommonUtil::close_connection(__wt_connection*) [222] std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl::_Vector_impl() [183] std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const& std::forward<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&>(std::remove_reference<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&>::type&)
 [198] CommonUtil::check_graph_params(graph_opts) [223] std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl::~_Vector_impl() [249] void std::_Destroy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*)
 [199] graph_opts::graph_opts(graph_opts const&) [224] std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_deallocate(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, unsigned long) [250] void std::_Destroy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >&)
 [200] graph_opts::~graph_opts() [191] std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl_data::_Vector_impl_data() [251] std::iterator_traits<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>::difference_type std::distance<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*)
 [201] CmdLineBase::CmdLineBase(int, char**) [180] std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_get_Tp_allocator() [44] std::iterator_traits<char const*>::difference_type std::distance<char const*>(char const*, char const*)
 [138] node::node()          [225] std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_base() [152] std::iterator_traits<char*>::difference_type std::distance<char*>(char*, char*)
   [4] EdgeKey::get_out_nodes(int) [226] std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_base(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [137] std::operator|(std::_Ios_Openmode, std::_Ios_Openmode)
  [77] EdgeKey::get_out_degree(int) [227] std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::~_Vector_base() [163] std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&, char const*)
  [38] EdgeKey::get_edge_cursor() [228] std::__uniq_ptr_impl<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_deleter() [162] std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&)
  [78] EdgeKey::get_random_node() [229] std::__uniq_ptr_impl<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_ptr() [71] std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(char const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&)
  [74] EdgeKey::get_src_idx_cur() [165] std::allocator_traits<std::allocator<node> >::deallocate(std::allocator<node>&, node*, unsigned long) [192] std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&, char const*)
   [7] EdgeKey::__record_to_node(__wt_cursor*, node*) [134] void std::allocator_traits<std::allocator<node> >::destroy<node>(std::allocator<node>&, node*) [129] operator new(unsigned long, void*)
  [89] EdgeKey::__restore_from_db(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >) [158] std::allocator_traits<std::allocator<node> >::allocate(std::allocator<node>&, unsigned long) [29] __curfile_search (cur_file.c)
  [75] EdgeKey::_get_index_cursor(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, __wt_cursor**) [145] std::allocator_traits<std::allocator<node> >::max_size(std::allocator<node> const&) [51] __curindex_get_value (cur_index.c)
  [73] EdgeKey::_get_table_cursor(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, __wt_cursor**, bool) [49] void std::allocator_traits<std::allocator<node> >::construct<node, node const&>(std::allocator<node>&, node*, node const&) [52] __curindex_move (cur_index.c)
  [47] EdgeKey::extract_from_string(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, int*, int*) [28] void std::allocator_traits<std::allocator<node> >::construct<node, node>(std::allocator<node>&, node*, node&&) [30] __curindex_next (cur_index.c)
  [72] EdgeKey::has_node(int) [230] std::allocator_traits<std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::allocate(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >&, unsigned long) [53] __curtable_search (cur_table.c)
 [164] __gnu_cxx::new_allocator<node>::deallocate(node*, unsigned long) [231] std::allocator_traits<std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::max_size(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [2] __global_once (global.c)
 [133] void __gnu_cxx::new_allocator<node>::destroy<node>(node*) [232] std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::__uninitialized_copy<false>::__uninit_copy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*) [54] __pack_write (packing.i)
 [153] __gnu_cxx::new_allocator<node>::allocate(unsigned long, void const*) [174] void std::deque<int, std::allocator<int> >::_M_push_back_aux<int const&>(int const&) [55] __schema_open_index (schema_open.c)
  [48] void __gnu_cxx::new_allocator<node>::construct<node, node const&>(node*, node const&) [146] std::vector<node, std::allocator<node> >::_S_max_size(std::allocator<node> const&) [31] __session_open_cursor_int (session_api.c)
  [45] void __gnu_cxx::new_allocator<node>::construct<node, node>(node*, node&&) [12] std::vector<node, std::allocator<node> >::_S_relocate(node*, node*, node*, std::allocator<node>&) [56] __unpack_read (packing.i)
 [166] __gnu_cxx::new_allocator<node>::new_allocator() [15] std::vector<node, std::allocator<node> >::_S_do_relocate(node*, node*, node*, std::allocator<node>&, std::integral_constant<bool, true>) [19] __wt_btcur_close
 [202] __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::allocate(unsigned long, void const*) [11] void std::vector<node, std::allocator<node> >::_M_realloc_insert<node const&>(__gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >, node const&) [57] __wt_btcur_iterate_setup
 [184] __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::new_allocator(__gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [159] std::vector<node, std::allocator<node> >::end() [20] __wt_btcur_next
 [185] __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::new_allocator() [160] std::vector<node, std::allocator<node> >::begin() [32] __wt_btcur_search
 [178] __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::~new_allocator() [10] std::vector<node, std::allocator<node> >::push_back(node const&) [58] __wt_btcur_search_near
 [130] bool __gnu_cxx::__is_null_pointer<char const>(char const*) [171] std::vector<node, std::allocator<node> >::vector() [59] __wt_clock_to_nsec
 [148] bool __gnu_cxx::__is_null_pointer<char>(char*) [50] std::vector<node, std::allocator<node> >::~vector() [33] __wt_config_gets_def
 [141] __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >::__normal_iterator(node* const&) [233] std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_S_max_size(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [13] __wt_config_next
 [154] __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >::difference_type __gnu_cxx::operator-<node*, std::vector<node, std::allocator<node> > >(__gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > > const&, __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > > const&) [177] void std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_realloc_insert<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >(__gnu_cxx::__normal_iterator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > > >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&) [24] __wt_curfile_open
 [139] __gnu_cxx::new_allocator<node>::max_size() const [234] std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_S_check_init_len(unsigned long, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [34] __wt_cursor_cache_get
 [186] __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::max_size() const [235] void std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_range_initialize<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::forward_iterator_tag) [25] __wt_cursor_close
 [135] __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >::base() const [236] std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::vector() [26] __wt_cursor_set_keyv
 [187] std::filesystem::file_status::type() const [237] std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::vector(std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [60] __wt_curtable_get_key
 [142] std::_Vector_base<node, std::allocator<node> >::_M_get_Tp_allocator() const [238] std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::~vector() [61] __wt_curtable_get_value
 [203] std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::end() const [172] void std::vector<int, std::allocator<int> >::_M_realloc_insert<int const&>(__gnu_cxx::__normal_iterator<int*, std::vector<int, std::allocator<int> > >, int const&) [62] __wt_curtable_open
 [204] std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::size() const [239] std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_replace(unsigned long, unsigned long, char const*, unsigned long) [63] __wt_curtable_set_key
 [188] std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::begin() const [40] void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char const*>(char const*, char const*) [1] __wt_hazard_clear
 [205] std::ctype<char>::do_widen(char) const [41] void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char const*>(char const*, char const*, std::forward_iterator_tag) [21] __wt_hazard_set
 [155] std::vector<node, std::allocator<node> >::_M_check_len(unsigned long, char const*) const [149] void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char*>(char*, char*, std::forward_iterator_tag) [35] __wt_malloc
 [136] std::vector<node, std::allocator<node> >::size() const [42] void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct_aux<char const*>(char const*, char const*, std::__false_type) [36] __wt_metadata_cursor
 [143] std::vector<node, std::allocator<node> >::max_size() const [176] std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_mutate(unsigned long, unsigned long, char const*, unsigned long) [37] __wt_page_in_func
 [206] std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::size() const [9] std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::basic_string<std::allocator<char> >(char const*, std::allocator<char> const&) [64] __wt_readlock
 [167] std::allocator<node>::allocator() [173] std::_Rb_tree<int, int, std::_Identity<int>, std::less<int>, std::allocator<int> >::_M_erase(std::_Rb_tree_node<int>*) [65] __wt_readunlock
 [189] std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::allocator(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [181] void std::_Construct<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&) [66] __wt_realloc_noclear
 [190] std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::allocator() [240] std::iterator_traits<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>::difference_type std::__distance<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::random_access_iterator_tag) [5] __wt_row_search
 [179] std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::~allocator() [43] std::iterator_traits<char const*>::difference_type std::__distance<char const*>(char const*, char const*, std::random_access_iterator_tag) [67] __wt_schema_open_index
 [207] std::_Head_base<0ul, std::filesystem::__cxx11::path::_List::_Impl*, false>::_M_head(std::_Head_base<0ul, std::filesystem::__cxx11::path::_List::_Impl*, false>&) [150] std::iterator_traits<char*>::difference_type std::__distance<char*>(char*, char*, std::random_access_iterator_tag) [27] __wt_schema_open_table
 [208] std::_Head_base<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter, true>::_M_head(std::_Head_base<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter, true>&) [126] node* std::__addressof<node>(node&) [8] __wt_schema_project_out
 [209] std::filesystem::status_known(std::filesystem::file_status) [182] std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::__addressof<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&) [6] __wt_schema_project_slice
 [210] std::filesystem::exists(std::filesystem::file_status) [241] std::filesystem::__cxx11::path::_List::_Impl*& std::__get_helper<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::_Tuple_impl<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [68] __wt_session_gen_leave
 [211] std::filesystem::exists(std::filesystem::__cxx11::path const&) [242] std::filesystem::__cxx11::path::_List::_Impl_deleter& std::__get_helper<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::_Tuple_impl<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [69] __wt_strndup
 [212] std::filesystem::__cxx11::path::_List::~_List() [132] node* std::__niter_base<node*>(node*) [22] __wt_struct_packv (packing.i)
 [213] std::filesystem::__cxx11::path::path(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&, std::filesystem::__cxx11::path::format) [16] node* std::__relocate_a<node*, node*, std::allocator<node> >(node*, node*, node*, std::allocator<node>&) [23] __wt_struct_sizev (packing.i)
 [214] std::filesystem::__cxx11::path::~path() [17] node* std::__relocate_a_1<node*, node*, std::allocator<node> >(node*, node*, node*, std::allocator<node>&) [70] __wt_writelock
 [215] std::unique_ptr<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::get_deleter() [243] std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::uninitialized_copy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*) [39] wiredtiger_crc32c_func
 [216] std::unique_ptr<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::~unique_ptr() [244] std::iterator_traits<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>::iterator_category std::__iterator_category<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const* const&)
