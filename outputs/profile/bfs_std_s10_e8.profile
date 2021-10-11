Flat profile:

Each sample counts as 0.01 seconds.
  %   cumulative   self              self     total           
 time   seconds   seconds    calls  ms/call  ms/call  name    
 60.10      2.35     2.35                             __wt_hazard_clear
  8.18      2.67     0.32                             __global_once
  6.14      2.91     0.24                             __wt_row_search
  4.09      3.07     0.16                             bfs_info* bfs<StandardGraph>(StandardGraph&, int)
  2.56      3.17     0.10                             __wt_schema_project_out
  1.28      3.22     0.05                             __wt_session_gen_leave
  1.28      3.27     0.05                             __wt_cursor_valid
  1.02      3.31     0.04                             __wt_config_next
  0.77      3.34     0.03                             __curfile_search
  0.77      3.37     0.03                             __wt_curtable_open
  0.77      3.40     0.03                             __wt_hazard_set
  0.77      3.43     0.03                             __wt_schema_project_slice
  0.64      3.46     0.03                             __wt_cursor_set_keyv
  0.51      3.48     0.02   354300     0.00     0.00  void std::_Destroy<edge*, edge>(edge*, edge*, std::allocator<edge>&)
  0.51      3.50     0.02                             __curindex_get_value
  0.51      3.52     0.02                             __cursor_func_init.constprop.0
  0.51      3.54     0.02                             __wt_btcur_search
  0.51      3.56     0.02                             __wt_clock_to_nsec
  0.51      3.58     0.02                             __wt_curindex_open
  0.51      3.60     0.02                             __wt_page_in_func
  0.51      3.62     0.02                             __wt_session_copy_values
  0.38      3.63     0.02                             __wt_cursor_set_key
  0.26      3.64     0.01  2284980     0.00     0.00  edge* std::__niter_base<edge*>(edge*)
  0.26      3.65     0.01  1641420     0.00     0.00  std::vector<edge, std::allocator<edge> >::size() const
  0.26      3.66     0.01  1371250     0.00     0.00  bool __gnu_cxx::operator!=<edge*, std::vector<edge, std::allocator<edge> > >(__gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > > const&, __gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > > const&)
  0.26      3.67     0.01  1268670     0.00     0.00  void std::allocator_traits<std::allocator<node> >::construct<node, node const&>(std::allocator<node>&, node*, node const&)
  0.26      3.68     0.01   441423     0.00     0.00  std::char_traits<char>::length(char const*)
  0.26      3.69     0.01   118100     0.00     0.00  StandardGraph::get_out_nodes(int)
  0.26      3.70     0.01                             __curindex_compare
  0.26      3.71     0.01                             __curindex_search
  0.26      3.72     0.01                             __cursor_row_slot_return
  0.26      3.73     0.01                             __curtable_search
  0.26      3.74     0.01                             __pack_write
  0.26      3.75     0.01                             __schema_open_index
  0.26      3.76     0.01                             __session_find_dhandle
  0.26      3.77     0.01                             __unpack_read
  0.26      3.78     0.01                             __wt_btcur_close
  0.26      3.79     0.01                             __wt_btcur_search_near
  0.26      3.80     0.01                             __wt_buf_catfmt
  0.26      3.81     0.01                             __wt_config_gets_def
  0.26      3.82     0.01                             __wt_curtable_set_key
  0.26      3.83     0.01                             __wt_hash_city64
  0.26      3.84     0.01                             __wt_readunlock
  0.26      3.85     0.01                             __wt_schema_open_index
  0.26      3.86     0.01                             __wt_schema_open_table
  0.26      3.87     0.01                             __wt_scr_alloc_func
  0.26      3.88     0.01                             __wt_session_gen_enter
  0.26      3.89     0.01                             __wt_strndup
  0.26      3.90     0.01                             __wt_struct_plan
  0.13      3.91     0.01   338813     0.00     0.00  std::iterator_traits<char*>::difference_type std::__distance<char*>(char*, char*, std::random_access_iterator_tag)
  0.13      3.91     0.01        1     5.00     5.00  std::_Tuple_impl<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_head(std::_Tuple_impl<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&)
  0.00      3.91     0.00  4761630     0.00     0.00  node* std::__addressof<node>(node&)
  0.00      3.91     0.00  4265820     0.00     0.00  __gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > >::base() const
  0.00      3.91     0.00  4124554     0.00     0.00  operator new(unsigned long, void*)
  0.00      3.91     0.00  3174420     0.00     0.00  node&& std::forward<node>(std::remove_reference<node>::type&)
  0.00      3.91     0.00  2918170     0.00     0.00  edge const& std::forward<edge const&>(std::remove_reference<edge const&>::type&)
  0.00      3.91     0.00  2918170     0.00     0.00  node const& std::forward<node const&>(std::remove_reference<node const&>::type&)
  0.00      3.91     0.00  2284980     0.00     0.00  node* std::__niter_base<node*>(node*)
  0.00      3.91     0.00  1587210     0.00     0.00  void __gnu_cxx::new_allocator<node>::destroy<node>(node*)
  0.00      3.91     0.00  1587210     0.00     0.00  void __gnu_cxx::new_allocator<node>::construct<node, node>(node*, node&&)
  0.00      3.91     0.00  1587210     0.00     0.00  void std::allocator_traits<std::allocator<node> >::destroy<node>(std::allocator<node>&, node*)
  0.00      3.91     0.00  1587210     0.00     0.00  void std::allocator_traits<std::allocator<node> >::construct<node, node>(std::allocator<node>&, node*, node&&)
  0.00      3.91     0.00  1587210     0.00     0.00  void std::__relocate_object_a<node, node, std::allocator<node> >(node*, node*, std::allocator<node>&)
  0.00      3.91     0.00  1587210     0.00     0.00  std::remove_reference<node&>::type&& std::move<node&>(node&)
  0.00      3.91     0.00  1523322     0.00     0.00  unsigned long const& std::min<unsigned long>(unsigned long const&, unsigned long const&)
  0.00      3.91     0.00  1523320     0.00     0.00  __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >::base() const
  0.00      3.91     0.00  1523320     0.00     0.00  std::vector<node, std::allocator<node> >::size() const
  0.00      3.91     0.00  1371090     0.00     0.00  StandardGraph::__read_from_edge_idx(__wt_cursor*, edge*)
  0.00      3.91     0.00  1352160     0.00     0.00  std::_Vector_base<edge, std::allocator<edge> >::_M_get_Tp_allocator()
  0.00      3.91     0.00  1268706     0.00     0.00  StandardGraph::__record_to_node(__wt_cursor*)
  0.00      3.91     0.00  1268670     0.00     0.00  void __gnu_cxx::new_allocator<edge>::construct<edge, edge const&>(edge*, edge const&)
  0.00      3.91     0.00  1268670     0.00     0.00  void __gnu_cxx::new_allocator<node>::construct<node, node const&>(node*, node const&)
  0.00      3.91     0.00  1268670     0.00     0.00  __gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > >::operator++()
  0.00      3.91     0.00  1268670     0.00     0.00  __gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > >::operator*() const
  0.00      3.91     0.00  1268670     0.00     0.00  void std::allocator_traits<std::allocator<edge> >::construct<edge, edge const&>(std::allocator<edge>&, edge*, edge const&)
  0.00      3.91     0.00  1268670     0.00     0.00  std::vector<edge, std::allocator<edge> >::push_back(edge const&)
  0.00      3.91     0.00  1268670     0.00     0.00  std::vector<node, std::allocator<node> >::push_back(node const&)
  0.00      3.91     0.00  1142490     0.00     0.00  __gnu_cxx::new_allocator<edge>::max_size() const
  0.00      3.91     0.00  1142490     0.00     0.00  __gnu_cxx::new_allocator<node>::max_size() const
  0.00      3.91     0.00   966820     0.00     0.00  __gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > >::__normal_iterator(edge* const&)
  0.00      3.91     0.00   944804     0.00     0.00  std::remove_reference<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&>::type&& std::move<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&)
  0.00      3.91     0.00   879760     0.00     0.00  std::_Vector_base<edge, std::allocator<edge> >::_M_get_Tp_allocator() const
  0.00      3.91     0.00   761660     0.00     0.00  __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >::__normal_iterator(node* const&)
  0.00      3.91     0.00   761660     0.00     0.00  std::_Vector_base<node, std::allocator<node> >::_M_get_Tp_allocator() const
  0.00      3.91     0.00   761660     0.00     0.00  std::vector<edge, std::allocator<edge> >::max_size() const
  0.00      3.91     0.00   761660     0.00     0.00  std::vector<node, std::allocator<node> >::max_size() const
  0.00      3.91     0.00   761660     0.00     0.00  std::_Vector_base<node, std::allocator<node> >::_M_get_Tp_allocator()
  0.00      3.91     0.00   761660     0.00     0.00  std::allocator_traits<std::allocator<edge> >::max_size(std::allocator<edge> const&)
  0.00      3.91     0.00   761660     0.00     0.00  std::allocator_traits<std::allocator<node> >::max_size(std::allocator<node> const&)
  0.00      3.91     0.00   761660     0.00     0.00  std::vector<edge, std::allocator<edge> >::_S_max_size(std::allocator<edge> const&)
  0.00      3.91     0.00   761660     0.00     0.00  std::vector<edge, std::allocator<edge> >::_S_relocate(edge*, edge*, edge*, std::allocator<edge>&)
  0.00      3.91     0.00   761660     0.00     0.00  std::vector<edge, std::allocator<edge> >::_S_do_relocate(edge*, edge*, edge*, std::allocator<edge>&, std::integral_constant<bool, true>)
  0.00      3.91     0.00   761660     0.00     0.00  std::vector<node, std::allocator<node> >::_S_max_size(std::allocator<node> const&)
  0.00      3.91     0.00   761660     0.00     0.00  std::vector<node, std::allocator<node> >::_S_relocate(node*, node*, node*, std::allocator<node>&)
  0.00      3.91     0.00   761660     0.00     0.00  std::vector<node, std::allocator<node> >::_S_do_relocate(node*, node*, node*, std::allocator<node>&, std::integral_constant<bool, true>)
  0.00      3.91     0.00   761660     0.00     0.00  edge* std::__relocate_a<edge*, edge*, std::allocator<edge> >(edge*, edge*, edge*, std::allocator<edge>&)
  0.00      3.91     0.00   761660     0.00     0.00  node* std::__relocate_a<node*, node*, std::allocator<node> >(node*, node*, node*, std::allocator<node>&)
  0.00      3.91     0.00   761660     0.00     0.00  std::enable_if<std::__is_bitwise_relocatable<edge, void>::value, edge*>::type std::__relocate_a_1<edge, edge>(edge*, edge*, edge*, std::allocator<edge>&)
  0.00      3.91     0.00   761660     0.00     0.00  node* std::__relocate_a_1<node*, node*, std::allocator<node> >(node*, node*, node*, std::allocator<node>&)
  0.00      3.91     0.00   761660     0.00     0.00  unsigned long const& std::max<unsigned long>(unsigned long const&, unsigned long const&)
  0.00      3.91     0.00   735130     0.00     0.00  std::_Vector_base<edge, std::allocator<edge> >::_M_deallocate(edge*, unsigned long)
  0.00      3.91     0.00   708600     0.00     0.00  std::_Vector_base<edge, std::allocator<edge> >::_Vector_impl_data::_M_copy_data(std::_Vector_base<edge, std::allocator<edge> >::_Vector_impl_data const&)
  0.00      3.91     0.00   590500     0.00     0.00  std::_Vector_base<edge, std::allocator<edge> >::_Vector_impl_data::_Vector_impl_data()
  0.00      3.91     0.00   483410     0.00     0.00  std::vector<edge, std::allocator<edge> >::end()
  0.00      3.91     0.00   483410     0.00     0.00  std::vector<edge, std::allocator<edge> >::begin()
  0.00      3.91     0.00   472403     0.00     0.00  std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&)
  0.00      3.91     0.00   472400     0.00     0.00  __gnu_cxx::new_allocator<edge>::~new_allocator()
  0.00      3.91     0.00   472400     0.00     0.00  std::allocator<edge>::~allocator()
  0.00      3.91     0.00   472400     0.00     0.00  std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&, char const*)
  0.00      3.91     0.00   380830     0.00     0.00  __gnu_cxx::new_allocator<edge>::deallocate(edge*, unsigned long)
  0.00      3.91     0.00   380830     0.00     0.00  __gnu_cxx::new_allocator<edge>::allocate(unsigned long, void const*)
  0.00      3.91     0.00   380830     0.00     0.00  __gnu_cxx::new_allocator<node>::allocate(unsigned long, void const*)
  0.00      3.91     0.00   380830     0.00     0.00  __gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > >::difference_type __gnu_cxx::operator-<edge*, std::vector<edge, std::allocator<edge> > >(__gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > > const&, __gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > > const&)
  0.00      3.91     0.00   380830     0.00     0.00  __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >::difference_type __gnu_cxx::operator-<node*, std::vector<node, std::allocator<node> > >(__gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > > const&, __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > > const&)
  0.00      3.91     0.00   380830     0.00     0.00  std::vector<edge, std::allocator<edge> >::_M_check_len(unsigned long, char const*) const
  0.00      3.91     0.00   380830     0.00     0.00  std::vector<node, std::allocator<node> >::_M_check_len(unsigned long, char const*) const
  0.00      3.91     0.00   380830     0.00     0.00  std::_Vector_base<edge, std::allocator<edge> >::_M_allocate(unsigned long)
  0.00      3.91     0.00   380830     0.00     0.00  std::_Vector_base<node, std::allocator<node> >::_M_allocate(unsigned long)
  0.00      3.91     0.00   380830     0.00     0.00  std::_Vector_base<node, std::allocator<node> >::_M_deallocate(node*, unsigned long)
  0.00      3.91     0.00   380830     0.00     0.00  std::allocator_traits<std::allocator<edge> >::deallocate(std::allocator<edge>&, edge*, unsigned long)
  0.00      3.91     0.00   380830     0.00     0.00  std::allocator_traits<std::allocator<edge> >::allocate(std::allocator<edge>&, unsigned long)
  0.00      3.91     0.00   380830     0.00     0.00  std::allocator_traits<std::allocator<node> >::allocate(std::allocator<node>&, unsigned long)
  0.00      3.91     0.00   380830     0.00     0.00  void std::vector<edge, std::allocator<edge> >::_M_realloc_insert<edge const&>(__gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > >, edge const&)
  0.00      3.91     0.00   380830     0.00     0.00  void std::vector<node, std::allocator<node> >::_M_realloc_insert<node const&>(__gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >, node const&)
  0.00      3.91     0.00   380830     0.00     0.00  std::vector<node, std::allocator<node> >::end()
  0.00      3.91     0.00   380830     0.00     0.00  std::vector<node, std::allocator<node> >::begin()
  0.00      3.91     0.00   354300     0.00     0.00  void std::_Destroy_aux<true>::__destroy<edge*>(edge*, edge*)
  0.00      3.91     0.00   354300     0.00     0.00  std::_Vector_base<edge, std::allocator<edge> >::_Vector_impl::~_Vector_impl()
  0.00      3.91     0.00   354300     0.00     0.00  std::_Vector_base<edge, std::allocator<edge> >::~_Vector_base()
  0.00      3.91     0.00   354300     0.00     0.00  std::vector<edge, std::allocator<edge> >::~vector()
  0.00      3.91     0.00   354300     0.00     0.00  void std::_Destroy<edge*>(edge*, edge*)
  0.00      3.91     0.00   338813     0.00     0.00  bool __gnu_cxx::__is_null_pointer<char>(char*)
  0.00      3.91     0.00   338813     0.00     0.00  void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char*>(char*, char*, std::forward_iterator_tag)
  0.00      3.91     0.00   338813     0.00     0.00  std::iterator_traits<char*>::iterator_category std::__iterator_category<char*>(char* const&)
  0.00      3.91     0.00   338813     0.00     0.00  std::iterator_traits<char*>::difference_type std::distance<char*>(char*, char*)
  0.00      3.91     0.00   338805     0.00     0.00  std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(char const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&)
  0.00      3.91     0.00   278250     0.00     0.00  __gnu_cxx::new_allocator<node>::deallocate(node*, unsigned long)
  0.00      3.91     0.00   278250     0.00     0.00  std::allocator_traits<std::allocator<node> >::deallocate(std::allocator<node>&, node*, unsigned long)
  0.00      3.91     0.00   236200     0.00     0.00  __gnu_cxx::new_allocator<edge>::new_allocator(__gnu_cxx::new_allocator<edge> const&)
  0.00      3.91     0.00   236200     0.00     0.00  __gnu_cxx::new_allocator<edge>::new_allocator()
  0.00      3.91     0.00   236200     0.00     0.00  std::allocator<edge>::allocator(std::allocator<edge> const&)
  0.00      3.91     0.00   236200     0.00     0.00  std::allocator<edge>::allocator()
  0.00      3.91     0.00   236200     0.00     0.00  std::_Vector_base<edge, std::allocator<edge> >::_Vector_impl::_Vector_impl()
  0.00      3.91     0.00   236200     0.00     0.00  std::_Vector_base<edge, std::allocator<edge> >::_Vector_impl_data::_M_swap_data(std::_Vector_base<edge, std::allocator<edge> >::_Vector_impl_data&)
  0.00      3.91     0.00   236200     0.00     0.00  std::_Vector_base<edge, std::allocator<edge> >::_Vector_base()
  0.00      3.91     0.00   236200     0.00     0.00  std::vector<edge, std::allocator<edge> >::vector()
  0.00      3.91     0.00   118100     0.00     0.00  StandardGraph::get_out_edges(int)
  0.00      3.91     0.00   118100     0.00     0.00  StandardGraph::_get_index_cursor(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, __wt_cursor**)
  0.00      3.91     0.00   118100     0.00     0.00  StandardGraph::has_node(int)
  0.00      3.91     0.00   118100     0.00     0.00  __gnu_cxx::new_allocator<node>::new_allocator()
  0.00      3.91     0.00   118100     0.00     0.00  std::_Vector_base<edge, std::allocator<edge> >::get_allocator() const
  0.00      3.91     0.00   118100     0.00     0.00  std::allocator<node>::allocator()
  0.00      3.91     0.00   118100     0.00     0.00  std::_Vector_base<edge, std::allocator<edge> >::_Vector_impl::_Vector_impl(std::allocator<edge> const&)
  0.00      3.91     0.00   118100     0.00     0.00  std::_Vector_base<edge, std::allocator<edge> >::_Vector_base(std::allocator<edge> const&)
  0.00      3.91     0.00   118100     0.00     0.00  std::_Vector_base<node, std::allocator<node> >::_Vector_impl::_Vector_impl()
  0.00      3.91     0.00   118100     0.00     0.00  std::_Vector_base<node, std::allocator<node> >::_Vector_impl_data::_Vector_impl_data()
  0.00      3.91     0.00   118100     0.00     0.00  std::_Vector_base<node, std::allocator<node> >::_Vector_base()
  0.00      3.91     0.00   118100     0.00     0.00  std::vector<edge, std::allocator<edge> >::_M_move_assign(std::vector<edge, std::allocator<edge> >&&, std::integral_constant<bool, true>)
  0.00      3.91     0.00   118100     0.00     0.00  std::vector<edge, std::allocator<edge> >::vector(std::allocator<edge> const&)
  0.00      3.91     0.00   118100     0.00     0.00  std::vector<edge, std::allocator<edge> >::operator=(std::vector<edge, std::allocator<edge> >&&)
  0.00      3.91     0.00   118100     0.00     0.00  std::vector<node, std::allocator<node> >::vector()
  0.00      3.91     0.00   118100     0.00     0.00  void std::__alloc_on_move<std::allocator<edge> >(std::allocator<edge>&, std::allocator<edge>&)
  0.00      3.91     0.00   118100     0.00     0.00  void std::__do_alloc_on_move<std::allocator<edge> >(std::allocator<edge>&, std::allocator<edge>&, std::integral_constant<bool, true>)
  0.00      3.91     0.00   118100     0.00     0.00  std::remove_reference<std::allocator<edge>&>::type&& std::move<std::allocator<edge>&>(std::allocator<edge>&)
  0.00      3.91     0.00   118100     0.00     0.00  std::remove_reference<std::vector<edge, std::allocator<edge> >&>::type&& std::move<std::vector<edge, std::allocator<edge> >&>(std::vector<edge, std::allocator<edge> >&)
  0.00      3.91     0.00   102618     0.00     0.00  bool __gnu_cxx::__is_null_pointer<char const>(char const*)
  0.00      3.91     0.00   102618     0.00     0.00  void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char const*>(char const*, char const*)
  0.00      3.91     0.00   102618     0.00     0.00  void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char const*>(char const*, char const*, std::forward_iterator_tag)
  0.00      3.91     0.00   102618     0.00     0.00  void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct_aux<char const*>(char const*, char const*, std::__false_type)
  0.00      3.91     0.00   102618     0.00     0.00  std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::basic_string<std::allocator<char> >(char const*, std::allocator<char> const&)
  0.00      3.91     0.00   102618     0.00     0.00  std::iterator_traits<char const*>::difference_type std::__distance<char const*>(char const*, char const*, std::random_access_iterator_tag)
  0.00      3.91     0.00   102618     0.00     0.00  std::iterator_traits<char const*>::iterator_category std::__iterator_category<char const*>(char const* const&)
  0.00      3.91     0.00   102618     0.00     0.00  std::iterator_traits<char const*>::difference_type std::distance<char const*>(char const*, char const*)
  0.00      3.91     0.00   102602     0.00     0.00  StandardGraph::_get_table_cursor(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, __wt_cursor**, bool)
  0.00      3.91     0.00   102580     0.00     0.00  CommonUtil::check_return(int, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >)
  0.00      3.91     0.00     1760     0.00     0.00  void std::vector<int, std::allocator<int> >::_M_realloc_insert<int const&>(__gnu_cxx::__normal_iterator<int*, std::vector<int, std::allocator<int> > >, int const&)
  0.00      3.91     0.00     1370     0.00     0.00  std::_Rb_tree<int, int, std::_Identity<int>, std::less<int>, std::allocator<int> >::_M_erase(std::_Rb_tree_node<int>*)
  0.00      3.91     0.00      800     0.00     0.00  void std::deque<int, std::allocator<int> >::_M_push_back_aux<int const&>(int const&)
  0.00      3.91     0.00      160     0.00     0.00  std::_Deque_base<int, std::allocator<int> >::_M_initialize_map(unsigned long)
  0.00      3.91     0.00       19     0.00     0.00  std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_mutate(unsigned long, unsigned long, char const*, unsigned long)
  0.00      3.91     0.00       18     0.00     0.00  StandardGraph::get_out_degree(int)
  0.00      3.91     0.00       18     0.00     0.00  StandardGraph::get_random_node()
  0.00      3.91     0.00       18     0.00     0.00  StandardGraph::get_node(int)
  0.00      3.91     0.00       18     0.00     0.00  node::node()
  0.00      3.91     0.00        5     0.00     0.00  __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::~new_allocator()
  0.00      3.91     0.00        5     0.00     0.00  std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::~allocator()
  0.00      3.91     0.00        5     0.00     0.00  std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_get_Tp_allocator()
  0.00      3.91     0.00        5     0.00     0.00  void std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_realloc_insert<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >(__gnu_cxx::__normal_iterator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > > >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&)
  0.00      3.91     0.00        4     0.00     0.00  __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::new_allocator(__gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&)
  0.00      3.91     0.00        4     0.00     0.00  __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::max_size() const
  0.00      3.91     0.00        4     0.00     0.00  std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::begin() const
  0.00      3.91     0.00        4     0.00     0.00  std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::allocator(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&)
  0.00      3.91     0.00        4     0.00     0.00  void std::_Construct<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&)
  0.00      3.91     0.00        4     0.00     0.00  std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::__addressof<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&)
  0.00      3.91     0.00        4     0.00     0.00  std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const& std::forward<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&>(std::remove_reference<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&>::type&)
  0.00      3.91     0.00        3     0.00     0.00  __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::new_allocator()
  0.00      3.91     0.00        3     0.00     0.00  std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::allocator()
  0.00      3.91     0.00        3     0.00     0.00  std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl_data::_Vector_impl_data()
  0.00      3.91     0.00        2     0.00     0.00  __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::allocate(unsigned long, void const*)
  0.00      3.91     0.00        2     0.00     0.00  std::filesystem::file_status::type() const
  0.00      3.91     0.00        2     0.00     0.00  std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::end() const
  0.00      3.91     0.00        2     0.00     0.00  std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::size() const
  0.00      3.91     0.00        2     0.00     0.00  std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_allocate(unsigned long)
  0.00      3.91     0.00        2     0.00     0.00  std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl::_Vector_impl(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&)
  0.00      3.91     0.00        2     0.00     0.00  std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_base(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&)
  0.00      3.91     0.00        2     0.00     0.00  std::allocator_traits<std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::allocate(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >&, unsigned long)
  0.00      3.91     0.00        2     0.00     0.00  std::allocator_traits<std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::max_size(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&)
  0.00      3.91     0.00        2     0.00     0.00  std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::__uninitialized_copy<false>::__uninit_copy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*)
  0.00      3.91     0.00        2     0.00     0.00  std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_S_max_size(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&)
  0.00      3.91     0.00        2     0.00     0.00  std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_S_check_init_len(unsigned long, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&)
  0.00      3.91     0.00        2     0.00     0.00  void std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_range_initialize<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::forward_iterator_tag)
  0.00      3.91     0.00        2     0.00     0.00  std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::vector(std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&)
  0.00      3.91     0.00        2     0.00     0.00  std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_replace(unsigned long, unsigned long, char const*, unsigned long)
  0.00      3.91     0.00        2     0.00     0.00  std::iterator_traits<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>::difference_type std::__distance<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::random_access_iterator_tag)
  0.00      3.91     0.00        2     0.00     0.00  std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::uninitialized_copy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*)
  0.00      3.91     0.00        2     0.00     0.00  std::iterator_traits<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>::iterator_category std::__iterator_category<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const* const&)
  0.00      3.91     0.00        2     0.00     0.00  std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::__uninitialized_copy_a<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >&)
  0.00      3.91     0.00        2     0.00     0.00  std::iterator_traits<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>::difference_type std::distance<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*)
  0.00      3.91     0.00        2     0.00     0.00  std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&, char const*)
  0.00      3.91     0.00        1     0.00     0.00  _GLOBAL__sub_I__Z14print_csv_infoNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEiP8bfs_info
  0.00      3.91     0.00        1     0.00     0.00  _GLOBAL__sub_I__Z8METADATAB5cxx11
  0.00      3.91     0.00        1     0.00     0.00  _GLOBAL__sub_I__ZN13StandardGraphC2Ev
  0.00      3.91     0.00        1     0.00     0.00  _GLOBAL__sub_I__ZN7AdjListC2Ev
  0.00      3.91     0.00        1     0.00     0.00  _GLOBAL__sub_I__ZN7EdgeKeyC2Ev
  0.00      3.91     0.00        1     0.00     0.00  __static_initialization_and_destruction_0(int, int)
  0.00      3.91     0.00        1     0.00     0.00  __static_initialization_and_destruction_0(int, int)
  0.00      3.91     0.00        1     0.00     0.00  __static_initialization_and_destruction_0(int, int)
  0.00      3.91     0.00        1     0.00     0.00  __static_initialization_and_destruction_0(int, int)
  0.00      3.91     0.00        1     0.00     0.00  CommonUtil::pack_int_wt(int, __wt_session*)
  0.00      3.91     0.00        1     0.00     0.00  CommonUtil::open_session(__wt_connection*, __wt_session**)
  0.00      3.91     0.00        1     0.00     0.00  CommonUtil::unpack_int_wt(char const*, __wt_session*)
  0.00      3.91     0.00        1     0.00     0.00  CommonUtil::open_connection(char*, __wt_connection**)
  0.00      3.91     0.00        1     0.00     0.00  CommonUtil::close_connection(__wt_connection*)
  0.00      3.91     0.00        1     0.00     0.00  CommonUtil::check_graph_params(graph_opts)
  0.00      3.91     0.00        1     0.00     0.00  graph_opts::graph_opts(graph_opts const&)
  0.00      3.91     0.00        1     0.00     0.00  graph_opts::~graph_opts()
  0.00      3.91     0.00        1     0.00     0.00  CmdLineBase::CmdLineBase(int, char**)
  0.00      3.91     0.00        1     0.00     0.00  StandardGraph::insert_metadata(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, char*)
  0.00      3.91     0.00        1     0.00     0.00  StandardGraph::__restore_from_db(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >)
  0.00      3.91     0.00        1     0.00     0.00  std::ctype<char>::do_widen(char) const
  0.00      3.91     0.00        1     0.00     0.00  std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::size() const
  0.00      3.91     0.00        1     0.00     0.00  std::_Head_base<0ul, std::filesystem::__cxx11::path::_List::_Impl*, false>::_M_head(std::_Head_base<0ul, std::filesystem::__cxx11::path::_List::_Impl*, false>&)
  0.00      3.91     0.00        1     0.00     0.00  std::_Head_base<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter, true>::_M_head(std::_Head_base<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter, true>&)
  0.00      3.91     0.00        1     0.00     0.00  std::filesystem::status_known(std::filesystem::file_status)
  0.00      3.91     0.00        1     0.00     0.00  std::filesystem::exists(std::filesystem::file_status)
  0.00      3.91     0.00        1     0.00     0.00  std::filesystem::exists(std::filesystem::__cxx11::path const&)
  0.00      3.91     0.00        1     0.00     5.00  std::filesystem::__cxx11::path::_List::~_List()
  0.00      3.91     0.00        1     0.00     0.00  std::filesystem::__cxx11::path::path(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&, std::filesystem::__cxx11::path::format)
  0.00      3.91     0.00        1     0.00     5.00  std::filesystem::__cxx11::path::~path()
  0.00      3.91     0.00        1     0.00     0.00  std::unique_ptr<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::get_deleter()
  0.00      3.91     0.00        1     0.00     5.00  std::unique_ptr<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::~unique_ptr()
  0.00      3.91     0.00        1     0.00     0.00  std::_Tuple_impl<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_head(std::_Tuple_impl<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>&)
  0.00      3.91     0.00        1     0.00     0.00  void std::_Destroy_aux<false>::__destroy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*)
  0.00      3.91     0.00        1     0.00     0.00  std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl::_Vector_impl()
  0.00      3.91     0.00        1     0.00     0.00  std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl::~_Vector_impl()
  0.00      3.91     0.00        1     0.00     0.00  std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_deallocate(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, unsigned long)
  0.00      3.91     0.00        1     0.00     0.00  std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_base()
  0.00      3.91     0.00        1     0.00     0.00  std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::~_Vector_base()
  0.00      3.91     0.00        1     0.00     0.00  std::__uniq_ptr_impl<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_deleter()
  0.00      3.91     0.00        1     0.00     5.00  std::__uniq_ptr_impl<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_ptr()
  0.00      3.91     0.00        1     0.00     0.00  std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::vector()
  0.00      3.91     0.00        1     0.00     0.00  std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::~vector()
  0.00      3.91     0.00        1     0.00     5.00  std::filesystem::__cxx11::path::_List::_Impl*& std::__get_helper<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::_Tuple_impl<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&)
  0.00      3.91     0.00        1     0.00     0.00  std::filesystem::__cxx11::path::_List::_Impl_deleter& std::__get_helper<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::_Tuple_impl<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>&)
  0.00      3.91     0.00        1     0.00     5.00  std::tuple_element<0ul, std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter> >::type& std::get<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&)
  0.00      3.91     0.00        1     0.00     0.00  std::tuple_element<1ul, std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter> >::type& std::get<1ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&)
  0.00      3.91     0.00        1     0.00     0.00  std::remove_reference<std::filesystem::__cxx11::path::_List::_Impl*&>::type&& std::move<std::filesystem::__cxx11::path::_List::_Impl*&>(std::filesystem::__cxx11::path::_List::_Impl*&)
  0.00      3.91     0.00        1     0.00     0.00  void std::_Destroy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*)
  0.00      3.91     0.00        1     0.00     0.00  void std::_Destroy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >&)

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


