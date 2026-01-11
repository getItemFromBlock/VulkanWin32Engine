#include "Resource/Video.hpp"

using namespace Resource;
using namespace Maths;

#include <fstream>

namespace MP4D
{
#define MINIMP4_IMPLEMENTATION
#include "minimp4.h"
}

std::vector<u8> read_file(const std::string &path)
{
	std::ifstream input = std::ifstream(path, std::ios_base::binary | std::ios_base::ate);
	std::vector<u8> result;
	if (!input.is_open() || !input.good())
	{
		return result;
	}
	size_t len = input.tellg();
	input.seekg(0, std::ios::beg);
	result.resize(len);
	input.read(reinterpret_cast<char*>(result.data()), len);
	return result;
}

struct INPUT_BUFFER
{
	uint8_t *buffer;
	size_t size;
};
#define USE_SHORT_SYNC 0

static int read_callback(int64_t offset, void *buffer, size_t size, void *token)
{
	INPUT_BUFFER *buf = (INPUT_BUFFER *)token;
	size_t to_copy = MINIMP4_MIN(size, buf->size - offset - size);
	memcpy(buffer, buf->buffer + offset, to_copy);
	return to_copy != size;
}

Video::Video()
{
}

Video::~Video()
{
}

std::vector<std::vector<u8>> Video::ReadVideoFrames(const std::string &path)
{
	std::vector<u8> buf = read_file(path);
	std::vector<std::vector<u8>> res;

	INPUT_BUFFER bufToken = { buf.data(), buf.size() };
	MP4D::MP4D_demux_t mp4 = { 0 };
	MP4D::MP4D_open(&mp4, read_callback, &bufToken, buf.size());

	for (u32 ntrack = 0; ntrack < mp4.track_count; ntrack++)
	{
		MP4D::MP4D_track_t *tr = mp4.track + ntrack;
		u32 i = 0;
		s32 spspps_bytes = 0;
		const void *spspps;

		unsigned sum_duration = 0;
		if (tr->handler_type == MP4D_HANDLER_TYPE_VIDE)
		{   // assume h264
			std::vector<u8> first_frame;

			char sync[4] = { 0, 0, 0, 1 };
			while (spspps = MP4D_read_sps(&mp4, ntrack, i, &spspps_bytes))
			{
				u64 prev = first_frame.size();
				first_frame.resize(first_frame.size() + 4 - USE_SHORT_SYNC + spspps_bytes);
				std::copy(sync + USE_SHORT_SYNC, sync + 4, first_frame.data() + prev);
				prev += 4;
				std::copy(reinterpret_cast<const u8*>(spspps), reinterpret_cast<const u8 *>(spspps) + spspps_bytes, first_frame.data() + prev);
				i++;
			}
			i = 0;
			while (spspps = MP4D_read_pps(&mp4, ntrack, i, &spspps_bytes))
			{
				u64 prev = first_frame.size();
				first_frame.resize(first_frame.size() + 4 - USE_SHORT_SYNC + spspps_bytes);
				std::copy(sync + USE_SHORT_SYNC, sync + 4, first_frame.data() + prev);
				std::copy(reinterpret_cast<const u8 *>(spspps), reinterpret_cast<const u8 *>(spspps) + spspps_bytes, first_frame.data() + prev);
				i++;
			}
			res.push_back(first_frame);
			for (i = 0; i < mp4.track[ntrack].sample_count; i++)
			{
				std::vector<u8> frame;
				unsigned frame_bytes, timestamp, duration;
				MP4D::MP4D_file_offset_t ofs = MP4D_frame_offset(&mp4, ntrack, i, &frame_bytes, &timestamp, &duration);
				uint8_t *mem = buf.data() + ofs;
				sum_duration += duration;
				while (frame_bytes)
				{
					uint32_t size = ((uint32_t)mem[0] << 24) | ((uint32_t)mem[1] << 16) | ((uint32_t)mem[2] << 8) | mem[3];
					size += 4;
					mem[0] = 0; mem[1] = 0; mem[2] = 0; mem[3] = 1;
					u64 prev = frame.size();
					frame.resize(frame.size() + size - USE_SHORT_SYNC);
					std::copy(mem + USE_SHORT_SYNC, mem + size, frame.data() + prev);
					if (frame_bytes < size)
					{
						printf("error: demux sample failed\n");
						exit(1);
					}
					frame_bytes -= size;
					mem += size;
				}
				res.push_back(frame);
			}
			break;
		}
		else if (tr->handler_type == MP4D_HANDLER_TYPE_SOUN)
		{   // assume aac
		}
	}

	MP4D_close(&mp4);
	return res;
}
