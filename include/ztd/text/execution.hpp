// =============================================================================
//
// ztd.text
// Copyright © 2021 JeanHeyd "ThePhD" Meneide and Shepherd's Oasis, LLC
// Contact: opensource@soasis.org
//
// Commercial License Usage
// Licensees holding valid commercial ztd.text licenses may use this file in
// accordance with the commercial license agreement provided with the
// Software or, alternatively, in accordance with the terms contained in
// a written agreement between you and Shepherd's Oasis, LLC.
// For licensing terms and conditions see your agreement. For
// further information contact opensource@soasis.org.
//
// Apache License Version 2 Usage
// Alternatively, this file may be used under the terms of Apache License
// Version 2.0 (the "License") for non-commercial use; you may not use this
// file except in compliance with the License. You may obtain a copy of the
// License at
//
//		http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// ============================================================================>

#pragma once

#ifndef ZTD_TEXT_EXECUTION_HPP
#define ZTD_TEXT_EXECUTION_HPP

#include <ztd/text/version.hpp>

#include <ztd/text/encode_result.hpp>
#include <ztd/text/decode_result.hpp>
#include <ztd/text/encoding_error.hpp>
#include <ztd/text/error_handler.hpp>
#include <ztd/text/is_ignorable_error_handler.hpp>
#include <ztd/text/unicode_code_point.hpp>
#include <ztd/text/utf8.hpp>
#include <ztd/text/utf16.hpp>

#include <ztd/text/detail/type_traits.hpp>
#include <ztd/text/detail/range.hpp>
#include <ztd/text/detail/span.hpp>
#include <ztd/text/detail/progress_handler.hpp>
#include <ztd/text/detail/windows.hpp>
#include <ztd/text/detail/assert.hpp>

#include <cuchar>
#include <cwchar>
#include <cstdint>

namespace ztd { namespace text {
	ZTD_TEXT_INLINE_ABI_NAMESPACE_OPEN_I_

	//////
	/// @addtogroup ztd_text_encodings Encodings
	/// @{
	//////

	//////
	/// @brief The Encoding that represents the "Execution" (narrow locale-based) encoding. The execution encoding is
	/// typically associated with the locale, which is tied to the C standard library's setlocale function.
	///
	/// @remarks Use of this type is subject to the C Standard Library. Some locales (such as the Big5 Hong King
	/// Supplementary Character Set (Big5-HKSCS)) are broken when accessed without @c ZTD_TEXT_USE_CUNEICODE is not
	/// defined, due to fundamental design issues in the C Standard Library and bugs in glibc/musl libc's current
	/// locale encoding support.
	//////
	class execution {
	private:
		class __decode_state {
		public:
			::std::mbstate_t __narrow_state;
			bool __output_pending;

			__decode_state() noexcept : __narrow_state(), __output_pending(false) {
				char32_t __ghost_space[2];
				::std::size_t __init_result = ::std::mbrtoc32(__ghost_space, "\0", 1, &__narrow_state);
				// make sure it is initialized
				ZTD_TEXT_ASSERT_I_(__init_result == 0 && __ghost_space[0] == U'\0');
				ZTD_TEXT_ASSERT_I_(::std::mbsinit(&__narrow_state) != 0);
			}
		};

		class __encode_state {
		public:
			::std::mbstate_t __narrow_state;
			bool __output_pending;

			__encode_state() noexcept : __narrow_state(), __output_pending(false) {
				char __ghost_space[MB_LEN_MAX];
				::std::size_t __init_result = ::std::c32rtomb(__ghost_space, U'\0', &__narrow_state);
				// make sure it is initialized
				ZTD_TEXT_ASSERT_I_(__init_result == 1 && __ghost_space[0] == '\0');
				ZTD_TEXT_ASSERT_I_(::std::mbsinit(&__narrow_state) != 0);
			}
		};

	public:
		//////
		/// @brief The state of the execution encoding used between decode calls, which may potentially manage shift
		/// state.
		///
		/// @remarks This type can potentially have lots of state due to the way the C API is specified. It is
		/// important it is preserved between calls, or text may become mangled / data may become lost.
		//////
		using decode_state = __decode_state;