granularity: each sample hit covers 2 byte(s) for 0.26% of 3.91 seconds

index % time    self  children    called     name
                                                 <spontaneous>
[1]     60.1    2.35    0.00                 __wt_hazard_clear [1]
-----------------------------------------------
                                                 <spontaneous>
[2]      8.2    0.32    0.00                 __global_once [2]
-----------------------------------------------
                                                 <spontaneous>
[3]      6.3    0.16    0.08                 bfs_info* bfs<StandardGraph>(StandardGraph&, int) [3]
                0.01    0.07  118100/118100      StandardGraph::get_out_nodes(int) [6]
                0.00    0.00    1760/1760        void std::vector<int, std::allocator<int> >::_M_realloc_insert<int const&>(__gnu_cxx::__normal_iterator<int*, std::vector<int, std::allocator<int> > >, int const&) [244]
                0.00    0.00    1370/1370        std::_Rb_tree<int, int, std::_Identity<int>, std::less<int>, std::allocator<int> >::_M_erase(std::_Rb_tree_node<int>*) [245]
                0.00    0.00     800/800         void std::deque<int, std::allocator<int> >::_M_push_back_aux<int const&>(int const&) [246]
                0.00    0.00     160/160         std::_Deque_base<int, std::allocator<int> >::_M_initialize_map(unsigned long) [247]
-----------------------------------------------
                                                 <spontaneous>
[4]      6.1    0.24    0.00                 __wt_row_search [4]
-----------------------------------------------
                                                 <spontaneous>
[5]      2.6    0.10    0.00                 __wt_schema_project_out [5]
-----------------------------------------------
                0.01    0.07  118100/118100      bfs_info* bfs<StandardGraph>(StandardGraph&, int) [3]
[6]      2.2    0.01    0.07  118100         StandardGraph::get_out_nodes(int) [6]
                0.00    0.03  118100/118100      StandardGraph::get_out_edges(int) [14]
                0.00    0.01  236200/354300      std::vector<edge, std::allocator<edge> >::~vector() [16]
                0.01    0.00 1371250/1371250     bool __gnu_cxx::operator!=<edge*, std::vector<edge, std::allocator<edge> > >(__gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > > const&, __gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > > const&) [30]
                0.00    0.01 1268670/1268670     std::vector<node, std::allocator<node> >::push_back(node const&) [32]
                0.00    0.01  118100/118100      std::vector<edge, std::allocator<edge> >::operator=(std::vector<edge, std::allocator<edge> >&&) [61]
                0.00    0.00  102580/102602      StandardGraph::_get_table_cursor(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, __wt_cursor**, bool) [76]
                0.00    0.00  102580/102618      std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::basic_string<std::allocator<char> >(char const*, std::allocator<char> const&) [75]
                0.00    0.00  102580/338813      void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char*>(char*, char*, std::forward_iterator_tag) [70]
                0.00    0.00  118100/1641420     std::vector<edge, std::allocator<edge> >::size() const [29]
                0.00    0.00 1268670/1268670     __gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > >::operator*() const [161]
                0.00    0.00 1268670/1268706     StandardGraph::__record_to_node(__wt_cursor*) [157]
                0.00    0.00 1268670/1268670     __gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > >::operator++() [160]
                0.00    0.00  118100/236200      std::vector<edge, std::allocator<edge> >::vector() [221]
                0.00    0.00  118100/118100      std::vector<node, std::allocator<node> >::vector() [231]
                0.00    0.00  102580/102580      CommonUtil::check_return(int, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >) [243]
                0.00    0.00  102580/483410      std::vector<edge, std::allocator<edge> >::begin() [187]
                0.00    0.00  102580/483410      std::vector<edge, std::allocator<edge> >::end() [186]
-----------------------------------------------
                                                 <spontaneous>
[7]      1.3    0.05    0.00                 __wt_session_gen_leave [7]
-----------------------------------------------
                                                 <spontaneous>
[8]      1.3    0.05    0.00                 __wt_cursor_valid [8]
-----------------------------------------------
                                                 <spontaneous>
[9]      1.0    0.04    0.00                 __wt_config_next [9]
-----------------------------------------------
                                                 <spontaneous>
[10]     0.8    0.03    0.00                 __curfile_search [10]
-----------------------------------------------
                                                 <spontaneous>
[11]     0.8    0.03    0.00                 __wt_curtable_open [11]
-----------------------------------------------
                                                 <spontaneous>
[12]     0.8    0.03    0.00                 __wt_hazard_set [12]
-----------------------------------------------
                                                 <spontaneous>
[13]     0.8    0.03    0.00                 __wt_schema_project_slice [13]
-----------------------------------------------
                0.00    0.03  118100/118100      StandardGraph::get_out_nodes(int) [6]
[14]     0.7    0.00    0.03  118100         StandardGraph::get_out_edges(int) [14]
                0.00    0.02 1268670/1268670     std::vector<edge, std::allocator<edge> >::push_back(edge const&) [25]
                0.00    0.00  236200/338813      void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char*>(char*, char*, std::forward_iterator_tag) [70]
                0.00    0.00  118100/338805      std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(char const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&) [59]
                0.00    0.00  118100/118100      StandardGraph::_get_index_cursor(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, __wt_cursor**) [74]
                0.00    0.00  118100/118100      StandardGraph::has_node(int) [90]
                0.00    0.00 1371090/1371090     StandardGraph::__read_from_edge_idx(__wt_cursor*, edge*) [155]
                0.00    0.00  354300/472400      std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&, char const*) [191]
                0.00    0.00  236200/472403      std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&) [188]
                0.00    0.00  118100/236200      std::vector<edge, std::allocator<edge> >::vector() [221]
-----------------------------------------------
                                                 <spontaneous>
[15]     0.6    0.03    0.00                 __wt_cursor_set_keyv [15]
-----------------------------------------------
                0.00    0.01  118100/354300      std::vector<edge, std::allocator<edge> >::_M_move_assign(std::vector<edge, std::allocator<edge> >&&, std::integral_constant<bool, true>) [60]
                0.00    0.01  236200/354300      StandardGraph::get_out_nodes(int) [6]
[16]     0.5    0.00    0.02  354300         std::vector<edge, std::allocator<edge> >::~vector() [16]
                0.02    0.00  354300/354300      void std::_Destroy<edge*, edge>(edge*, edge*, std::allocator<edge>&) [17]
                0.00    0.00  354300/1352160     std::_Vector_base<edge, std::allocator<edge> >::_M_get_Tp_allocator() [156]
                0.00    0.00  354300/354300      std::_Vector_base<edge, std::allocator<edge> >::~_Vector_base() [208]
-----------------------------------------------
                0.02    0.00  354300/354300      std::vector<edge, std::allocator<edge> >::~vector() [16]
[17]     0.5    0.02    0.00  354300         void std::_Destroy<edge*, edge>(edge*, edge*, std::allocator<edge>&) [17]
                0.00    0.00  354300/354300      void std::_Destroy<edge*>(edge*, edge*) [209]
-----------------------------------------------
                                                 <spontaneous>
[18]     0.5    0.02    0.00                 __curindex_get_value [18]
-----------------------------------------------
                                                 <spontaneous>
[19]     0.5    0.02    0.00                 __cursor_func_init.constprop.0 [19]
-----------------------------------------------
                                                 <spontaneous>
[20]     0.5    0.02    0.00                 __wt_btcur_search [20]
-----------------------------------------------
                                                 <spontaneous>
[21]     0.5    0.02    0.00                 __wt_clock_to_nsec [21]
-----------------------------------------------
                                                 <spontaneous>
[22]     0.5    0.02    0.00                 __wt_curindex_open [22]
-----------------------------------------------
                                                 <spontaneous>
[23]     0.5    0.02    0.00                 __wt_page_in_func [23]
-----------------------------------------------
                                                 <spontaneous>
[24]     0.5    0.02    0.00                 __wt_session_copy_values [24]
-----------------------------------------------
                0.00    0.02 1268670/1268670     StandardGraph::get_out_edges(int) [14]
