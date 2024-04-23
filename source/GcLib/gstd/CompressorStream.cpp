#include "source/GcLib/pch.h"
#include "CompressorStream.hpp"

using namespace gstd;

//*******************************************************************
//CompressorStream
//*******************************************************************

optional<size_t> CompressorStream::Deflate(in_stream_t& bufIn, out_stream_t& bufOut, size_t count) {
	auto reader = [&](char* _bIn, size_t reading, int* _flushType) -> size_t {
		bufIn.read(_bIn, reading);
		size_t read = bufIn.gcount();
		if (read > count) {
			*_flushType = Z_FINISH;
			read = count;
		}
		else if (read < reading) {
			*_flushType = Z_FINISH;
		}
		return read;
	};
	auto writer = [&](char* _bOut, size_t writing) {
		bufOut.write(_bOut, writing);
	};
	auto advance = [&](size_t advancing) { count -= advancing; };
	auto ended = [&]() { return count > 0U; };

	return InternalDeflate(reader, writer, advance, ended);
}
optional<size_t> CompressorStream::Deflate(ByteBuffer& bufIn, out_stream_t& bufOut, size_t count) {
	size_t readPos = 0;

	auto reader = [&](char* _bIn, size_t reading, int* _flushType) -> size_t {
		size_t read = std::min(reading, count);
		if (read > count) {
			*_flushType = Z_FINISH;
			read = count;
		}
		else if (read < reading) {
			*_flushType = Z_FINISH;
		}
		memcpy(_bIn, bufIn.GetPointer(readPos), read);
		readPos += read;
		return read;
	};
	auto writer = [&](char* _bOut, size_t writing) {
		bufOut.write(_bOut, writing);
	};
	auto advance = [&](size_t advancing) { count -= advancing; };
	auto ended = [&]() { return count > 0U; };

	return InternalDeflate(reader, writer, advance, ended);
}

optional<size_t> CompressorStream::Inflate(in_stream_t& bufIn, out_stream_t& bufOut, size_t count) {
	auto reader = [&](char* _bIn, size_t reading) -> size_t {
		bufIn.read(_bIn, reading);
		size_t read = bufIn.gcount();
		return read > count ? count : read;
	};
	auto writer = [&](char* _bOut, size_t writing) {
		bufOut.write(_bOut, writing);
	};
	auto advance = [&](size_t advancing) { count -= advancing; };
	auto ended = [&]() { return count > 0U; };

	return InternalInflate(reader, writer, advance, ended);
}
optional<size_t> CompressorStream::Inflate(ByteBuffer& bufIn, out_stream_t& bufOut, size_t count) {
	size_t readPos = 0;

	auto reader = [&](char* _bIn, size_t reading) -> size_t {
		size_t read = std::min(reading, count);
		memcpy(_bIn, bufIn.GetPointer(readPos), read);
		readPos += read;
		return read;
	};
	auto writer = [&](char* _bOut, size_t writing) {
		bufOut.write(_bOut, writing);
	};
	auto advance = [&](size_t advancing) { count -= advancing; };
	auto ended = [&]() { return count > 0U; };

	return InternalInflate(reader, writer, advance, ended);
}
optional<size_t> CompressorStream::Inflate(in_stream_t& bufIn, ByteBuffer& bufOut, size_t count) {
	size_t readPos = 0;

	auto reader = [&](char* _bIn, size_t reading) -> size_t {
		bufIn.read(_bIn, reading);
		size_t read = bufIn.gcount();
		return read > count ? count : read;
	};
	auto writer = [&](char* _bOut, size_t writing) {
		bufOut.Write(_bOut, writing);
	};
	auto advance = [&](size_t advancing) { count -= advancing; };
	auto ended = [&]() { return count > 0U; };

	return InternalInflate(reader, writer, advance, ended);
}
optional<size_t> CompressorStream::Inflate(ByteBuffer& bufIn, ByteBuffer& bufOut, size_t count) {
	size_t readPos = 0;

	auto reader = [&](char* _bIn, size_t reading) -> size_t {
		size_t read = std::min(reading, count);
		memcpy(_bIn, bufIn.GetPointer(readPos), read);
		readPos += read;
		return read;
	};
	auto writer = [&](char* _bOut, size_t writing) {
		bufOut.Write(_bOut, writing);
	};
	auto advance = [&](size_t advancing) { count -= advancing; };
	auto ended = [&]() { return count > 0U; };

	return InternalInflate(reader, writer, advance, ended);
}