		//////
		/// @brief The state of the execution encoding used between encode calls, which may potentially manage shift
		/// state.
		///
		/// @remarks This type can potentially have lots of state due to the way the C API is specified. It is
		/// important it is preserved between calls, or text may become mangled / data may become lost.
		//////
		using encode_state = __encode_state;
		//////
		/// @brief The individual units that result from an encode operation or are used as input to a decode
		/// operation.
		///
		/// @remarks Please note that char can be either signed or unsigned, and so generally can result in bad
		/// results when promoted to a plain @c int when working with code units or working with the C Standard
		/// Library.
		//////
		using code_unit = char;
		//////
		/// @brief The individual units that result from a decode operation or as used as input to an encode
		/// operation. For most encodings, this is going to be a Unicode Code Point or a Unicode Scalar Value.
		//////
		using code_point = unicode_code_point;
		//////
		/// @brief Whether or not the decode operation can process all forms of input into code point values.
		///
		/// @remarks All known execution encodings can decode into Unicode just fine. However, someone may define a
		/// platform encoding on their machine that does not transform cleanly. Therefore, decoding is not marked as
		/// injective.
		//////
		using is_decode_injective = ::std::false_type;
		//////
		/// @brief Whether or not the encode operation can process all forms of input into code unit values. This is
		/// absolutely not true: many unicode code point values cannot be safely converted to a large number of
		/// existing (legacy) encodings.
		//////
		using is_encode_injective = ::std::false_type;
		//////
		/// @brief The maximum code units a single complete operation of encoding can produce.
		///
		/// @remarks There are encodings for which one input can produce 3 code points (some Tamil encodings) and
		/// there are rumours of an encoding that can produce 7 code points from a handful of input. We use a
		/// conservative "7", here.
		//////
		inline static constexpr ::std::size_t max_code_points = 7;
		//////
		/// @brief The maximum number of code points a single complete operation of decoding can produce.
		///
		/// @remarks This is bounded by the platform's @c MB_LEN_MAX macro, which is an integral constant expression
		/// representing the maximum value of output all C locales can produce from a single complete operation.
		//////
		inline static constexpr ::std::size_t max_code_units = MB_LEN_MAX;
		//////
		/// @brief A range of code unit values that can be used as a replacement value, instead of the ones used in
		/// @ref ztd::text::default_handler.
		///
		/// @remarks The default replacement code point / code unit is U+FFFD (). This, obviously, does not fit in the
		/// majority of the (legacy) locale encodings in C and C++. '?' is a much more conservative option, here, and
		/// most (all?) locale encodings have some form of representation for it.
		//////
		inline static constexpr code_unit replacement_code_units[1] = { '?' };