[25]     0.5    0.00    0.02 1268670         std::vector<edge, std::allocator<edge> >::push_back(edge const&) [25]
                0.00    0.02  380830/380830      void std::vector<edge, std::allocator<edge> >::_M_realloc_insert<edge const&>(__gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > >, edge const&) [26]
                0.00    0.00  887840/1268670     void std::allocator_traits<std::allocator<edge> >::construct<edge, edge const&>(std::allocator<edge>&, edge*, edge const&) [162]
                0.00    0.00  380830/483410      std::vector<edge, std::allocator<edge> >::end() [186]
-----------------------------------------------
                0.00    0.02  380830/380830      std::vector<edge, std::allocator<edge> >::push_back(edge const&) [25]
[26]     0.5    0.00    0.02  380830         void std::vector<edge, std::allocator<edge> >::_M_realloc_insert<edge const&>(__gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > >, edge const&) [26]
                0.00    0.01  761660/761660      std::vector<edge, std::allocator<edge> >::_S_relocate(edge*, edge*, edge*, std::allocator<edge>&) [33]
                0.00    0.01  380830/380830      std::vector<edge, std::allocator<edge> >::_M_check_len(unsigned long, char const*) const [58]
                0.00    0.00  761660/1352160     std::_Vector_base<edge, std::allocator<edge> >::_M_get_Tp_allocator() [156]
                0.00    0.00  761660/4265820     __gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > >::base() const [140]
                0.00    0.00  380830/483410      std::vector<edge, std::allocator<edge> >::begin() [187]
                0.00    0.00  380830/380830      __gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > >::difference_type __gnu_cxx::operator-<edge*, std::vector<edge, std::allocator<edge> > >(__gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > > const&, __gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > > const&) [195]
                0.00    0.00  380830/380830      std::_Vector_base<edge, std::allocator<edge> >::_M_allocate(unsigned long) [198]
                0.00    0.00  380830/2918170     edge const& std::forward<edge const&>(std::remove_reference<edge const&>::type&) [143]
                0.00    0.00  380830/1268670     void std::allocator_traits<std::allocator<edge> >::construct<edge, edge const&>(std::allocator<edge>&, edge*, edge const&) [162]
                0.00    0.00  380830/735130      std::_Vector_base<edge, std::allocator<edge> >::_M_deallocate(edge*, unsigned long) [183]
-----------------------------------------------
                                                 <spontaneous>
[27]     0.4    0.02    0.00                 __wt_cursor_set_key [27]
-----------------------------------------------
                0.01    0.00 2284980/2284980     edge* std::__relocate_a<edge*, edge*, std::allocator<edge> >(edge*, edge*, edge*, std::allocator<edge>&) [35]
[28]     0.3    0.01    0.00 2284980         edge* std::__niter_base<edge*>(edge*) [28]
-----------------------------------------------
                0.00    0.00  118100/1641420     StandardGraph::get_out_nodes(int) [6]
                0.01    0.00 1523320/1641420     std::vector<edge, std::allocator<edge> >::_M_check_len(unsigned long, char const*) const [58]
[29]     0.3    0.01    0.00 1641420         std::vector<edge, std::allocator<edge> >::size() const [29]
-----------------------------------------------
                0.01    0.00 1371250/1371250     StandardGraph::get_out_nodes(int) [6]
[30]     0.3    0.01    0.00 1371250         bool __gnu_cxx::operator!=<edge*, std::vector<edge, std::allocator<edge> > >(__gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > > const&, __gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > > const&) [30]
                0.00    0.00 2742500/4265820     __gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > >::base() const [140]
-----------------------------------------------
                0.00    0.00  380830/1268670     void std::vector<node, std::allocator<node> >::_M_realloc_insert<node const&>(__gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >, node const&) [73]
                0.01    0.00  887840/1268670     std::vector<node, std::allocator<node> >::push_back(node const&) [32]
[31]     0.3    0.01    0.00 1268670         void std::allocator_traits<std::allocator<node> >::construct<node, node const&>(std::allocator<node>&, node*, node const&) [31]
                0.00    0.00 1268670/2918170     node const& std::forward<node const&>(std::remove_reference<node const&>::type&) [144]
                0.00    0.00 1268670/1268670     void __gnu_cxx::new_allocator<node>::construct<node, node const&>(node*, node const&) [159]
-----------------------------------------------
                0.00    0.01 1268670/1268670     StandardGraph::get_out_nodes(int) [6]
[32]     0.3    0.00    0.01 1268670         std::vector<node, std::allocator<node> >::push_back(node const&) [32]
                0.01    0.00  887840/1268670     void std::allocator_traits<std::allocator<node> >::construct<node, node const&>(std::allocator<node>&, node*, node const&) [31]
                0.00    0.00  380830/380830      void std::vector<node, std::allocator<node> >::_M_realloc_insert<node const&>(__gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >, node const&) [73]
                0.00    0.00  380830/380830      std::vector<node, std::allocator<node> >::end() [204]
-----------------------------------------------
                0.00    0.01  761660/761660      void std::vector<edge, std::allocator<edge> >::_M_realloc_insert<edge const&>(__gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > >, edge const&) [26]
[33]     0.3    0.00    0.01  761660         std::vector<edge, std::allocator<edge> >::_S_relocate(edge*, edge*, edge*, std::allocator<edge>&) [33]
                0.00    0.01  761660/761660      std::vector<edge, std::allocator<edge> >::_S_do_relocate(edge*, edge*, edge*, std::allocator<edge>&, std::integral_constant<bool, true>) [34]
-----------------------------------------------
                0.00    0.01  761660/761660      std::vector<edge, std::allocator<edge> >::_S_relocate(edge*, edge*, edge*, std::allocator<edge>&) [33]
[34]     0.3    0.00    0.01  761660         std::vector<edge, std::allocator<edge> >::_S_do_relocate(edge*, edge*, edge*, std::allocator<edge>&, std::integral_constant<bool, true>) [34]
                0.00    0.01  761660/761660      edge* std::__relocate_a<edge*, edge*, std::allocator<edge> >(edge*, edge*, edge*, std::allocator<edge>&) [35]
-----------------------------------------------
                0.00    0.01  761660/761660      std::vector<edge, std::allocator<edge> >::_S_do_relocate(edge*, edge*, edge*, std::allocator<edge>&, std::integral_constant<bool, true>) [34]
[35]     0.3    0.00    0.01  761660         edge* std::__relocate_a<edge*, edge*, std::allocator<edge> >(edge*, edge*, edge*, std::allocator<edge>&) [35]
                0.01    0.00 2284980/2284980     edge* std::__niter_base<edge*>(edge*) [28]
                0.00    0.00  761660/761660      std::enable_if<std::__is_bitwise_relocatable<edge, void>::value, edge*>::type std::__relocate_a_1<edge, edge>(edge*, edge*, edge*, std::allocator<edge>&) [180]
-----------------------------------------------
                0.00    0.00  102618/441423      std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::basic_string<std::allocator<char> >(char const*, std::allocator<char> const&) [75]
                0.01    0.00  338805/441423      std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(char const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&) [59]
[36]     0.3    0.01    0.00  441423         std::char_traits<char>::length(char const*) [36]
-----------------------------------------------
                                                 <spontaneous>
[37]     0.3    0.01    0.00                 __curindex_compare [37]
-----------------------------------------------
                                                 <spontaneous>
[38]     0.3    0.01    0.00                 __curindex_search [38]
-----------------------------------------------
                                                 <spontaneous>
[39]     0.3    0.01    0.00                 __cursor_row_slot_return [39]
-----------------------------------------------
                                                 <spontaneous>
[40]     0.3    0.01    0.00                 __curtable_search [40]
-----------------------------------------------
                                                 <spontaneous>
[41]     0.3    0.01    0.00                 __pack_write [41]
-----------------------------------------------
                                                 <spontaneous>
[42]     0.3    0.01    0.00                 __schema_open_index [42]
-----------------------------------------------
                                                 <spontaneous>
[43]     0.3    0.01    0.00                 __session_find_dhandle [43]
-----------------------------------------------
                                                 <spontaneous>
[44]     0.3    0.01    0.00                 __unpack_read [44]
-----------------------------------------------
                                                 <spontaneous>
[45]     0.3    0.01    0.00                 __wt_btcur_close [45]
-----------------------------------------------
                                                 <spontaneous>
[46]     0.3    0.01    0.00                 __wt_btcur_search_near [46]
-----------------------------------------------
                                                 <spontaneous>
[47]     0.3    0.01    0.00                 __wt_buf_catfmt [47]
-----------------------------------------------
                                                 <spontaneous>
[48]     0.3    0.01    0.00                 __wt_config_gets_def [48]
-----------------------------------------------
                                                 <spontaneous>
[49]     0.3    0.01    0.00                 __wt_curtable_set_key [49]
-----------------------------------------------
                                                 <spontaneous>
[50]     0.3    0.01    0.00                 __wt_hash_city64 [50]
-----------------------------------------------
                                                 <spontaneous>
[51]     0.3    0.01    0.00                 __wt_readunlock [51]
-----------------------------------------------
                                                 <spontaneous>
[52]     0.3    0.01    0.00                 __wt_schema_open_index [52]
-----------------------------------------------
                                                 <spontaneous>
[53]     0.3    0.01    0.00                 __wt_schema_open_table [53]
-----------------------------------------------
                                                 <spontaneous>
[54]     0.3    0.01    0.00                 __wt_scr_alloc_func [54]
-----------------------------------------------
                                                 <spontaneous>
[55]     0.3    0.01    0.00                 __wt_session_gen_enter [55]
-----------------------------------------------
                                                 <spontaneous>
[56]     0.3    0.01    0.00                 __wt_strndup [56]
-----------------------------------------------
                                                 <spontaneous>
[57]     0.3    0.01    0.00                 __wt_struct_plan [57]
-----------------------------------------------
                0.00    0.01  380830/380830      void std::vector<edge, std::allocator<edge> >::_M_realloc_insert<edge const&>(__gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > >, edge const&) [26]
[58]     0.2    0.00    0.01  380830         std::vector<edge, std::allocator<edge> >::_M_check_len(unsigned long, char const*) const [58]
                0.01    0.00 1523320/1641420     std::vector<edge, std::allocator<edge> >::size() const [29]
                0.00    0.00  761660/761660      std::vector<edge, std::allocator<edge> >::max_size() const [170]
                0.00    0.00  380830/761660      unsigned long const& std::max<unsigned long>(unsigned long const&, unsigned long const&) [182]
-----------------------------------------------
                0.00    0.00       3/338805      __static_initialization_and_destruction_0(int, int) [79]
                0.00    0.00  102602/338805      StandardGraph::_get_table_cursor(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, __wt_cursor**, bool) [76]
                0.00    0.00  118100/338805      StandardGraph::_get_index_cursor(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, __wt_cursor**) [74]
                0.00    0.00  118100/338805      StandardGraph::get_out_edges(int) [14]
[59]     0.2    0.00    0.01  338805         std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(char const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&) [59]
                0.01    0.00  338805/441423      std::char_traits<char>::length(char const*) [36]
-----------------------------------------------
                0.00    0.01  118100/118100      std::vector<edge, std::allocator<edge> >::operator=(std::vector<edge, std::allocator<edge> >&&) [61]
[60]     0.2    0.00    0.01  118100         std::vector<edge, std::allocator<edge> >::_M_move_assign(std::vector<edge, std::allocator<edge> >&&, std::integral_constant<bool, true>) [60]
                0.00    0.01  118100/354300      std::vector<edge, std::allocator<edge> >::~vector() [16]
                0.00    0.00  236200/236200      std::_Vector_base<edge, std::allocator<edge> >::_Vector_impl_data::_M_swap_data(std::_Vector_base<edge, std::allocator<edge> >::_Vector_impl_data&) [219]
                0.00    0.00  236200/1352160     std::_Vector_base<edge, std::allocator<edge> >::_M_get_Tp_allocator() [156]
                0.00    0.00  118100/118100      std::_Vector_base<edge, std::allocator<edge> >::get_allocator() const [223]
                0.00    0.00  118100/118100      std::vector<edge, std::allocator<edge> >::vector(std::allocator<edge> const&) [230]
                0.00    0.00  118100/472400      std::allocator<edge>::~allocator() [190]
                0.00    0.00  118100/118100      void std::__alloc_on_move<std::allocator<edge> >(std::allocator<edge>&, std::allocator<edge>&) [232]
-----------------------------------------------
                0.00    0.01  118100/118100      StandardGraph::get_out_nodes(int) [6]
[61]     0.2    0.00    0.01  118100         std::vector<edge, std::allocator<edge> >::operator=(std::vector<edge, std::allocator<edge> >&&) [61]
                0.00    0.01  118100/118100      std::vector<edge, std::allocator<edge> >::_M_move_assign(std::vector<edge, std::allocator<edge> >&&, std::integral_constant<bool, true>) [60]
                0.00    0.00  118100/118100      std::remove_reference<std::vector<edge, std::allocator<edge> >&>::type&& std::move<std::vector<edge, std::allocator<edge> >&>(std::vector<edge, std::allocator<edge> >&) [235]
-----------------------------------------------
                                                 <spontaneous>
[62]     0.1    0.00    0.01                 StandardGraph::StandardGraph(graph_opts) [62]
                0.00    0.01       1/1           std::filesystem::__cxx11::path::~path() [64]
                0.00    0.00       3/102618      std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::basic_string<std::allocator<char> >(char const*, std::allocator<char> const&) [75]
                0.00    0.00       4/338813      void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char*>(char*, char*, std::forward_iterator_tag) [70]
                0.00    0.00       2/2           std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::vector(std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [87]
                0.00    0.00       1/1           StandardGraph::__restore_from_db(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >) [93]
                0.00    0.00       2/2           std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&, char const*) [94]
                0.00    0.00       2/3           std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::allocator() [261]
                0.00    0.00       2/5           std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::~allocator() [251]
                0.00    0.00       2/472403      std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&) [188]
                0.00    0.00       1/1           graph_opts::graph_opts(graph_opts const&) [284]
                0.00    0.00       1/1           CommonUtil::check_graph_params(graph_opts) [283]
                0.00    0.00       1/1           graph_opts::~graph_opts() [285]
                0.00    0.00       1/1           std::filesystem::__cxx11::path::path(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&, std::filesystem::__cxx11::path::format) [294]
                0.00    0.00       1/1           std::filesystem::exists(std::filesystem::__cxx11::path const&) [293]
-----------------------------------------------
                0.00    0.01       1/1           std::filesystem::__cxx11::path::~path() [64]
[63]     0.1    0.00    0.01       1         std::filesystem::__cxx11::path::_List::~_List() [63]
                0.00    0.01       1/1           std::unique_ptr<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::~unique_ptr() [65]
-----------------------------------------------
                0.00    0.01       1/1           StandardGraph::StandardGraph(graph_opts) [62]
[64]     0.1    0.00    0.01       1         std::filesystem::__cxx11::path::~path() [64]
                0.00    0.01       1/1           std::filesystem::__cxx11::path::_List::~_List() [63]
-----------------------------------------------
                0.00    0.01       1/1           std::filesystem::__cxx11::path::_List::~_List() [63]
[65]     0.1    0.00    0.01       1         std::unique_ptr<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::~unique_ptr() [65]
                0.00    0.01       1/1           std::__uniq_ptr_impl<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_ptr() [67]
                0.00    0.00       1/1           std::unique_ptr<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::get_deleter() [295]
                0.00    0.00       1/1           std::remove_reference<std::filesystem::__cxx11::path::_List::_Impl*&>::type&& std::move<std::filesystem::__cxx11::path::_List::_Impl*&>(std::filesystem::__cxx11::path::_List::_Impl*&) [308]
-----------------------------------------------
                0.01    0.00       1/1           std::filesystem::__cxx11::path::_List::_Impl*& std::__get_helper<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::_Tuple_impl<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [68]
[66]     0.1    0.01    0.00       1         std::_Tuple_impl<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_head(std::_Tuple_impl<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [66]
                0.00    0.00       1/1           std::_Head_base<0ul, std::filesystem::__cxx11::path::_List::_Impl*, false>::_M_head(std::_Head_base<0ul, std::filesystem::__cxx11::path::_List::_Impl*, false>&) [289]
-----------------------------------------------
                0.00    0.01       1/1           std::unique_ptr<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::~unique_ptr() [65]
[67]     0.1    0.00    0.01       1         std::__uniq_ptr_impl<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_ptr() [67]
                0.00    0.01       1/1           std::tuple_element<0ul, std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter> >::type& std::get<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [69]
-----------------------------------------------
                0.00    0.01       1/1           std::tuple_element<0ul, std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter> >::type& std::get<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [69]
[68]     0.1    0.00    0.01       1         std::filesystem::__cxx11::path::_List::_Impl*& std::__get_helper<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::_Tuple_impl<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [68]
                0.01    0.00       1/1           std::_Tuple_impl<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_head(std::_Tuple_impl<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [66]
-----------------------------------------------
                0.00    0.01       1/1           std::__uniq_ptr_impl<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_ptr() [67]
[69]     0.1    0.00    0.01       1         std::tuple_element<0ul, std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter> >::type& std::get<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [69]
                0.00    0.01       1/1           std::filesystem::__cxx11::path::_List::_Impl*& std::__get_helper<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::_Tuple_impl<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [68]
-----------------------------------------------
                0.00    0.00       1/338813      StandardGraph::insert_metadata(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, char*) [92]
                0.00    0.00       1/338813      StandardGraph::__restore_from_db(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >) [93]
                0.00    0.00       1/338813      StandardGraph::close() [83]
                0.00    0.00       1/338813      StandardGraph::has_node(int) [90]
                0.00    0.00       1/338813      StandardGraph::get_random_node() [91]
                0.00    0.00       2/338813      std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&, char const*) [94]
                0.00    0.00       4/338813      void std::_Construct<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&) [84]
                0.00    0.00       4/338813      StandardGraph::StandardGraph(graph_opts) [62]
                0.00    0.00      18/338813      StandardGraph::get_node(int) [82]
                0.00    0.00  102580/338813      StandardGraph::get_out_nodes(int) [6]
                0.00    0.00  236200/338813      StandardGraph::get_out_edges(int) [14]
[70]     0.1    0.00    0.01  338813         void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char*>(char*, char*, std::forward_iterator_tag) [70]
                0.00    0.01  338813/338813      std::iterator_traits<char*>::difference_type std::distance<char*>(char*, char*) [72]
                0.00    0.00  338813/338813      bool __gnu_cxx::__is_null_pointer<char>(char*) [210]
-----------------------------------------------
                0.01    0.00  338813/338813      std::iterator_traits<char*>::difference_type std::distance<char*>(char*, char*) [72]
[71]     0.1    0.01    0.00  338813         std::iterator_traits<char*>::difference_type std::__distance<char*>(char*, char*, std::random_access_iterator_tag) [71]
-----------------------------------------------
                0.00    0.01  338813/338813      void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char*>(char*, char*, std::forward_iterator_tag) [70]
[72]     0.1    0.00    0.01  338813         std::iterator_traits<char*>::difference_type std::distance<char*>(char*, char*) [72]
                0.01    0.00  338813/338813      std::iterator_traits<char*>::difference_type std::__distance<char*>(char*, char*, std::random_access_iterator_tag) [71]
                0.00    0.00  338813/338813      std::iterator_traits<char*>::iterator_category std::__iterator_category<char*>(char* const&) [211]
-----------------------------------------------
                0.00    0.00  380830/380830      std::vector<node, std::allocator<node> >::push_back(node const&) [32]
