/*
 * COPYRIGHT (C) 2017-2019, zhllxt
 *
 * author   : zhllxt
 * email    : 37792738@qq.com
 *
 * Distributed under the GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
 * (See accompanying file LICENSE or see <http://www.gnu.org/licenses/>)
 */

#ifndef __ASIO2_RPC_PROTOCOL_HPP__
#define __ASIO2_RPC_PROTOCOL_HPP__

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
#pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include <string>
#include <string_view>

#include <asio2/rpc/detail/serialization.hpp>

namespace asio2::detail
{
	/*
	 * request  : message type + request id + function name + parameters value...
	 * response : message type + request id + function name + error code + result value
	 *
	 * message type : q - request, p - response
	 *
	 * if result type is void, then result type will wrapped to std::int8_t
	 */

	static constexpr char rpc_type_req = 'q';
	static constexpr char rpc_type_rep = 'p';

	class header
	{
	public:
		using id_type = std::uint64_t;

		header() {}
		header(char type, id_type id, std::string_view name)
			: type_(type), id_(id), name_(name) {}
		~header() = default;

		header(const header& r) : type_(r.type_), id_(r.id_), name_(r.name_) {}
		header(header&& r) : type_(r.type_), id_(r.id_), name_(std::move(r.name_)) {}

		inline header& operator=(const header& r)
		{
			type_ = r.type_;
			id_ = r.id_;
			name_ = r.name_;
			return (*this);
		}
		inline header& operator=(header&& r)
		{
			type_ = r.type_;
			id_ = r.id_;
			name_ = std::move(r.name_);
			return (*this);
		}

		template <class Archive>
		inline void serialize(Archive & ar)
		{
			ar(type_, id_, name_);
		}

		inline const char         type() const { return this->type_; }
		inline const id_type      id()   const { return this->id_;   }
		inline const std::string& name() const { return this->name_; }

		inline bool is_request()  { return this->type_ == rpc_type_req; }
		inline bool is_response() { return this->type_ == rpc_type_rep; }

		inline header& type(char type            ) { this->type_ = type; return (*this); }
		inline header& id  (id_type id           ) { this->id_   = id  ; return (*this); }
		inline header& name(std::string_view name) { this->name_ = name; return (*this); }

	protected:
		char           type_;
		id_type        id_ = 0;
		std::string    name_;
	};

	template<class ...Args>
	class request : public header
	{
	public:
		request() : header() { this->type_ = rpc_type_req; }
		request(id_type id, std::string_view name, Args&&... args)
			: header(rpc_type_req, id, name), tp_(std::forward_as_tuple(std::forward<Args>(args)...)) {}
		~request() = default;

		request(const request& r) : header(r), tp_(r.tp_) {}
		request(request&& r) : header(std::move(r)), tp_(std::move(r.tp_)) {}

		inline request& operator=(const request& r)
		{
			static_cast<header&>(*this) = r;
			tp_ = r.tp_;
			return (*this);
		}
		inline request& operator=(request&& r)
		{
			static_cast<header&>(*this) = std::move(r);
			tp_ = std::move(r.tp_);
			return (*this);
		}

		template <class Archive>
		void serialize(Archive & ar)
		{
			ar(cereal::base_class<header>(this));
			ar(tp_);
		}

	protected:
		std::tuple<Args...> tp_;
	};

	template<class T>
	class response : public header
	{
	public:
		response() : header() { this->type_ = rpc_type_rep; }
		response(id_type id, std::string_view name) : header(rpc_type_rep, id, name) {}
		response(id_type id, std::string_view name, const error_code& ec, T&& ret)
			: header(rpc_type_rep, id, name), ec_(ec), ret_(std::forward<T>(ret)) {}
		~response() = default;

		response(const response& r) : header(r), ec_(r.ec_), ret_(r.ret_) {}
		response(response&& r) : header(std::move(r)), ec_(std::move(r.ec_)), ret_(std::move(r.ret_)) {}

		inline response& operator=(const response& r)
		{
			static_cast<header&>(*this) = r;
			ec_ = r.ec_;
			ret_ = r.ret_;
			return (*this);
		}
		inline response& operator=(response&& r)
		{
			static_cast<header&>(*this) = std::move(r);
			ec_ = std::move(r.ec_);
			ret_ = std::move(r.ret_);
			return (*this);
		}

		template <class Archive>
		void serialize(Archive & ar)
		{
			ar(cereal::base_class<header>(this));
			ar(ec_.value());
			ar(ret_);
		}

	protected:
		error_code ec_;
		T ret_;
	};
}

#endif // !__ASIO2_RPC_PROTOCOL_HPP__