		//////
		/// @brief Encodes a single complete unit of information as code units and produces a result with the
		/// input and output ranges moved past what was successfully read and written; or, produces an error and
		/// returns the input and output ranges untouched.
		///
		/// @param[in] __input The input view to read code uunits from.
		/// @param[in] __output The output view to write code points into.
		/// @param[in] __error_handler The error handler to invoke if encoding fails.
		/// @param[in, out] __s The necessary state information. Most encodings have no state, but because this is
		/// effectively a runtime encoding and therefore it is important to preserve and manage this state.
		///
		/// @returns A @ref ztd::text::encode_result object that contains the reconstructed input range,
		/// reconstructed output range, error handler, and a reference to the passed-in state.
		///
		/// @remarks Platform APIs and/or the C Standard Library may be used to properly decode one complete unit of
		/// information (alongside std::mbstate_t usage). Whether or not the state is used is based on the
		/// implementation and what it chooses. If @c ZTD_TEXT_USE_CUNEICODE is defined, the ztd.cuneicode library may
		/// be used to fulfill this functionality.
		///
		/// @remarks To the best ability of the implementation, the iterators will be
		/// returned untouched (e.g., the input models at least a view and a forward_range). If it is not possible,
		/// returned ranges may be incremented even if an error occurs due to the semantics of any view that models an
		/// input_range.
		//////
		template <typename _InputRange, typename _OutputRange, typename _ErrorHandler>
		static constexpr auto encode_one(
			_InputRange&& __input, _OutputRange&& __output, _ErrorHandler&& __error_handler, encode_state& __s) {
			using _UInputRange   = __detail::__remove_cvref_t<_InputRange>;
			using _UOutputRange  = __detail::__remove_cvref_t<_OutputRange>;
			using _UErrorHandler = __detail::__remove_cvref_t<_ErrorHandler>;
			using _Result = __detail::__reconstruct_encode_result_t<_UInputRange, _UOutputRange, encode_state>;
			constexpr bool __call_error_handler = !is_ignorable_error_handler_v<_UErrorHandler>;

#if ZTD_TEXT_IS_ON(ZTD_TEXT_PLATFORM_WINDOWS_I_)
			if (__detail::__windows::__determine_active_code_page() == CP_UTF8) {
				// just go straight to UTF8
				using __exec_utf8 = __impl::__utf8_with<void, code_unit>;
				__exec_utf8 __u8enc {};
				encode_state_of_t<__exec_utf8> __intermediate_s {};
				__detail::__progress_handler<!__call_error_handler, execution> __intermediate_handler {};
				auto __intermediate_result = __u8enc.encode_one(::std::forward<_InputRange>(__input),
					::std::forward<_OutputRange>(__output), __intermediate_handler, __intermediate_s);
				if constexpr (__call_error_handler) {
					if (__intermediate_result.error_code != encoding_error::ok) {
						execution __self {};
						return __error_handler(__self,
							_Result(::std::move(__intermediate_result.input),
							     ::std::move(__intermediate_result.output), __s,
							     __intermediate_result.error_code),
							::std::span<code_point>(__intermediate_handler._M_code_points.data(),
							     __intermediate_handler._M_code_points_size));
					}
				}
				return _Result(::std::move(__intermediate_result.input), ::std::move(__intermediate_result.output),
					__s, __intermediate_result.error_code);
			}
#endif // Windows Hell

#if ZTD_TEXT_IS_ON(ZTD_TEXT_PLATFORM_WINDOWS_I_)
			auto __outit   = __detail::__adl::__adl_begin(__output);
			auto __outlast = __detail::__adl::__adl_end(__output);

			if constexpr (__call_error_handler) {
				if (__outit == __outlast) {
					return __error_handler(execution {},
						_Result(::std::forward<_InputRange>(__input), ::std::forward<_OutputRange>(__output), __s,
						     encoding_error::insufficient_output_space),
						::std::span<code_point, 0>());
				}
			}

			using __u16e               = __impl::__utf16_with<void, wchar_t, false>;
			using __intermediate_state = typename __u16e::state;

			__u16e __u16enc {};
			__intermediate_state __intermediate_s {};
			__detail::__progress_handler<!__call_error_handler, execution> __intermediate_handler {};
			wchar_t __wide_intermediary[8] {};
			::std::span<wchar_t> __wide_write_buffer(__wide_intermediary);
			auto __intermediate_result = __u16enc.encode_one(::std::forward<_InputRange>(__input),
				__wide_write_buffer, __intermediate_handler, __intermediate_s);
			if constexpr (__call_error_handler) {
				if (__intermediate_result.error_code != encoding_error::ok) {
					execution __self {};
					return __error_handler(__self,
						_Result(::std::move(__intermediate_result.input), ::std::forward<_OutputRange>(__output),
						     __s, __intermediate_result.error_code),
						::std::span<code_point>(__intermediate_handler._M_code_points.data(),
						     __intermediate_handler._M_code_points_size));
				}
			}
			constexpr const ::std::size_t __state_count_max = 12;
			code_unit __intermediary_output[__state_count_max] {};
			int __used_default_char = false;
			::std::span<const wchar_t> __wide_read_buffer(__wide_intermediary, __intermediate_result.output.data());
			int __res = ::WideCharToMultiByte(__detail::__windows::__determine_active_code_page(),
				WC_ERR_INVALID_CHARS, __wide_read_buffer.data(), static_cast<int>(__wide_read_buffer.size()),
				__intermediary_output, __state_count_max, ::std::addressof(replacement_code_units[0]),
				::std::addressof(__used_default_char));
			if constexpr (__call_error_handler) {
				if (__res == 0) {
					execution __self {};
					return __error_handler(__self,
						_Result(::std::move(__intermediate_result.input), ::std::forward<_OutputRange>(__output),
						     __s,
						     ::GetLastError() == ERROR_INSUFFICIENT_BUFFER
						          ? encoding_error::insufficient_output_space
						          : encoding_error::invalid_sequence),
						::std::span<code_point>(__intermediate_handler._M_code_points.data(),
						     __intermediate_handler._M_code_points_size));
				}
			}
			for (auto __intermediary_it = __intermediary_output; __res-- > 0;) {
				if constexpr (__call_error_handler) {
					if (__outit == __outlast) {
						execution __self {};
						return __error_handler(__self,
							_Result(__detail::__reconstruct(::std::in_place_type<_UInputRange>,
							             __detail::__adl::__adl_begin(__intermediate_result.input),
							             __detail::__adl::__adl_end(__intermediate_result.input)),
							     __detail::__reconstruct(
							          ::std::in_place_type<_UOutputRange>, __outit, __outlast),
							     __s, encoding_error::insufficient_output_space),
							::std::span<code_point>(__intermediate_handler._M_code_points.data(),
							     __intermediate_handler._M_code_points_size));
					}
				}
				__detail::__dereference(__outit) = __detail::__dereference(__intermediary_it);
				__outit                          = __detail::__next(__outit);
			}
			return _Result(::std::move(__intermediate_result.input),
				__detail::__reconstruct(::std::in_place_type<_UOutputRange>, __outit, __outlast), __s,
				__intermediate_result.error_code);
#else

			auto __init   = __detail::__adl::__adl_cbegin(__input);
			auto __inlast = __detail::__adl::__adl_cend(__input);

			if (__init == __inlast) {
				// an exhausted sequence is fine
				return _Result(::std::forward<_InputRange>(__input), ::std::forward<_OutputRange>(__output), __s,
					encoding_error::ok);
			}

			auto __outit   = __detail::__adl::__adl_begin(__output);
			auto __outlast = __detail::__adl::__adl_end(__output);

			if constexpr (__call_error_handler) {
				if (__outit == __outlast) {
					execution __self {};
					return __error_handler(__self,
						_Result(::std::forward<_InputRange>(__input), ::std::forward<_OutputRange>(__output), __s,
						     encoding_error::insufficient_output_space),
						::std::span<code_point, 0>());
				}
			}

			code_point __codepoint = __detail::__dereference(__init);
			__init                 = __detail::__next(__init);
			code_unit __intermediary_output[MB_LEN_MAX] {};
			::std::size_t __res
				= ::std::c32rtomb(__intermediary_output, __codepoint, ::std::addressof(__s.__narrow_state));
			if constexpr (__call_error_handler) {
				if (__res == static_cast<::std::size_t>(-1)) {
					execution __self {};
					return __error_handler(__self,
						_Result(__detail::__reconstruct(::std::in_place_type<_UInputRange>, __init, __inlast),
						     __detail::__reconstruct(::std::in_place_type<_UOutputRange>, __outit, __outlast),
						     __s, encoding_error::invalid_sequence),
						::std::span<code_point, 1>(&__codepoint, 1));
				}
			}

			for (auto __intermediary_it = __intermediary_output; __res-- > 0; ++__intermediary_it) {
				if constexpr (__call_error_handler) {
					if (__outit == __outlast) {
						execution __self {};
						return __error_handler(__self,
							_Result(
							     __detail::__reconstruct(::std::in_place_type<_UInputRange>, __init, __inlast),
							     __detail::__reconstruct(
							          ::std::in_place_type<_UOutputRange>, __outit, __outlast),
							     __s, encoding_error::insufficient_output_space),
							::std::span<code_point, 1>(&__codepoint, 1));
					}
				}
				__detail::__dereference(__outit) = __detail::__dereference(__intermediary_it);
				__outit                          = __detail::__next(__outit);
			}

			return _Result(__detail::__reconstruct(::std::in_place_type<_UInputRange>, __init, __inlast),
				__detail::__reconstruct(::std::in_place_type<_UOutputRange>, __outit, __outlast), __s,
				encoding_error::ok);
#endif // Windows is hell
		}