[73]     0.1    0.00    0.00  380830         void std::vector<node, std::allocator<node> >::_M_realloc_insert<node const&>(__gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >, node const&) [73]
                0.00    0.00  380830/1268670     void std::allocator_traits<std::allocator<node> >::construct<node, node const&>(std::allocator<node>&, node*, node const&) [31]
                0.00    0.00  761660/761660      std::_Vector_base<node, std::allocator<node> >::_M_get_Tp_allocator() [172]
                0.00    0.00  761660/1523320     __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >::base() const [153]
                0.00    0.00  761660/761660      std::vector<node, std::allocator<node> >::_S_relocate(node*, node*, node*, std::allocator<node>&) [177]
                0.00    0.00  380830/380830      std::vector<node, std::allocator<node> >::_M_check_len(unsigned long, char const*) const [197]
                0.00    0.00  380830/380830      std::vector<node, std::allocator<node> >::begin() [205]
                0.00    0.00  380830/380830      __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >::difference_type __gnu_cxx::operator-<node*, std::vector<node, std::allocator<node> > >(__gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > > const&, __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > > const&) [196]
                0.00    0.00  380830/380830      std::_Vector_base<node, std::allocator<node> >::_M_allocate(unsigned long) [199]
                0.00    0.00  380830/2918170     node const& std::forward<node const&>(std::remove_reference<node const&>::type&) [144]
                0.00    0.00  380830/380830      std::_Vector_base<node, std::allocator<node> >::_M_deallocate(node*, unsigned long) [200]
-----------------------------------------------
                0.00    0.00  118100/118100      StandardGraph::get_out_edges(int) [14]
[74]     0.1    0.00    0.00  118100         StandardGraph::_get_index_cursor(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, __wt_cursor**) [74]
                0.00    0.00  118100/338805      std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(char const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&) [59]
                0.00    0.00  236200/472403      std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&) [188]
                0.00    0.00  118100/472400      std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&, char const*) [191]
-----------------------------------------------
                0.00    0.00       1/102618      __static_initialization_and_destruction_0(int, int) [98]
                0.00    0.00       1/102618      CommonUtil::pack_int_wt(int, __wt_session*) [101]
                0.00    0.00       1/102618      __static_initialization_and_destruction_0(int, int) [99]
                0.00    0.00       1/102618      StandardGraph::close() [83]
                0.00    0.00       1/102618      __static_initialization_and_destruction_0(int, int) [100]
                0.00    0.00       3/102618      StandardGraph::StandardGraph(graph_opts) [62]
                0.00    0.00      30/102618      __static_initialization_and_destruction_0(int, int) [79]
                0.00    0.00  102580/102618      StandardGraph::get_out_nodes(int) [6]
[75]     0.1    0.00    0.00  102618         std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::basic_string<std::allocator<char> >(char const*, std::allocator<char> const&) [75]
                0.00    0.00  102618/441423      std::char_traits<char>::length(char const*) [36]
                0.00    0.00  102618/102618      void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char const*>(char const*, char const*) [237]
-----------------------------------------------
                0.00    0.00       1/102602      StandardGraph::insert_metadata(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, char*) [92]
                0.00    0.00       1/102602      StandardGraph::__restore_from_db(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >) [93]
                0.00    0.00       1/102602      StandardGraph::has_node(int) [90]
                0.00    0.00       1/102602      StandardGraph::get_random_node() [91]
                0.00    0.00      18/102602      StandardGraph::get_node(int) [82]
                0.00    0.00  102580/102602      StandardGraph::get_out_nodes(int) [6]
[76]     0.1    0.00    0.00  102602         StandardGraph::_get_table_cursor(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, __wt_cursor**, bool) [76]
                0.00    0.00  102602/338805      std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(char const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&) [59]
-----------------------------------------------
                                                 <spontaneous>
[77]     0.0    0.00    0.00                 __libc_csu_init [77]
                0.00    0.00       1/1           _GLOBAL__sub_I__Z8METADATAB5cxx11 [78]
                0.00    0.00       1/1           _GLOBAL__sub_I__ZN13StandardGraphC2Ev [95]
                0.00    0.00       1/1           _GLOBAL__sub_I__ZN7EdgeKeyC2Ev [97]
                0.00    0.00       1/1           _GLOBAL__sub_I__ZN7AdjListC2Ev [96]
                0.00    0.00       1/1           _GLOBAL__sub_I__Z14print_csv_infoNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEiP8bfs_info [278]
-----------------------------------------------
                0.00    0.00       1/1           __libc_csu_init [77]
[78]     0.0    0.00    0.00       1         _GLOBAL__sub_I__Z8METADATAB5cxx11 [78]
                0.00    0.00       1/1           __static_initialization_and_destruction_0(int, int) [79]
-----------------------------------------------
                0.00    0.00       1/1           _GLOBAL__sub_I__Z8METADATAB5cxx11 [78]
[79]     0.0    0.00    0.00       1         __static_initialization_and_destruction_0(int, int) [79]
                0.00    0.00      30/102618      std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::basic_string<std::allocator<char> >(char const*, std::allocator<char> const&) [75]
                0.00    0.00       3/338805      std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(char const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&) [59]
                0.00    0.00       1/472403      std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&) [188]
-----------------------------------------------
                                                 <spontaneous>
[80]     0.0    0.00    0.00                 int find_random_start<StandardGraph>(StandardGraph&) [80]
                0.00    0.00      18/18          StandardGraph::get_out_degree(int) [81]
                0.00    0.00      18/18          StandardGraph::get_random_node() [91]
-----------------------------------------------
                0.00    0.00      18/18          int find_random_start<StandardGraph>(StandardGraph&) [80]
[81]     0.0    0.00    0.00      18         StandardGraph::get_out_degree(int) [81]
                0.00    0.00      18/18          StandardGraph::get_node(int) [82]
-----------------------------------------------
                0.00    0.00      18/18          StandardGraph::get_out_degree(int) [81]
[82]     0.0    0.00    0.00      18         StandardGraph::get_node(int) [82]
                0.00    0.00      18/102602      StandardGraph::_get_table_cursor(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, __wt_cursor**, bool) [76]
                0.00    0.00      18/338813      void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char*>(char*, char*, std::forward_iterator_tag) [70]
                0.00    0.00      18/1268706     StandardGraph::__record_to_node(__wt_cursor*) [157]
-----------------------------------------------
                                                 <spontaneous>
[83]     0.0    0.00    0.00                 StandardGraph::close() [83]
                0.00    0.00       1/1           StandardGraph::insert_metadata(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, char*) [92]
                0.00    0.00       1/1           CommonUtil::pack_int_wt(int, __wt_session*) [101]
                0.00    0.00       1/102618      std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::basic_string<std::allocator<char> >(char const*, std::allocator<char> const&) [75]
                0.00    0.00       1/338813      void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char*>(char*, char*, std::forward_iterator_tag) [70]
                0.00    0.00       1/1           CommonUtil::close_connection(__wt_connection*) [282]
-----------------------------------------------
                0.00    0.00       4/4           std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::__uninitialized_copy<false>::__uninit_copy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*) [85]
[84]     0.0    0.00    0.00       4         void std::_Construct<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&) [84]
                0.00    0.00       4/338813      void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char*>(char*, char*, std::forward_iterator_tag) [70]
                0.00    0.00       4/4           std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const& std::forward<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&>(std::remove_reference<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&>::type&) [259]
                0.00    0.00       4/4124554     operator new(unsigned long, void*) [141]
-----------------------------------------------
                0.00    0.00       2/2           std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::uninitialized_copy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*) [88]
[85]     0.0    0.00    0.00       2         std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::__uninitialized_copy<false>::__uninit_copy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*) [85]
                0.00    0.00       4/4           void std::_Construct<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&) [84]
                0.00    0.00       4/4           std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::__addressof<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&) [258]
-----------------------------------------------
                0.00    0.00       2/2           std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::vector(std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [87]
[86]     0.0    0.00    0.00       2         void std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_range_initialize<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::forward_iterator_tag) [86]
                0.00    0.00       2/2           std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::__uninitialized_copy_a<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >&) [89]
                0.00    0.00       4/5           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_get_Tp_allocator() [252]
                0.00    0.00       2/2           std::iterator_traits<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>::difference_type std::distance<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*) [277]
                0.00    0.00       2/2           std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_S_check_init_len(unsigned long, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [273]
                0.00    0.00       2/2           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_allocate(unsigned long) [267]
-----------------------------------------------
                0.00    0.00       2/2           StandardGraph::StandardGraph(graph_opts) [62]
[87]     0.0    0.00    0.00       2         std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::vector(std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [87]
                0.00    0.00       2/2           void std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_range_initialize<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::forward_iterator_tag) [86]
                0.00    0.00       2/2           std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::end() const [265]
                0.00    0.00       2/2           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_base(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [269]
                0.00    0.00       2/4           std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::begin() const [256]
-----------------------------------------------
                0.00    0.00       2/2           std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::__uninitialized_copy_a<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >&) [89]
[88]     0.0    0.00    0.00       2         std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::uninitialized_copy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*) [88]
                0.00    0.00       2/2           std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::__uninitialized_copy<false>::__uninit_copy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*) [85]
-----------------------------------------------
                0.00    0.00       2/2           void std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_range_initialize<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::forward_iterator_tag) [86]
[89]     0.0    0.00    0.00       2         std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::__uninitialized_copy_a<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >&) [89]
                0.00    0.00       2/2           std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::uninitialized_copy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*) [88]
-----------------------------------------------
                0.00    0.00  118100/118100      StandardGraph::get_out_edges(int) [14]
[90]     0.0    0.00    0.00  118100         StandardGraph::has_node(int) [90]
                0.00    0.00       1/102602      StandardGraph::_get_table_cursor(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, __wt_cursor**, bool) [76]
                0.00    0.00       1/338813      void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char*>(char*, char*, std::forward_iterator_tag) [70]
-----------------------------------------------
                0.00    0.00      18/18          int find_random_start<StandardGraph>(StandardGraph&) [80]
[91]     0.0    0.00    0.00      18         StandardGraph::get_random_node() [91]
                0.00    0.00       1/102602      StandardGraph::_get_table_cursor(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, __wt_cursor**, bool) [76]
                0.00    0.00       1/338813      void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char*>(char*, char*, std::forward_iterator_tag) [70]
                0.00    0.00      18/18          node::node() [249]
                0.00    0.00      18/1268706     StandardGraph::__record_to_node(__wt_cursor*) [157]
-----------------------------------------------
                0.00    0.00       1/1           StandardGraph::close() [83]
[92]     0.0    0.00    0.00       1         StandardGraph::insert_metadata(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, char*) [92]
                0.00    0.00       1/102602      StandardGraph::_get_table_cursor(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, __wt_cursor**, bool) [76]
                0.00    0.00       1/338813      void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char*>(char*, char*, std::forward_iterator_tag) [70]
-----------------------------------------------
                0.00    0.00       1/1           StandardGraph::StandardGraph(graph_opts) [62]
[93]     0.0    0.00    0.00       1         StandardGraph::__restore_from_db(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >) [93]
                0.00    0.00       1/102602      StandardGraph::_get_table_cursor(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, __wt_cursor**, bool) [76]
                0.00    0.00       1/338813      void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char*>(char*, char*, std::forward_iterator_tag) [70]
                0.00    0.00       1/1           CommonUtil::open_connection(char*, __wt_connection**) [281]
                0.00    0.00       1/1           CommonUtil::open_session(__wt_connection*, __wt_session**) [279]
                0.00    0.00       1/2           std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_replace(unsigned long, unsigned long, char const*, unsigned long) [274]
                0.00    0.00       1/1           CommonUtil::unpack_int_wt(char const*, __wt_session*) [280]
-----------------------------------------------
                0.00    0.00       2/2           StandardGraph::StandardGraph(graph_opts) [62]
[94]     0.0    0.00    0.00       2         std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&, char const*) [94]
                0.00    0.00       2/338813      void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char*>(char*, char*, std::forward_iterator_tag) [70]
-----------------------------------------------
                0.00    0.00       1/1           __libc_csu_init [77]
[95]     0.0    0.00    0.00       1         _GLOBAL__sub_I__ZN13StandardGraphC2Ev [95]
                0.00    0.00       1/1           __static_initialization_and_destruction_0(int, int) [100]
-----------------------------------------------
                0.00    0.00       1/1           __libc_csu_init [77]
[96]     0.0    0.00    0.00       1         _GLOBAL__sub_I__ZN7AdjListC2Ev [96]
                0.00    0.00       1/1           __static_initialization_and_destruction_0(int, int) [98]
-----------------------------------------------
                0.00    0.00       1/1           __libc_csu_init [77]
[97]     0.0    0.00    0.00       1         _GLOBAL__sub_I__ZN7EdgeKeyC2Ev [97]
                0.00    0.00       1/1           __static_initialization_and_destruction_0(int, int) [99]
-----------------------------------------------
                0.00    0.00       1/1           _GLOBAL__sub_I__ZN7AdjListC2Ev [96]
[98]     0.0    0.00    0.00       1         __static_initialization_and_destruction_0(int, int) [98]
                0.00    0.00       1/102618      std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::basic_string<std::allocator<char> >(char const*, std::allocator<char> const&) [75]
-----------------------------------------------
                0.00    0.00       1/1           _GLOBAL__sub_I__ZN7EdgeKeyC2Ev [97]
[99]     0.0    0.00    0.00       1         __static_initialization_and_destruction_0(int, int) [99]
                0.00    0.00       1/102618      std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::basic_string<std::allocator<char> >(char const*, std::allocator<char> const&) [75]
-----------------------------------------------
                0.00    0.00       1/1           _GLOBAL__sub_I__ZN13StandardGraphC2Ev [95]
[100]    0.0    0.00    0.00       1         __static_initialization_and_destruction_0(int, int) [100]
                0.00    0.00       1/102618      std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::basic_string<std::allocator<char> >(char const*, std::allocator<char> const&) [75]
-----------------------------------------------
                0.00    0.00       1/1           StandardGraph::close() [83]
[101]    0.0    0.00    0.00       1         CommonUtil::pack_int_wt(int, __wt_session*) [101]
                0.00    0.00       1/102618      std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::basic_string<std::allocator<char> >(char const*, std::allocator<char> const&) [75]
-----------------------------------------------
                0.00    0.00 1587210/4761630     void std::__relocate_object_a<node, node, std::allocator<node> >(node*, node*, std::allocator<node>&) [150]
                0.00    0.00 3174420/4761630     node* std::__relocate_a_1<node*, node*, std::allocator<node> >(node*, node*, node*, std::allocator<node>&) [181]
[139]    0.0    0.00    0.00 4761630         node* std::__addressof<node>(node&) [139]
-----------------------------------------------
                0.00    0.00  761660/4265820     void std::vector<edge, std::allocator<edge> >::_M_realloc_insert<edge const&>(__gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > >, edge const&) [26]
                0.00    0.00  761660/4265820     __gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > >::difference_type __gnu_cxx::operator-<edge*, std::vector<edge, std::allocator<edge> > >(__gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > > const&, __gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > > const&) [195]
                0.00    0.00 2742500/4265820     bool __gnu_cxx::operator!=<edge*, std::vector<edge, std::allocator<edge> > >(__gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > > const&, __gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > > const&) [30]
[140]    0.0    0.00    0.00 4265820         __gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > >::base() const [140]
-----------------------------------------------
                0.00    0.00       4/4124554     void std::_Construct<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&) [84]
                0.00    0.00 1268670/4124554     void __gnu_cxx::new_allocator<node>::construct<node, node const&>(node*, node const&) [159]
                0.00    0.00 1268670/4124554     void __gnu_cxx::new_allocator<edge>::construct<edge, edge const&>(edge*, edge const&) [158]
                0.00    0.00 1587210/4124554     void __gnu_cxx::new_allocator<node>::construct<node, node>(node*, node&&) [147]
[141]    0.0    0.00    0.00 4124554         operator new(unsigned long, void*) [141]
-----------------------------------------------
                0.00    0.00 1587210/3174420     void std::allocator_traits<std::allocator<node> >::construct<node, node>(std::allocator<node>&, node*, node&&) [149]
                0.00    0.00 1587210/3174420     void __gnu_cxx::new_allocator<node>::construct<node, node>(node*, node&&) [147]
[142]    0.0    0.00    0.00 3174420         node&& std::forward<node>(std::remove_reference<node>::type&) [142]
-----------------------------------------------
                0.00    0.00  380830/2918170     void std::vector<edge, std::allocator<edge> >::_M_realloc_insert<edge const&>(__gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > >, edge const&) [26]
                0.00    0.00 1268670/2918170     void std::allocator_traits<std::allocator<edge> >::construct<edge, edge const&>(std::allocator<edge>&, edge*, edge const&) [162]
                0.00    0.00 1268670/2918170     void __gnu_cxx::new_allocator<edge>::construct<edge, edge const&>(edge*, edge const&) [158]
[143]    0.0    0.00    0.00 2918170         edge const& std::forward<edge const&>(std::remove_reference<edge const&>::type&) [143]
-----------------------------------------------
                0.00    0.00  380830/2918170     void std::vector<node, std::allocator<node> >::_M_realloc_insert<node const&>(__gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >, node const&) [73]
                0.00    0.00 1268670/2918170     void std::allocator_traits<std::allocator<node> >::construct<node, node const&>(std::allocator<node>&, node*, node const&) [31]
                0.00    0.00 1268670/2918170     void __gnu_cxx::new_allocator<node>::construct<node, node const&>(node*, node const&) [159]
[144]    0.0    0.00    0.00 2918170         node const& std::forward<node const&>(std::remove_reference<node const&>::type&) [144]
-----------------------------------------------
                0.00    0.00 2284980/2284980     node* std::__relocate_a<node*, node*, std::allocator<node> >(node*, node*, node*, std::allocator<node>&) [179]
[145]    0.0    0.00    0.00 2284980         node* std::__niter_base<node*>(node*) [145]
-----------------------------------------------
                0.00    0.00 1587210/1587210     void std::allocator_traits<std::allocator<node> >::destroy<node>(std::allocator<node>&, node*) [148]
[146]    0.0    0.00    0.00 1587210         void __gnu_cxx::new_allocator<node>::destroy<node>(node*) [146]
-----------------------------------------------
                0.00    0.00 1587210/1587210     void std::allocator_traits<std::allocator<node> >::construct<node, node>(std::allocator<node>&, node*, node&&) [149]
[147]    0.0    0.00    0.00 1587210         void __gnu_cxx::new_allocator<node>::construct<node, node>(node*, node&&) [147]
                0.00    0.00 1587210/3174420     node&& std::forward<node>(std::remove_reference<node>::type&) [142]
                0.00    0.00 1587210/4124554     operator new(unsigned long, void*) [141]
-----------------------------------------------
                0.00    0.00 1587210/1587210     void std::__relocate_object_a<node, node, std::allocator<node> >(node*, node*, std::allocator<node>&) [150]
[148]    0.0    0.00    0.00 1587210         void std::allocator_traits<std::allocator<node> >::destroy<node>(std::allocator<node>&, node*) [148]
                0.00    0.00 1587210/1587210     void __gnu_cxx::new_allocator<node>::destroy<node>(node*) [146]
-----------------------------------------------
                0.00    0.00 1587210/1587210     void std::__relocate_object_a<node, node, std::allocator<node> >(node*, node*, std::allocator<node>&) [150]
[149]    0.0    0.00    0.00 1587210         void std::allocator_traits<std::allocator<node> >::construct<node, node>(std::allocator<node>&, node*, node&&) [149]
                0.00    0.00 1587210/3174420     node&& std::forward<node>(std::remove_reference<node>::type&) [142]
                0.00    0.00 1587210/1587210     void __gnu_cxx::new_allocator<node>::construct<node, node>(node*, node&&) [147]
-----------------------------------------------
                0.00    0.00 1587210/1587210     node* std::__relocate_a_1<node*, node*, std::allocator<node> >(node*, node*, node*, std::allocator<node>&) [181]
[150]    0.0    0.00    0.00 1587210         void std::__relocate_object_a<node, node, std::allocator<node> >(node*, node*, std::allocator<node>&) [150]
                0.00    0.00 1587210/1587210     std::remove_reference<node&>::type&& std::move<node&>(node&) [151]
                0.00    0.00 1587210/1587210     void std::allocator_traits<std::allocator<node> >::construct<node, node>(std::allocator<node>&, node*, node&&) [149]
                0.00    0.00 1587210/4761630     node* std::__addressof<node>(node&) [139]
                0.00    0.00 1587210/1587210     void std::allocator_traits<std::allocator<node> >::destroy<node>(std::allocator<node>&, node*) [148]
