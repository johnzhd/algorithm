#pragma once

#include <assert.h>

#include <algorithm>
#include <map>

#include <list>

#include <memory>

namespace de
{


	template<typename T, T default_null, typename K = wchar_t>
	class match_node : public std::enable_shared_from_this<match_node<T,default_null,K>>
	{
	public:
		typedef T ret_key_type;
		typedef K pattern_type;
		typedef std::shared_ptr<match_node> shared_opt;
		typedef std::weak_ptr<match_node> weak_opt;

		static const ret_key_type d_null = default_null;
	public:
		match_node():b_tail(false),
			fail_node(),
			result_key(default_null){};
		~match_node(){};
	public:
		bool b_tail;
		T result_key;
		weak_opt fail_node;
		std::map<K, shared_opt> next;
	public:
		void clear()
		{
			b_tail = false;
			result_key = d_null;
			fail_node.reset();
			next.clear();
		}
	public:
		inline weak_opt set_next(K pattern)
		{
			auto it = next.find(pattern);
			if ( it == next.end() )
			{
				auto ret = next.insert(std::make_pair(pattern, new_node()));
				if ( ret.second == false || ret.first == next.end() )
					return weak_opt();

				it = ret.first;
			}
			if ( !it->second )
			{
				it->second = new_node();
			}
			return it->second;
		}


		inline weak_opt get_next(K pattern)
		{
			auto it = next.find(pattern);
			if ( it == next.end() )
				return weak_opt();
			return it->second;
		}

		inline weak_opt get_fail(K pattern)
		{
			return fail_node;
		}

		inline weak_opt find_next(weak_opt base, K pattern)
		{
			assert(!base.expired());

			weak_opt ret;
			while( !base.expired() )
			{
				ret = base.lock()->get_next(pattern);
				if ( !ret.expired() )
					break;

				base = base.lock()->get_fail(pattern);
			}

			return ret;
		}

		inline bool is_tail(T& ret_key)
		{
			if (b_tail)
				ret_key = result_key;
			return b_tail;
		}
	public:
		static shared_opt new_node()
		{
			return std::make_shared<match_node>();
		};

	};



	template<typename T>
	class ac_match_api
	{
	public:
		ac_match_api(){};
		~ac_match_api(){root = nullptr;};
	protected:
		typename T::shared_opt root;
	public:
		bool push_init()
		{
			if ( !root )
				root = typename T::new_node();
			if ( !root )
				return false;

			root->clear();
			return true;
		}

		bool push_pattern(typename T::ret_key_type key, typename T::pattern_type * value, size_t value_size)
		{
			if ( !root || !value || !value_size )
				return false;

			typename T::shared_opt p;
			p = root;

			for ( size_t i = 0; i < value_size; i++ )
			{
				if ( !p )
					return false;
				p = p->set_next(value[i]).lock();
			}

			if ( p )
			{
				p->b_tail = true;
				p->result_key = key;
				return true;
			}

			return false;
		}

		bool push_end()
		{
			if ( !root )
				return false;

			std::pair<typename T::shared_opt, typename T::weak_opt> work_pair;
			typename T::shared_opt work_opt;
			typename T::shared_opt child_opt;
			typename T::weak_opt could_be_faild_opt;
			std::list< decltype(work_pair) > queue;



			for ( auto it = root->next.begin(); it != root->next.end(); it++ )
			{
				it->second->fail_node.reset();
				queue.push_back(std::make_pair(it->second, it->second->fail_node));
			}

			while ( !queue.empty() )
			{
				work_pair = queue.front();
				queue.pop_front();
				work_opt = work_pair.first;
				could_be_faild_opt = work_pair.second;

				assert(work_opt);

				for ( auto it = work_opt->next.begin(); it != work_opt->next.end(); it++ )
				{
					child_opt = it->second;
					if ( !child_opt )
						continue;

					while ( false == could_be_faild_opt.expired() )
					{
						child_opt->fail_node = could_be_faild_opt.lock()->get_next(it->first);
						if ( false == child_opt->fail_node.expired() )
							break;

						could_be_faild_opt = could_be_faild_opt.lock()->get_fail(it->first);
					}

					if ( true == child_opt->fail_node.expired() )
						child_opt->fail_node = root->get_next(it->first);

					queue.push_back(std::make_pair(child_opt, child_opt->fail_node));
				}
			}

			return true;

		}

		bool search(std::list<std::pair<size_t, typename T::ret_key_type>> & ret_list,
			typename T::pattern_type * match, size_t match_size )
		{
			if ( !match || !match_size )
				return false;

			typename T::ret_key_type ret_key;
			typename T::shared_opt p;
			typename T::weak_opt p_next;

			p = root;
			for( size_t i = 0; i< match_size; i++ )
			{

				p_next = p->find_next(p, match[i]);
				if ( p_next.expired() )
				{
					if ( p != root )
					{
						p_next = p->find_next(root, match[i]);
					}
				}

				if ( p_next.expired() )
				{
					p = root;
					continue;
				}
				// else 
				p = p_next.lock();


				if ( true == p->is_tail(ret_key) )
				{
					ret_list.push_back(std::make_pair(i, ret_key));
				}
			}

			return !ret_list.empty();
		}


	};






};



