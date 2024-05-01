#pragma once
#include "../pch.h"

#include "File.hpp"

namespace gstd {
	//*******************************************************************
	//Compressor
	//*******************************************************************
	class CompressorStream {
	public:
		using in_stream_t = std::basic_istream<char, std::char_traits<char>>;
		using out_stream_t = std::basic_ostream<char, std::char_traits<char>>;
	private:
		static constexpr const size_t BASIC_CHUNK = 1U << 12;
	private:
		/// <typeparam name="_FnRead"></typeparam>
		/// <typeparam name="_FnCheck"></typeparam>
		/// <typeparam name="_FnAdvance"></typeparam>
		/// <typeparam name="_FnWrite"></typeparam>
		/// <typeparam name="CHUNK"></typeparam>
		/// <param name="fnRead">: size_t (char*, size_t)</param>
		/// <param name="fnWrite">: void (char*, size_t)</param>
		/// <param name="fnAdvance">: void (size_t)</param>
		/// <param name="fnCheck">: bool ()</param>
		/// <returns></returns>
		template<typename _FnRead, typename _FnWrite, typename _FnAdvance, typename _FnCheck, size_t CHUNK = BASIC_CHUNK>
		static optional<size_t> InternalDeflate(
			_FnRead&& fnRead, _FnWrite&& fnWrite,
			_FnAdvance&& fnAdvance, _FnCheck&& fnCheck);

		/// <typeparam name="_FnRead"></typeparam>
		/// <typeparam name="_FnCheck"></typeparam>
		/// <typeparam name="_FnAdvance"></typeparam>
		/// <typeparam name="_FnWrite"></typeparam>
		/// <typeparam name="CHUNK"></typeparam>
		/// <param name="fnRead">: size_t (char*, size_t)</param>
		/// <param name="fnWrite">: void (char*, size_t)</param>
		/// <param name="fnAdvance">: void (size_t)</param>
		/// <param name="fnCheck">: bool ()</param>
		/// <returns></returns>
		template<typename _FnRead, typename _FnWrite, typename _FnAdvance, typename _FnCheck, size_t CHUNK = BASIC_CHUNK>
		static optional<size_t> InternalInflate(
			_FnRead&& fnRead, _FnWrite&& fnWrite,
			_FnAdvance&& fnAdvance, _FnCheck&& fnCheck);
	public:
		static optional<size_t> Deflate(in_stream_t& bufIn, out_stream_t& bufOut, size_t count);
		static optional<size_t> Deflate(ByteBuffer& bufIn, out_stream_t& bufOut, size_t count);
		
		static optional<size_t> Inflate(in_stream_t& bufIn, out_stream_t& bufOut, size_t count);
		static optional<size_t> Inflate(ByteBuffer& bufIn, out_stream_t& bufOut, size_t count);
		static optional<size_t> Inflate(in_stream_t& bufIn, ByteBuffer& bufOut, size_t count);
		static optional<size_t> Inflate(ByteBuffer& bufIn, ByteBuffer& bufOut, size_t count);
	};

#pragma region impl
	// impl

	template<typename _FnRead, typename _FnWrite, typename _FnAdvance, typename _FnCheck, size_t CHUNK>
	optional<size_t> CompressorStream::InternalDeflate(
		_FnRead&& fnRead, _FnWrite&& fnWrite,
		_FnAdvance&& fnAdvance, _FnCheck&& fnCheck)
	{
		auto in = make_unique<char[]>(CHUNK);
		auto out = make_unique<char[]>(CHUNK);

		z_stream stream{};
		stream.zalloc = Z_NULL;
		stream.zfree = Z_NULL;
		stream.opaque = Z_NULL;

		int state = deflateInit(&stream, Z_DEFAULT_COMPRESSION);
		if (state != Z_OK)
			return {};

		size_t countBytes = 0U;

		try {
			int flushType = Z_NO_FLUSH;

			do {
				size_t read = fnRead(in.get(), CHUNK, &flushType);

				if (read > 0) {
					stream.next_in = (Bytef*)in.get();
					stream.avail_in = read;

					do {
						stream.next_out = (Bytef*)out.get();
						stream.avail_out = CHUNK;

						state = deflate(&stream, flushType);

						size_t availWrite = CHUNK - stream.avail_out;
						countBytes += availWrite;

						if (state != Z_STREAM_ERROR)
							fnWrite(out.get(), availWrite);
						else throw state;

					} while (stream.avail_out == 0);
				}

				fnAdvance(read);
			} while (fnCheck() && flushType != Z_FINISH);
		}
		catch (int&) {
			// Fail
			countBytes = 0;
		}

		deflateEnd(&stream);
		return countBytes;
	}

	template<typename _FnRead, typename _FnWrite, typename _FnAdvance, typename _FnCheck, size_t CHUNK>
	optional<size_t> CompressorStream::InternalInflate(
		_FnRead&& fnRead, _FnWrite&& fnWrite,
		_FnAdvance&& fnAdvance, _FnCheck&& fnCheck)
	{
		auto in = make_unique<char[]>(CHUNK);
		auto out = make_unique<char[]>(CHUNK);

		z_stream stream{};
		stream.zalloc = Z_NULL;
		stream.zfree = Z_NULL;
		stream.opaque = Z_NULL;
		stream.avail_in = 0;
		stream.next_in = Z_NULL;

		int state = inflateInit(&stream);
		if (state != Z_OK)
			return {};

		size_t countBytes = 0U;

		try {
			size_t read = 0U;

			do {
				read = fnRead(in.get(), CHUNK);

				if (read > 0U) {
					stream.next_in = (Bytef*)in.get();
					stream.avail_in = read;

					do {
						stream.next_out = (Bytef*)out.get();
						stream.avail_out = CHUNK;

						state = inflate(&stream, Z_NO_FLUSH);
						switch (state) {
						case Z_NEED_DICT:
						case Z_DATA_ERROR:
						case Z_MEM_ERROR:
						case Z_STREAM_ERROR:
							throw state;
						}

						size_t availWrite = CHUNK - stream.avail_out;
						countBytes += availWrite;

						fnWrite(out.get(), availWrite);
					} while (stream.avail_out == 0);
				}

				fnAdvance(read);
			} while (fnCheck() && read > 0U);
		}
		catch (int&) {
			// Fail
			countBytes = 0;
		}

		inflateEnd(&stream);
		return countBytes;
	}

#pragma endregion impl
}