-----------------------------------------------
                0.00    0.00 1587210/1587210     void std::__relocate_object_a<node, node, std::allocator<node> >(node*, node*, std::allocator<node>&) [150]
[151]    0.0    0.00    0.00 1587210         std::remove_reference<node&>::type&& std::move<node&>(node&) [151]
-----------------------------------------------
                0.00    0.00       2/1523322     std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_S_max_size(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [272]
                0.00    0.00  761660/1523322     std::vector<node, std::allocator<node> >::_S_max_size(std::allocator<node> const&) [176]
                0.00    0.00  761660/1523322     std::vector<edge, std::allocator<edge> >::_S_max_size(std::allocator<edge> const&) [175]
[152]    0.0    0.00    0.00 1523322         unsigned long const& std::min<unsigned long>(unsigned long const&, unsigned long const&) [152]
-----------------------------------------------
                0.00    0.00  761660/1523320     void std::vector<node, std::allocator<node> >::_M_realloc_insert<node const&>(__gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >, node const&) [73]
                0.00    0.00  761660/1523320     __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >::difference_type __gnu_cxx::operator-<node*, std::vector<node, std::allocator<node> > >(__gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > > const&, __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > > const&) [196]
[153]    0.0    0.00    0.00 1523320         __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >::base() const [153]
-----------------------------------------------
                0.00    0.00 1523320/1523320     std::vector<node, std::allocator<node> >::_M_check_len(unsigned long, char const*) const [197]
[154]    0.0    0.00    0.00 1523320         std::vector<node, std::allocator<node> >::size() const [154]
-----------------------------------------------
                0.00    0.00 1371090/1371090     StandardGraph::get_out_edges(int) [14]
[155]    0.0    0.00    0.00 1371090         StandardGraph::__read_from_edge_idx(__wt_cursor*, edge*) [155]
-----------------------------------------------
                0.00    0.00  236200/1352160     std::vector<edge, std::allocator<edge> >::_M_move_assign(std::vector<edge, std::allocator<edge> >&&, std::integral_constant<bool, true>) [60]
                0.00    0.00  354300/1352160     std::vector<edge, std::allocator<edge> >::~vector() [16]
                0.00    0.00  761660/1352160     void std::vector<edge, std::allocator<edge> >::_M_realloc_insert<edge const&>(__gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > >, edge const&) [26]
[156]    0.0    0.00    0.00 1352160         std::_Vector_base<edge, std::allocator<edge> >::_M_get_Tp_allocator() [156]
-----------------------------------------------
                0.00    0.00      18/1268706     StandardGraph::get_node(int) [82]
                0.00    0.00      18/1268706     StandardGraph::get_random_node() [91]
                0.00    0.00 1268670/1268706     StandardGraph::get_out_nodes(int) [6]
[157]    0.0    0.00    0.00 1268706         StandardGraph::__record_to_node(__wt_cursor*) [157]
-----------------------------------------------
                0.00    0.00 1268670/1268670     void std::allocator_traits<std::allocator<edge> >::construct<edge, edge const&>(std::allocator<edge>&, edge*, edge const&) [162]
[158]    0.0    0.00    0.00 1268670         void __gnu_cxx::new_allocator<edge>::construct<edge, edge const&>(edge*, edge const&) [158]
                0.00    0.00 1268670/2918170     edge const& std::forward<edge const&>(std::remove_reference<edge const&>::type&) [143]
                0.00    0.00 1268670/4124554     operator new(unsigned long, void*) [141]
-----------------------------------------------
                0.00    0.00 1268670/1268670     void std::allocator_traits<std::allocator<node> >::construct<node, node const&>(std::allocator<node>&, node*, node const&) [31]
[159]    0.0    0.00    0.00 1268670         void __gnu_cxx::new_allocator<node>::construct<node, node const&>(node*, node const&) [159]
                0.00    0.00 1268670/2918170     node const& std::forward<node const&>(std::remove_reference<node const&>::type&) [144]
                0.00    0.00 1268670/4124554     operator new(unsigned long, void*) [141]
-----------------------------------------------
                0.00    0.00 1268670/1268670     StandardGraph::get_out_nodes(int) [6]
[160]    0.0    0.00    0.00 1268670         __gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > >::operator++() [160]
-----------------------------------------------
                0.00    0.00 1268670/1268670     StandardGraph::get_out_nodes(int) [6]
[161]    0.0    0.00    0.00 1268670         __gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > >::operator*() const [161]
-----------------------------------------------
                0.00    0.00  380830/1268670     void std::vector<edge, std::allocator<edge> >::_M_realloc_insert<edge const&>(__gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > >, edge const&) [26]
                0.00    0.00  887840/1268670     std::vector<edge, std::allocator<edge> >::push_back(edge const&) [25]
[162]    0.0    0.00    0.00 1268670         void std::allocator_traits<std::allocator<edge> >::construct<edge, edge const&>(std::allocator<edge>&, edge*, edge const&) [162]
                0.00    0.00 1268670/2918170     edge const& std::forward<edge const&>(std::remove_reference<edge const&>::type&) [143]
                0.00    0.00 1268670/1268670     void __gnu_cxx::new_allocator<edge>::construct<edge, edge const&>(edge*, edge const&) [158]
-----------------------------------------------
                0.00    0.00  380830/1142490     __gnu_cxx::new_allocator<edge>::allocate(unsigned long, void const*) [193]
                0.00    0.00  761660/1142490     std::allocator_traits<std::allocator<edge> >::max_size(std::allocator<edge> const&) [173]
[163]    0.0    0.00    0.00 1142490         __gnu_cxx::new_allocator<edge>::max_size() const [163]
-----------------------------------------------
                0.00    0.00  380830/1142490     __gnu_cxx::new_allocator<node>::allocate(unsigned long, void const*) [194]
                0.00    0.00  761660/1142490     std::allocator_traits<std::allocator<node> >::max_size(std::allocator<node> const&) [174]
[164]    0.0    0.00    0.00 1142490         __gnu_cxx::new_allocator<node>::max_size() const [164]
-----------------------------------------------
                0.00    0.00  483410/966820      std::vector<edge, std::allocator<edge> >::end() [186]
                0.00    0.00  483410/966820      std::vector<edge, std::allocator<edge> >::begin() [187]
[165]    0.0    0.00    0.00  966820         __gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > >::__normal_iterator(edge* const&) [165]
-----------------------------------------------
                0.00    0.00       1/944804      std::filesystem::__cxx11::path::path(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&, std::filesystem::__cxx11::path::format) [294]
                0.00    0.00  472400/944804      std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&, char const*) [191]
                0.00    0.00  472403/944804      std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&) [188]
[166]    0.0    0.00    0.00  944804         std::remove_reference<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&>::type&& std::move<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&) [166]
-----------------------------------------------
                0.00    0.00  118100/879760      std::_Vector_base<edge, std::allocator<edge> >::get_allocator() const [223]
                0.00    0.00  761660/879760      std::vector<edge, std::allocator<edge> >::max_size() const [170]
[167]    0.0    0.00    0.00  879760         std::_Vector_base<edge, std::allocator<edge> >::_M_get_Tp_allocator() const [167]
-----------------------------------------------
                0.00    0.00  380830/761660      std::vector<node, std::allocator<node> >::end() [204]
                0.00    0.00  380830/761660      std::vector<node, std::allocator<node> >::begin() [205]
[168]    0.0    0.00    0.00  761660         __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >::__normal_iterator(node* const&) [168]
-----------------------------------------------
                0.00    0.00  761660/761660      std::vector<node, std::allocator<node> >::max_size() const [171]
[169]    0.0    0.00    0.00  761660         std::_Vector_base<node, std::allocator<node> >::_M_get_Tp_allocator() const [169]
-----------------------------------------------
                0.00    0.00  761660/761660      std::vector<edge, std::allocator<edge> >::_M_check_len(unsigned long, char const*) const [58]
[170]    0.0    0.00    0.00  761660         std::vector<edge, std::allocator<edge> >::max_size() const [170]
                0.00    0.00  761660/879760      std::_Vector_base<edge, std::allocator<edge> >::_M_get_Tp_allocator() const [167]
                0.00    0.00  761660/761660      std::vector<edge, std::allocator<edge> >::_S_max_size(std::allocator<edge> const&) [175]
-----------------------------------------------
                0.00    0.00  761660/761660      std::vector<node, std::allocator<node> >::_M_check_len(unsigned long, char const*) const [197]
[171]    0.0    0.00    0.00  761660         std::vector<node, std::allocator<node> >::max_size() const [171]
                0.00    0.00  761660/761660      std::vector<node, std::allocator<node> >::_S_max_size(std::allocator<node> const&) [176]
                0.00    0.00  761660/761660      std::_Vector_base<node, std::allocator<node> >::_M_get_Tp_allocator() const [169]
-----------------------------------------------
                0.00    0.00  761660/761660      void std::vector<node, std::allocator<node> >::_M_realloc_insert<node const&>(__gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >, node const&) [73]
[172]    0.0    0.00    0.00  761660         std::_Vector_base<node, std::allocator<node> >::_M_get_Tp_allocator() [172]
-----------------------------------------------
                0.00    0.00  761660/761660      std::vector<edge, std::allocator<edge> >::_S_max_size(std::allocator<edge> const&) [175]
[173]    0.0    0.00    0.00  761660         std::allocator_traits<std::allocator<edge> >::max_size(std::allocator<edge> const&) [173]
                0.00    0.00  761660/1142490     __gnu_cxx::new_allocator<edge>::max_size() const [163]
-----------------------------------------------
                0.00    0.00  761660/761660      std::vector<node, std::allocator<node> >::_S_max_size(std::allocator<node> const&) [176]
[174]    0.0    0.00    0.00  761660         std::allocator_traits<std::allocator<node> >::max_size(std::allocator<node> const&) [174]
                0.00    0.00  761660/1142490     __gnu_cxx::new_allocator<node>::max_size() const [164]
-----------------------------------------------
                0.00    0.00  761660/761660      std::vector<edge, std::allocator<edge> >::max_size() const [170]
[175]    0.0    0.00    0.00  761660         std::vector<edge, std::allocator<edge> >::_S_max_size(std::allocator<edge> const&) [175]
                0.00    0.00  761660/761660      std::allocator_traits<std::allocator<edge> >::max_size(std::allocator<edge> const&) [173]
                0.00    0.00  761660/1523322     unsigned long const& std::min<unsigned long>(unsigned long const&, unsigned long const&) [152]
-----------------------------------------------
                0.00    0.00  761660/761660      std::vector<node, std::allocator<node> >::max_size() const [171]
[176]    0.0    0.00    0.00  761660         std::vector<node, std::allocator<node> >::_S_max_size(std::allocator<node> const&) [176]
                0.00    0.00  761660/761660      std::allocator_traits<std::allocator<node> >::max_size(std::allocator<node> const&) [174]
                0.00    0.00  761660/1523322     unsigned long const& std::min<unsigned long>(unsigned long const&, unsigned long const&) [152]
-----------------------------------------------
                0.00    0.00  761660/761660      void std::vector<node, std::allocator<node> >::_M_realloc_insert<node const&>(__gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >, node const&) [73]
[177]    0.0    0.00    0.00  761660         std::vector<node, std::allocator<node> >::_S_relocate(node*, node*, node*, std::allocator<node>&) [177]
                0.00    0.00  761660/761660      std::vector<node, std::allocator<node> >::_S_do_relocate(node*, node*, node*, std::allocator<node>&, std::integral_constant<bool, true>) [178]
-----------------------------------------------
                0.00    0.00  761660/761660      std::vector<node, std::allocator<node> >::_S_relocate(node*, node*, node*, std::allocator<node>&) [177]
[178]    0.0    0.00    0.00  761660         std::vector<node, std::allocator<node> >::_S_do_relocate(node*, node*, node*, std::allocator<node>&, std::integral_constant<bool, true>) [178]
                0.00    0.00  761660/761660      node* std::__relocate_a<node*, node*, std::allocator<node> >(node*, node*, node*, std::allocator<node>&) [179]
-----------------------------------------------
                0.00    0.00  761660/761660      std::vector<node, std::allocator<node> >::_S_do_relocate(node*, node*, node*, std::allocator<node>&, std::integral_constant<bool, true>) [178]
[179]    0.0    0.00    0.00  761660         node* std::__relocate_a<node*, node*, std::allocator<node> >(node*, node*, node*, std::allocator<node>&) [179]
                0.00    0.00 2284980/2284980     node* std::__niter_base<node*>(node*) [145]
                0.00    0.00  761660/761660      node* std::__relocate_a_1<node*, node*, std::allocator<node> >(node*, node*, node*, std::allocator<node>&) [181]
-----------------------------------------------
                0.00    0.00  761660/761660      edge* std::__relocate_a<edge*, edge*, std::allocator<edge> >(edge*, edge*, edge*, std::allocator<edge>&) [35]
[180]    0.0    0.00    0.00  761660         std::enable_if<std::__is_bitwise_relocatable<edge, void>::value, edge*>::type std::__relocate_a_1<edge, edge>(edge*, edge*, edge*, std::allocator<edge>&) [180]
-----------------------------------------------
                0.00    0.00  761660/761660      node* std::__relocate_a<node*, node*, std::allocator<node> >(node*, node*, node*, std::allocator<node>&) [179]
[181]    0.0    0.00    0.00  761660         node* std::__relocate_a_1<node*, node*, std::allocator<node> >(node*, node*, node*, std::allocator<node>&) [181]
                0.00    0.00 3174420/4761630     node* std::__addressof<node>(node&) [139]
                0.00    0.00 1587210/1587210     void std::__relocate_object_a<node, node, std::allocator<node> >(node*, node*, std::allocator<node>&) [150]
-----------------------------------------------
                0.00    0.00  380830/761660      std::vector<node, std::allocator<node> >::_M_check_len(unsigned long, char const*) const [197]
                0.00    0.00  380830/761660      std::vector<edge, std::allocator<edge> >::_M_check_len(unsigned long, char const*) const [58]
[182]    0.0    0.00    0.00  761660         unsigned long const& std::max<unsigned long>(unsigned long const&, unsigned long const&) [182]
-----------------------------------------------
                0.00    0.00  354300/735130      std::_Vector_base<edge, std::allocator<edge> >::~_Vector_base() [208]
                0.00    0.00  380830/735130      void std::vector<edge, std::allocator<edge> >::_M_realloc_insert<edge const&>(__gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > >, edge const&) [26]
[183]    0.0    0.00    0.00  735130         std::_Vector_base<edge, std::allocator<edge> >::_M_deallocate(edge*, unsigned long) [183]
                0.00    0.00  380830/380830      std::allocator_traits<std::allocator<edge> >::deallocate(std::allocator<edge>&, edge*, unsigned long) [201]
-----------------------------------------------
                0.00    0.00  708600/708600      std::_Vector_base<edge, std::allocator<edge> >::_Vector_impl_data::_M_swap_data(std::_Vector_base<edge, std::allocator<edge> >::_Vector_impl_data&) [219]
[184]    0.0    0.00    0.00  708600         std::_Vector_base<edge, std::allocator<edge> >::_Vector_impl_data::_M_copy_data(std::_Vector_base<edge, std::allocator<edge> >::_Vector_impl_data const&) [184]
-----------------------------------------------
                0.00    0.00  118100/590500      std::_Vector_base<edge, std::allocator<edge> >::_Vector_impl::_Vector_impl(std::allocator<edge> const&) [225]
                0.00    0.00  236200/590500      std::_Vector_base<edge, std::allocator<edge> >::_Vector_impl::_Vector_impl() [218]
                0.00    0.00  236200/590500      std::_Vector_base<edge, std::allocator<edge> >::_Vector_impl_data::_M_swap_data(std::_Vector_base<edge, std::allocator<edge> >::_Vector_impl_data&) [219]
[185]    0.0    0.00    0.00  590500         std::_Vector_base<edge, std::allocator<edge> >::_Vector_impl_data::_Vector_impl_data() [185]
-----------------------------------------------
                0.00    0.00  102580/483410      StandardGraph::get_out_nodes(int) [6]
                0.00    0.00  380830/483410      std::vector<edge, std::allocator<edge> >::push_back(edge const&) [25]
[186]    0.0    0.00    0.00  483410         std::vector<edge, std::allocator<edge> >::end() [186]
                0.00    0.00  483410/966820      __gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > >::__normal_iterator(edge* const&) [165]
-----------------------------------------------
                0.00    0.00  102580/483410      StandardGraph::get_out_nodes(int) [6]
                0.00    0.00  380830/483410      void std::vector<edge, std::allocator<edge> >::_M_realloc_insert<edge const&>(__gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > >, edge const&) [26]
[187]    0.0    0.00    0.00  483410         std::vector<edge, std::allocator<edge> >::begin() [187]
                0.00    0.00  483410/966820      __gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > >::__normal_iterator(edge* const&) [165]
-----------------------------------------------
                0.00    0.00       1/472403      __static_initialization_and_destruction_0(int, int) [79]
                0.00    0.00       2/472403      StandardGraph::StandardGraph(graph_opts) [62]
                0.00    0.00  236200/472403      StandardGraph::_get_index_cursor(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, __wt_cursor**) [74]
                0.00    0.00  236200/472403      StandardGraph::get_out_edges(int) [14]
[188]    0.0    0.00    0.00  472403         std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&) [188]
                0.00    0.00  472403/944804      std::remove_reference<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&>::type&& std::move<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&) [166]
-----------------------------------------------
                0.00    0.00  472400/472400      std::allocator<edge>::~allocator() [190]
[189]    0.0    0.00    0.00  472400         __gnu_cxx::new_allocator<edge>::~new_allocator() [189]
-----------------------------------------------
                0.00    0.00  118100/472400      std::vector<edge, std::allocator<edge> >::_M_move_assign(std::vector<edge, std::allocator<edge> >&&, std::integral_constant<bool, true>) [60]
                0.00    0.00  354300/472400      std::_Vector_base<edge, std::allocator<edge> >::_Vector_impl::~_Vector_impl() [207]
[190]    0.0    0.00    0.00  472400         std::allocator<edge>::~allocator() [190]
                0.00    0.00  472400/472400      __gnu_cxx::new_allocator<edge>::~new_allocator() [189]
-----------------------------------------------
                0.00    0.00  118100/472400      StandardGraph::_get_index_cursor(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, __wt_cursor**) [74]
                0.00    0.00  354300/472400      StandardGraph::get_out_edges(int) [14]
[191]    0.0    0.00    0.00  472400         std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&, char const*) [191]
                0.00    0.00  472400/944804      std::remove_reference<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&>::type&& std::move<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&) [166]
-----------------------------------------------
                0.00    0.00  380830/380830      std::allocator_traits<std::allocator<edge> >::deallocate(std::allocator<edge>&, edge*, unsigned long) [201]
[192]    0.0    0.00    0.00  380830         __gnu_cxx::new_allocator<edge>::deallocate(edge*, unsigned long) [192]
-----------------------------------------------
                0.00    0.00  380830/380830      std::allocator_traits<std::allocator<edge> >::allocate(std::allocator<edge>&, unsigned long) [202]
[193]    0.0    0.00    0.00  380830         __gnu_cxx::new_allocator<edge>::allocate(unsigned long, void const*) [193]
                0.00    0.00  380830/1142490     __gnu_cxx::new_allocator<edge>::max_size() const [163]
-----------------------------------------------
                0.00    0.00  380830/380830      std::allocator_traits<std::allocator<node> >::allocate(std::allocator<node>&, unsigned long) [203]
[194]    0.0    0.00    0.00  380830         __gnu_cxx::new_allocator<node>::allocate(unsigned long, void const*) [194]
                0.00    0.00  380830/1142490     __gnu_cxx::new_allocator<node>::max_size() const [164]
-----------------------------------------------
                0.00    0.00  380830/380830      void std::vector<edge, std::allocator<edge> >::_M_realloc_insert<edge const&>(__gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > >, edge const&) [26]
[195]    0.0    0.00    0.00  380830         __gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > >::difference_type __gnu_cxx::operator-<edge*, std::vector<edge, std::allocator<edge> > >(__gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > > const&, __gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > > const&) [195]
                0.00    0.00  761660/4265820     __gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > >::base() const [140]