		//////
		/// @brief Decodes a single complete unit of information as code points and produces a result with the
		/// input and output ranges moved past what was successfully read and written; or, produces an error and
		/// returns the input and output ranges untouched.
		///
		/// @param[in] __input The input view to read code uunits from.
		/// @param[in] __output The output view to write code points into.
		/// @param[in] __error_handler The error handler to invoke if encoding fails.
		/// @param[in, out] __s The necessary state information. Most encodings have no state, but because this is
		/// effectively a runtime encoding and therefore it is important to preserve and manage this state.
		///
		/// @returns A @ref ztd::text::decode_result object that contains the reconstructed input range,
		/// reconstructed output range, error handler, and a reference to the passed-in state.
		///
		/// @remarks Platform APIs and/or the C Standard Library may be used to properly decode one complete unit of
		/// information (alongside std::mbstate_t usage). Whether or not the state is used is based on the
		/// implementation and what it chooses. If @c ZTD_TEXT_USE_CUNEICODE is defined, the ztd.cuneicode library may
		/// be used to fulfill this functionality.
		///
		/// @remarks To the best ability of the implementation, the iterators will be
		/// returned untouched (e.g., the input models at least a view and a forward_range). If it is not possible,
		/// returned ranges may be incremented even if an error occurs due to the semantics of any view that models an
		/// input_range.
		//////
		template <typename _InputRange, typename _OutputRange, typename _ErrorHandler>
		static constexpr auto decode_one(
			_InputRange&& __input, _OutputRange&& __output, _ErrorHandler&& __error_handler, decode_state& __s) {
			using _UInputRange   = __detail::__remove_cvref_t<_InputRange>;
			using _UOutputRange  = __detail::__remove_cvref_t<_OutputRange>;
			using _UErrorHandler = __detail::__remove_cvref_t<_ErrorHandler>;
			using _Result = __detail::__reconstruct_decode_result_t<_UInputRange, _UOutputRange, decode_state>;
			constexpr bool __call_error_handler = !is_ignorable_error_handler_v<_UErrorHandler>;

#if ZTD_TEXT_IS_ON(ZTD_TEXT_PLATFORM_WINDOWS_I_)
			if (__detail::__windows::__determine_active_code_page() == CP_UTF8) {
				// just use utf8 directly
				// just go straight to UTF8
				using __char_utf8 = __impl::__utf8_with<void, code_unit>;
				__char_utf8 __u8enc {};
				decode_state_of_t<__char_utf8> __intermediate_s {};
				__detail::__progress_handler<!__call_error_handler, execution> __intermediate_handler {};
				auto __intermediate_result = __u8enc.decode_one(::std::forward<_InputRange>(__input),
					::std::forward<_OutputRange>(__output), __intermediate_handler, __intermediate_s);
				if constexpr (__call_error_handler) {
					if (__intermediate_result.error_code != encoding_error::ok) {
						execution __self {};
						return __error_handler(__self,
							_Result(::std::move(__intermediate_result.input),
							     ::std::move(__intermediate_result.output), __s,
							     __intermediate_result.error_code),
							::std::span<code_unit>(__intermediate_handler._M_code_units.data(),
							     __intermediate_handler._M_code_units_size));
					}
				}
				return _Result(::std::move(__intermediate_result.input), ::std::move(__intermediate_result.output),
					__s, __intermediate_result.error_code);
			}
#endif

			auto __init   = __detail::__adl::__adl_cbegin(__input);
			auto __inlast = __detail::__adl::__adl_cend(__input);

			if (__init == __inlast) {
				// an exhausted sequence is fine
				return _Result(::std::forward<_InputRange>(__input), ::std::forward<_OutputRange>(__output), __s,
					encoding_error::ok);
			}

			auto __outit   = __detail::__adl::__adl_begin(__output);
			auto __outlast = __detail::__adl::__adl_end(__output);

			if constexpr (__call_error_handler) {
				if (__outit == __outlast) {
					execution __self {};
					return __error_handler(__self,
						_Result(::std::forward<_InputRange>(__input), ::std::forward<_OutputRange>(__output), __s,
						     encoding_error::insufficient_output_space),
						::std::span<code_unit, 0>());
				}
			}

			code_unit __intermediary_input[max_code_units] {};
#if ZTD_TEXT_IS_ON(ZTD_TEXT_PLATFORM_WINDOWS_I_) && (ZTD_TEXT_PLATFORM_MINGW == 0)
			__intermediary_input[0]     = __detail::__dereference(__init);
			__init                      = __detail::__next(__init);
			::std::size_t __state_count = 1;
			for (; __state_count < max_code_units; ++__state_count) {
				using __u16e               = __impl::__utf16_with<void, wchar_t, false>;
				using __intermediate_state = typename __u16e::state;

				constexpr const int __wide_intermediary_size = 4;
				wchar_t __wide_intermediary[__wide_intermediary_size] {};
				int __res = ::MultiByteToWideChar(__detail::__windows::__determine_active_code_page(),
					MB_ERR_INVALID_CHARS, __intermediary_input, static_cast<int>(__state_count),
					__wide_intermediary, __wide_intermediary_size);
				if (__res == 0) {
					if (::GetLastError() == ERROR_NO_UNICODE_TRANSLATION) {
						// loopback; we might just not have enough code units
						if constexpr (__call_error_handler) {
							if (__init == __inlast) {
								execution __self {};
								return __error_handler(__self,
									_Result(__detail::__reconstruct(
									             ::std::in_place_type<_UInputRange>, __init, __inlast),
									     __detail::__reconstruct(
									          ::std::in_place_type<_UOutputRange>, __outit, __outlast),
									     __s, encoding_error::incomplete_sequence),
									::std::span<code_unit>(__intermediary_input, __state_count));
							}
						}
						__intermediary_input[__state_count] = __detail::__dereference(__init);
						__init                              = __detail::__next(__init);
						continue;
					}
					if constexpr (__call_error_handler) {
						execution __self {};
						return __error_handler(__self,
							_Result(
							     __detail::__reconstruct(::std::in_place_type<_UInputRange>, __init, __inlast),
							     __detail::__reconstruct(
							          ::std::in_place_type<_UOutputRange>, __outit, __outlast),
							     __s, encoding_error::invalid_sequence),
							::std::span<code_unit>(__intermediary_input, __state_count));
					}
				}

				__u16e __u16enc {};
				__intermediate_state __intermediate_s {};
				__detail::__pass_through_handler_with<!__call_error_handler> __intermediate_handler {};
				auto __intermediate_result
					= __u16enc.encode_one(wc_string_view(__wide_intermediary, static_cast<::std::size_t>(__res)),
					     ::std::forward<_OutputRange>(__output), __intermediate_handler, __intermediate_s);
				if constexpr (__call_error_handler) {
					if (__intermediate_result.error_code != encoding_error::ok) {
						return __error_handler(execution {},
							_Result(
							     __detail::__reconstruct(::std::in_place_type<_UInputRange>, __init, __inlast),
							     ::std::move(__intermediate_result.output), __s,
							     __intermediate_result.error_code),
							::std::span<code_unit>(__intermediary_input, __state_count));
					}
				}
				return _Result(__detail::__reconstruct(::std::in_place_type<_UInputRange>, __init, __inlast),
					::std::move(__intermediate_result.output), __s, __intermediate_result.error_code);
#else
			if (__s.__output_pending) {
				// need to drain potential mbstate_t of any leftover code points?
				char32_t __intermediary_output[max_code_points] {};
				::std::size_t __res = ::std::mbrtoc32(
					::std::addressof(__intermediary_output[0]), nullptr, 0, ::std::addressof(__s.__narrow_state));
				if constexpr (__call_error_handler) {
					if (__res == static_cast<::std::size_t>(-1)) {
						execution __self {};
						return __error_handler(__self,
							_Result(
							     __detail::__reconstruct(::std::in_place_type<_UInputRange>, __init, __inlast),
							     __detail::__reconstruct(
							          ::std::in_place_type<_UOutputRange>, __outit, __outlast),
							     __s, encoding_error::invalid_sequence),
							::std::span<code_unit, 0>());
					}
				}
				__detail::__dereference(__outit) = __intermediary_output[0];
				__outit                          = __detail::__next(__outit);
				__s.__output_pending             = __res == static_cast<::std::size_t>(-3);
				return _Result(__detail::__reconstruct(::std::in_place_type<_UInputRange>, __init, __inlast),
					__detail::__reconstruct(::std::in_place_type<_UOutputRange>, __outit, __outlast), __s,
					encoding_error::ok);
			}

			::std::size_t __state_offset = 0;
			::std::size_t __state_count  = 1;
			for (; __state_offset < max_code_units; (void)++__state_offset, (void)++__state_count) {
				::std::mbstate_t __preserved_state   = __s.__narrow_state;
				__intermediary_input[__state_offset] = __detail::__dereference(__init);
				__init                               = __detail::__next(__init);
				char32_t __intermediary_output[1] {};
				::std::size_t __res = ::std::mbrtoc32(::std::addressof(__intermediary_output[0]),
					::std::addressof(__intermediary_input[0]), __state_count, ::std::addressof(__preserved_state));

				switch (__res) {
				case static_cast<::std::size_t>(-2):
					// cycle around and continue
					if constexpr (__call_error_handler) {
						if (__init == __inlast) {
							execution __self {};
							return __error_handler(__self,
								_Result(__detail::__reconstruct(
								             ::std::in_place_type<_UInputRange>, __init, __inlast),
								     __detail::__reconstruct(
								          ::std::in_place_type<_UOutputRange>, __outit, __outlast),
								     __s, encoding_error::incomplete_sequence),
								::std::span<code_unit>(__intermediary_input, __state_count));
						}
					}
					break;
				case static_cast<::std::size_t>(-3):
					__detail::__dereference(__outit) = __intermediary_output[0];
					__outit                          = __detail::__next(__outit);
					__s.__narrow_state               = __preserved_state;
					__s.__output_pending             = true;
					__state_offset                   = __state_count;
					return _Result(__detail::__reconstruct(::std::in_place_type<_UInputRange>, __init, __inlast),
						__detail::__reconstruct(::std::in_place_type<_UOutputRange>, __outit, __outlast), __s,
						encoding_error::ok);
				case static_cast<::std::size_t>(-1):
					if constexpr (__call_error_handler) {
						// OH GOD PANIC AAAAAAH
						// seriously we're out of spec here:
						// everything has gone to shit
						// even the __narrow_state is unspecified ;;
						execution __self {};
						return __error_handler(__self,
							_Result(
							     __detail::__reconstruct(::std::in_place_type<_UInputRange>, __init, __inlast),
							     __detail::__reconstruct(
							          ::std::in_place_type<_UOutputRange>, __outit, __outlast),
							     __s, encoding_error::invalid_sequence),
							::std::span<code_unit>(::std::addressof(__intermediary_input[0]), __state_count));
					}
					else {
						break;
					}
				case static_cast<::std::size_t>(0):
					// 0 means null character; ok
					return _Result(__detail::__reconstruct(::std::in_place_type<_UInputRange>, __init, __inlast),
						__detail::__reconstruct(::std::in_place_type<_UOutputRange>, __outit, __outlast), __s,
						encoding_error::ok);
				default:
					__detail::__dereference(__outit) = __intermediary_output[0];
					__outit                          = __detail::__next(__outit);
					__s.__narrow_state               = __preserved_state;
					return _Result(__detail::__reconstruct(::std::in_place_type<_UInputRange>, __init, __inlast),
						__detail::__reconstruct(::std::in_place_type<_UOutputRange>, __outit, __outlast), __s,
						encoding_error::ok);
				}
#endif
			}
			if constexpr (__call_error_handler) {
				// if it was invalid, we would have caught it before
				// this is for incomplete sequences only
				execution __self {};
				return __error_handler(__self,
					_Result(__detail::__reconstruct(::std::in_place_type<_UInputRange>, __init, __inlast),
					     __detail::__reconstruct(::std::in_place_type<_UOutputRange>, __outit, __outlast), __s,
					     encoding_error::incomplete_sequence),
					::std::span<code_unit>(::std::addressof(__intermediary_input[0]), __state_count));
			}
			else {
				// ... I mean.
				// You asked for it???
				_Result(__detail::__reconstruct(::std::in_place_type<_UInputRange>, __init, __inlast),
					__detail::__reconstruct(::std::in_place_type<_UOutputRange>, __outit, __outlast), __s,
					encoding_error::ok);
			}
		}
	};

	//////
	/// @}
	//////

	ZTD_TEXT_INLINE_ABI_NAMESPACE_CLOSE_I_
}} // namespace ztd::text

#endif // ZTD_TEXT_EXECUTION_HPP