-----------------------------------------------
                0.00    0.00  380830/380830      void std::vector<node, std::allocator<node> >::_M_realloc_insert<node const&>(__gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >, node const&) [73]
[196]    0.0    0.00    0.00  380830         __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >::difference_type __gnu_cxx::operator-<node*, std::vector<node, std::allocator<node> > >(__gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > > const&, __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > > const&) [196]
                0.00    0.00  761660/1523320     __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >::base() const [153]
-----------------------------------------------
                0.00    0.00  380830/380830      void std::vector<node, std::allocator<node> >::_M_realloc_insert<node const&>(__gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >, node const&) [73]
[197]    0.0    0.00    0.00  380830         std::vector<node, std::allocator<node> >::_M_check_len(unsigned long, char const*) const [197]
                0.00    0.00 1523320/1523320     std::vector<node, std::allocator<node> >::size() const [154]
                0.00    0.00  761660/761660      std::vector<node, std::allocator<node> >::max_size() const [171]
                0.00    0.00  380830/761660      unsigned long const& std::max<unsigned long>(unsigned long const&, unsigned long const&) [182]
-----------------------------------------------
                0.00    0.00  380830/380830      void std::vector<edge, std::allocator<edge> >::_M_realloc_insert<edge const&>(__gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > >, edge const&) [26]
[198]    0.0    0.00    0.00  380830         std::_Vector_base<edge, std::allocator<edge> >::_M_allocate(unsigned long) [198]
                0.00    0.00  380830/380830      std::allocator_traits<std::allocator<edge> >::allocate(std::allocator<edge>&, unsigned long) [202]
-----------------------------------------------
                0.00    0.00  380830/380830      void std::vector<node, std::allocator<node> >::_M_realloc_insert<node const&>(__gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >, node const&) [73]
[199]    0.0    0.00    0.00  380830         std::_Vector_base<node, std::allocator<node> >::_M_allocate(unsigned long) [199]
                0.00    0.00  380830/380830      std::allocator_traits<std::allocator<node> >::allocate(std::allocator<node>&, unsigned long) [203]
-----------------------------------------------
                0.00    0.00  380830/380830      void std::vector<node, std::allocator<node> >::_M_realloc_insert<node const&>(__gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >, node const&) [73]
[200]    0.0    0.00    0.00  380830         std::_Vector_base<node, std::allocator<node> >::_M_deallocate(node*, unsigned long) [200]
                0.00    0.00  278250/278250      std::allocator_traits<std::allocator<node> >::deallocate(std::allocator<node>&, node*, unsigned long) [213]
-----------------------------------------------
                0.00    0.00  380830/380830      std::_Vector_base<edge, std::allocator<edge> >::_M_deallocate(edge*, unsigned long) [183]
[201]    0.0    0.00    0.00  380830         std::allocator_traits<std::allocator<edge> >::deallocate(std::allocator<edge>&, edge*, unsigned long) [201]
                0.00    0.00  380830/380830      __gnu_cxx::new_allocator<edge>::deallocate(edge*, unsigned long) [192]
-----------------------------------------------
                0.00    0.00  380830/380830      std::_Vector_base<edge, std::allocator<edge> >::_M_allocate(unsigned long) [198]
[202]    0.0    0.00    0.00  380830         std::allocator_traits<std::allocator<edge> >::allocate(std::allocator<edge>&, unsigned long) [202]
                0.00    0.00  380830/380830      __gnu_cxx::new_allocator<edge>::allocate(unsigned long, void const*) [193]
-----------------------------------------------
                0.00    0.00  380830/380830      std::_Vector_base<node, std::allocator<node> >::_M_allocate(unsigned long) [199]
[203]    0.0    0.00    0.00  380830         std::allocator_traits<std::allocator<node> >::allocate(std::allocator<node>&, unsigned long) [203]
                0.00    0.00  380830/380830      __gnu_cxx::new_allocator<node>::allocate(unsigned long, void const*) [194]
-----------------------------------------------
                0.00    0.00  380830/380830      std::vector<node, std::allocator<node> >::push_back(node const&) [32]
[204]    0.0    0.00    0.00  380830         std::vector<node, std::allocator<node> >::end() [204]
                0.00    0.00  380830/761660      __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >::__normal_iterator(node* const&) [168]
-----------------------------------------------
                0.00    0.00  380830/380830      void std::vector<node, std::allocator<node> >::_M_realloc_insert<node const&>(__gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >, node const&) [73]
[205]    0.0    0.00    0.00  380830         std::vector<node, std::allocator<node> >::begin() [205]
                0.00    0.00  380830/761660      __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >::__normal_iterator(node* const&) [168]
-----------------------------------------------
                0.00    0.00  354300/354300      void std::_Destroy<edge*>(edge*, edge*) [209]
[206]    0.0    0.00    0.00  354300         void std::_Destroy_aux<true>::__destroy<edge*>(edge*, edge*) [206]
-----------------------------------------------
                0.00    0.00  354300/354300      std::_Vector_base<edge, std::allocator<edge> >::~_Vector_base() [208]
[207]    0.0    0.00    0.00  354300         std::_Vector_base<edge, std::allocator<edge> >::_Vector_impl::~_Vector_impl() [207]
                0.00    0.00  354300/472400      std::allocator<edge>::~allocator() [190]
-----------------------------------------------
                0.00    0.00  354300/354300      std::vector<edge, std::allocator<edge> >::~vector() [16]
[208]    0.0    0.00    0.00  354300         std::_Vector_base<edge, std::allocator<edge> >::~_Vector_base() [208]
                0.00    0.00  354300/735130      std::_Vector_base<edge, std::allocator<edge> >::_M_deallocate(edge*, unsigned long) [183]
                0.00    0.00  354300/354300      std::_Vector_base<edge, std::allocator<edge> >::_Vector_impl::~_Vector_impl() [207]
-----------------------------------------------
                0.00    0.00  354300/354300      void std::_Destroy<edge*, edge>(edge*, edge*, std::allocator<edge>&) [17]
[209]    0.0    0.00    0.00  354300         void std::_Destroy<edge*>(edge*, edge*) [209]
                0.00    0.00  354300/354300      void std::_Destroy_aux<true>::__destroy<edge*>(edge*, edge*) [206]
-----------------------------------------------
                0.00    0.00  338813/338813      void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char*>(char*, char*, std::forward_iterator_tag) [70]
[210]    0.0    0.00    0.00  338813         bool __gnu_cxx::__is_null_pointer<char>(char*) [210]
-----------------------------------------------
                0.00    0.00  338813/338813      std::iterator_traits<char*>::difference_type std::distance<char*>(char*, char*) [72]
[211]    0.0    0.00    0.00  338813         std::iterator_traits<char*>::iterator_category std::__iterator_category<char*>(char* const&) [211]
-----------------------------------------------
                0.00    0.00  278250/278250      std::allocator_traits<std::allocator<node> >::deallocate(std::allocator<node>&, node*, unsigned long) [213]
[212]    0.0    0.00    0.00  278250         __gnu_cxx::new_allocator<node>::deallocate(node*, unsigned long) [212]
-----------------------------------------------
                0.00    0.00  278250/278250      std::_Vector_base<node, std::allocator<node> >::_M_deallocate(node*, unsigned long) [200]
[213]    0.0    0.00    0.00  278250         std::allocator_traits<std::allocator<node> >::deallocate(std::allocator<node>&, node*, unsigned long) [213]
                0.00    0.00  278250/278250      __gnu_cxx::new_allocator<node>::deallocate(node*, unsigned long) [212]
-----------------------------------------------
                0.00    0.00  236200/236200      std::allocator<edge>::allocator(std::allocator<edge> const&) [216]
[214]    0.0    0.00    0.00  236200         __gnu_cxx::new_allocator<edge>::new_allocator(__gnu_cxx::new_allocator<edge> const&) [214]
-----------------------------------------------
                0.00    0.00  236200/236200      std::allocator<edge>::allocator() [217]
[215]    0.0    0.00    0.00  236200         __gnu_cxx::new_allocator<edge>::new_allocator() [215]
-----------------------------------------------
                0.00    0.00  118100/236200      std::_Vector_base<edge, std::allocator<edge> >::get_allocator() const [223]
                0.00    0.00  118100/236200      std::_Vector_base<edge, std::allocator<edge> >::_Vector_impl::_Vector_impl(std::allocator<edge> const&) [225]
[216]    0.0    0.00    0.00  236200         std::allocator<edge>::allocator(std::allocator<edge> const&) [216]
                0.00    0.00  236200/236200      __gnu_cxx::new_allocator<edge>::new_allocator(__gnu_cxx::new_allocator<edge> const&) [214]
-----------------------------------------------
                0.00    0.00  236200/236200      std::_Vector_base<edge, std::allocator<edge> >::_Vector_impl::_Vector_impl() [218]
[217]    0.0    0.00    0.00  236200         std::allocator<edge>::allocator() [217]
                0.00    0.00  236200/236200      __gnu_cxx::new_allocator<edge>::new_allocator() [215]
-----------------------------------------------
                0.00    0.00  236200/236200      std::_Vector_base<edge, std::allocator<edge> >::_Vector_base() [220]
[218]    0.0    0.00    0.00  236200         std::_Vector_base<edge, std::allocator<edge> >::_Vector_impl::_Vector_impl() [218]
                0.00    0.00  236200/236200      std::allocator<edge>::allocator() [217]
                0.00    0.00  236200/590500      std::_Vector_base<edge, std::allocator<edge> >::_Vector_impl_data::_Vector_impl_data() [185]
-----------------------------------------------
                0.00    0.00  236200/236200      std::vector<edge, std::allocator<edge> >::_M_move_assign(std::vector<edge, std::allocator<edge> >&&, std::integral_constant<bool, true>) [60]
[219]    0.0    0.00    0.00  236200         std::_Vector_base<edge, std::allocator<edge> >::_Vector_impl_data::_M_swap_data(std::_Vector_base<edge, std::allocator<edge> >::_Vector_impl_data&) [219]
                0.00    0.00  708600/708600      std::_Vector_base<edge, std::allocator<edge> >::_Vector_impl_data::_M_copy_data(std::_Vector_base<edge, std::allocator<edge> >::_Vector_impl_data const&) [184]
                0.00    0.00  236200/590500      std::_Vector_base<edge, std::allocator<edge> >::_Vector_impl_data::_Vector_impl_data() [185]
-----------------------------------------------
                0.00    0.00  236200/236200      std::vector<edge, std::allocator<edge> >::vector() [221]
[220]    0.0    0.00    0.00  236200         std::_Vector_base<edge, std::allocator<edge> >::_Vector_base() [220]
                0.00    0.00  236200/236200      std::_Vector_base<edge, std::allocator<edge> >::_Vector_impl::_Vector_impl() [218]
-----------------------------------------------
                0.00    0.00  118100/236200      StandardGraph::get_out_edges(int) [14]
                0.00    0.00  118100/236200      StandardGraph::get_out_nodes(int) [6]
[221]    0.0    0.00    0.00  236200         std::vector<edge, std::allocator<edge> >::vector() [221]
                0.00    0.00  236200/236200      std::_Vector_base<edge, std::allocator<edge> >::_Vector_base() [220]
-----------------------------------------------
                0.00    0.00  118100/118100      std::allocator<node>::allocator() [224]
[222]    0.0    0.00    0.00  118100         __gnu_cxx::new_allocator<node>::new_allocator() [222]
-----------------------------------------------
                0.00    0.00  118100/118100      std::vector<edge, std::allocator<edge> >::_M_move_assign(std::vector<edge, std::allocator<edge> >&&, std::integral_constant<bool, true>) [60]
[223]    0.0    0.00    0.00  118100         std::_Vector_base<edge, std::allocator<edge> >::get_allocator() const [223]
                0.00    0.00  118100/879760      std::_Vector_base<edge, std::allocator<edge> >::_M_get_Tp_allocator() const [167]
                0.00    0.00  118100/236200      std::allocator<edge>::allocator(std::allocator<edge> const&) [216]
-----------------------------------------------
                0.00    0.00  118100/118100      std::_Vector_base<node, std::allocator<node> >::_Vector_impl::_Vector_impl() [227]
[224]    0.0    0.00    0.00  118100         std::allocator<node>::allocator() [224]
                0.00    0.00  118100/118100      __gnu_cxx::new_allocator<node>::new_allocator() [222]
-----------------------------------------------
                0.00    0.00  118100/118100      std::_Vector_base<edge, std::allocator<edge> >::_Vector_base(std::allocator<edge> const&) [226]
[225]    0.0    0.00    0.00  118100         std::_Vector_base<edge, std::allocator<edge> >::_Vector_impl::_Vector_impl(std::allocator<edge> const&) [225]
                0.00    0.00  118100/236200      std::allocator<edge>::allocator(std::allocator<edge> const&) [216]
                0.00    0.00  118100/590500      std::_Vector_base<edge, std::allocator<edge> >::_Vector_impl_data::_Vector_impl_data() [185]
-----------------------------------------------
                0.00    0.00  118100/118100      std::vector<edge, std::allocator<edge> >::vector(std::allocator<edge> const&) [230]
[226]    0.0    0.00    0.00  118100         std::_Vector_base<edge, std::allocator<edge> >::_Vector_base(std::allocator<edge> const&) [226]
                0.00    0.00  118100/118100      std::_Vector_base<edge, std::allocator<edge> >::_Vector_impl::_Vector_impl(std::allocator<edge> const&) [225]
-----------------------------------------------
                0.00    0.00  118100/118100      std::_Vector_base<node, std::allocator<node> >::_Vector_base() [229]
[227]    0.0    0.00    0.00  118100         std::_Vector_base<node, std::allocator<node> >::_Vector_impl::_Vector_impl() [227]
                0.00    0.00  118100/118100      std::allocator<node>::allocator() [224]
                0.00    0.00  118100/118100      std::_Vector_base<node, std::allocator<node> >::_Vector_impl_data::_Vector_impl_data() [228]
-----------------------------------------------
                0.00    0.00  118100/118100      std::_Vector_base<node, std::allocator<node> >::_Vector_impl::_Vector_impl() [227]
[228]    0.0    0.00    0.00  118100         std::_Vector_base<node, std::allocator<node> >::_Vector_impl_data::_Vector_impl_data() [228]
-----------------------------------------------
                0.00    0.00  118100/118100      std::vector<node, std::allocator<node> >::vector() [231]
[229]    0.0    0.00    0.00  118100         std::_Vector_base<node, std::allocator<node> >::_Vector_base() [229]
                0.00    0.00  118100/118100      std::_Vector_base<node, std::allocator<node> >::_Vector_impl::_Vector_impl() [227]
-----------------------------------------------
                0.00    0.00  118100/118100      std::vector<edge, std::allocator<edge> >::_M_move_assign(std::vector<edge, std::allocator<edge> >&&, std::integral_constant<bool, true>) [60]
[230]    0.0    0.00    0.00  118100         std::vector<edge, std::allocator<edge> >::vector(std::allocator<edge> const&) [230]
                0.00    0.00  118100/118100      std::_Vector_base<edge, std::allocator<edge> >::_Vector_base(std::allocator<edge> const&) [226]
-----------------------------------------------
                0.00    0.00  118100/118100      StandardGraph::get_out_nodes(int) [6]
[231]    0.0    0.00    0.00  118100         std::vector<node, std::allocator<node> >::vector() [231]
                0.00    0.00  118100/118100      std::_Vector_base<node, std::allocator<node> >::_Vector_base() [229]
-----------------------------------------------
                0.00    0.00  118100/118100      std::vector<edge, std::allocator<edge> >::_M_move_assign(std::vector<edge, std::allocator<edge> >&&, std::integral_constant<bool, true>) [60]
[232]    0.0    0.00    0.00  118100         void std::__alloc_on_move<std::allocator<edge> >(std::allocator<edge>&, std::allocator<edge>&) [232]
                0.00    0.00  118100/118100      void std::__do_alloc_on_move<std::allocator<edge> >(std::allocator<edge>&, std::allocator<edge>&, std::integral_constant<bool, true>) [233]
-----------------------------------------------
                0.00    0.00  118100/118100      void std::__alloc_on_move<std::allocator<edge> >(std::allocator<edge>&, std::allocator<edge>&) [232]
[233]    0.0    0.00    0.00  118100         void std::__do_alloc_on_move<std::allocator<edge> >(std::allocator<edge>&, std::allocator<edge>&, std::integral_constant<bool, true>) [233]
                0.00    0.00  118100/118100      std::remove_reference<std::allocator<edge>&>::type&& std::move<std::allocator<edge>&>(std::allocator<edge>&) [234]
-----------------------------------------------
                0.00    0.00  118100/118100      void std::__do_alloc_on_move<std::allocator<edge> >(std::allocator<edge>&, std::allocator<edge>&, std::integral_constant<bool, true>) [233]
[234]    0.0    0.00    0.00  118100         std::remove_reference<std::allocator<edge>&>::type&& std::move<std::allocator<edge>&>(std::allocator<edge>&) [234]
-----------------------------------------------
                0.00    0.00  118100/118100      std::vector<edge, std::allocator<edge> >::operator=(std::vector<edge, std::allocator<edge> >&&) [61]
[235]    0.0    0.00    0.00  118100         std::remove_reference<std::vector<edge, std::allocator<edge> >&>::type&& std::move<std::vector<edge, std::allocator<edge> >&>(std::vector<edge, std::allocator<edge> >&) [235]
-----------------------------------------------
                0.00    0.00  102618/102618      void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char const*>(char const*, char const*, std::forward_iterator_tag) [238]
[236]    0.0    0.00    0.00  102618         bool __gnu_cxx::__is_null_pointer<char const>(char const*) [236]
-----------------------------------------------
                0.00    0.00  102618/102618      std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::basic_string<std::allocator<char> >(char const*, std::allocator<char> const&) [75]
[237]    0.0    0.00    0.00  102618         void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char const*>(char const*, char const*) [237]
                0.00    0.00  102618/102618      void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct_aux<char const*>(char const*, char const*, std::__false_type) [239]
-----------------------------------------------
                0.00    0.00  102618/102618      void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct_aux<char const*>(char const*, char const*, std::__false_type) [239]
[238]    0.0    0.00    0.00  102618         void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char const*>(char const*, char const*, std::forward_iterator_tag) [238]
                0.00    0.00  102618/102618      bool __gnu_cxx::__is_null_pointer<char const>(char const*) [236]
                0.00    0.00  102618/102618      std::iterator_traits<char const*>::difference_type std::distance<char const*>(char const*, char const*) [242]
-----------------------------------------------
                0.00    0.00  102618/102618      void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char const*>(char const*, char const*) [237]
[239]    0.0    0.00    0.00  102618         void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct_aux<char const*>(char const*, char const*, std::__false_type) [239]
                0.00    0.00  102618/102618      void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char const*>(char const*, char const*, std::forward_iterator_tag) [238]
-----------------------------------------------
                0.00    0.00  102618/102618      std::iterator_traits<char const*>::difference_type std::distance<char const*>(char const*, char const*) [242]
[240]    0.0    0.00    0.00  102618         std::iterator_traits<char const*>::difference_type std::__distance<char const*>(char const*, char const*, std::random_access_iterator_tag) [240]
-----------------------------------------------
                0.00    0.00  102618/102618      std::iterator_traits<char const*>::difference_type std::distance<char const*>(char const*, char const*) [242]
[241]    0.0    0.00    0.00  102618         std::iterator_traits<char const*>::iterator_category std::__iterator_category<char const*>(char const* const&) [241]
-----------------------------------------------
                0.00    0.00  102618/102618      void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char const*>(char const*, char const*, std::forward_iterator_tag) [238]
[242]    0.0    0.00    0.00  102618         std::iterator_traits<char const*>::difference_type std::distance<char const*>(char const*, char const*) [242]
                0.00    0.00  102618/102618      std::iterator_traits<char const*>::iterator_category std::__iterator_category<char const*>(char const* const&) [241]
                0.00    0.00  102618/102618      std::iterator_traits<char const*>::difference_type std::__distance<char const*>(char const*, char const*, std::random_access_iterator_tag) [240]
-----------------------------------------------
                0.00    0.00  102580/102580      StandardGraph::get_out_nodes(int) [6]
[243]    0.0    0.00    0.00  102580         CommonUtil::check_return(int, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >) [243]
-----------------------------------------------
                0.00    0.00    1760/1760        bfs_info* bfs<StandardGraph>(StandardGraph&, int) [3]
[244]    0.0    0.00    0.00    1760         void std::vector<int, std::allocator<int> >::_M_realloc_insert<int const&>(__gnu_cxx::__normal_iterator<int*, std::vector<int, std::allocator<int> > >, int const&) [244]
-----------------------------------------------
                              116730             std::_Rb_tree<int, int, std::_Identity<int>, std::less<int>, std::allocator<int> >::_M_erase(std::_Rb_tree_node<int>*) [245]
                0.00    0.00    1370/1370        bfs_info* bfs<StandardGraph>(StandardGraph&, int) [3]
[245]    0.0    0.00    0.00    1370+116730  std::_Rb_tree<int, int, std::_Identity<int>, std::less<int>, std::allocator<int> >::_M_erase(std::_Rb_tree_node<int>*) [245]
                              116730             std::_Rb_tree<int, int, std::_Identity<int>, std::less<int>, std::allocator<int> >::_M_erase(std::_Rb_tree_node<int>*) [245]
-----------------------------------------------
                0.00    0.00     800/800         bfs_info* bfs<StandardGraph>(StandardGraph&, int) [3]
[246]    0.0    0.00    0.00     800         void std::deque<int, std::allocator<int> >::_M_push_back_aux<int const&>(int const&) [246]
-----------------------------------------------
                0.00    0.00     160/160         bfs_info* bfs<StandardGraph>(StandardGraph&, int) [3]
[247]    0.0    0.00    0.00     160         std::_Deque_base<int, std::allocator<int> >::_M_initialize_map(unsigned long) [247]
-----------------------------------------------
                0.00    0.00       1/19          CmdLineApp::CmdLineApp(int, char**) [317]
                0.00    0.00       2/19          std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_replace(unsigned long, unsigned long, char const*, unsigned long) [274]
                0.00    0.00      16/19          print_csv_info(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, int, bfs_info*) [311]
[248]    0.0    0.00    0.00      19         std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_mutate(unsigned long, unsigned long, char const*, unsigned long) [248]
-----------------------------------------------
                0.00    0.00      18/18          StandardGraph::get_random_node() [91]
[249]    0.0    0.00    0.00      18         node::node() [249]
-----------------------------------------------
                0.00    0.00       5/5           std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::~allocator() [251]
[250]    0.0    0.00    0.00       5         __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::~new_allocator() [250]
-----------------------------------------------
                0.00    0.00       1/5           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl::~_Vector_impl() [299]
                0.00    0.00       2/5           std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_S_check_init_len(unsigned long, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [273]
                0.00    0.00       2/5           StandardGraph::StandardGraph(graph_opts) [62]
[251]    0.0    0.00    0.00       5         std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::~allocator() [251]
                0.00    0.00       5/5           __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::~new_allocator() [250]
-----------------------------------------------
                0.00    0.00       1/5           std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::~vector() [305]
                0.00    0.00       4/5           void std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_range_initialize<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::forward_iterator_tag) [86]
[252]    0.0    0.00    0.00       5         std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_get_Tp_allocator() [252]
-----------------------------------------------
                0.00    0.00       5/5           CmdLineBase::CmdLineBase(int, char**) [286]
[253]    0.0    0.00    0.00       5         void std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_realloc_insert<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >(__gnu_cxx::__normal_iterator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > > >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&) [253]
-----------------------------------------------
                0.00    0.00       4/4           std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::allocator(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [257]
[254]    0.0    0.00    0.00       4         __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::new_allocator(__gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [254]
-----------------------------------------------
                0.00    0.00       2/4           std::allocator_traits<std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::max_size(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [271]
                0.00    0.00       2/4           __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::allocate(unsigned long, void const*) [263]
[255]    0.0    0.00    0.00       4         __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::max_size() const [255]
-----------------------------------------------
                0.00    0.00       2/4           std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::vector(std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [87]
                0.00    0.00       2/4           std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::end() const [265]
[256]    0.0    0.00    0.00       4         std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::begin() const [256]
-----------------------------------------------
                0.00    0.00       2/4           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl::_Vector_impl(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [268]
                0.00    0.00       2/4           std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_S_check_init_len(unsigned long, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [273]
[257]    0.0    0.00    0.00       4         std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::allocator(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [257]
                0.00    0.00       4/4           __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::new_allocator(__gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [254]
-----------------------------------------------
                0.00    0.00       4/4           std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::__uninitialized_copy<false>::__uninit_copy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*) [85]
[258]    0.0    0.00    0.00       4         std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::__addressof<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&) [258]
-----------------------------------------------
                0.00    0.00       4/4           void std::_Construct<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&) [84]
[259]    0.0    0.00    0.00       4         std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const& std::forward<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&>(std::remove_reference<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&>::type&) [259]
-----------------------------------------------
                0.00    0.00       3/3           std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::allocator() [261]
[260]    0.0    0.00    0.00       3         __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::new_allocator() [260]
-----------------------------------------------
                0.00    0.00       1/3           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl::_Vector_impl() [298]
                0.00    0.00       2/3           StandardGraph::StandardGraph(graph_opts) [62]
[261]    0.0    0.00    0.00       3         std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::allocator() [261]
                0.00    0.00       3/3           __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::new_allocator() [260]
-----------------------------------------------
                0.00    0.00       1/3           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl::_Vector_impl() [298]
                0.00    0.00       2/3           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl::_Vector_impl(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [268]
[262]    0.0    0.00    0.00       3         std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl_data::_Vector_impl_data() [262]
-----------------------------------------------
                0.00    0.00       2/2           std::allocator_traits<std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::allocate(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >&, unsigned long) [270]
[263]    0.0    0.00    0.00       2         __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::allocate(unsigned long, void const*) [263]
                0.00    0.00       2/4           __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::max_size() const [255]
-----------------------------------------------
                0.00    0.00       1/2           std::filesystem::exists(std::filesystem::file_status) [292]
                0.00    0.00       1/2           std::filesystem::status_known(std::filesystem::file_status) [291]
[264]    0.0    0.00    0.00       2         std::filesystem::file_status::type() const [264]
-----------------------------------------------
                0.00    0.00       2/2           std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::vector(std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [87]
[265]    0.0    0.00    0.00       2         std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::end() const [265]
                0.00    0.00       2/4           std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::begin() const [256]
                0.00    0.00       2/2           std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::size() const [266]
-----------------------------------------------
                0.00    0.00       2/2           std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::end() const [265]
[266]    0.0    0.00    0.00       2         std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::size() const [266]
-----------------------------------------------
                0.00    0.00       2/2           void std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_range_initialize<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::forward_iterator_tag) [86]
[267]    0.0    0.00    0.00       2         std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_allocate(unsigned long) [267]
                0.00    0.00       2/2           std::allocator_traits<std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::allocate(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >&, unsigned long) [270]
-----------------------------------------------
                0.00    0.00       2/2           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_base(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [269]
[268]    0.0    0.00    0.00       2         std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl::_Vector_impl(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [268]
                0.00    0.00       2/4           std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::allocator(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [257]
                0.00    0.00       2/3           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl_data::_Vector_impl_data() [262]
-----------------------------------------------
                0.00    0.00       2/2           std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::vector(std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [87]
[269]    0.0    0.00    0.00       2         std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_base(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [269]
                0.00    0.00       2/2           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl::_Vector_impl(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [268]
-----------------------------------------------
                0.00    0.00       2/2           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_allocate(unsigned long) [267]
[270]    0.0    0.00    0.00       2         std::allocator_traits<std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::allocate(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >&, unsigned long) [270]
                0.00    0.00       2/2           __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::allocate(unsigned long, void const*) [263]
-----------------------------------------------
                0.00    0.00       2/2           std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_S_max_size(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [272]
[271]    0.0    0.00    0.00       2         std::allocator_traits<std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::max_size(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [271]
                0.00    0.00       2/4           __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::max_size() const [255]
-----------------------------------------------
                0.00    0.00       2/2           std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_S_check_init_len(unsigned long, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [273]
[272]    0.0    0.00    0.00       2         std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_S_max_size(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [272]
                0.00    0.00       2/2           std::allocator_traits<std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::max_size(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [271]
                0.00    0.00       2/1523322     unsigned long const& std::min<unsigned long>(unsigned long const&, unsigned long const&) [152]
-----------------------------------------------
                0.00    0.00       2/2           void std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_range_initialize<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::forward_iterator_tag) [86]
[273]    0.0    0.00    0.00       2         std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_S_check_init_len(unsigned long, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [273]
                0.00    0.00       2/4           std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::allocator(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [257]
                0.00    0.00       2/2           std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_S_max_size(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [272]
                0.00    0.00       2/5           std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::~allocator() [251]
-----------------------------------------------
                0.00    0.00       1/2           CmdLineApp::CmdLineApp(int, char**) [317]
                0.00    0.00       1/2           StandardGraph::__restore_from_db(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >) [93]
[274]    0.0    0.00    0.00       2         std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_replace(unsigned long, unsigned long, char const*, unsigned long) [274]
                0.00    0.00       2/19          std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_mutate(unsigned long, unsigned long, char const*, unsigned long) [248]
-----------------------------------------------
                0.00    0.00       2/2           std::iterator_traits<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>::difference_type std::distance<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*) [277]
[275]    0.0    0.00    0.00       2         std::iterator_traits<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>::difference_type std::__distance<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::random_access_iterator_tag) [275]
-----------------------------------------------
                0.00    0.00       2/2           std::iterator_traits<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>::difference_type std::distance<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*) [277]
[276]    0.0    0.00    0.00       2         std::iterator_traits<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>::iterator_category std::__iterator_category<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const* const&) [276]
-----------------------------------------------
                0.00    0.00       2/2           void std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_range_initialize<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::forward_iterator_tag) [86]
[277]    0.0    0.00    0.00       2         std::iterator_traits<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>::difference_type std::distance<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*) [277]
                0.00    0.00       2/2           std::iterator_traits<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>::iterator_category std::__iterator_category<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const* const&) [276]
                0.00    0.00       2/2           std::iterator_traits<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>::difference_type std::__distance<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::random_access_iterator_tag) [275]
-----------------------------------------------
                0.00    0.00       1/1           __libc_csu_init [77]
[278]    0.0    0.00    0.00       1         _GLOBAL__sub_I__Z14print_csv_infoNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEiP8bfs_info [278]
-----------------------------------------------
                0.00    0.00       1/1           StandardGraph::__restore_from_db(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >) [93]
[279]    0.0    0.00    0.00       1         CommonUtil::open_session(__wt_connection*, __wt_session**) [279]
-----------------------------------------------
                0.00    0.00       1/1           StandardGraph::__restore_from_db(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >) [93]
[280]    0.0    0.00    0.00       1         CommonUtil::unpack_int_wt(char const*, __wt_session*) [280]
-----------------------------------------------
                0.00    0.00       1/1           StandardGraph::__restore_from_db(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >) [93]
[281]    0.0    0.00    0.00       1         CommonUtil::open_connection(char*, __wt_connection**) [281]
-----------------------------------------------
                0.00    0.00       1/1           StandardGraph::close() [83]
[282]    0.0    0.00    0.00       1         CommonUtil::close_connection(__wt_connection*) [282]
-----------------------------------------------
                0.00    0.00       1/1           StandardGraph::StandardGraph(graph_opts) [62]
[283]    0.0    0.00    0.00       1         CommonUtil::check_graph_params(graph_opts) [283]
                0.00    0.00       1/1           std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::vector() [304]
                0.00    0.00       1/1           std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::size() const [288]
                0.00    0.00       1/1           std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::~vector() [305]
-----------------------------------------------
                0.00    0.00       1/1           StandardGraph::StandardGraph(graph_opts) [62]
[284]    0.0    0.00    0.00       1         graph_opts::graph_opts(graph_opts const&) [284]
-----------------------------------------------
                0.00    0.00       1/1           StandardGraph::StandardGraph(graph_opts) [62]
[285]    0.0    0.00    0.00       1         graph_opts::~graph_opts() [285]
-----------------------------------------------
                0.00    0.00       1/1           CmdLineApp::CmdLineApp(int, char**) [317]
[286]    0.0    0.00    0.00       1         CmdLineBase::CmdLineBase(int, char**) [286]
                0.00    0.00       5/5           void std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_realloc_insert<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >(__gnu_cxx::__normal_iterator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > > >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&) [253]
-----------------------------------------------
                0.00    0.00       1/1           CmdLineApp::CmdLineApp(int, char**) [317]
[287]    0.0    0.00    0.00       1         std::ctype<char>::do_widen(char) const [287]
-----------------------------------------------
                0.00    0.00       1/1           CommonUtil::check_graph_params(graph_opts) [283]
[288]    0.0    0.00    0.00       1         std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::size() const [288]
-----------------------------------------------
                0.00    0.00       1/1           std::_Tuple_impl<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_head(std::_Tuple_impl<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [66]
[289]    0.0    0.00    0.00       1         std::_Head_base<0ul, std::filesystem::__cxx11::path::_List::_Impl*, false>::_M_head(std::_Head_base<0ul, std::filesystem::__cxx11::path::_List::_Impl*, false>&) [289]
-----------------------------------------------
                0.00    0.00       1/1           std::_Tuple_impl<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_head(std::_Tuple_impl<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [296]
[290]    0.0    0.00    0.00       1         std::_Head_base<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter, true>::_M_head(std::_Head_base<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter, true>&) [290]
-----------------------------------------------
                0.00    0.00       1/1           std::filesystem::exists(std::filesystem::file_status) [292]
[291]    0.0    0.00    0.00       1         std::filesystem::status_known(std::filesystem::file_status) [291]
                0.00    0.00       1/2           std::filesystem::file_status::type() const [264]
-----------------------------------------------
                0.00    0.00       1/1           std::filesystem::exists(std::filesystem::__cxx11::path const&) [293]
[292]    0.0    0.00    0.00       1         std::filesystem::exists(std::filesystem::file_status) [292]
                0.00    0.00       1/1           std::filesystem::status_known(std::filesystem::file_status) [291]
                0.00    0.00       1/2           std::filesystem::file_status::type() const [264]
-----------------------------------------------
                0.00    0.00       1/1           StandardGraph::StandardGraph(graph_opts) [62]
[293]    0.0    0.00    0.00       1         std::filesystem::exists(std::filesystem::__cxx11::path const&) [293]
                0.00    0.00       1/1           std::filesystem::exists(std::filesystem::file_status) [292]
-----------------------------------------------
                0.00    0.00       1/1           StandardGraph::StandardGraph(graph_opts) [62]
[294]    0.0    0.00    0.00       1         std::filesystem::__cxx11::path::path(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&, std::filesystem::__cxx11::path::format) [294]
                0.00    0.00       1/944804      std::remove_reference<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&>::type&& std::move<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&) [166]
-----------------------------------------------
                0.00    0.00       1/1           std::unique_ptr<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::~unique_ptr() [65]
[295]    0.0    0.00    0.00       1         std::unique_ptr<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::get_deleter() [295]
                0.00    0.00       1/1           std::__uniq_ptr_impl<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_deleter() [303]
-----------------------------------------------
                0.00    0.00       1/1           std::filesystem::__cxx11::path::_List::_Impl_deleter& std::__get_helper<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::_Tuple_impl<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [306]
[296]    0.0    0.00    0.00       1         std::_Tuple_impl<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_head(std::_Tuple_impl<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [296]
                0.00    0.00       1/1           std::_Head_base<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter, true>::_M_head(std::_Head_base<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter, true>&) [290]
-----------------------------------------------
                0.00    0.00       1/1           void std::_Destroy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*) [309]
[297]    0.0    0.00    0.00       1         void std::_Destroy_aux<false>::__destroy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*) [297]
-----------------------------------------------
                0.00    0.00       1/1           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_base() [301]
[298]    0.0    0.00    0.00       1         std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl::_Vector_impl() [298]
                0.00    0.00       1/3           std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::allocator() [261]
                0.00    0.00       1/3           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl_data::_Vector_impl_data() [262]
-----------------------------------------------
                0.00    0.00       1/1           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::~_Vector_base() [302]
[299]    0.0    0.00    0.00       1         std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl::~_Vector_impl() [299]
                0.00    0.00       1/5           std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::~allocator() [251]
-----------------------------------------------
                0.00    0.00       1/1           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::~_Vector_base() [302]
[300]    0.0    0.00    0.00       1         std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_deallocate(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, unsigned long) [300]
-----------------------------------------------
                0.00    0.00       1/1           std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::vector() [304]
[301]    0.0    0.00    0.00       1         std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_base() [301]
                0.00    0.00       1/1           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl::_Vector_impl() [298]
-----------------------------------------------
                0.00    0.00       1/1           std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::~vector() [305]
[302]    0.0    0.00    0.00       1         std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::~_Vector_base() [302]
                0.00    0.00       1/1           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_deallocate(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, unsigned long) [300]
                0.00    0.00       1/1           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl::~_Vector_impl() [299]
-----------------------------------------------
                0.00    0.00       1/1           std::unique_ptr<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::get_deleter() [295]
[303]    0.0    0.00    0.00       1         std::__uniq_ptr_impl<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_deleter() [303]
                0.00    0.00       1/1           std::tuple_element<1ul, std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter> >::type& std::get<1ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [307]
-----------------------------------------------
                0.00    0.00       1/1           CommonUtil::check_graph_params(graph_opts) [283]
[304]    0.0    0.00    0.00       1         std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::vector() [304]
                0.00    0.00       1/1           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_base() [301]
-----------------------------------------------
                0.00    0.00       1/1           CommonUtil::check_graph_params(graph_opts) [283]
[305]    0.0    0.00    0.00       1         std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::~vector() [305]
                0.00    0.00       1/5           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_get_Tp_allocator() [252]
                0.00    0.00       1/1           void std::_Destroy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >&) [310]
                0.00    0.00       1/1           std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::~_Vector_base() [302]
-----------------------------------------------
                0.00    0.00       1/1           std::tuple_element<1ul, std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter> >::type& std::get<1ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [307]
[306]    0.0    0.00    0.00       1         std::filesystem::__cxx11::path::_List::_Impl_deleter& std::__get_helper<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::_Tuple_impl<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [306]
                0.00    0.00       1/1           std::_Tuple_impl<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_head(std::_Tuple_impl<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [296]
-----------------------------------------------
                0.00    0.00       1/1           std::__uniq_ptr_impl<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_deleter() [303]
[307]    0.0    0.00    0.00       1         std::tuple_element<1ul, std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter> >::type& std::get<1ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [307]
                0.00    0.00       1/1           std::filesystem::__cxx11::path::_List::_Impl_deleter& std::__get_helper<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::_Tuple_impl<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [306]
-----------------------------------------------
                0.00    0.00       1/1           std::unique_ptr<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::~unique_ptr() [65]
[308]    0.0    0.00    0.00       1         std::remove_reference<std::filesystem::__cxx11::path::_List::_Impl*&>::type&& std::move<std::filesystem::__cxx11::path::_List::_Impl*&>(std::filesystem::__cxx11::path::_List::_Impl*&) [308]
-----------------------------------------------
                0.00    0.00       1/1           void std::_Destroy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >&) [310]
[309]    0.0    0.00    0.00       1         void std::_Destroy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*) [309]
                0.00    0.00       1/1           void std::_Destroy_aux<false>::__destroy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*) [297]
-----------------------------------------------
                0.00    0.00       1/1           std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::~vector() [305]
[310]    0.0    0.00    0.00       1         void std::_Destroy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >&) [310]
                0.00    0.00       1/1           void std::_Destroy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*) [309]
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

 [278] _GLOBAL__sub_I__Z14print_csv_infoNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEEiP8bfs_info (bfs.cpp) [63] std::filesystem::__cxx11::path::_List::~_List() [245] std::_Rb_tree<int, int, std::_Identity<int>, std::less<int>, std::allocator<int> >::_M_erase(std::_Rb_tree_node<int>*)
  [78] _GLOBAL__sub_I__Z8METADATAB5cxx11 (common.cpp) [294] std::filesystem::__cxx11::path::path(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&, std::filesystem::__cxx11::path::format) [84] void std::_Construct<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&)
  [95] _GLOBAL__sub_I__ZN13StandardGraphC2Ev (standard_graph.cpp) [64] std::filesystem::__cxx11::path::~path() [275] std::iterator_traits<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>::difference_type std::__distance<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::random_access_iterator_tag)
  [96] _GLOBAL__sub_I__ZN7AdjListC2Ev (adj_list.cpp) [295] std::unique_ptr<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::get_deleter() [240] std::iterator_traits<char const*>::difference_type std::__distance<char const*>(char const*, char const*, std::random_access_iterator_tag)
  [97] _GLOBAL__sub_I__ZN7EdgeKeyC2Ev (edgekey.cpp) [65] std::unique_ptr<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::~unique_ptr() [71] std::iterator_traits<char*>::difference_type std::__distance<char*>(char*, char*, std::random_access_iterator_tag)
   [3] bfs_info* bfs<StandardGraph>(StandardGraph&, int) [247] std::_Deque_base<int, std::allocator<int> >::_M_initialize_map(unsigned long) [139] node* std::__addressof<node>(node&)
  [98] __static_initialization_and_destruction_0(int, int) (adj_list.cpp) [66] std::_Tuple_impl<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_head(std::_Tuple_impl<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [258] std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::__addressof<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&)
  [79] __static_initialization_and_destruction_0(int, int) (common.cpp) [296] std::_Tuple_impl<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_head(std::_Tuple_impl<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>&) [68] std::filesystem::__cxx11::path::_List::_Impl*& std::__get_helper<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::_Tuple_impl<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&)
  [99] __static_initialization_and_destruction_0(int, int) (edgekey.cpp) [36] std::char_traits<char>::length(char const*) [306] std::filesystem::__cxx11::path::_List::_Impl_deleter& std::__get_helper<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::_Tuple_impl<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter>&)
 [100] __static_initialization_and_destruction_0(int, int) (standard_graph.cpp) [297] void std::_Destroy_aux<false>::__destroy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*) [28] edge* std::__niter_base<edge*>(edge*)
 [101] CommonUtil::pack_int_wt(int, __wt_session*) [206] void std::_Destroy_aux<true>::__destroy<edge*>(edge*, edge*) [145] node* std::__niter_base<node*>(node*)
 [243] CommonUtil::check_return(int, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >) [198] std::_Vector_base<edge, std::allocator<edge> >::_M_allocate(unsigned long) [35] edge* std::__relocate_a<edge*, edge*, std::allocator<edge> >(edge*, edge*, edge*, std::allocator<edge>&)
 [279] CommonUtil::open_session(__wt_connection*, __wt_session**) [225] std::_Vector_base<edge, std::allocator<edge> >::_Vector_impl::_Vector_impl(std::allocator<edge> const&) [179] node* std::__relocate_a<node*, node*, std::allocator<node> >(node*, node*, node*, std::allocator<node>&)
 [280] CommonUtil::unpack_int_wt(char const*, __wt_session*) [218] std::_Vector_base<edge, std::allocator<edge> >::_Vector_impl::_Vector_impl() [180] std::enable_if<std::__is_bitwise_relocatable<edge, void>::value, edge*>::type std::__relocate_a_1<edge, edge>(edge*, edge*, edge*, std::allocator<edge>&)
 [281] CommonUtil::open_connection(char*, __wt_connection**) [207] std::_Vector_base<edge, std::allocator<edge> >::_Vector_impl::~_Vector_impl() [181] node* std::__relocate_a_1<node*, node*, std::allocator<node> >(node*, node*, node*, std::allocator<node>&)
 [282] CommonUtil::close_connection(__wt_connection*) [183] std::_Vector_base<edge, std::allocator<edge> >::_M_deallocate(edge*, unsigned long) [232] void std::__alloc_on_move<std::allocator<edge> >(std::allocator<edge>&, std::allocator<edge>&)
 [283] CommonUtil::check_graph_params(graph_opts) [184] std::_Vector_base<edge, std::allocator<edge> >::_Vector_impl_data::_M_copy_data(std::_Vector_base<edge, std::allocator<edge> >::_Vector_impl_data const&) [233] void std::__do_alloc_on_move<std::allocator<edge> >(std::allocator<edge>&, std::allocator<edge>&, std::integral_constant<bool, true>)
 [284] graph_opts::graph_opts(graph_opts const&) [219] std::_Vector_base<edge, std::allocator<edge> >::_Vector_impl_data::_M_swap_data(std::_Vector_base<edge, std::allocator<edge> >::_Vector_impl_data&) [88] std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::uninitialized_copy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*)
 [285] graph_opts::~graph_opts() [185] std::_Vector_base<edge, std::allocator<edge> >::_Vector_impl_data::_Vector_impl_data() [276] std::iterator_traits<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>::iterator_category std::__iterator_category<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const* const&)
 [286] CmdLineBase::CmdLineBase(int, char**) [156] std::_Vector_base<edge, std::allocator<edge> >::_M_get_Tp_allocator() [241] std::iterator_traits<char const*>::iterator_category std::__iterator_category<char const*>(char const* const&)
  [14] StandardGraph::get_out_edges(int) [226] std::_Vector_base<edge, std::allocator<edge> >::_Vector_base(std::allocator<edge> const&) [211] std::iterator_traits<char*>::iterator_category std::__iterator_category<char*>(char* const&)
   [6] StandardGraph::get_out_nodes(int) [220] std::_Vector_base<edge, std::allocator<edge> >::_Vector_base() [150] void std::__relocate_object_a<node, node, std::allocator<node> >(node*, node*, std::allocator<node>&)
  [81] StandardGraph::get_out_degree(int) [208] std::_Vector_base<edge, std::allocator<edge> >::~_Vector_base() [89] std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::__uninitialized_copy_a<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >&)
  [91] StandardGraph::get_random_node() [199] std::_Vector_base<node, std::allocator<node> >::_M_allocate(unsigned long) [69] std::tuple_element<0ul, std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter> >::type& std::get<0ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&)
  [92] StandardGraph::insert_metadata(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, char*) [227] std::_Vector_base<node, std::allocator<node> >::_Vector_impl::_Vector_impl() [307] std::tuple_element<1ul, std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter> >::type& std::get<1ul, std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>(std::tuple<std::filesystem::__cxx11::path::_List::_Impl*, std::filesystem::__cxx11::path::_List::_Impl_deleter>&)
 [157] StandardGraph::__record_to_node(__wt_cursor*) [200] std::_Vector_base<node, std::allocator<node> >::_M_deallocate(node*, unsigned long) [182] unsigned long const& std::max<unsigned long>(unsigned long const&, unsigned long const&)
  [93] StandardGraph::__restore_from_db(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >) [228] std::_Vector_base<node, std::allocator<node> >::_Vector_impl_data::_Vector_impl_data() [152] unsigned long const& std::min<unsigned long>(unsigned long const&, unsigned long const&)
  [74] StandardGraph::_get_index_cursor(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, __wt_cursor**) [172] std::_Vector_base<node, std::allocator<node> >::_M_get_Tp_allocator() [151] std::remove_reference<node&>::type&& std::move<node&>(node&)
  [76] StandardGraph::_get_table_cursor(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, __wt_cursor**, bool) [229] std::_Vector_base<node, std::allocator<node> >::_Vector_base() [166] std::remove_reference<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&>::type&& std::move<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&)
 [155] StandardGraph::__read_from_edge_idx(__wt_cursor*, edge*) [267] std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_allocate(unsigned long) [308] std::remove_reference<std::filesystem::__cxx11::path::_List::_Impl*&>::type&& std::move<std::filesystem::__cxx11::path::_List::_Impl*&>(std::filesystem::__cxx11::path::_List::_Impl*&)
  [82] StandardGraph::get_node(int) [268] std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl::_Vector_impl(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [234] std::remove_reference<std::allocator<edge>&>::type&& std::move<std::allocator<edge>&>(std::allocator<edge>&)
  [90] StandardGraph::has_node(int) [298] std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl::_Vector_impl() [235] std::remove_reference<std::vector<edge, std::allocator<edge> >&>::type&& std::move<std::vector<edge, std::allocator<edge> >&>(std::vector<edge, std::allocator<edge> >&)
 [249] node::node()          [299] std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl::~_Vector_impl() [142] node&& std::forward<node>(std::remove_reference<node>::type&)
 [192] __gnu_cxx::new_allocator<edge>::deallocate(edge*, unsigned long) [300] std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_deallocate(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, unsigned long) [143] edge const& std::forward<edge const&>(std::remove_reference<edge const&>::type&)
 [193] __gnu_cxx::new_allocator<edge>::allocate(unsigned long, void const*) [262] std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_impl_data::_Vector_impl_data() [144] node const& std::forward<node const&>(std::remove_reference<node const&>::type&)
 [158] void __gnu_cxx::new_allocator<edge>::construct<edge, edge const&>(edge*, edge const&) [252] std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_get_Tp_allocator() [259] std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const& std::forward<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&>(std::remove_reference<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&>::type&)
 [214] __gnu_cxx::new_allocator<edge>::new_allocator(__gnu_cxx::new_allocator<edge> const&) [301] std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_base() [209] void std::_Destroy<edge*>(edge*, edge*)
 [215] __gnu_cxx::new_allocator<edge>::new_allocator() [269] std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_Vector_base(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [17] void std::_Destroy<edge*, edge>(edge*, edge*, std::allocator<edge>&)
 [189] __gnu_cxx::new_allocator<edge>::~new_allocator() [302] std::_Vector_base<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::~_Vector_base() [309] void std::_Destroy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*)
 [212] __gnu_cxx::new_allocator<node>::deallocate(node*, unsigned long) [303] std::__uniq_ptr_impl<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_deleter() [310] void std::_Destroy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >&)
 [146] void __gnu_cxx::new_allocator<node>::destroy<node>(node*) [67] std::__uniq_ptr_impl<std::filesystem::__cxx11::path::_List::_Impl, std::filesystem::__cxx11::path::_List::_Impl_deleter>::_M_ptr() [277] std::iterator_traits<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>::difference_type std::distance<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*)
 [194] __gnu_cxx::new_allocator<node>::allocate(unsigned long, void const*) [201] std::allocator_traits<std::allocator<edge> >::deallocate(std::allocator<edge>&, edge*, unsigned long) [242] std::iterator_traits<char const*>::difference_type std::distance<char const*>(char const*, char const*)
 [159] void __gnu_cxx::new_allocator<node>::construct<node, node const&>(node*, node const&) [202] std::allocator_traits<std::allocator<edge> >::allocate(std::allocator<edge>&, unsigned long) [72] std::iterator_traits<char*>::difference_type std::distance<char*>(char*, char*)
 [147] void __gnu_cxx::new_allocator<node>::construct<node, node>(node*, node&&) [173] std::allocator_traits<std::allocator<edge> >::max_size(std::allocator<edge> const&) [191] std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&, char const*)
 [222] __gnu_cxx::new_allocator<node>::new_allocator() [162] void std::allocator_traits<std::allocator<edge> >::construct<edge, edge const&>(std::allocator<edge>&, edge*, edge const&) [188] std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&)
 [263] __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::allocate(unsigned long, void const*) [213] std::allocator_traits<std::allocator<node> >::deallocate(std::allocator<node>&, node*, unsigned long) [59] std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(char const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&)
 [254] __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::new_allocator(__gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [148] void std::allocator_traits<std::allocator<node> >::destroy<node>(std::allocator<node>&, node*) [94] std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > std::operator+<char, std::char_traits<char>, std::allocator<char> >(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const&, char const*)
 [260] __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::new_allocator() [203] std::allocator_traits<std::allocator<node> >::allocate(std::allocator<node>&, unsigned long) [141] operator new(unsigned long, void*)
 [250] __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::~new_allocator() [174] std::allocator_traits<std::allocator<node> >::max_size(std::allocator<node> const&) [10] __curfile_search (cur_file.c)
 [236] bool __gnu_cxx::__is_null_pointer<char const>(char const*) [31] void std::allocator_traits<std::allocator<node> >::construct<node, node const&>(std::allocator<node>&, node*, node const&) [37] __curindex_compare (cur_index.c)
 [210] bool __gnu_cxx::__is_null_pointer<char>(char*) [149] void std::allocator_traits<std::allocator<node> >::construct<node, node>(std::allocator<node>&, node*, node&&) [18] __curindex_get_value (cur_index.c)
 [165] __gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > >::__normal_iterator(edge* const&) [270] std::allocator_traits<std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::allocate(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >&, unsigned long) [38] __curindex_search (cur_index.c)
 [160] __gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > >::operator++() [271] std::allocator_traits<std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::max_size(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [19] __cursor_func_init.constprop.0 (cursor.i)
 [168] __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >::__normal_iterator(node* const&) [85] std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >* std::__uninitialized_copy<false>::__uninit_copy<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*) [39] __cursor_row_slot_return (cursor.i)
 [195] __gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > >::difference_type __gnu_cxx::operator-<edge*, std::vector<edge, std::allocator<edge> > >(__gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > > const&, __gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > > const&) [246] void std::deque<int, std::allocator<int> >::_M_push_back_aux<int const&>(int const&) [40] __curtable_search (cur_table.c)
 [196] __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >::difference_type __gnu_cxx::operator-<node*, std::vector<node, std::allocator<node> > >(__gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > > const&, __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > > const&) [175] std::vector<edge, std::allocator<edge> >::_S_max_size(std::allocator<edge> const&) [2] __global_once (global.c)
  [30] bool __gnu_cxx::operator!=<edge*, std::vector<edge, std::allocator<edge> > >(__gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > > const&, __gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > > const&) [33] std::vector<edge, std::allocator<edge> >::_S_relocate(edge*, edge*, edge*, std::allocator<edge>&) [41] __pack_write (packing.i)
 [163] __gnu_cxx::new_allocator<edge>::max_size() const [60] std::vector<edge, std::allocator<edge> >::_M_move_assign(std::vector<edge, std::allocator<edge> >&&, std::integral_constant<bool, true>) [42] __schema_open_index (schema_open.c)
 [164] __gnu_cxx::new_allocator<node>::max_size() const [34] std::vector<edge, std::allocator<edge> >::_S_do_relocate(edge*, edge*, edge*, std::allocator<edge>&, std::integral_constant<bool, true>) [43] __session_find_dhandle (session_dhandle.c)
 [255] __gnu_cxx::new_allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::max_size() const [26] void std::vector<edge, std::allocator<edge> >::_M_realloc_insert<edge const&>(__gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > >, edge const&) [44] __unpack_read (packing.i)
 [140] __gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > >::base() const [186] std::vector<edge, std::allocator<edge> >::end() [45] __wt_btcur_close
 [161] __gnu_cxx::__normal_iterator<edge*, std::vector<edge, std::allocator<edge> > >::operator*() const [187] std::vector<edge, std::allocator<edge> >::begin() [20] __wt_btcur_search
 [153] __gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >::base() const [25] std::vector<edge, std::allocator<edge> >::push_back(edge const&) [46] __wt_btcur_search_near
 [264] std::filesystem::file_status::type() const [230] std::vector<edge, std::allocator<edge> >::vector(std::allocator<edge> const&) [47] __wt_buf_catfmt
 [223] std::_Vector_base<edge, std::allocator<edge> >::get_allocator() const [221] std::vector<edge, std::allocator<edge> >::vector() [21] __wt_clock_to_nsec
 [167] std::_Vector_base<edge, std::allocator<edge> >::_M_get_Tp_allocator() const [16] std::vector<edge, std::allocator<edge> >::~vector() [48] __wt_config_gets_def
 [169] std::_Vector_base<node, std::allocator<node> >::_M_get_Tp_allocator() const [61] std::vector<edge, std::allocator<edge> >::operator=(std::vector<edge, std::allocator<edge> >&&) [9] __wt_config_next
 [265] std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::end() const [176] std::vector<node, std::allocator<node> >::_S_max_size(std::allocator<node> const&) [22] __wt_curindex_open
 [266] std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::size() const [177] std::vector<node, std::allocator<node> >::_S_relocate(node*, node*, node*, std::allocator<node>&) [27] __wt_cursor_set_key
 [256] std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::begin() const [178] std::vector<node, std::allocator<node> >::_S_do_relocate(node*, node*, node*, std::allocator<node>&, std::integral_constant<bool, true>) [15] __wt_cursor_set_keyv
 [287] std::ctype<char>::do_widen(char) const [73] void std::vector<node, std::allocator<node> >::_M_realloc_insert<node const&>(__gnu_cxx::__normal_iterator<node*, std::vector<node, std::allocator<node> > >, node const&) [8] __wt_cursor_valid
  [58] std::vector<edge, std::allocator<edge> >::_M_check_len(unsigned long, char const*) const [204] std::vector<node, std::allocator<node> >::end() [11] __wt_curtable_open
  [29] std::vector<edge, std::allocator<edge> >::size() const [205] std::vector<node, std::allocator<node> >::begin() [49] __wt_curtable_set_key
 [170] std::vector<edge, std::allocator<edge> >::max_size() const [32] std::vector<node, std::allocator<node> >::push_back(node const&) [50] __wt_hash_city64
 [197] std::vector<node, std::allocator<node> >::_M_check_len(unsigned long, char const*) const [231] std::vector<node, std::allocator<node> >::vector() [1] __wt_hazard_clear
 [154] std::vector<node, std::allocator<node> >::size() const [272] std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_S_max_size(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [12] __wt_hazard_set
 [171] std::vector<node, std::allocator<node> >::max_size() const [253] void std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_realloc_insert<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >(__gnu_cxx::__normal_iterator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >*, std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > > >, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >&&) [23] __wt_page_in_func
 [288] std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::size() const [273] std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_S_check_init_len(unsigned long, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [51] __wt_readunlock
 [216] std::allocator<edge>::allocator(std::allocator<edge> const&) [86] void std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::_M_range_initialize<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*>(std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > const*, std::forward_iterator_tag) [4] __wt_row_search
 [217] std::allocator<edge>::allocator() [304] std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::vector() [52] __wt_schema_open_index
 [190] std::allocator<edge>::~allocator() [87] std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::vector(std::initializer_list<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [53] __wt_schema_open_table
 [224] std::allocator<node>::allocator() [305] std::vector<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >::~vector() [5] __wt_schema_project_out
 [257] std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::allocator(std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > const&) [244] void std::vector<int, std::allocator<int> >::_M_realloc_insert<int const&>(__gnu_cxx::__normal_iterator<int*, std::vector<int, std::allocator<int> > >, int const&) [13] __wt_schema_project_slice
 [261] std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::allocator() [274] std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_replace(unsigned long, unsigned long, char const*, unsigned long) [54] __wt_scr_alloc_func
 [251] std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >::~allocator() [237] void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char const*>(char const*, char const*) [24] __wt_session_copy_values
 [289] std::_Head_base<0ul, std::filesystem::__cxx11::path::_List::_Impl*, false>::_M_head(std::_Head_base<0ul, std::filesystem::__cxx11::path::_List::_Impl*, false>&) [238] void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char const*>(char const*, char const*, std::forward_iterator_tag) [55] __wt_session_gen_enter
 [290] std::_Head_base<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter, true>::_M_head(std::_Head_base<1ul, std::filesystem::__cxx11::path::_List::_Impl_deleter, true>&) [70] void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct<char*>(char*, char*, std::forward_iterator_tag) [7] __wt_session_gen_leave
 [291] std::filesystem::status_known(std::filesystem::file_status) [239] void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_construct_aux<char const*>(char const*, char const*, std::__false_type) [56] __wt_strndup
 [292] std::filesystem::exists(std::filesystem::file_status) [248] std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_mutate(unsigned long, unsigned long, char const*, unsigned long) [57] __wt_struct_plan
 [293] std::filesystem::exists(std::filesystem::__cxx11::path const&) [75] std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::basic_string<std::allocator<char> >(char const*, std::allocator<char> const&)
